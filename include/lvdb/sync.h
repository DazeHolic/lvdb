#pragma once


#include "lvdb.h"
#include "lvdb/binlog.h"
#include "toolkits/thread.h"


namespace lv
{
	class Sync;
	class Copy;
	class Sync_Processor;



	//////////////////////////////////////////////////////////////////////////
	class Sync : public toolkit::Thread
	{
	public:
		Sync(const std::string& name, LVDB* db, Sync_Processor*);

	private:
		virtual int svc();

	private:
		std::string name_;
		LVDB* db_;
		Sync_Processor *sync_;
	};



	//////////////////////////////////////////////////////////////////////////
	class Copy : public toolkit::Thread
	{
	public:
		Copy(const std::string& name, LVDB* db, Sync_Processor*);

	private:
		virtual int svc();

	private:
		std::string name_;
		LVDB* db_;
		Sync_Processor *sync_;
	};



	//////////////////////////////////////////////////////////////////////////
	class Sync_Processor
	{
	public:
		virtual int do_sync(Binlog& log, const char* val, int len) = 0;
	};



	//////////////////////////////////////////////////////////////////////////
	class Null_Sync_Processor : public Sync_Processor
	{
	public:
		virtual int do_sync(Binlog& log, const char* val, int len)
		{
			return 0;
		}
	};



	template<typename LINK>
	class Backup_Client_Processor : public Sync_Processor
	{
	public:
		Backup_Client_Processor(LINK *link) : link_(link) {}

		virtual int do_sync(Binlog& log, const char* val, int len)
		{
			return link_->send_lvdb_sync(log.repr(), val);
		}

	private:
		LINK *link_;
	};



	//////////////////////////////////////////////////////////////////////////
	class Backup_Server_Processor : public Sync_Processor
	{
	public:
		Backup_Server_Processor(LVDB* db);

		virtual int do_sync(Binlog& log, const char* val, int len);

	protected:
		LVDB* db_;
	};


}
