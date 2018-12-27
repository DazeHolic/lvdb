#include "lvdb/http.h"
#include "toolkits/string_utils.h"
#include "toolkits/log.h"
#include "pbcpp/dbapi.pb.h"
#include <memory>
#include <unordered_map>
#include <string>



namespace
{
	static const std::string key_dbname = "db";
	static const std::string key_name = "name";
	static const std::string key_key = "key";
	static const std::string key_key1 = "key";
	static const std::string key_key2 = "key2";
	static const std::string key_key3 = "key3";
	static const std::string key_limit = "limit";


	static const std::string prefix_string = "=";
	static const std::string prefix_i8 = "i8=";
	static const std::string prefix_u8 = "u8=";
	static const std::string prefix_i16 = "i16=";
	static const std::string prefix_u16 = "u16=";
	static const std::string prefix_i32 = "i32=";
	static const std::string prefix_u32 = "u32=";
	static const std::string prefix_i64 = "i64=";
	static const std::string prefix_u64 = "u64=";

	static const std::string error_ok = "ok";
	static const std::string error_unknown = "unknown error";
	static const std::string error_db_not_exist = "db not exist";
	static const std::string error_query_arg = "miss one or more uri arg";
	static const std::string error_not_impl = "Not Implemented";
	static const std::string error_not_found = "key not found";
	static const std::string error_already_exist = "key already exist";

	static const std::string pb_content_type = "application/x-protobuf";

	lv::Bytes* get_key_query_arg(toolkit::WebSocket* ws, const std::string& key_prefix)
	{
		std::string value;
		if (ws->query_arg_prefix(key_prefix, &value) <= 0)
		{ 
			return NULL; 
		}
		
#define KEY_ARG(type, ret)  if(toolkit::starts_with(value.c_str(), prefix_ ##type.c_str())) { ret; }
		KEY_ARG(string, return new lv::Bytes_string(value.c_str() + 1));
		KEY_ARG(i32, return new lv::Bytes_int32(atoi(value.c_str() + 4)));
		KEY_ARG(u32, return new lv::Bytes_uint32(atoi(value.c_str() + 4)));
		KEY_ARG(i64, return new lv::Bytes_int64(atoll(value.c_str() + 4)));
		KEY_ARG(u64, return new lv::Bytes_uint64(atoll(value.c_str() + 4)));
		KEY_ARG(i16, return new lv::Bytes_int16(atoi(value.c_str() + 4)));
		KEY_ARG(u16, return new lv::Bytes_uint16(atoi(value.c_str() + 4)));
		KEY_ARG(i8, return new lv::Bytes_int8(atoi(value.c_str() + 3)));
		KEY_ARG(u8, return new lv::Bytes_uint8(atoi(value.c_str() + 3)));
		return NULL;
#undef KEY_ARG
	}
}

#define HTTP_RESPONSE(status, content) ws->http_response(404, content.c_str(), content.length() ) 

#define HTTP_RESPONSE_OK HTTP_RESPONSE(200, error_ok)
#define HTTP_RESPONSE_UNKNOW HTTP_RESPONSE(500, error_unknown)
#define HTTP_RESPONSE_NOT_IMPL HTTP_RESPONSE(501, error_not_impl)
#define HTTP_RESPONSE_URI_ARG HTTP_RESPONSE(400, error_query_arg)
#define HTTP_RESPONSE_NOT_FOUND HTTP_RESPONSE(404, error_not_found)
#define HTTP_RESPONSE_ALREADY_EXIST HTTP_RESPONSE(404, error_already_exist)

#define DEF_PROC(f) int http_##f(LVDB* db, const std::string& path, toolkit::WebSocket_Server* ws)
#define URI_ARG(n)  std::string n; if(ws->query_arg(key_ ##n, &n) <= 0) { LOG_WARN("unexist uri arg: "#n); HTTP_RESPONSE_URI_ARG; return -2; }
#define URI_ARG_DEFAULT(n, d)  std::string n; if(ws->query_arg(key_ ##n, &n) <= 0) { n = d; }

#define KEY_ARG_NTH(N) std::unique_ptr<lv::Bytes> key##N(get_key_query_arg(ws, key_key ##N)); if(key##N.get() == NULL) { LOG_WARN("uri arg key" #N " unexist"); HTTP_RESPONSE_URI_ARG; return -3;}
#define KEY_ARG std::unique_ptr<lv::Bytes> key(get_key_query_arg(ws, key_key)); if(key.get() == NULL) { LOG_WARN("uri arg key unexist"); HTTP_RESPONSE_URI_ARG; return -3;}
#define LIMIT_ARG uint64_t limit(UINT64_MAX); { std::string limit_string__; if(ws->query_arg(key_limit, &limit_string__) > 0) { limit = atoll(limit_string__.c_str()); } }



