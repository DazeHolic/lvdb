#pragma once


#include "lvdb/bytes.h"
#include <string>


namespace leveldb
{
	class Slice;
}


namespace lv
{
	class Binlog
	{
	public:
		Binlog(){}
		Binlog(uint64_t seq, char type, char cmd, const leveldb::Slice &key);

	public:
		int load(const Bytes &s);
		int load(const leveldb::Slice &s);
		int load(const std::string &s);

		uint64_t seq() const;
		char type() const;
		char cmd() const;
		const Bytes key() const;

		const char* data() const{
			return buf.data();
		}
		int size() const{
			return (int)buf.size();
		}
		const std::string& repr() const{
			return this->buf;
		}
		std::string dumps() const;

	private:
		std::string buf;
		static const unsigned int HEADER_LEN = sizeof(uint64_t) + 2;

	};

}
