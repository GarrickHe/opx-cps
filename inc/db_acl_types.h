/**
 * filename: db_acl_types.h
 * (c) Copyright 2014 Dell Inc. All Rights Reserved.
 **/

/** OPENSOURCELICENSE */

#ifndef __DB_ACL_TYPES_H
#define __DB_ACL_TYPES_H

#include <stdint.h>
#include "stddef.h"
#include "ds_common_types.h"
#include "db_acl_qualifier.h"
#include "db_acl_action.h"
#include "db_acl_qualifier_masks.h"
#include "db_acl_qualifier_mask_union.h"


#define ACL_ALL_PORTS 0xffff
#define ACL_ALL_UNITS 0xff

#define ACL_FEATURE_NAME_LEN_MAX 30
#define ACL_ROOT_PATH  "/etc/dn/acl"

typedef enum {
    db_acl_STAGE_INGRESS=0,
    db_acl_STAGE_EGRESS,
    db_acl_STAGE_PRE_INGRESS,
} db_acl_stage_t;

typedef enum {
    db_acl_DELETE_ENTRY = 1,
    db_acl_ADD_STATS = 2,
} db_acl_entry_flags_t;

typedef struct  {
    uint32_t feature_id;
    size_t size;
} db_acl_feature_info_t;

/*
* qual_enum identifies the qualifier
* acl_qualmask is union of all possible qualifiers
*/
typedef struct {
    db_acl_qualifier_enum_t qual_enum;
    db_acl_qualifier_mask_t acl_qualmask;
} db_acl_qualmask_detail_t;

typedef struct
{
    db_acl_action_enum_t action_enum;
    int action_arg1;
    int action_arg2;
}db_acl_action_detail_t;

/*
* details of the entry to be installed in TCAM
*/

typedef struct {
    db_acl_stage_t acl_stage;
    int num_qualifiers;
    db_acl_qualmask_detail_t *qual_array;
    int num_actions;
    db_acl_action_detail_t *action_array;
    port_t num_ports;
    port_t *port_array;
    uint32_t entry_index;
    uint32_t entry_virtual_index;
    db_acl_entry_flags_t  entry_flags;
} db_acl_entry_metadata_t;

#endif