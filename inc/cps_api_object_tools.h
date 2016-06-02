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
 */

#ifndef CPS_API_INC_CPS_API_OBJECT_TOOLS_H_
#define CPS_API_INC_CPS_API_OBJECT_TOOLS_H_

/** @addtogroup CPSAPI
 *  @{
 *  @addtogroup ObjectAndAttributes Object and Object Attribute Handling
 *  @{
*/

#include "cps_api_object_attr.h"
#include "cps_api_object.h"
#include "cps_api_operation.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 *  @addtogroup ObjectAttributesDetails Object Attribute Details
 *  These functions are used to manipulate and retrieve object attribute details.
    <p>In order to access the functionality provided by this module, applications need to add the following instruction:</p>

 @verbatim
 #include <cps_api_object_tools.h>
 @endverbatim

 *  @{
 */

/**
 * Create an object and set the key of the object to the key of the specified attribute ID
 * @param qual the qualifier of the key (target, observed, etc..
 * @param id the object that contains the key that will be set on the object
 * @param use_create_defaults if there is defaults found for the object, also set those attributes in the object
 *             default attributes are only available on objects that support it
 *
 * @return a cps_api_object_t with the key and attributes set appropriately or a NULL on error
 */
cps_api_object_t cps_api_obj_tool_create(cps_api_qualifier_t qual, cps_api_attr_id_t id, bool use_create_defaults);


/**
 * Compare the filter and the specified object.  Return true if all of the attributes in the filter
 *    matches the corresponding attributes in the object.
 * @param filter the filter object containing the attributes/values to look for
 * @param obj the object that as the attributes to compare
 * @param require_all_attribs if this is true, all attributes in the filter must be present in the object and they must
 *  match otherwise only compare found attribute
 *
 * @return true if the filter and object match based on the require_all_attrib setting
 */
bool cps_api_obj_tool_matches_filter(cps_api_object_t filter, cps_api_object_t obj, bool require_all_attribs);


bool cps_api_obj_tool_merge(cps_api_object_t main, cps_api_object_t overlay);

#ifdef __cplusplus
}
#endif
/**
 * @}
 * @}
 * @}
 */

#endif /* CPS_API_INC_CPS_API_OBJECT_TOOLS_H_ */
