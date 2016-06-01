/*
 * Copyright (c) 2016 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN  *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *  LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/*
 * filename: hal_event_service.c
 */


#include "cps_api_node_private.h"
#include "cps_api_db.h"
#include "cps_api_db_response.h"
#include "cps_api_events.h"
#include "cps_api_event_init.h"

#include "cps_api_operation.h"
#include "cps_api_service.h"
#include "private/cps_ns.h"
#include "cps_api_service.h"
#include "std_mutex_lock.h"

#include "event_log.h"

#include "std_time_tools.h"
#include "std_select_tools.h"

#include <thread>
#include <stdlib.h>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <memory>

struct _reg_data {
    bool _sync = false;
    std::vector<char> _key;
};

struct _handle_data {
	std::mutex _mutex;

	cps_db::connection_cache _sub;
	cps_db::connection_cache _pub;
	std::unordered_map<std::string,std::vector<_reg_data>> _group_keys;

	std::unordered_map<std::string,std::unique_ptr<cps_db::connection>> _connections;

	std::unordered_map<std::string,bool> _group_updated;
};


inline _handle_data* handle_to_data(cps_api_event_service_handle_t handle) {
    return (_handle_data*) handle;
}

static std::mutex _mutex;
static cps_api_node_alias *_aliases = new cps_api_node_alias;
static cps_api_nodes *_nodes = new cps_api_nodes;
static std::unordered_set<_handle_data *> _handles;

void handle_node_updates(void) {
	while (true) {
		bool changed = false;
		std_usleep(MILLI_TO_MICRO(1000*30));
		{
			std::lock_guard<std::mutex> lg(_mutex);
			changed = _nodes->load();
			_aliases->load();
		}
		if (changed) {
			std::lock_guard<std::mutex> lg(_mutex);

		}
	}
}

void one_time_only() {
	std::thread *p = new std::thread(handle_node_updates);
	(void)p;
}


bool cps_api_db_get_node_group(const std::string &group,std::vector<std::string> &lst) {
	std::lock_guard<std::mutex> lg(_mutex);
	_nodes->load();
	if (!_nodes->address_list(group,lst)) return false;
	return true;
}

static bool iterate_group(const std::string &group_name,const std::function<void (const std::string &node, void*context)> &operation,
		void *context) {
	std::vector<std::string> lst;

	if (!cps_api_db_get_node_group(group_name,lst)) return false;

	for (auto node_it : lst ) {
		operation(node_it,context);
	}
	return true;
}

static void __resync_regs(cps_api_event_service_handle_t handle) {
	_handle_data *nd = handle_to_data(handle);

	for (auto it : nd->_group_keys) {

		bool success = true;
		bool group_updated = nd->_group_updated[it.first];

		if (!iterate_group(it.first,[nd,&it,&success,&group_updated](const std::string &node,void *context) {
			auto con_it = nd->_connections.find(node);
			if (con_it==nd->_connections.end()) {
				return;
			}

			//for each client key associated with the group
			for ( auto reg_it : it.second) {
				if (!reg_it._sync || group_updated) { //best we can do for now but come back and fix this condition
					if (!cps_db::subscribe(*con_it->second,reg_it._key)) {
						nd->_connections.erase(con_it);
						break;
					}
				}
			}

		},handle)) {
			success=false;
		}

		//nodes in group totally subscribed for
		if (success) {
			nd->_group_updated[it.first] = false;
			for ( auto reg_it : it.second) {
				reg_it._sync = true;
			}
		}
	}
}

static bool __check_connections(cps_api_event_service_handle_t handle) {
	_handle_data *nd = handle_to_data(handle);

	bool changed = false;

	for (auto it : nd->_group_keys) {
		if (iterate_group(it.first,[&nd,&changed](const std::string &node,void *context) {
				auto con_it = nd->_connections.find(node);
				if (con_it==nd->_connections.end()) {
					std::unique_ptr<cps_db::connection> c(new cps_db::connection);
					if (!c->connect(node)) {
						return;
					}
					nd->_connections[node] = std::move(c);
					nd->_group_updated[node] = true;
					changed = true;
				}
			},handle)) {

		}
	}

	return changed;
}

static pthread_once_t _once = PTHREAD_ONCE_INIT;
static cps_api_return_code_t _cps_api_event_service_client_connect(cps_api_event_service_handle_t * handle) {
	std::unique_ptr<_handle_data> _h(new _handle_data);

	pthread_once(&_once,one_time_only);

	*handle = _h.release();
	return cps_api_ret_code_OK;
}

static cps_api_return_code_t _register_one_object(cps_api_event_service_handle_t handle,
        cps_api_object_t object) {

	_handle_data *nh = handle_to_data(handle);
	std::lock_guard<std::mutex> lg(nh->_mutex);

	const char *_group = cps_api_key_get_group(object);

	if (_group==nullptr) _group = CPS_API_NODE_LOCAL_GROUP;

	_reg_data rd ;
	rd._sync = false;
	cps_db::dbkey_from_instance_key(rd._key,object);

	nh->_group_keys[_group].push_back(rd);
	nh->_group_updated[_group] = true;

	return cps_api_ret_code_OK;
}


