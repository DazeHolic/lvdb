#pragma once


#include "toolkits/websocket.h"
#include "lvdb/lvdb.h"
#include <unordered_map>
#include <string>


namespace lv
{

	typedef int(*HTTP_HANDLER)(LVDB*, const std::string& path, toolkit::WebSocket_Server* ws);
	class HTTP_Dispatcher
	{
	public:
		HTTP_Dispatcher(const std::string& prefix);

		bool dispatch(const std::string& path, toolkit::WebSocket_Server* ws);

		void register_db(const std::string& dbname, LVDB* db);
		void register_handler(const std::string& name, HTTP_HANDLER handler);

	private:
		typedef std::unordered_map<std::string, HTTP_HANDLER> HTTP_HANDLER_MAP;
		typedef std::unordered_map<std::string, LVDB*> DB_MAP;
		HTTP_HANDLER_MAP handlers_;
		DB_MAP dbs_;
		std::string prefix_;
	};

}

