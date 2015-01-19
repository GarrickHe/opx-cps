/*
 * filename: cps_api_event_channel.h
 * (c) Copyright 2014 Dell Inc. All Rights Reserved.
 */

/** OPENSOURCELICENSE */
/*
 * cps_api_event_channel.h
 *
 */
#ifndef _DS_EVENT_CHANNEL_H_
#define _DS_EVENT_CHANNEL_H_

#include "cps_api_errors.h"
#include "cps_api_object.h"

#include "cps_api_events.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup CPSAPI The CPS API
 * This API handles the initialization of the event portion of the CPS API
@{
*/

/**
 * This API handles client registration requests.  In this case, all event services will need to handle this call.
 * It will be up to the CPS API to use the appropriate handle for further requests
 * @param handle a token that can be used to register, send or wait for messages
 * @return
 */
typedef cps_api_return_code_t (*cps_api_event_service_client_connect_t)(cps_api_event_service_handle_t * handle);

/**
 * @brief register the specified event key with the event service - when these types of
 * events are sent, the client is sent a copy
 *
 * @param handle created from a register client API
 * @param reg the registration message
 * @return cps_api_ret_code_OK if the API completes successfully otherwise an error.
 */
typedef cps_api_return_code_t (*cps_api_event_service_event_register_t)(cps_api_event_service_handle_t handle,
        cps_api_event_reg_t * req);

/**
 * Send an event to the event service for publishing using a previously registered handle
 * @param handle the handle to the hal event service
 * @param msg the message to send
 * @return cps_api_ret_code_OK if the publish was successful
 */
typedef cps_api_return_code_t (*cps_api_event_service_publish_event_t)(cps_api_event_service_handle_t handle,
        cps_api_object_t msg);

/**
 * Deregister with the event service and therefore remove any registration requests
 * destined for the handle
 *
 * @param handle the handle to the event service
 * @return cps_api_ret_code_OK if the disconnect was successful
 */
typedef cps_api_return_code_t (*cps_api_event_service_client_disconnect_t)(cps_api_event_service_handle_t handle);

/**
 * @brief wait for a message from the event service
 * @param handle opened from a previous client registration
 * @param msg message to receive.  must be initialized before hand and have enough space
 *        max_data_len will be used to determine max space and data_len will be updated
 *        with the actual size of data
 * @return cps_api_ret_code_OK if the wait was completed with an event returned
 *   or a specific return code indicating a failure or retry request is requrired
 */
typedef cps_api_return_code_t (*cps_api_wait_for_event_t)(cps_api_event_service_handle_t handle,
        cps_api_object_t msg);


/**
 * The registration method to handle the sending and receiving of events.
 */
typedef struct {
    cps_api_key_t *keys;
    size_t keys_len;
    cps_api_event_service_client_connect_t connect_function;
    cps_api_event_service_event_register_t register_function;
    cps_api_event_service_publish_event_t publish_function;
    cps_api_event_service_client_disconnect_t deregister_function;
    cps_api_wait_for_event_t wait_for_event_function;
}cps_api_event_methods_reg_t;


/**
 * This API initializes the DB event sub system.  This must be done before anyone
 * tries to use the DS event service and must only be done by one process in AR.
 *
 * @return will return cps_api_ret_code_OK on successful execution or a specific return code
 *        based on the failure.
 *
 */
cps_api_return_code_t cps_api_event_channel_init(void);

/**
 * Register the methods to be used with the CPS API
 * @param the pointer to the registration functions
 * @return cps_api_ret_code_OK on successful otherwise an error code
 */
cps_api_return_code_t cps_api_event_method_register( cps_api_event_methods_reg_t * methods ) ;

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* CPS_API_EVENT_CHANNEL_H_ */