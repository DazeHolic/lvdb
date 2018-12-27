#pragma once


#include "lvdb/bytes.h"
#include "lvdb/lvdb.h"
#include "toolkits/scoped_ptr.h"
#include "toolkits/binary.h"
#include "toolkits/string_piece.h"
#include <list>


namespace lv
{
	class Auto_Save_Object;
	class Auto_Save_Manager;


	class Auto_Save_Manager
	{
	public:
		Auto_Save_Manager();

		void add(Auto_Save_Object* obj);

		void tick(int t);

	private:
		void* timers;
	};


	template<typename PB>
	static inline int encode_pb_T(toolkit::Binary& buf, const PB* pb) {
		//message type name as cstring format
		buf.encode_cstring(pb->GetTypeName());
		//pb content as string format
		int byte_size = pb->ByteSize();
		buf.ensure_space(byte_size);
		//encode pb content
		uint8_t* start = reinterpret_cast<uint8_t*>(buf.wr_ptr());
		uint8_t* end = pb->SerializeWithCachedSizesToArray(start);
		buf.wr_ptr((int)(end - start));
		return byte_size;
	}

	template<typename PB>
	static inline bool decode_pb_T(PB* pb, const char* pb_ptr, int pb_len) {
		toolkit::Binary buf(pb_ptr, pb_len, false);
		toolkit::String_Piece type_name;
		buf.decode_cstring(type_name);
		return pb->ParseFromArray(buf.rd_ptr(), buf.length() );
	}


	class Auto_Save_Object
	{
	public:
		Auto_Save_Object();
		virtual ~Auto_Save_Object();

		inline bool dirty() const {
			return dirty_;
		}

		inline void dirty(bool d) {
			if (d && am_ && !saving_) {
				saving_ = true;
				am_->add(this);
			}
			dirty_ = d;
		}

		virtual bool load() { return false; }
		void save() {
			if (dirty_) {
				saving_ = false;
				do_save();
			}
		}
		virtual void do_save() {};

		inline const void* timer() const {
			return timer_;
		}

		int priority() const {
			return priority_;
		}

		void priority(int p) {
			priority_ = p;
		}

		void auto_save(Auto_Save_Manager* am, int p) {
			am_ = am;
			priority_ = p;
		}

	private:
		Auto_Save_Manager* am_;
		int priority_;
		bool dirty_;
		bool saving_;
		void* timer_;
	};


	template<typename SCOPE, typename OBJ>
	class Auto_Save_Queue : public Auto_Save_Object
	{
	public:
		Auto_Save_Queue(LVDB*& db, const Bytes& name, int max_save_per_tick = 5) :
			db_(db), name_(name), max_save_per_tick_(max_save_per_tick) {}

		void push(OBJ* obj) {
			objs_.push_back(obj);
			dirty(true);
		}

		virtual void do_save() {
			if (db_) {
				int c = max_save_per_tick_;
				while (!objs_.empty())
				{
					SCOPE obj(objs_.front());
					db_->qpush_back(name_, Bytes_Local_T<OBJ>(obj));
					objs_.pop_front();
					if (--c < 0) break;
				}
				if (!objs_.empty())
					dirty(true);
			}
		}

	private:
		typedef std::list<OBJ> OBJ_LIST;
		LVDB*& db_;
		Bytes name_;
		OBJ_LIST objs_;
		int	max_save_per_tick_;
	};


	template<typename SCOPE_PB, typename PB>
	class Auto_Save_PB_Queue : public Auto_Save_Object
	{
	public:
		Auto_Save_PB_Queue(LVDB*& db, const Bytes& name, int max_save_per_tick = 5) :
			db_(db), name_(name), max_save_per_tick_(max_save_per_tick) {}

		void push(PB* pb) {
			pbs_.push_back(pb);
			dirty(true);
		}

		virtual void do_save() {
			if (db_) {
				int c = max_save_per_tick_;
				while (!pbs_.empty())
				{
					SCOPE_PB pb(pbs_.front());
					pbs_.pop_front();
					toolkit::Binary buf;
					encode_pb_T(buf, pb.get());
					db_->qpush_back(name_, Bytes(buf ) );
					if (--c < 0) break;
				}
				if (!pbs_.empty())
					dirty(true);
			}
		}

