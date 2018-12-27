#include "lvdb/options.h"
#include "yaml-cpp/yaml.h"
#include <fstream>
#include "lvdb/strings.h"


namespace
{
	template< typename T>
	bool update_vaule(const YAML::Node& node, const std::string& key, T& t)
	{
		const YAML::Node tn = node[key];
		if (tn.IsDefined()) {
			t = tn.as<T>();
			return true;
		}
		return false;
	}

	template< typename T>
	bool update_vaule(const YAML::Node& node, const std::string& key, const std::string& subkey, T& t)
	{
		const YAML::Node tn = node[key];
		if (!tn.IsDefined()) {
			return false;
		}
		const YAML::Node tn1 = tn[subkey];
		if (tn1.IsDefined()) {
			t = tn1.as<T>();
			return true;
		}
		return false;
	}

	void load_value(lv::Options& opt, const YAML::Node& root) {
		if (!root.IsDefined())
			return;
		update_vaule<std::string>(root, "dir", opt.dir);
		update_vaule<size_t>(root, "cache_size", opt.cache_size);
		update_vaule<size_t>(root, "max_open_files", opt.max_open_files);
		update_vaule<size_t>(root, "write_buffer_size", opt.write_buffer_size);
		update_vaule<size_t>(root, "block_size", opt.block_size);
		update_vaule<int>(root, "compaction_speed", opt.compaction_speed);
		update_vaule<bool>(root, "compression", opt.compression);
		update_vaule<bool>(root, "replication", "binlog", opt.binlog);
		update_vaule<size_t>(root, "replication", "capacity", opt.binlog_capacity);
		if (opt.binlog_capacity <= 0){
			opt.binlog_capacity = lv::Options::LOG_QUEUE_SIZE;
		}
		if (opt.cache_size <= 0){
			opt.cache_size = 16;
		}
		if (opt.write_buffer_size <= 0){
			opt.write_buffer_size = 16;
		}
		if (opt.block_size <= 0){
			opt.block_size = 16;
		}
		if (opt.max_open_files <= 0){
			opt.max_open_files = opt.cache_size / 1024 * 300;
			if (opt.max_open_files < 500){
				opt.max_open_files = 500;
			}
			if (opt.max_open_files > 1000){
				opt.max_open_files = 1000;
			}
		}
	}
}

namespace lv
{

	Options Options::load(const char* fn, const char* db)
	{
		Options opt;
		std::ifstream fin;
		fin.open(fn);
		YAML::Node doc = YAML::Load(fin);
		YAML::Node lvdb = doc["lvdb"];
		load_value(opt, lvdb);
		if (db) {
			load_value(opt, lvdb[db]);
			opt.dir += db;
		}
		return opt;
	}

}