namespace lv
{

	DEF_PROC(flushdb);
	DEF_PROC(size);
	DEF_PROC(info);
	DEF_PROC(compact);
	DEF_PROC(keyrange);

	//meta
	DEF_PROC(metaget);
	DEF_PROC(metaset);
	DEF_PROC(metadel);
	DEF_PROC(metalist);

	//kv
	DEF_PROC(get);
	DEF_PROC(getset);
	DEF_PROC(set);
	DEF_PROC(setnx);
	DEF_PROC(del);
	DEF_PROC(scan);
	DEF_PROC(rscan);


	//hash
	DEF_PROC(hsize);
	DEF_PROC(hset);
	DEF_PROC(hdel);
	DEF_PROC(hclear);
	DEF_PROC(hget);
	DEF_PROC(hscan);
	DEF_PROC(hrscan);

	bool HTTP_Dispatcher::dispatch(const std::string& path, toolkit::WebSocket_Server* ws)
	{
		HTTP_HANDLER_MAP::iterator it = handlers_.find(path);
		if (it != handlers_.end()) {
			URI_ARG(dbname);
			DB_MAP::iterator itdb = dbs_.find(dbname);
			if (itdb == dbs_.end()) {
				LOG_WARN("unknown db: " << dbname);
				ws->http_response(404, error_db_not_exist.c_str(), error_db_not_exist.length());
				return -3;
			}
			LVDB *db = itdb->second;
			it->second(db, path, ws);
			return true;
		}
		else {
			if (toolkit::starts_with(path.c_str(), prefix_.c_str())) {
				HTTP_RESPONSE_NOT_IMPL;
				return true;
			}
		}
		return false;
	}

	HTTP_Dispatcher::HTTP_Dispatcher(const std::string& prefix) :
		prefix_(prefix)
	{
#pragma push_macro("DEF_PROC")
#undef DEF_PROC
#define DEF_PROC(NAME) handlers_[prefix + #NAME] = http_##NAME

		DEF_PROC(flushdb);
		DEF_PROC(size);
		DEF_PROC(info);
		DEF_PROC(compact);
		DEF_PROC(keyrange);

		//meta
		DEF_PROC(metaget);
		DEF_PROC(metaset);
		DEF_PROC(metadel);
		DEF_PROC(metalist);

		//kv
		DEF_PROC(get);
		DEF_PROC(getset);
		DEF_PROC(set);
		DEF_PROC(setnx);
		DEF_PROC(del);
		DEF_PROC(scan);
		DEF_PROC(rscan);


		//hash
		DEF_PROC(hsize);
		DEF_PROC(hset);
		DEF_PROC(hdel);
		DEF_PROC(hclear);
		DEF_PROC(hget);
		DEF_PROC(hscan);
		DEF_PROC(hrscan);

#undef DEF_PROC
#pragma pop_macro("DEF_PROC")

	}

	void HTTP_Dispatcher::register_db(const std::string& dbname, LVDB* db)
	{
		dbs_[dbname] = db;
	}

	void HTTP_Dispatcher::register_handler(const std::string& name, HTTP_HANDLER handler)
	{
		handlers_[prefix_ + name] = handler;
	}





	//////////////////////////////////////////////////////////////////////////
	DEF_PROC(flushdb)
	{
		HTTP_RESPONSE_NOT_IMPL;
		return 0;
	}
	DEF_PROC(size)
	{
		HTTP_RESPONSE_NOT_IMPL;
		return 0;
	}

	DEF_PROC(info)
	{
		std::vector<std::string> vs = db->info();
		if (vs.empty()) {
			ws->http_response(200, "", 0);
		}
		else {
			iovec *vec = STACK_ARRAY(iovec, vs.size());
			for (int i = 0; i < vs.size(); ++i) {
				vec[i].iov_base = (void*)vs[i].c_str();
				vec[i].iov_len = vs[i].length();
			}
			ws->http_responsev(200, vec, vs.size());
		}
		return 0;
	}

	DEF_PROC(compact)
	{
		HTTP_RESPONSE_NOT_IMPL;
		return 0;
	}

