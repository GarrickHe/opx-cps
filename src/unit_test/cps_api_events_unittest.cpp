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
 * cps_api_events_unittest.cpp
 */


#include "cps_api_events.h"


#include <stdio.h>
#include <stdlib.h>


#include "gtest/gtest.h"


#include "cps_api_operation.h"
#include "cps_api_events.h"
#include "cps_api_event_init.h"
#include "std_event_service.h"
#include "cps_api_service.h"

#include <pthread.h>
#include <sys/select.h>

cps_api_event_service_handle_t handle;


bool _cps_api_event_thread_callback(cps_api_object_t object,void * context) {
     char buff[1024];
     static int cnt=0;
     printf("1(%d)- Obj %s\n",cnt,cps_api_object_to_string(object,buff,sizeof(buff)));
     ++cnt;
     return true;
}

bool _cps_api_event_thread_callback_2(cps_api_object_t object,void * context) {
     char buff[1024];
     static int cnt=0;
     printf("2(%d) - Obj %s\n",cnt,cps_api_object_to_string(object,buff,sizeof(buff)));
     ++cnt;
     return true;
}


bool threaded_client_test() {

    cps_api_event_reg_t reg;
    reg.priority = 0;

    cps_api_key_t keys[5];
    cps_api_key_init(&keys[0],cps_api_qualifier_OBSERVED,
            cps_api_obj_cat_INTERFACE,1,0);

    cps_api_key_init(&keys[1],cps_api_qualifier_OBSERVED,
            cps_api_obj_cat_ROUTE,1,0);

    cps_api_key_init(&keys[2],cps_api_qualifier_OBSERVED,
            cps_api_obj_cat_ROUTE,1,1,1);

    cps_api_key_init(&keys[3],cps_api_qualifier_TARGET,
            cps_api_obj_cat_ROUTE,1,0);

    cps_api_key_init(&keys[4],cps_api_qualifier_TARGET,
            cps_api_obj_cat_INTERFACE,1,0);

    reg.objects = keys;
    reg.number_of_objects = 5 ; //sizeof(keys)/sizeof(*keys);

    if (cps_api_event_thread_reg(&reg,
            _cps_api_event_thread_callback,NULL)!=cps_api_ret_code_OK)
        return false;

    cps_api_key_set_len(keys,1);
    reg.number_of_objects = 1;
    if (cps_api_event_thread_reg(&reg,
            _cps_api_event_thread_callback_2,NULL)!=cps_api_ret_code_OK)
        return false;
    return true;
}

bool push_running = true;
void * push_client_messages(void *) {

    size_t ix = 0;
    size_t mx = 10000;

    cps_api_event_service_handle_t handle;
    if (cps_api_event_client_connect(&handle)!=cps_api_ret_code_OK) return false;

    cps_api_object_t obj = cps_api_object_create();
    cps_api_object_attr_add(obj,1,"Cliff",6);
    cps_api_qualifier_t q[2] ={ cps_api_qualifier_TARGET, cps_api_qualifier_OBSERVED};
    cps_api_object_category_types_t c[3]={cps_api_obj_cat_INTERFACE,
            cps_api_obj_cat_ROUTE,cps_api_obj_cat_QOS
    };

    for ( ; ix < mx ; ++ix ) {
        if ((ix %10)==0) {
            if (cps_api_event_client_disconnect(handle)!=cps_api_ret_code_OK) return false;
            if (cps_api_event_client_connect(&handle)!=cps_api_ret_code_OK) return false;
        }
        cps_api_key_init(cps_api_object_key(obj),q[ix%2],c[ix%3],1,2,ix%3,ix);
        if (cps_api_event_publish(handle,obj)!=cps_api_ret_code_OK) exit(1);
    }
    push_running = false;
    return NULL;
}

bool simple_client_use() {
    cps_api_event_service_handle_t handle;
    if (cps_api_event_client_connect(&handle)!=cps_api_ret_code_OK) return false;

    cps_api_event_reg_t reg;
    reg.priority = 0;

    cps_api_key_t keys[5];
    cps_api_key_init(&keys[0],cps_api_qualifier_OBSERVED,
            cps_api_obj_cat_INTERFACE,1,0);

    cps_api_key_init(&keys[1],cps_api_qualifier_OBSERVED,
            cps_api_obj_cat_ROUTE,1,0);

    cps_api_key_init(&keys[2],cps_api_qualifier_OBSERVED,
            cps_api_obj_cat_ROUTE,1,1,1);

    cps_api_key_init(&keys[3],cps_api_qualifier_TARGET,
            cps_api_obj_cat_ROUTE,1,0);

    cps_api_key_init(&keys[4],cps_api_qualifier_TARGET,
            cps_api_obj_cat_INTERFACE,1,0);


    reg.objects = keys;
    reg.number_of_objects =5; //sizeof(keys)/sizeof(*keys);

    if (cps_api_event_client_register(handle,&reg)!=cps_api_ret_code_OK) return false;

    cps_api_object_t rec = cps_api_object_create();
    pthread_t id;
    pthread_create(&id,NULL,push_client_messages,NULL);
    char buff[1024];
    int cnt=0;
    while(true) {
        if (cps_api_wait_for_event(handle,rec)!=cps_api_ret_code_OK) return false;
        printf("3(%d) -  %s\n",cnt,cps_api_object_to_string(rec,buff,sizeof(buff)));
        ++cnt;
    }
    return true;
}

bool _cps_api_event_term(cps_api_object_t object,void * context) {
     char buff[1024];
     static int cnt=0;
     printf("%d(%d) - Obj %s\n",__LINE__,cnt,cps_api_object_to_string(object,buff,sizeof(buff)));
     ++cnt;
     if (cnt==(10000-1)) exit(0);
     return true;
}

bool full_reg() {

    cps_api_event_reg_t reg;
    reg.priority = 0;

    cps_api_key_t key[2];

    cps_api_key_init(&key[0],cps_api_qualifier_OBSERVED,0,0,0);
    cps_api_key_init(&key[1],cps_api_qualifier_TARGET,0,0,0);

    reg.objects = key;
    reg.number_of_objects = 2 ;

    if (cps_api_event_thread_reg(&reg, _cps_api_event_term,NULL)!=cps_api_ret_code_OK)
        return false;

    if (cps_api_event_thread_reg(&reg,_cps_api_event_term,NULL)!=cps_api_ret_code_OK)
        return false;

    return true;
}

bool test_init() {
    static std_event_server_handle_t _handle=NULL;
    std_event_server_init(&_handle,CPS_API_EVENT_CHANNEL_NAME,CPS_API_EVENT_THREADS );
    return (cps_api_event_service_init()==cps_api_ret_code_OK);
}

TEST(cps_api_events,init) {
    ASSERT_TRUE(test_init());
    ASSERT_TRUE(cps_api_event_thread_init()==cps_api_ret_code_OK);
    ASSERT_TRUE(full_reg());
    ASSERT_TRUE(threaded_client_test());
    ASSERT_TRUE(simple_client_use());

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
