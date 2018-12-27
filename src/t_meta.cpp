#include "lvdb_impl.h"
#include "lvdb/t_meta.h"
#include "toolkits/log.h"


namespace lv
{
	int LVDB_Impl::meta_set(const Bytes &key, const Bytes &val)
	{
		Transaction trans(binlogs);

		std::string buf = encode_meta_key(key);
		binlogs->Put(buf, slice(val));
		binlogs->add_log(BinlogType::CTRL, BinlogCommand::META_SET, buf);
		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("set error: " << s.ToString().c_str());
			return -1;
		}
		return 1;
	}

	int LVDB_Impl::meta_del(const Bytes &key)
	{
		Transaction trans(binlogs);

		std::string buf = encode_meta_key(key);
		binlogs->Delete(buf);
		binlogs->add_log(BinlogType::CTRL, BinlogCommand::META_DEL, buf);
		leveldb::Status s = binlogs->commit();
		if (!s.ok()){
			LOG_INFO("del error: " << s.ToString().c_str());
			return -1;
		}
		return 1;
	}

	int LVDB_Impl::meta_get(const Bytes &key, std::string *val)
	{
		std::string buf = encode_meta_key(key);
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

	int LVDB_Impl::meta_list(std::vector<std::string> *list)
	{
		std::string start;
		std::string end;
		start = encode_meta_key(LVDB::KeyMin);
		end = encode_meta_key(LVDB::KeyMax);
		Iterator *it = this->iterator(start, end, INT32_MAX);
		while (it->next()) {
			list->push_back(std::string(it->key().String()));
		}
		return -1;
	}

}
