#include "lvdb/sync.h"
#include "lvdb_impl.h"
#include "toolkits/log.h"


namespace lv
{

	Sync::Sync(const std::string& name, LVDB* db, Sync_Processor* sync) :
		name_(name),
		db_(db),
		sync_(sync)
	{

	}

	int Sync::svc()
	{
		LVDB_Impl* db = (LVDB_Impl*)db_;
		Binlog_Queue *logs = db->binlogs;
		std::string sync_seq = name_ + ":sync:seq";
		LOG_INFO("logs seq[" << logs->min_seq() << ", " << logs->max_seq() << "]");
		std::string val;
		uint64_t last_seq(0);
		if (db_->meta_get(sync_seq, &val) == 1) {
			Bytes_uint64::FromString(last_seq, val);
		}
		LOG_INFO(name_ << " last sync seq: " << last_seq);

		Binlog log;
		while (1) {
			int ret = 0;
			uint64_t expect_seq = last_seq + 1;
			if (last_seq == 0) {
				ret = logs->find_last(&log);
			}
			else {
				ret = logs->find_next(expect_seq, &log);
			}
			if (ret == 0)
				return 0;
			last_seq = log.seq();
			switch (log.cmd()) {
			case BinlogCommand::KSET:
			case BinlogCommand::HSET:
			case BinlogCommand::ZSET:
			case BinlogCommand::QSET:
			case BinlogCommand::QPUSH_BACK:
			case BinlogCommand::QPUSH_FRONT:
			{
				std::string val;
				int ret = db->raw_get(log.key(), &val);
				if (ret == -1) {
					LOG_ERROR(" raw_get error!");
					break;
				}
				else if (ret == 0) {
					LOG_ERROR("skip not found, " << log.dumps());
				}
				else {
					//
					sync_->do_sync(log, val.c_str(), val.length());
				}
				break;
			}

			case BinlogCommand::KDEL:
			case BinlogCommand::HDEL:
			case BinlogCommand::ZDEL:
			case BinlogCommand::QPOP_BACK:
			case BinlogCommand::QPOP_FRONT:
				sync_->do_sync(log, NULL, 0);
				break;
			}
		}
		db_->meta_set(sync_seq, Bytes_uint64(last_seq));
		return 0;
	}



	Copy::Copy(const std::string& name, LVDB* db, Sync_Processor* sync) :
		name_(name),
		db_(db),
		sync_(sync)
	{

	}


	int Copy::svc()
	{
		LVDB_Impl* db = (LVDB_Impl*)db_;
		Binlog_Queue *logs = db->binlogs;
		std::string copy_key = name_ + ":copy:key";
		std::string last_key;
		db_->meta_get(last_key, &last_key);
		if (last_key.empty()) {
			last_key.push_back(DataType::MIN_PREFIX);
		}

		LOG_INFO(name_ << " copy, last key: " << last_key);
		Iterator *iter = db_->iterator(last_key, "", -1);
		Binlog log;
		while (1) {
			if (!iter->next()) {
				LOG_INFO("copy finish");
				break;
			}

			Bytes key = iter->key();
			if (key.size() == 0) {
				continue;
			}
			// finish copying all valid data types
			if (key.data()[0] > DataType::MAX_PREFIX) {
				break;
			}

			Bytes val = iter->val();
			last_key = key.String();
			char cmd = 0;
			char data_type = key.data()[0];
			if (data_type == DataType::KV) {
				cmd = BinlogCommand::KSET;
			}
			else if (data_type == DataType::HASH) {
				cmd = BinlogCommand::HSET;
			}
			else if (data_type == DataType::ZSET) {
				cmd = BinlogCommand::ZSET;
			}
			else if (data_type == DataType::QUEUE) {
				cmd = BinlogCommand::QPUSH_BACK;
			}
			else {
				continue;
			}

			Binlog log(0, BinlogType::COPY, cmd, slice(key));
			if (!sync_->do_sync(log, val.data(), val.size())) {
				//save
				db_->meta_set(copy_key, last_key);
			}
		}

		return 0;
	}



	//////////////////////////////////////////////////////////////////////////
	Backup_Server_Processor::Backup_Server_Processor(LVDB* db)
	{

	}