static cps_api_return_code_t _cps_api_event_service_register_objs_function_(cps_api_event_service_handle_t handle,
        cps_api_object_list_t objects) {

	for ( size_t ix = 0, mx = cps_api_object_list_size(objects); ix < mx ; ++ix ) {
		cps_api_return_code_t rc = cps_api_ret_code_OK;
		cps_api_object_t o = cps_api_object_list_get(objects,ix);
		rc = _register_one_object(handle,o);
		if (rc!=cps_api_ret_code_OK) return rc;
	}

	if (__check_connections(handle)) __resync_regs(handle);

	return cps_api_ret_code_OK;
}

static cps_api_return_code_t _cps_api_event_service_publish_msg(cps_api_event_service_handle_t handle,
        cps_api_object_t msg) {

	_handle_data *nh = handle_to_data(handle);
	std::lock_guard<std::mutex> lg(nh->_mutex);


    STD_ASSERT(msg!=NULL);
    STD_ASSERT(handle!=NULL);

    cps_api_key_t *_okey = cps_api_object_key(msg);

    if (cps_api_key_get_len(_okey) < CPS_OBJ_KEY_SUBCAT_POS) {
        return cps_api_ret_code_ERR;
    }

	const char *_group = cps_api_key_get_group(msg);
	if (_group==nullptr) _group = CPS_API_NODE_LOCAL_GROUP;

	std::vector<std::string> lst;
	{
		std::lock_guard<std::mutex> lg(_mutex);
		_nodes->load();
		if (!_nodes->address_list(_group,lst)) {
			_group = _aliases->addr(_group);
			lst.push_back(_group);
		}
	}
	bool sent =true;
	for (auto it : lst) {
		cps_db::connection_request r(cps_db::ProcessDBCache(),it.c_str());
		sent &= cps_db::publish(r.get(),msg);
	}

    return sent? cps_api_ret_code_OK : cps_api_ret_code_ERR;
}

static cps_api_return_code_t _cps_api_event_service_client_deregister(cps_api_event_service_handle_t handle) {
	_handle_data *nh = handle_to_data(handle);

    //no point in locking the handle on close..
    //clients will be messed up anyway if they try to have multiple threads
    //using and destroying event channels

    delete nh;

    return cps_api_ret_code_OK;
}

static cps_api_return_code_t _cps_api_wait_for_event(
        cps_api_event_service_handle_t handle,
        cps_api_object_t msg) {

	_handle_data *nh = handle_to_data(handle);

    while (true) {
    	fd_set _set;
    	int max_fd = -1;
    	{
    		{
				std::lock_guard<std::mutex> lg(nh->_mutex);
				FD_ZERO(&_set);
				for (auto &it : nh->_connections) {
					FD_SET(it.second->get_fd(), &_set);
					if (max_fd < it.second->get_fd())  max_fd = it.second->get_fd();
				}
    		}
			if (max_fd == -1) { std_usleep(1000*1000*1);continue; }
    	}
    	struct timeval tv; tv.tv_sec=1; tv.tv_usec =0;
    	ssize_t rc = std_select_ignore_intr(max_fd+1,&_set,nullptr,nullptr,&tv,nullptr);

    	std::lock_guard<std::mutex> lg(nh->_mutex);

    	if (rc==-1) {
    		if (__check_connections(handle)) {
    			__resync_regs(handle);
    		}
    		continue;
    	}

    	if (rc==0) continue;
    	for (auto &it : nh->_connections) {
    		if (FD_ISSET(it.second->get_fd(),&_set)) {
    			cps_db::response_set set;
    			if (!it.second->get_event(set.get())) {
    				nh->_connections.erase(it.first);
    				break;
    			}

    			if (set.size()>0) {
    				cps_db::response r = set.get_response(0);
    				if (r.elements()>2) {
    					cps_db::response type (r.element_at(0));
    					if (strcasecmp(type.get_str(),"pmessage")==0) {
    						cps_db::response data(r.element_at(3));
    				        if (cps_api_array_to_object(data.get_str(),data.get_str_len(),msg)) {
    				        	return cps_api_ret_code_OK;
    				        }
    					}
    				}
    			}
    		}
    	}
    }
    return cps_api_ret_code_OK;

}

extern "C" {

static cps_api_event_methods_reg_t functions = {
        NULL,
        0,
		_cps_api_event_service_client_connect,
        nullptr,
        _cps_api_event_service_publish_msg,
        _cps_api_event_service_client_deregister,
        _cps_api_wait_for_event,
		_cps_api_event_service_register_objs_function_
};

cps_api_return_code_t cps_api_event_service_init(void) {
    cps_api_event_method_register(&functions);
    return cps_api_ret_code_OK;
}

cps_api_return_code_t cps_api_services_start() {
    return cps_api_ret_code_OK;
}

}