	DEF_PROC(keyrange)
	{
		std::vector<std::string> vs;
		db->key_range(&vs);
		iovec *vec = STACK_ARRAY(iovec, vs.size() * 2);
		static const std::string newline = "\n";
		for (int i = 0; i < vs.size(); ++i) {
			vec[2 * i].iov_base = (void*)vs[i].c_str();
			vec[2 * i].iov_len = vs[i].length();
			vec[2 * i + 1].iov_base = (void*)newline.c_str();
			vec[2 * i + 1].iov_len = 1;
		}
		ws->http_responsev(200, vec, vs.size() * 2);
		return 0;
	}




	
	//////////////////////////////////////////////////////////////////////////
	//meta
	DEF_PROC(metaget)
	{
		HTTP_RESPONSE_NOT_IMPL;
		return 0;
	}

	DEF_PROC(metaset)
	{
		HTTP_RESPONSE_NOT_IMPL;
		return 0;
	}

	DEF_PROC(metadel)
	{
		HTTP_RESPONSE_NOT_IMPL;
		return 0;
	}

	DEF_PROC(metalist)
	{
		HTTP_RESPONSE_NOT_IMPL;
		return 0;
	}


#define PB_RESPONSE(S, PB) \
		{\
	std::string pb; \
	res.SerializeToString(&pb); \
	ws->http_response(S, pb.c_str(), pb.length(), pb_content_type.c_str()); \
	}


	//////////////////////////////////////////////////////////////////////////
	//kv
	DEF_PROC(get)
	{
		KEY_ARG;
		std::string value;
		int ret = db->get(*key, &value);
		dbapipb::Response res;
		res.set_errcode(ret);
		if (ret == 1) {
			res.set_success(true);
			dbapipb::KV* kv = res.add_values();
			kv->set_key(key->data(), key->size());
			kv->set_value(value);
			PB_RESPONSE(200, res);
		}
		else
		{
			res.set_success(false);
			PB_RESPONSE(404, res);
		}
		return 0;
	}

	DEF_PROC(getset)
	{
		KEY_ARG;
		std::string value;
		Bytes newVal(ws->http_body(), ws->http_body_length());
		int ret = db->getset(*key, &value, newVal); 
		dbapipb::Response res;
		res.set_errcode(ret);
		switch (ret)
		{
		case 0:
			res.set_success(true);
			PB_RESPONSE(200, res);
			break;

		case 1:
		{
			res.set_success(true);
			dbapipb::KV* kv = res.add_values();
			kv->set_key(key->data(), key->size());
			kv->set_value(value);
			PB_RESPONSE(201, res);
			break;
		}

		default:
			res.set_success(false);
			PB_RESPONSE(404, res);
			break;
		}
		return 0;
	}

	DEF_PROC(set)
	{
		KEY_ARG;
		Bytes newVal(ws->http_body(), ws->http_body_length());
		switch (db->set(*key, newVal))
		{
		case 1:
			HTTP_RESPONSE_OK;
			break;

		default:
			HTTP_RESPONSE_NOT_FOUND;
			break;
		}
		return 0;
	}

	DEF_PROC(setnx)
	{
		KEY_ARG;
		Bytes newVal(ws->http_body(), ws->http_body_length());
		switch (db->setnx(*key, newVal))
		{
		case 0:
			HTTP_RESPONSE_ALREADY_EXIST;
			break;

		case 1:
			HTTP_RESPONSE_OK;
			break;

		default:
			HTTP_RESPONSE_UNKNOW;
			break;
		}
		return 0;
	}

	DEF_PROC(del)
	{
		KEY_ARG;
		switch (db->del(*key))
		{
		case 1:
			HTTP_RESPONSE_OK;
			break;

		default:
			HTTP_RESPONSE_UNKNOW;
			break;
		}		
		return 0;
	}

	DEF_PROC(scan)
	{
		KEY_ARG_NTH(1);
		KEY_ARG_NTH(2);
		LIMIT_ARG;
		KIterator *it = db->scan(*key1, *key2, limit);
		std::vector<std::string> vs;
		while (it->next()){
			vs.push_back(it->key);
			vs.push_back(it->val);
		}
		delete it;

		iovec *vec = STACK_ARRAY(iovec, vs.size() * 2);
		static const std::string newline = "\n";
		for (int i = 0; i < vs.size(); ++i) {
			vec[2 * i].iov_base = (void*)vs[i].c_str();
			vec[2 * i].iov_len = vs[i].length();
			vec[2 * i + 1].iov_base = (void*)newline.c_str();
			vec[2 * i + 1].iov_len = 1;
		}
		ws->http_responsev(200, vec, vs.size() * 2);

		return 0;
	}

