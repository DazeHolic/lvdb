/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "binlog_queue.h"
#include "lvdb/binlog.h"
#include "lvdb/const.h"
#include "lvdb/strings.h"
#include "toolkits/log.h"
#include "toolkits/util.h"
#include "pthread.h"
#include <map>


#ifdef WIN32
#define strerror(x) ""
#endif


namespace lv
{	/* SyncLogQueue */

	static inline std::string encode_seq_key(uint64_t seq){
		seq = big_endian(seq);
		std::string ret;
		ret.push_back(DataType::SYNCLOG);
		ret.append((char *)&seq, sizeof(seq));
		return ret;
	}

	static inline uint64_t decode_seq_key(const leveldb::Slice &key){
		uint64_t seq = 0;
		if (key.size() == (sizeof(uint64_t) + 1) && key.data()[0] == DataType::SYNCLOG){
			seq = *((uint64_t *)(key.data() + 1));
			seq = big_endian(seq);
		}
		return seq;
	}

	Binlog_Queue::Binlog_Queue(leveldb::DB *db, bool enabled, int capacity){
		this->db = db;
		this->min_seq_ = 0;
		this->last_seq = 0;
		this->tran_seq = 0;
		this->capacity = capacity;
		this->enabled = enabled;

		Binlog log;
		if (this->find_last(&log) == 1){
			this->last_seq = log.seq();
		}
		// 下面这段代码是可能性能非常差!
		//if(this->find_next(0, &log) == 1){
		//	this->min_seq_ = log.seq();
		//}
		if (this->last_seq > this->capacity){
			this->min_seq_ = this->last_seq - this->capacity;
		}
		else{
			this->min_seq_ = 0;
		}
		if (this->find_next(this->min_seq_, &log) == 1){
			this->min_seq_ = log.seq();
		}
		if (this->enabled){
			LOG_INFO("binlogs capacity: " << this->capacity << ", min: " << this->min_seq_ << ", max: %" << this->last_seq );
			// 这个方法有性能问题
			// 但是, 如果不执行清理, 如果将 capacity 修改大, 可能会导致主从同步问题
			//this->clean_obsolete_binlogs();
		}

		// start cleaning thread
		if (this->enabled){
			thread_quit = false;
			pthread_t tid;
			int err = pthread_create(&tid, NULL, &Binlog_Queue::log_clean_thread_func, this);
			if (err != 0){
				LOG_ERROR("can't create thread: " << strerror(err));
				exit(0);
			}
		}
	}

	Binlog_Queue::~Binlog_Queue(){
		if (this->enabled){
			thread_quit = true;
			for (int i = 0; i < 100; i++){
				if (thread_quit == false){
					break;
				}
				Sleep(10);
			}
		}
		db = NULL;
	}

	std::string Binlog_Queue::stats() const{
		std::string s;
		s.append("    capacity : " + str(capacity) + "\n");
		s.append("    min_seq  : " + str(min_seq_) + "\n");
		s.append("    max_seq  : " + str(last_seq) + "");
		return s;
	}

	void Binlog_Queue::begin(){
		tran_seq = last_seq;
		batch.Clear();
	}

	void Binlog_Queue::rollback(){
		tran_seq = 0;
	}

	leveldb::Status Binlog_Queue::commit(){
		leveldb::WriteOptions write_opts;
		leveldb::Status s = db->Write(write_opts, &batch);
		if (s.ok()){
			last_seq = tran_seq;
			tran_seq = 0;
		}
		return s;
	}

	void Binlog_Queue::add_log(char type, char cmd, const leveldb::Slice &key){
		if (!enabled){
			return;
		}
		tran_seq++;
		Binlog log(tran_seq, type, cmd, key);
		batch.Put(encode_seq_key(tran_seq), log.repr());
	}

	void Binlog_Queue::add_log(char type, char cmd, const std::string &key){
		if (!enabled){
			return;
		}
		leveldb::Slice s(key);
		this->add_log(type, cmd, s);
	}

	// leveldb put
	void Binlog_Queue::Put(const leveldb::Slice& key, const leveldb::Slice& value){
		batch.Put(key, value);
	}

	// leveldb delete
	void Binlog_Queue::Delete(const leveldb::Slice& key){
		batch.Delete(key);
	}

	int Binlog_Queue::find_next(uint64_t next_seq, Binlog *log) const{
		if (this->get(next_seq, log) == 1){
			return 1;
		}
		uint64_t ret = 0;
		std::string key_str = encode_seq_key(next_seq);
		leveldb::ReadOptions iterate_options;
		leveldb::Iterator *it = db->NewIterator(iterate_options);
		it->Seek(key_str);
		if (it->Valid()){
			leveldb::Slice key = it->key();
			if (decode_seq_key(key) != 0){
				leveldb::Slice val = it->value();
				if (log->load(val) == -1){
					ret = -1;
				}
				else{
					ret = 1;
				}
			}
		}
		delete it;
		return ret;
	}

