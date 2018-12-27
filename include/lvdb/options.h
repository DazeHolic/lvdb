#pragma once


#include <string>


namespace lv
{
	struct Options
	{
	public:
#ifdef NDEBUG
		static const int LOG_QUEUE_SIZE = 20 * 1000 * 1000;
#else
		static const int LOG_QUEUE_SIZE = 10000;
#endif

	public:
		std::string dir;
		size_t cache_size;
		size_t max_open_files;
		size_t write_buffer_size;
		size_t block_size;
		int compaction_speed;
		bool compression;
		bool binlog;
		size_t binlog_capacity;

		Options() {
			dir = "lvdb/";
			cache_size = 16;
			write_buffer_size = 16;
			block_size = 16;
			compaction_speed = 0;
			compression = true;
			binlog = true;
			max_open_files = 500;
			binlog_capacity = LOG_QUEUE_SIZE;
		};

		static Options load(const char* fn, const char* db);
	};


}
