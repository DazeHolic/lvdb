#pragma once


#include "binlog_queue.h"

#include "lvdb/lvdb.h"
#include "lvdb/binlog.h"
#include "lvdb/iterator.h"
#include "lvdb/t_kv.h"
#include "lvdb/t_hash.h"
#include "lvdb/t_zset.h"
#include "lvdb/t_queue.h"

#include "leveldb/db.h"
#include "leveldb/slice.h"


namespace lv
{
	inline static leveldb::Slice slice(const Bytes &b) {
		return leveldb::Slice(b.data(), b.size());
	}

	class LVDB_Impl : public LVDB
	{
	private:
		friend class LVDB;
		leveldb::DB* ldb;
		leveldb::Options options;

		LVDB_Impl();

	public:
		Binlog_Queue *binlogs;

		virtual ~LVDB_Impl();

		virtual int release();

		virtual int flushdb();

		// return (start, end], not include start
		virtual Iterator* iterator(const std::string &start, const std::string &end, uint64_t limit);
		virtual Iterator* rev_iterator(const std::string &start, const std::string &end, uint64_t limit);

		//void flushdb();
		virtual uint64_t size();
		virtual std::vector<std::string> info();
		virtual void compact();
		virtual int key_range(std::vector<std::string> *keys);

		/* raw operates */

		// repl: whether to sync this operation to slaves
		virtual int raw_set(const Bytes &key, const Bytes &val);
		virtual int raw_del(const Bytes &key);
		virtual int raw_get(const Bytes &key, std::string *val);

		/* meta operates */
		virtual int meta_set(const Bytes &key, const Bytes &val);
		virtual int meta_get(const Bytes &key, std::string *val);
		virtual int meta_del(const Bytes &key);
		virtual int meta_list(std::vector<std::string> *list);

		/* key value */

		virtual int set(const Bytes &key, const Bytes &val, char log_type = BinlogType::SYNC);
		virtual int setnx(const Bytes &key, const Bytes &val, char log_type = BinlogType::SYNC);
		virtual int del(const Bytes &key, char log_type = BinlogType::SYNC);
		// -1: error, 1: ok, 0: value is not an integer or out of range
		virtual int incr(const Bytes &key, int64_t by, int64_t *new_val, char log_type = BinlogType::SYNC);
		virtual int multi_set(const std::vector<Bytes> &kvs, int offset = 0, char log_type = BinlogType::SYNC);
		virtual int multi_del(const std::vector<Bytes> &keys, int offset = 0, char log_type = BinlogType::SYNC);
		virtual int set_bit(const Bytes &key, int bitoffset, int on, char log_type = BinlogType::SYNC);
		virtual int get_bit(const Bytes &key, int bitoffset);

		virtual int get(const Bytes &key, std::string *val);
		virtual int getset(const Bytes &key, std::string *val, const Bytes &newval, char log_type = BinlogType::SYNC);
		// return (start, end]
		virtual KIterator* scan(const Bytes &start, const Bytes &end, uint64_t limit);
		virtual KIterator* rscan(const Bytes &start, const Bytes &end, uint64_t limit);

		/* hash */

		virtual int hset(const Bytes &name, const Bytes &key, const Bytes &val, char log_type = BinlogType::SYNC);
		virtual int hdel(const Bytes &name, const Bytes &key, char log_type = BinlogType::SYNC);
		// -1: error, 1: ok, 0: value is not an integer or out of range
		virtual int hincr(const Bytes &name, const Bytes &key, int64_t by, int64_t *new_val, char log_type = BinlogType::SYNC);
		//int multi_hset(const Bytes &name, const std::vector<Bytes> &kvs, int offset=0, char log_type=BinlogType::SYNC);
		//int multi_hdel(const Bytes &name, const std::vector<Bytes> &keys, int offset=0, char log_type=BinlogType::SYNC);

		virtual int64_t hsize(const Bytes &name);
		virtual int64_t hclear(const Bytes &name);
		virtual int hget(const Bytes &name, const Bytes &key, std::string *val);
		virtual int hlist(const Bytes &name_s, const Bytes &name_e, uint64_t limit,
			std::vector<std::string> *list);
		virtual int hrlist(const Bytes &name_s, const Bytes &name_e, uint64_t limit,
			std::vector<std::string> *list);
		virtual HIterator* hscan(const Bytes &name, const Bytes &start, const Bytes &end, uint64_t limit);
		virtual HIterator* hrscan(const Bytes &name, const Bytes &start, const Bytes &end, uint64_t limit);