	private:
		typedef std::list<PB*> PB_LIST;
		LVDB*& db_;
		Bytes name_;
		PB_LIST pbs_;
		int	max_save_per_tick_;
	};



	template<typename OBJ>
	class Auto_Save_Object_T :
		public Auto_Save_Object,
		public toolkit::scoped_ptr<OBJ>
	{
	public:
		Auto_Save_Object_T() {}
		Auto_Save_Object_T(OBJ* obj) : toolkit::scoped_ptr<OBJ>(obj) {}

		inline OBJ* edit() {
			dirty(true);
			return toolkit::scoped_ptr<OBJ>::get();
		}
	};


	//
	//value
	template<typename KEY, typename VALUE>
	class Auto_Save_KV_Value_T :
		public Auto_Save_Object_T<VALUE>
	{
	public:
		Auto_Save_KV_Value_T(LVDB*& db, const KEY& key) :
			Auto_Save_Object_T<VALUE>(&value_),
			db_(db),
			key_(key)
		{ }

		virtual bool load() {
			if (db_) {
				std::string buf;
				if (db_->get(key_, &buf) == 1) {
					Bytes_Local_T<VALUE>::FromString(value_, buf);
					return true;
				}
			}
			return false;
		}

		virtual void do_save() {
			if (db_) {
				db_->set(key_, Bytes_Local_T<VALUE>(value_));
			}
		}

	private:
		LVDB*& db_;
		Bytes_Local_T<KEY> key_;
		VALUE value_;
	};



	template<typename KEY, typename VALUE>
	class Auto_Save_Hash_Value_T :
		public Auto_Save_Object_T<VALUE>
	{
	public:
		Auto_Save_Hash_Value_T(LVDB*& db, const Bytes& name, const KEY& key)
			: Auto_Save_Object_T<VALUE>(&value_),
			db_(db),
			name_(name),
			key_(key)
		{ }

		virtual bool load() {
			if (db_) {
				std::string buf;
				if (db_->hget(name_, key_, &buf) == 1) {
					Bytes_Local_T<VALUE>::FromString(value_, buf);
					return true;
				}
			}
			return false;
		}

		virtual void do_save() {
			if (db_) {
				db_->hset(name_, key_, Bytes_Local_T<VALUE>(value_));
			}
		}

	private:
		LVDB*& db_;
		Bytes name_;
		Bytes_Local_T<KEY> key_;
		VALUE value_;
	};


	//string


	//protobuf
	template<typename KEY, typename OBJ>
	class Auto_Save_KV_PB_T :
		public Auto_Save_Object_T<OBJ>
	{
	public:
		Auto_Save_KV_PB_T(LVDB*& db, const KEY& key, OBJ* obj) :
			Auto_Save_Object_T<OBJ>(obj),
			db_(db),
			key_(key)
		{ }

		virtual bool load() {
			if (db_) {
				std::string buf;
				if (db_->get(key_, &buf) == 1) {
					return decode_pb_T(Auto_Save_Object_T<OBJ>::get(), buf.c_str(), buf.length());
				}
			}
			return false;
		}

		virtual void do_save() {
			if (db_) {
				toolkit::Binary buf;
				encode_pb_T(buf, Auto_Save_Object_T<OBJ>::get());
				db_->set(key_, Bytes(buf) );
			}
		}

	private:
		LVDB*& db_;
		Bytes_Local_T<KEY> key_;
	};



	template<typename KEY, typename OBJ>
	class Auto_Save_Hash_PB_T :
		public Auto_Save_Object_T<OBJ>
	{
	public:
		Auto_Save_Hash_PB_T(LVDB*& db, const Bytes& name, const KEY& key, OBJ* obj)
			: Auto_Save_Object_T<OBJ>(obj),
			db_(db),
			name_(name),
			key_(key)
		{ }

		virtual bool load() {
			if (db_) {
				std::string buf;
				if (db_->hget(name_, key_, &buf) == 1) {
					return decode_pb_T(Auto_Save_Object_T<OBJ>::get(), buf.c_str(), buf.length());
				}
			}
			return false;
		}

		virtual void do_save() {
			if (db_) {
				toolkit::Binary buf;
				encode_pb_T(buf, Auto_Save_Object_T<OBJ>::get());
				db_->hset(name_, key_, Bytes(buf) );
			}
		}

	private:
		LVDB*& db_;
		Bytes name_;
		Bytes_Local_T<KEY> key_;
	};

}
