#include "lvdb_impl.h"
#include "lvdb/t_kv.h"
#include "toolkits/log.h"


namespace lv
{
	int LVDB_Impl::multi_set(const std::vector<Bytes> &kvs, int offset, char log_type){
		Transaction trans(binlogs);

		std::vector<Bytes>::const_iterator it;
		it = kvs.begin() + offset;
		for (; it != kvs.end(); it += 2){
			const Bytes &key = *it;
			if (key.empty()){
				LOG_INFO("empty key!");
				return 0;
				//return -1;
			}
			const Bytes &val = *(it + 1);
			std::string buf = encode_kv_key(key);
			binlogs->Put(buf, slice(val));
			binlogs->add_log(log_type, BinlogCommand::KSET, buf);
		}
		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("multi_set error: " << s.ToString().c_str());
			return -1;
		}
		return (kvs.size() - offset) / 2;
	}

	int LVDB_Impl::multi_del(const std::vector<Bytes> &keys, int offset, char log_type){
		Transaction trans(binlogs);

		std::vector<Bytes>::const_iterator it;
		it = keys.begin() + offset;
		for (; it != keys.end(); it++){
			const Bytes &key = *it;
			std::string buf = encode_kv_key(key);
			binlogs->Delete(buf);
			binlogs->add_log(log_type, BinlogCommand::KDEL, buf);
		}
		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("multi_del error: " << s.ToString().c_str());
			return -1;
		}
		return keys.size() - offset;
	}

	int LVDB_Impl::set(const Bytes &key, const Bytes &val, char log_type){
		if (key.empty()){
			LOG_INFO("empty key!");
			//return -1;
			return 0;
		}
		Transaction trans(binlogs);

		std::string buf = encode_kv_key(key);
		binlogs->Put(buf, slice(val));
		binlogs->add_log(log_type, BinlogCommand::KSET, buf);
		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("set error: " << s.ToString().c_str());
			return -1;
		}
		return 1;
	}

	int LVDB_Impl::setnx(const Bytes &key, const Bytes &val, char log_type){
		if (key.empty()){
			LOG_INFO("empty key!");
			//return -1;
			return 0;
		}
		Transaction trans(binlogs);

		std::string tmp;
		int found = this->get(key, &tmp);
		if (found != 0){
			return 0;
		}
		std::string buf = encode_kv_key(key);
		binlogs->Put(buf, slice(val));
		binlogs->add_log(log_type, BinlogCommand::KSET, buf);
		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("set error: " << s.ToString().c_str());
			return -1;
		}
		return 1;
	}

	int LVDB_Impl::getset(const Bytes &key, std::string *val, const Bytes &newval, char log_type){
		if (key.empty()){
			LOG_INFO("empty key!");
			//return -1;
			return 0;
		}
		Transaction trans(binlogs);

		int found = this->get(key, val);
		std::string buf = encode_kv_key(key);
		binlogs->Put(buf, slice(newval));
		binlogs->add_log(log_type, BinlogCommand::KSET, buf);
		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("set error: " << s.ToString().c_str());
			return -1;
		}
		return found;
	}


	int LVDB_Impl::del(const Bytes &key, char log_type){
		Transaction trans(binlogs);

		std::string buf = encode_kv_key(key);
		binlogs->Delete(buf);
		binlogs->add_log(log_type, BinlogCommand::KDEL, buf);
		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("del error: " << s.ToString().c_str());
			return -1;
		}
		return 1;
	}

	int LVDB_Impl::incr(const Bytes &key, int64_t by, int64_t *new_val, char log_type){
		Transaction trans(binlogs);

		std::string old;
		int ret = this->get(key, &old);
		if (ret == -1){
			return -1;
		}
		else if (ret == 0){
			*new_val = by;
		}
		else{
			*new_val = str_to_int64(old) + by;
			if (errno != 0){
				return 0;
			}
		}

		std::string buf = encode_kv_key(key);
		binlogs->Put(buf, str(*new_val));
		binlogs->add_log(log_type, BinlogCommand::KSET, buf);

		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("del error: " << s.ToString().c_str());
			return -1;
		}
		return 1;
	}

	int LVDB_Impl::get(const Bytes &key, std::string *val){
		std::string buf = encode_kv_key(key);

		leveldb::Status s = ldb->Get(leveldb::ReadOptions(), buf, val);
		if (s.IsNotFound()){
			return 0;
		}
		if (!s.ok()){
			LOG_INFO("get error: " << s.ToString().c_str());
			return -1;
		}
		return 1;
	}

	KIterator* LVDB_Impl::scan(const Bytes &start, const Bytes &end, uint64_t limit){
		std::string key_start, key_end;
		key_start = encode_kv_key(start);
		if (end.empty()){
			key_end = "";
		}
		else{
			key_end = encode_kv_key(end);
		}
		//dump(key_start.data(), key_start.size(), "scan.start");
		//dump(key_end.data(), key_end.size(), "scan.end");

		return new KIterator(this->iterator(key_start, key_end, limit));
	}

	KIterator* LVDB_Impl::rscan(const Bytes &start, const Bytes &end, uint64_t limit){
		std::string key_start, key_end;

		key_start = encode_kv_key(start);
		if (start.empty()){
			key_start.append(1, 255);
		}
		if (!end.empty()){
			key_end = encode_kv_key(end);
		}
		//dump(key_start.data(), key_start.size(), "scan.start");
		//dump(key_end.data(), key_end.size(), "scan.end");

		return new KIterator(this->rev_iterator(key_start, key_end, limit));
	}

	int LVDB_Impl::set_bit(const Bytes &key, int bitoffset, int on, char log_type){
		if (key.empty()){
			LOG_INFO("empty key!");
			return 0;
		}
		Transaction trans(binlogs);

		std::string val;
		int ret = this->get(key, &val);
		if (ret == -1){
			return -1;
		}

		int len = bitoffset / 8;
		int bit = bitoffset % 8;
		if (len >= val.size()){
			val.resize(len + 1, 0);
		}
		int orig = val[len] & (1 << bit);
		if (on == 1){
			val[len] |= (1 << bit);
		}
		else{
			val[len] &= ~(1 << bit);
		}

		std::string buf = encode_kv_key(key);
		binlogs->Put(buf, val);
		binlogs->add_log(log_type, BinlogCommand::KSET, buf);
		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("set error: " << s.ToString().c_str());
			return -1;
		}
		return orig;
	}

	int LVDB_Impl::get_bit(const Bytes &key, int bitoffset){
		std::string val;
		int ret = this->get(key, &val);
		if (ret == -1){
			return -1;
		}

		int len = bitoffset / 8;
		int bit = bitoffset % 8;
		if (len >= val.size()){
			return 0;
		}
		return (val[len] & (1 << bit)) == 0 ? 0 : 1;
	}



}