		/* zset */

		virtual int zset(const Bytes &name, const Bytes &key, const Bytes &score, char log_type = BinlogType::SYNC);
		virtual int zdel(const Bytes &name, const Bytes &key, char log_type = BinlogType::SYNC);
		// -1: error, 1: ok, 0: value is not an integer or out of range
		virtual int zincr(const Bytes &name, const Bytes &key, int64_t by, int64_t *new_val, char log_type = BinlogType::SYNC);
		//int multi_zset(const Bytes &name, const std::vector<Bytes> &kvs, int offset=0, char log_type=BinlogType::SYNC);
		//int multi_zdel(const Bytes &name, const std::vector<Bytes> &keys, int offset=0, char log_type=BinlogType::SYNC);

		virtual int64_t zsize(const Bytes &name);
		/**
		 * @return -1: error; 0: not found; 1: found
		 */
		virtual int zget(const Bytes &name, const Bytes &key, std::string *score);
		virtual int64_t zrank(const Bytes &name, const Bytes &key);
		virtual int64_t zrrank(const Bytes &name, const Bytes &key);
		virtual ZIterator* zrange(const Bytes &name, uint64_t offset, uint64_t limit);
		virtual ZIterator* zrrange(const Bytes &name, uint64_t offset, uint64_t limit);
		/**
		 * scan by score, but won't return @key if key.score=score_start.
		 * return (score_start, score_end]
		 */
		virtual ZIterator* zscan(const Bytes &name, const Bytes &key,
			const Bytes &score_start, const Bytes &score_end, uint64_t limit);
		virtual ZIterator* zrscan(const Bytes &name, const Bytes &key,
			const Bytes &score_start, const Bytes &score_end, uint64_t limit);
		virtual int zlist(const Bytes &name_s, const Bytes &name_e, uint64_t limit,
			std::vector<std::string> *list);
		virtual int zrlist(const Bytes &name_s, const Bytes &name_e, uint64_t limit,
			std::vector<std::string> *list);
		virtual int64_t zfix(const Bytes &name);

		virtual int64_t qsize(const Bytes &name);
		// @return 0: empty queue, 1: item peeked, -1: error
		virtual int qfront(const Bytes &name, std::string *item);
		// @return 0: empty queue, 1: item peeked, -1: error
		virtual int qback(const Bytes &name, std::string *item);
		// @return -1: error, other: the new length of the queue
		virtual int64_t qpush_front(const Bytes &name, const Bytes &item, char log_type = BinlogType::SYNC);
		virtual int64_t qpush_back(const Bytes &name, const Bytes &item, char log_type = BinlogType::SYNC);
		// @return 0: empty queue, 1: item popped, -1: error
		virtual int qpop_front(const Bytes &name, std::string *item, char log_type = BinlogType::SYNC);
		virtual int qpop_back(const Bytes &name, std::string *item, char log_type = BinlogType::SYNC);
		virtual int qfix(const Bytes &name);
		virtual int qlist(const Bytes &name_s, const Bytes &name_e, uint64_t limit,
			std::vector<std::string> *list);
		virtual int qrlist(const Bytes &name_s, const Bytes &name_e, uint64_t limit,
			std::vector<std::string> *list);
		virtual int qslice(const Bytes &name, int64_t offset, int64_t limit,
			std::vector<std::string> *list);
		virtual int qget(const Bytes &name, int64_t index, std::string *item);
		virtual int qset(const Bytes &name, int64_t index, const Bytes &item, char log_type = BinlogType::SYNC);
		virtual int qset_by_seq(const Bytes &name, uint64_t seq, const Bytes &item, char log_type = BinlogType::SYNC);

	private:
		int64_t _qpush(const Bytes &name, const Bytes &item, uint64_t front_or_back_seq, char log_type = BinlogType::SYNC);
		int _qpop(const Bytes &name, std::string *item, uint64_t front_or_back_seq, char log_type = BinlogType::SYNC);
	};



}
