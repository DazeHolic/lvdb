#pragma  once


#include <inttypes.h>
#include <string>
#include "bytes.h"


namespace leveldb{
	class Iterator;
}

namespace lv
{
	class Iterator{
	public:
		enum Direction{
			FORWARD, BACKWARD
		};
		Iterator(leveldb::Iterator *it,
			const std::string &end,
			uint64_t limit,
			Direction direction = Iterator::FORWARD);
		~Iterator();
		int release();
		bool skip(uint64_t offset);
		bool next();
		Bytes key();
		Bytes val();

	private:
		leveldb::Iterator *it;
		std::string end;
		uint64_t limit;
		bool is_first;
		int direction;
	};


	class KIterator{
	public:
		std::string key;
		std::string val;

		KIterator(Iterator *it);
		~KIterator();
		int release();
		void return_val(bool onoff);
		bool next();
	private:
		Iterator *it;
		bool return_val_;
	};


	class HIterator{
	public:
		std::string name;
		std::string key;
		std::string val;

		HIterator(Iterator *it, const Bytes &name);
		~HIterator();
		int release();
		void return_val(bool onoff);
		bool next();
	private:
		Iterator *it;
		bool return_val_;
	};


	class ZIterator{
	public:
		std::string name;
		std::string key;
		std::string score;

		ZIterator(Iterator *it, const Bytes &name);
		~ZIterator();
		int release();
		bool skip(uint64_t offset);
		bool next();
	private:
		Iterator *it;
	};


}