	DEF_PROC(rscan)
	{
		KEY_ARG_NTH(1);
		KEY_ARG_NTH(2);
		LIMIT_ARG;
		KIterator *it = db->rscan(*key1, *key2, limit);
		std::vector<std::string> vs;
		while (it->next()){
			vs.push_back(it->key);
			vs.push_back(it->val);
		}
		delete it;

		iovec *vec = STACK_ARRAY(iovec, vs.size() * 2);
		static const std::string newline = "\n";
		for (int i = 0; i < vs.size(); ++i) {
			vec[2 * i].iov_base = (void*)vs[i].c_str();
			vec[2 * i].iov_len = vs[i].length();
			vec[2 * i + 1].iov_base = (void*)newline.c_str();
			vec[2 * i + 1].iov_len = 1;
		}
		ws->http_responsev(200, vec, vs.size() * 2);

		return 0;
	}



	//////////////////////////////////////////////////////////////////////////
	//hash
	DEF_PROC(hsize)
	{
		URI_ARG(name);
		std::string s = str(db->hsize(name));
		HTTP_RESPONSE(200, s);
		return 0;
	}


	DEF_PROC(hset)
	{
		URI_ARG(name);
		KEY_ARG;
		lv::Bytes val(ws->http_body(), ws->http_body_length());
		if (db->hset(name, *key, val) < 0) {
			ws->http_response(404, NULL, 0);
		}
		else {
			ws->http_response(200, "ok", 2);
		}
		return 0;
	}

	DEF_PROC(hdel)
	{
		URI_ARG(name);
		KEY_ARG;
		int ret = db->hdel(name, *key);
		if (ret == 1) {
			HTTP_RESPONSE_OK;
		}
		else
		{
			HTTP_RESPONSE_NOT_FOUND;
		}
		return 0;
	}

	DEF_PROC(hclear)
	{
		URI_ARG(name);
		std::string ret = str(db->hclear(name));
		HTTP_RESPONSE(200, ret);
		return 0;
	}

	DEF_PROC(hget)
	{
		URI_ARG(name);
		KEY_ARG;
		std::string value;
		if (db->hget(name, *key, &value) > 0) {
			ws->http_response(200, value.c_str(), value.length(), "application/octet-stream");
		}
		else {
			HTTP_RESPONSE_NOT_FOUND;
		}
		return 0;
	}

	DEF_PROC(hscan)
	{
		URI_ARG(name);
		KEY_ARG_NTH(1);
		KEY_ARG_NTH(2);
		LIMIT_ARG;
		HIterator *it = db->hscan(name, *key1, *key2, limit);
		std::vector<std::string> vs;
		while (it->next()){
			vs.push_back(it->key);
			vs.push_back(it->val);
		}
		delete it;

		iovec *vec = STACK_ARRAY(iovec, vs.size() * 2);
		static const std::string newline = "\n";
		for (int i = 0; i < vs.size(); ++i) {
			vec[2 * i].iov_base = (void*)vs[i].c_str();
			vec[2 * i].iov_len = vs[i].length();
			vec[2 * i + 1].iov_base = (void*)newline.c_str();
			vec[2 * i + 1].iov_len = 1;
		}
		ws->http_responsev(200, vec, vs.size() * 2);
		return 0;
	}

	DEF_PROC(hrscan)
	{
		URI_ARG(name);
		KEY_ARG_NTH(1);
		KEY_ARG_NTH(2);
		LIMIT_ARG;
		HIterator *it = db->hrscan(name, *key1, *key2, limit);
		std::vector<std::string> vs;
		while (it->next()){
			vs.push_back(it->key);
			vs.push_back(it->val);
		}
		delete it;

		iovec *vec = STACK_ARRAY(iovec, vs.size() * 2);
		static const std::string newline = "\n";
		for (int i = 0; i < vs.size(); ++i) {
			vec[2 * i].iov_base = (void*)vs[i].c_str();
			vec[2 * i].iov_len = vs[i].length();
			vec[2 * i + 1].iov_base = (void*)newline.c_str();
			vec[2 * i + 1].iov_len = 1;
		}
		ws->http_responsev(200, vec, vs.size() * 2);
		return 0;
	}




}
