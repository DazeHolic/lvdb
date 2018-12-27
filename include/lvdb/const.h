#pragma once


namespace lv
{
	static const int SSDB_SCORE_WIDTH = 9;
	static const int SSDB_KEY_LEN_MAX = 255;

	class DataType{
	public:
		static const char SYNCLOG = 1;
		static const char META = 2;
		static const char KV = 'k';
		static const char HASH = 'h'; // hashmap(sorted by key)
		static const char HSIZE = 'H';
		static const char ZSET = 's'; // key => score
		static const char ZSCORE = 'z'; // key|score => ""
		static const char ZSIZE = 'Z';
		static const char QUEUE = 'q';
		static const char QSIZE = 'Q';
		static const char MIN_PREFIX = HASH;
		static const char MAX_PREFIX = ZSET;
	};

	class KeyType
	{
	public:
		static const char KSTRING = 0;
		static const char KINT8 = 1;
		static const char KINT16 = 2;
		static const char KINT32 = 4;
		static const char KINT64 = 8;
		static const char KCUSTOM = -1;
	};

	class ValueType
	{
	public:
		static const char VBIN = 0;
		static const char VTEXT = 1;
		static const char VJSON = 2;
		static const char VPROTOBUF = 3;
	};


	class BinlogType{
	public:
		static const char NOOP = 0;
		static const char SYNC = 1;
		static const char MIRROR = 2;
		static const char COPY = 3;
		static const char CTRL = 4;
	};

	class BinlogCommand{
	public:
		static const char NONE = 0;

		static const char META_SET = -1;
		static const char META_DEL = -2;

		static const char KSET = 1;
		static const char KDEL = 2;
		static const char HSET = 3;
		static const char HDEL = 4;
		static const char ZSET = 5;
		static const char ZDEL = 6;

		static const char QPUSH_BACK = 10;
		static const char QPUSH_FRONT = 11;
		static const char QPOP_BACK = 12;
		static const char QPOP_FRONT = 13;
		static const char QSET = 14;

		static const char BEGIN = 7;
		static const char END = 8;
	};

}
