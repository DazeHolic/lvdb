#pragma once


#include <string>
#include "leveldb/db.h"
#include "leveldb/options.h"
#include "leveldb/slice.h"
#include "leveldb/status.h"
#include "leveldb/write_batch.h"
#include "toolkits/mutex.h"
#include "lvdb/bytes.h"


namespace lv
{
	class Binlog;


	// circular queue
	class Binlog_Queue
	{
	public:
		toolkit::Mutex mutex;

		Binlog_Queue(leveldb::DB *db, bool enabled = true, int capacity = 20000000);
		~Binlog_Queue();

		void begin();
		void rollback();
		leveldb::Status commit();
		// leveldb put
		void Put(const leveldb::Slice& key, const leveldb::Slice& value);
		// leveldb delete
		void Delete(const leveldb::Slice& key);
		void add_log(char type, char cmd, const leveldb::Slice &key);
		void add_log(char type, char cmd, const std::string &key);

		int get(uint64_t seq, Binlog *log) const;
		int update(uint64_t seq, char type, char cmd, const std::string &key);

		void flush();

		/** @returns
		 1 : log.seq greater than or equal to seq
		 0 : not found
		 -1: error
		 */
		int find_next(uint64_t seq, Binlog *log) const;
		int find_last(Binlog *log) const;

		uint64_t min_seq() const{
			return min_seq_;
		}

		uint64_t max_seq() const{
			return last_seq;
		}

		std::string stats() const;

	private:
		leveldb::DB *db;
		uint64_t min_seq_;
		uint64_t last_seq;
		uint64_t tran_seq;
		int capacity;
		leveldb::WriteBatch batch;

		volatile bool thread_quit;
		static void* log_clean_thread_func(void *arg);
		int del(uint64_t seq);
		// [start, end] includesive
		int del_range(uint64_t start, uint64_t end);

		void clean_obsolete_binlogs();
		void merge();
		bool enabled;
	};


	class Transaction
	{
	public:
		Transaction(Binlog_Queue *logs){
			this->logs = logs;
			logs->mutex.lock();
			logs->begin();
		}

		~Transaction(){
			// it is safe to call rollback after commit
			logs->rollback();
			logs->mutex.unlock();
		}

	private:
		Binlog_Queue *logs;
	};


}
