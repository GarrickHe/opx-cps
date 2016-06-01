/*
 * cps_redis.cpp
 *
 *  Created on: Apr 5, 2016
 *      Author: cwichmann
 */


#include "cps_api_object.h"
#include "cps_api_key.h"
#include "cps_api_object_key.h"

#include "cps_api_db.h"
#include "cps_api_db_response.h"
#include "cps_string_utils.h"

#include "cps_api_vector_utils.h"

#include <vector>
#include <functional>
#include <mutex>

#include <hiredis/hiredis.h>


static std::mutex _mutex;

bool cps_db::get_sequence(cps_db::connection &conn, std::vector<char> &key, ssize_t &cntr) {
	cps_db::connection::db_operation_atom_t e[2];
	e[0].from_string("INCR");
	e[1].from_string(&key[0],key.size());

	response_set resp;

	if (!conn.sync_operation(e,sizeof(e)/sizeof(*e),resp.get())) {
		return false;
	}

	if (resp.get().size()==1) {
		cps_db::response r = resp.get_response(0);
		if (r.is_int()) {
			cntr = (r.get_int());
			return true;
		}
		return false;
	}
	return false;
}

bool cps_db::fetch_all_keys(cps_db::connection &conn, const void *filt, size_t flen,
		const std::function<void(const void *key, size_t klen)> &fun) {

	cps_db::connection::db_operation_atom_t e[2];
	e[0].from_string("KEYS");
	e[1].from_string((const char *)filt,flen);

	response_set resp;
	if (!conn.sync_operation(e,sizeof(e)/sizeof(*e),resp)) {
		return false;
	}

	for ( auto it : resp.get()) {
		redisReply *p = (redisReply*) it;
		cps_db::response resp(p);
		resp.iterator([&fun](size_t ix, int type, const void *data, size_t len){ fun(data,len);});
	}

	return true;
}

bool cps_db::ping(cps_db::connection &conn) {
	cps_db::connection::db_operation_atom_t e;
	e.from_string("PING");
	response_set resp;

	if (!conn.sync_operation(&e,1,resp)) {
		return false;
	}
	bool rc = false;
	if (resp.size()>0) {
		cps_db::response r = resp.get_response(0);
		if (r.is_ok()) {
			rc = strcasecmp((const char *)r.get_str(),"PONG")==0;
		}
	}
	return rc;
}

bool cps_db::select_db(cps_db::connection &conn,const std::string &db_id) {
	cps_db::connection::db_operation_atom_t e[2];
	e[0].from_string("SELECT");
	e[1].from_string(db_id.c_str());

	response_set resp;
	if (!conn.sync_operation(e,2,resp.get())) {
		return false;
	}
	bool rc = false;
	if (resp.get().size()> 0) {
		cps_db::response r = resp.get_response(0);
		rc = (r.is_status() && strcmp("OK",r.get_str())==0);
	}
	return rc;
}

bool cps_db::multi_start(cps_db::connection &conn) {
	cps_db::connection::db_operation_atom_t e;
	e.from_string("MULTI");

	response_set resp;
	if (!conn.sync_operation(&e,1,resp.get())) {
		return false;
	}
	bool rc = false;
	if (resp.get().size()> 0) {
		cps_db::response r = resp.get_response(0);
		rc = (r.is_status() && strcmp("OK",r.get_str()))==0;
	}
	return rc;
}

bool cps_db::multi_end(cps_db::connection &conn, bool commit) {
	cps_db::connection::db_operation_atom_t e;
	e.from_string(commit ? "EXEC" : "DISCARD");

	response_set resp;
	if (!conn.sync_operation(&e,1,resp)) {
		return false;
	}
	bool rc = false;

	if (resp.get().size()> 0) {
		rc = true;
		cps_db::response r = resp.get_response(0);

		r.iterator([&rc](size_t ix, int type, const void *data, size_t len){
			if (type!=REDIS_REPLY_INTEGER) rc = false;
		});
	}
	return rc;
}

bool cps_db::delete_object(cps_db::connection &conn,std::vector<char> &key) {
	cps_db::connection::db_operation_atom_t e[2];
	e[0].from_string("DEL");
	e[1].from_string(&key[0],key.size());

	response_set resp;
	if (!conn.sync_operation(e,2,resp)) {
		return false;
	}
	//throw out error
	return true;
}

bool cps_db::delete_object(cps_db::connection &conn,cps_api_object_t obj) {
	cps_db::connection::db_operation_atom_t e[2];
	e[0].from_string("DEL");
	e[1]._atom_type = cps_db::connection::db_operation_atom_t::obj_fields_t::obj_field_OBJ_INSTANCE;
	e[1]._object = obj;

	response_set resp;
	if (!conn.sync_operation(e,2,resp)) {
		return false;
	}

	return true;
}

namespace {
bool op_on_objects(const char *op, cps_db::connection &conn,cps_api_object_list_t objs) {
    size_t ix = 0;
    size_t mx = cps_api_object_list_size(objs);
    for ( ; ix < mx ; ++ix ) {
    	cps_db::connection::db_operation_atom_t e[2];
    	e[0].from_string(op);
    	if (strcmp(op,"DEL")==0) {
    		e[1]._atom_type = cps_db::connection::db_operation_atom_t::obj_fields_t::obj_field_OBJ_INSTANCE;
    	} else {
    		e[1]._atom_type = cps_db::connection::db_operation_atom_t::obj_fields_t::obj_field_OBJ_KEY_AND_DATA;
    	}
    	e[1]._object = cps_api_object_list_get(objs,ix);;
    	if (!conn.operation(e,2)==cps_api_ret_code_OK) return false;
    }

    cps_db::response_set set;
	bool rc = (conn.response(set));	//bulk update

	if (!rc) return false;
	if (set.size()!=mx) return false;
	for ( auto it : set.get()) {
		cps_db::response r(it);
		if (r.is_ok()) continue;
		return false;
	}
	return true;
}
}