	int Backup_Server_Processor::do_sync(Binlog& log, const char* val, int len)
	{
		const char log_type = BinlogType::MIRROR;
		switch (log.cmd()) {
		case BinlogCommand::KSET:
		{
			if (!val) {
				break;
			}
			std::string key;
			if (decode_kv_key(log.key(), &key) == -1) {
				LOG_WARN("invalid kv key");
				break;
			}
			LOG_INFO("set %s" << hexmem(key.data(), key.size()) );
			if (db_->set(key, Bytes(val, len), log_type) == -1) {
				return -1;
			}
		}
		break;

		case BinlogCommand::KDEL:
		{
			std::string key;
			if (decode_kv_key(log.key(), &key) == -1) {
				break;
			}
			LOG_INFO("del %s" << hexmem(key.data(), key.size()) );
			if (db_->del(key, log_type) == -1) {
				return -1;
			}
		}
		break;

		case BinlogCommand::HSET:
		{
			if (!val) {
				break;
			}
			std::string name, key;
			if (decode_hash_key(log.key(), &name, &key) == -1) {
				break;
			}
			LOG_INFO("hset %s %s" << 
				hexmem(name.data(), name.size()) <<
				hexmem(key.data(), key.size()) );
			if (db_->hset(name, key, Bytes(val, len), log_type) == -1) {
				return -1;
			}
		}
		break;

		case BinlogCommand::HDEL:
		{
			std::string name, key;
			if (decode_hash_key(log.key(), &name, &key) == -1) {
				break;
			}
			LOG_INFO("hdel %s %s" <<
				hexmem(name.data(), name.size()) <<
				hexmem(key.data(), key.size()) );
			if (db_->hdel(name, key, log_type) == -1) {
				return -1;
			}
		}
		break;

		case BinlogCommand::ZSET:
		{
			if (!val) {
				break;
			}
			std::string name, key;
			if (decode_zset_key(log.key(), &name, &key) == -1) {
				break;
			}
			LOG_INFO("zset %s %s" <<
				hexmem(name.data(), name.size()) <<
				hexmem(key.data(), key.size()).c_str());
			if (db_->zset(name, key, Bytes(val, len), log_type) == -1) {
				return -1;
			}
		}
		break;

		case BinlogCommand::ZDEL:
		{
			std::string name, key;
			if (decode_zset_key(log.key(), &name, &key) == -1) {
				break;
			}
			LOG_INFO("zdel %s %s" << 
				hexmem(name.data(), name.size()) <<
				hexmem(key.data(), key.size()) );
			if (db_->zdel(name, key, log_type) == -1) {
				return -1;
			}
		}
		break;

		case BinlogCommand::QSET:
		case BinlogCommand::QPUSH_BACK:
		case BinlogCommand::QPUSH_FRONT:
		{
			if (!val) {
				break;
			}
			std::string name;
			uint64_t seq;
			if (decode_qitem_key(log.key(), &name, &seq) == -1) {
				break;
			}
			if (seq < QITEM_MIN_SEQ || seq > QITEM_MAX_SEQ) {
				break;
			}
			int ret;
			if (log.cmd() == BinlogCommand::QSET) {
				LOG_INFO("qset " << hexmem(name.data(), name.size()) << " " << seq);
				ret = db_->qset_by_seq(name, seq, Bytes(val, len), log_type);
			}
			else if (log.cmd() == BinlogCommand::QPUSH_BACK) {
				LOG_INFO("qpush_back " << hexmem(name.data(), name.size()) );
				ret = db_->qpush_back(name, Bytes(val, len), log_type);
			}
			else {
				LOG_INFO("qpush_front " << hexmem(name.data(), name.size()) );
				ret = db_->qpush_front(name, Bytes(val, len), log_type);
			}
			if (ret == -1) {
				return -1;
			}
		}
		break;

		case BinlogCommand::QPOP_BACK:
		case BinlogCommand::QPOP_FRONT:
		{
			int ret;
			const Bytes name = log.key();
			std::string tmp;
			if (log.cmd() == BinlogCommand::QPOP_BACK) {
				LOG_INFO("qpop_back " << hexmem(name.data(), name.size()) );
				ret = db_->qpop_back(name, &tmp, log_type);
			}
			else {
				LOG_INFO("qpop_front " << hexmem(name.data(), name.size()) );
				ret = db_->qpop_front(name, &tmp, log_type);
			}
			if (ret == -1) {
				return -1;
			}
		}
		break;

		default:
			LOG_ERROR("unknown binlog, type: " << log.type() << ", cmd: " << log.cmd());
			break;
		}

		return 0;
	}
}