	int Binlog_Queue::find_last(Binlog *log) const{
		uint64_t ret = 0;
		std::string key_str = encode_seq_key(UINT64_MAX);
		leveldb::ReadOptions iterate_options;
		leveldb::Iterator *it = db->NewIterator(iterate_options);
		it->Seek(key_str);
		if (!it->Valid()){
			// Iterator::prev requires Valid, so we seek to last
			it->SeekToLast();
		}
		else{
			// UINT64_MAX is not used 
			it->Prev();
		}
		if (it->Valid()){
			leveldb::Slice key = it->key();
			if (decode_seq_key(key) != 0){
				leveldb::Slice val = it->value();
				if (log->load(val) == -1){
					ret = -1;
				}
				else{
					ret = 1;
				}
			}
		}
		delete it;
		return ret;
	}

	int Binlog_Queue::get(uint64_t seq, Binlog *log) const{
		std::string val;
		leveldb::Status s = db->Get(leveldb::ReadOptions(), encode_seq_key(seq), &val);
		if (s.ok()){
			if (log->load(val) != -1){
				return 1;
			}
		}
		return 0;
	}

	int Binlog_Queue::update(uint64_t seq, char type, char cmd, const std::string &key){
		Binlog log(seq, type, cmd, key);
		leveldb::Status s = db->Put(leveldb::WriteOptions(), encode_seq_key(seq), log.repr());
		if (s.ok()){
			return 0;
		}
		return -1;
	}

	int Binlog_Queue::del(uint64_t seq){
		leveldb::Status s = db->Delete(leveldb::WriteOptions(), encode_seq_key(seq));
		if (!s.ok()){
			return -1;
		}
		return 0;
	}

	void Binlog_Queue::flush(){
		del_range(this->min_seq_, this->last_seq);
	}

	int Binlog_Queue::del_range(uint64_t start, uint64_t end){
		while (start <= end){
			leveldb::WriteBatch batch;
			for (int count = 0; start <= end && count < 1000; start++, count++){
				batch.Delete(encode_seq_key(start));
			}
			leveldb::Status s = db->Write(leveldb::WriteOptions(), &batch);
			if (!s.ok()){
				return -1;
			}
		}
		return 0;
	}

	void* Binlog_Queue::log_clean_thread_func(void *arg){
		Binlog_Queue *logs = (Binlog_Queue *)arg;

		while (!logs->thread_quit){
			if (!logs->db){
				break;
			}
			assert(logs->last_seq >= logs->min_seq_);

			if (logs->last_seq - logs->min_seq_ < logs->capacity + 10000){
				Sleep(50);
				continue;
			}

			uint64_t start = logs->min_seq_;
			uint64_t end = logs->last_seq - logs->capacity;
			logs->del_range(start, end);
			logs->min_seq_ = end + 1;
			LOG_INFO("clean " << (end - start + 1) << " logs[" << start << " ~ " << end << "], " << (logs->last_seq - logs->min_seq_ + 1) << " left, max: " << logs->last_seq);
		}
		LOG_INFO("binlog clean_thread quit");

		logs->thread_quit = false;
		return (void *)NULL;
	}

	// 因为老版本可能产生了断续的binlog
	// 例如, binlog-1 存在, 但后面的被删除了, 然后到 binlog-100000 时又开始存在.
	void Binlog_Queue::clean_obsolete_binlogs(){
		std::string key_str = encode_seq_key(this->min_seq_);
		leveldb::ReadOptions iterate_options;
		leveldb::Iterator *it = db->NewIterator(iterate_options);
		it->Seek(key_str);
		if (it->Valid()){
			it->Prev();
		}
		uint64_t count = 0;
		while (it->Valid()){
			leveldb::Slice key = it->key();
			uint64_t seq = decode_seq_key(key);
			if (seq == 0){
				break;
			}
			this->del(seq);

			it->Prev();
			count++;
		}
		delete it;
		if (count > 0){
			LOG_INFO("clean_obsolete_binlogs: " << count);
		}
	}

	// TESTING, slow, so not used
	void Binlog_Queue::merge(){
		std::map<std::string, uint64_t> key_map;
		uint64_t start = min_seq_;
		uint64_t end = last_seq;
		int reduce_count = 0;
		int total = 0;
		total = end - start + 1;
		(void)total; // suppresses warning
		LOG_INFO("merge begin");
		for (; start <= end; start++){
			Binlog log;
			if (this->get(start, &log) == 1){
				if (log.type() == BinlogType::NOOP){
					continue;
				}
				std::string key = log.key().String();
				std::map<std::string, uint64_t>::iterator it = key_map.find(key);
				if (it != key_map.end()){
					uint64_t seq = it->second;
					this->update(seq, BinlogType::NOOP, BinlogCommand::NONE, "");
					//log_trace("merge update %" PRIu64 " to NOOP", seq);
					reduce_count++;
				}
				key_map[key] = log.seq();
			}
		}
		LOG_INFO("merge reduce " << reduce_count << " of " << total  << " binlogs");
	}


}