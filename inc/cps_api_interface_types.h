/*
 * filename: cps_api_interface_types.h
 * (c) Copyright 2014 Dell Inc. All Rights Reserved.
 */

/** OPENSOURCELICENSE */


#ifndef cps_api_iNTERFACE_TYPES_H_
#define cps_api_iNTERFACE_TYPES_H_

#include "ds_common_types.h"

typedef enum  {
    ADMIN_DEF,
    DB_ADMIN_STATE_UP,
    DB_ADMIN_STATE_DN
}db_interface_state_t;

typedef enum  {
    DB_INTERFACE_OP_SET,
    DB_INTERFACE_OP_CREATE,
    DB_INTERFACE_OP_DELETE
}db_interface_operation_t;

typedef enum {
    OPER_DEF,
    DB_OPER_STATE_UP,
    DB_OPER_STATE_DN
}db_interface_operational_state_t ;


typedef enum {
    ADDR_DEFAULT,
    ADDR_ADD,
    ADDR_DEL
}db_if_addr_msg_type_t ;

typedef enum {
    ROUTE_DEFAULT,
    ROUTE_ADD,
    ROUTE_DEL,
    ROUTE_UPD
}db_route_msg_t;


typedef enum {
    QDISC_INVALID,
    QDISC_ADD,
    QDISC_DEL,
} db_qdisc_msg_type_t ;

typedef enum {
    TCA_KIND_INVALID = 0,
    TCA_KIND_PRIO,
    TCA_KIND_HTB,
    TCA_KIND_PFIFO_FAST,
    TCA_KIND_CBQ,
    TCA_KIND_RED,
    TCA_KIND_U32,
    TCA_KIND_INGRESS,
    TCA_KIND_TBF
} db_qos_tca_type_t;

typedef enum {
    NBR_DEFAULT,
    NBR_ADD,
    NBR_DEL,
    NBR_UPD
}db_nbr_event_type_t;

typedef enum {
    CLASS_INVALID,
    CLASS_ADD,
    CLASS_DEL,
} db_qos_class_msg_type_t;

typedef enum {
    DB_IF_TYPE_PHYS_INTER,
    DB_IF_TYPE_VLAN_INTER,
    DB_IF_TYPE_LAG_INTER
} db_if_type_t;

/**
 * The size of an interface name
 */
typedef char hal_ifname_t[HAL_IF_NAME_SZ];


//cps_api_int_obj_INTERFACE_ADDR

typedef enum {
    cps_api_if_ADDR_A_NAME=0, //char *
    cps_api_if_ADDR_A_IFINDEX=1,//uint32_t
    cps_api_if_ADDR_A_FLAGS=2, //uint32_t
    cps_api_if_ADDR_A_IF_ADDR=3,
    cps_api_if_ADDR_A_IF_MASK=4, //hal_ip_addr_t
    cps_api_if_ADDR_A_OPER=5,    //db_if_addr_msg_type_t
}cps_api_if_ADDR_ATTR;


#if 0
/**
 * Database structure for an interface
 */
typedef struct  {
    char if_name[HAL_IF_NAME_SZ+1];
    hal_ifindex_t   if_index;
    unsigned long   if_flags;
    long            namelen;
    hal_ip_addr_t   if_addr;
    hal_ip_addr_t   if_mask;
    db_if_addr_msg_type_t        if_msgtype;
} db_if_addr_t;
#endif


//cps_api_int_obj_INTERFACE
typedef enum {
    cps_api_if_STRUCT_A_NAME=0, //char *
    cps_api_if_STRUCT_A_IFINDEX=1,//uint32_t
    cps_api_if_STRUCT_A_FLAGS=2, //uint32_t
    cps_api_if_STRUCT_A_IF_MACADDR=3,    //hal_mac_addr_t
    cps_api_if_STRUCT_A_IF_VRF=4, //uint32_t
    cps_api_if_STRUCT_A_IF_FAMILY=5, //uint32_t
    cps_api_if_STRUCT_A_ADMIN_STATE=6,    //db_interface_state_t
    cps_api_if_STRUCT_A_OPER_STATE=7,    //db_interface_operational_state_t
    cps_api_if_STRUCT_A_MTU=8,    //uint32_t
    cps_api_if_STRUCT_A_IF_TYPE=9,    //db_if_type_t
    cps_api_if_STRUCT_A_VLAN_ID=10,    //uint32_t(hal_vlan_id_t)
    cps_api_if_STRUCT_A_OPERATION=11,    //db_interface_operation_t
    cps_api_if_STRUCT_A_MAX
}cps_api_if_STRUCT_ATTR;

#if 0
typedef struct  {
    char               if_name[HAL_IF_NAME_SZ+1];
    hal_ifindex_t     if_index;
    unsigned short     if_flags;
    long               namelen;
    hal_mac_addr_t          if_hwaddr;
    unsigned long      vrfid;
    unsigned short     family;
    db_interface_state_t  if_astate;
    db_interface_operational_state_t   if_ostate;
    unsigned int               mtu;
    db_if_type_t         if_type;
    hal_vlan_id_t         if_vlan;
}db_if_t;

typedef struct {
    db_if_t interface;
    db_interface_operation_t operation;
} db_if_event_t;
#endif



#define cps_api_if_QDISC (4)



#endif /* DB_EVENT_INTERFACE_H_ */