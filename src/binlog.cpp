/*
Copyright (c) 2012-2014 The SSDB Authors. All rights reserved.
Use of this source code is governed by a BSD-style license that can be
found in the LICENSE file.
*/
#include "lvdb/binlog.h"
#include "lvdb/const.h"
#include "lvdb/strings.h"
#include "toolkits/log.h"
#include "toolkits/util.h"
#include "leveldb/slice.h"
#include "pthread.h"
#include <map>


#ifdef WIN32
#define strerror(x) ""
#endif


namespace lv
{
	Binlog::Binlog(uint64_t seq, char type, char cmd, const leveldb::Slice &key)
	{
		buf.append((char *)(&seq), sizeof(uint64_t));
		buf.push_back(type);
		buf.push_back(cmd);
		buf.append(key.data(), key.size());
	}

	uint64_t Binlog::seq() const{
		return *((uint64_t *)(buf.data()));
	}

	char Binlog::type() const{
		return buf[sizeof(uint64_t)];
	}

	char Binlog::cmd() const{
		return buf[sizeof(uint64_t) + 1];
	}

	const Bytes Binlog::key() const{
		return Bytes(buf.data() + HEADER_LEN, buf.size() - HEADER_LEN);
	}

	int Binlog::load(const Bytes &s){
		if (s.size() < HEADER_LEN){
			return -1;
		}
		buf.assign(s.data(), s.size());
		return 0;
	}

	int Binlog::load(const leveldb::Slice &s){
		if (s.size() < HEADER_LEN){
			return -1;
		}
		buf.assign(s.data(), s.size());
		return 0;
	}

	int Binlog::load(const std::string &s){
		if (s.size() < HEADER_LEN){
			return -1;
		}
		buf.assign(s.data(), s.size());
		return 0;
	}

	std::string Binlog::dumps() const{
		std::string str;
		if (buf.size() < HEADER_LEN){
			return str;
		}
		char buf[20];
		snprintf(buf, sizeof(buf), "%" PRIu64 " ", this->seq());
		str.append(buf);

		switch (this->type()){
		case BinlogType::NOOP:
			str.append("noop ");
			break;
		case BinlogType::SYNC:
			str.append("sync ");
			break;
		case BinlogType::MIRROR:
			str.append("mirror ");
			break;
		case BinlogType::COPY:
			str.append("copy ");
			break;
		case BinlogType::CTRL:
			str.append("control ");
			break;
		}
		switch (this->cmd()){
		case BinlogCommand::NONE:
			str.append("none ");
			break;
		case BinlogCommand::KSET:
			str.append("set ");
			break;
		case BinlogCommand::KDEL:
			str.append("del ");
			break;
		case BinlogCommand::HSET:
			str.append("hset ");
			break;
		case BinlogCommand::HDEL:
			str.append("hdel ");
			break;
		case BinlogCommand::ZSET:
			str.append("zset ");
			break;
		case BinlogCommand::ZDEL:
			str.append("zdel ");
			break;
		case BinlogCommand::BEGIN:
			str.append("begin ");
			break;
		case BinlogCommand::END:
			str.append("end ");
			break;
		case BinlogCommand::QPUSH_BACK:
			str.append("qpush_back ");
			break;
		case BinlogCommand::QPUSH_FRONT:
			str.append("qpush_front ");
			break;
		case BinlogCommand::QPOP_BACK:
			str.append("qpop_back ");
			break;
		case BinlogCommand::QPOP_FRONT:
			str.append("qpop_front ");
			break;
		case BinlogCommand::QSET:
			str.append("qset ");
			break;
		}
		Bytes b = this->key();
		str.append(hexmem(b.data(), b.size()));
		return str;
	}


}