bool cps_db::delete_objects(cps_db::connection &conn,cps_api_object_list_t objs) {
	return op_on_objects("DEL",conn,objs);
}

bool cps_db::store_objects(cps_db::connection &conn,cps_api_object_list_t objs) {
	return op_on_objects("HSET",conn,objs);
}

bool cps_db::subscribe(cps_db::connection &conn, cps_api_object_t obj) {
	std::vector<char> key;
	if (!cps_db::dbkey_from_instance_key(key,obj)) return false;
	return subscribe(conn,key);
}

bool cps_db::publish(cps_db::connection &conn, cps_api_object_t obj) {
	cps_db::connection::db_operation_atom_t e[3];
	e[0].from_string("PUBLISH");
	e[1]._atom_type = cps_db::connection::db_operation_atom_t::obj_fields_t::obj_field_OBJ_INSTANCE;
	e[1]._object = obj;
	e[2]._atom_type = cps_db::connection::db_operation_atom_t::obj_fields_t::obj_field_OBJ_DATA;
	e[2]._object = obj;

	response_set resp;
	if (!conn.sync_operation(e,3,resp)) {
		return false;
	}

	bool rc = false;
	if (resp.size()>0) {
		cps_db::response r = resp.get_response(0);
		rc = r.is_int();
	}

	return rc;
}

bool cps_db::subscribe(cps_db::connection &conn, std::vector<char> &key) {
	cps_db::connection::db_operation_atom_t e[2];
	e[0].from_string("PSUBSCRIBE");
	if (key[key.size()-1]!='*') cps_utils::cps_api_vector_util_append(key,"*",1);
	e[1].from_string(&key[0],key.size());

	response_set resp;
	if (!conn.sync_operation(e,2,resp)) {
		return false;
	}

	if (resp.size()>0) {
		cps_db::response r = resp.get_response(0);
		if (r.elements()==3) {
			cps_db::response msg (r.element_at(0));
			cps_db::response status (r.element_at(2));
			if (msg.is_str() && strcasecmp(msg.get_str(),"psubscribe")==0) {
				return (status.is_int() && status.get_int()==1);
			}
		}
	}

	return false;
}

bool cps_db::store_object(cps_db::connection &conn,cps_api_object_t obj) {
	cps_db::connection::db_operation_atom_t e[2];
	e[0].from_string("HSET");
	e[1]._atom_type = cps_db::connection::db_operation_atom_t::obj_fields_t::obj_field_OBJ_KEY_AND_DATA;
	e[1]._object = obj;

	response_set resp;
	if (!conn.sync_operation(e,2,resp)) {
		return false;
	}
	bool rc = false;
	if (resp.size()>0) {
		cps_db::response r = resp.get_response(0);
		rc = r.is_int() && (r.get_int()==0 || r.get_int()==1);
	}

	return rc;
}

bool cps_db::get_object(cps_db::connection &conn, const std::vector<char> &key, cps_api_object_t obj) {
	cps_db::connection::db_operation_atom_t e[3];
	e[0].from_string("HGET");
	e[1].from_string(&key[0],key.size());
	e[2].from_string("object");
	response_set resp;
	if (!conn.sync_operation(e,3,resp)) {
		return false;
	}
	if (resp.size()==0) return false;

	cps_db::response r = resp.get_response(0);

	bool rc = false;

	if (r.is_str() && cps_api_array_to_object(r.get_str(),r.get_str_len(),obj)) {
		rc = true;
	}

	return rc;
}

bool cps_db::get_object(cps_db::connection &conn, cps_api_object_t obj) {
	std::vector<char> key;
	if (!cps_db::dbkey_from_instance_key(key,obj)) return false;
	return get_object(conn,key,obj);
}

#include <iostream>
#include "cps_string_utils.h"

bool cps_db::get_objects(cps_db::connection &conn,std::vector<char> &key,cps_api_object_list_t obj_list) {
	cps_utils::cps_api_vector_util_append(key,"*",1);

	std::vector<std::vector<char>> all_keys;
	bool rc = fetch_all_keys(conn, &key[0],key.size(),[&conn,&all_keys](const void *key, size_t len){
		std::vector<char> c;
		cps_utils::cps_api_vector_util_append(c,key,len);
		all_keys.push_back(std::move(c));
	});
	(void)rc;  //if (!rc) return false;

	size_t ix =0;
	size_t mx = all_keys.size();
	for ( ; ix < mx ; ++ix ) {
		cps_api_object_guard og (cps_api_object_create());

		if (get_object(conn,all_keys[ix],og.get())) {
			if (cps_api_object_list_append(obj_list,og.get())) {
				og.release();
			}
		}
	}
	return true;
}

bool cps_db::get_objects(cps_db::connection &conn, cps_api_object_t obj,cps_api_object_list_t obj_list) {
	std::vector<char> k;
	if (!cps_db::dbkey_from_class_key(k,cps_api_object_key(obj))) return false;
	return get_objects(conn,k,obj_list);
}

cps_db::response_set::~response_set() {
	for (auto it: _data) {
		freeReplyObject((redisReply*)it);
	}
}

