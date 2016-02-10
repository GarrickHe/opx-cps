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
 * filename: cps_api_event_api.h
 */

#ifndef CPS_API_EVENT_API_H
#define CPS_API_EVENT_API_H

/** @addtogroup CPSAPI
 *  @{
 *
 *  @addtogroup Events Event Handling
 *  APIs Used to publish and subscribe to events.
 *  <p>Applications need to add the following instruction:</p>

 @verbatim
 #include <cps_api_event.h>
 @endverbatim

 * @{
*/

#include "cps_api_errors.h"

#include "cps_api_object.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup typesandconstsEvents Types and Constants
 * @{
 */

/**
 * Handle for the event subsystem.
 */
typedef void * cps_api_event_service_handle_t;

/**
 * The priority for the event handler callback.
 */
typedef unsigned int cps_api_event_reg_prio_t;


/**
 * This is the event API registration structure that provides a priority for the
 * client's registration along with the clients events they want to be covered
 * under the priority.  This could be implemented via separate queues for the client
 * to receive messages on or some other type of implementation.
 */
typedef struct {
    cps_api_event_reg_prio_t priority; //!< priority of the registration, optional for the implementation
    cps_api_key_t *objects;    //!< list of objects
    size_t number_of_objects;  //!< number of objects in the list
} cps_api_event_reg_t;

/**
 * @}
 */

/**
 * Initialize the internal cps api event library.  This must be called by any process
 * using the cps event API.
 * @return standard event return code
 */
cps_api_return_code_t cps_api_event_service_init(void);

/**
 * @brief initialize to the CPS event forwarding service for to handle events.
 * @param handle the handle that is returned
 * @return standard return code
 */
cps_api_return_code_t cps_api_event_client_connect(cps_api_event_service_handle_t * handle);

/**
 * Deregister with the event service and therefore remove any registration requests
 * destined for the handle
 *
 * @param handle the handle to the event service
 * @return cps_api_ret_code_OK if successful
 */
cps_api_return_code_t cps_api_event_client_disconnect(cps_api_event_service_handle_t handle);

/**
 * Have the client register with the event service for one or more specific events
 * @param handle is the handle to the cps api events service
 * @param req the registration request that contains all objects that the client wants to register for
 * @return cps_api_ret_code_OK if success otherwise a failure
 */
cps_api_return_code_t cps_api_event_client_register(cps_api_event_service_handle_t handle,
        cps_api_event_reg_t * req);

/**
 * Send an object to the event service for publishing using a previously registered handle
 * @param handle the handle to the CPS event service
 * @param object the object to send
 * @return standard return code
 */
cps_api_return_code_t cps_api_event_publish(cps_api_event_service_handle_t handle,
        cps_api_object_t object);

/**
 * wait for a object from the event service
 * @param handle opened from a previous client registration
 * @param object the to object to receive.
 * @return cps_api_ret_code_OK if the wait was completed with an event returned
 *   or a specific return code indicating a failure or retry request is required
 */
cps_api_return_code_t cps_api_wait_for_event(cps_api_event_service_handle_t handle,
        cps_api_object_t object);

/** @addtogroup EventThread Event Thread Utilities
 *  This file consists of the APIs to publish and subscribe to events.
 *
 * The following APIs provides a wrapper over the event functionality and will create a
 * thread to receive events from the event service.  There can only be a single one of
 * these in a process.
 *
 * The Event Thread will wait for events (with a maximum size) and call the registered
 * callbacks.  There is only one thread so the events will be processed in order.
 *
 * Also remember since it is single threaded the time of the callback will delay the
 * processing of other events that are waiting to be received.
 *
 * @{
 * */


/**
 * Maximum size of event that can be received by the event thread helper
 * utilitiy
 */
#define CPS_API_EVENT_THREAD_MAX_OBJ (50000)

/**
 * The callback function when used with the CPS API event thread helper utility
 */
typedef bool (*cps_api_event_thread_callback_t)(cps_api_object_t object,void * context);

/**
 * Initialize the event thread utility.  This will connect to the CPS event
 * service and provide an internal global handle that applications can use
 * to register against events
 *
 * @return cps_api_ret_code_OK if successful and created otherwise an error.
 *     should only be called once in a process
 */
cps_api_return_code_t cps_api_event_thread_init(void);

/**
 * Register for an event and a callback function that will be executed when
 * an event arrives
 * @param reg is the structure that contains the keys and priority of the event
 *     registration
 * @param cb the callback function
 * @param context the context passed to the callback
 * @return cps_api_ret_code_OK if successful
 */
cps_api_return_code_t cps_api_event_thread_reg(cps_api_event_reg_t * reg,
        cps_api_event_thread_callback_t cb, void * context );

/**
 * Publish an event to the CPS API event service using the event thread's
 * global handle.
 *
 * @param object the object to publish
 * @return cps_api_ret_code_OK if successful
 */
cps_api_return_code_t cps_api_event_thread_publish(cps_api_object_t object);

/**
 * Shutdown the CPS API event thread utility.
 * @return cps_api_ret_code_OK if successful
 */
cps_api_return_code_t cps_api_event_thread_shutdown(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 * @}
 */

#endif
