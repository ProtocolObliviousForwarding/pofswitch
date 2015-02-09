/**
 * Copyright (c) 2012, 2013, Huawei Technologies Co., Ltd.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _POF_LOCALRESOURCE_H_
#define _POF_LOCALRESOURCE_H_

#include "pof_common.h"

/* The table numbers of each type. */
#define POFLR_MM_TBL_NUM   (6)
#define POFLR_LPM_TBL_NUM   (3)
#define POFLR_EM_TBL_NUM    (3)
#define POFLR_DT_TBL_NUM    (9)

/* Max size of flow table. */
#define POFLR_FLOW_TABLE_SIZE (8000)

/* The numbers of meter, counter and group. */
#define POFLR_METER_NUMBER (256)
#define POFLR_COUNTER_NUMBER (512)
#define POFLR_GROUP_NUMBER (128)

/* Max number of local physical port. */
#define POFLR_DEVICE_PORT_NUM_MAX (16)

/* Max key length. */
#define POFLR_KEY_LEN (160)

/* Port OpenFlow enable or disable. */
#define POFLR_PORT_ENABLE (1)
#define POFLR_PORT_DISABLE (0)

/* Define the valid and invalid value. */
#define POFLR_STATE_VALID (1)
#define POFLR_STATE_INVALID (0)

typedef struct poflr_flow_entry{
    pof_flow_entry entry;
    uint32_t state;   // POFLR_STATE_VALID or POFLR_STATE_INVALID
}poflr_flow_entry;

typedef struct poflr_flow_table{
    pof_flow_table tbl_base_info;
    poflr_flow_entry *entry_ptr;
    uint32_t entry_num;
    uint32_t state;   // POFLR_STATE_VALID or POFLR_STATE_INVALID
}poflr_flow_table;

typedef struct poflr_groups{
    uint32_t group_num;
    pof_group *group;
    uint32_t *state;   // POFLR_STATE_VALID or POFLR_STATE_INVALID
}poflr_groups;

typedef struct poflr_counters{
    uint32_t counter_num;
    pof_counter *counter;
    uint32_t *state; // POFLR_STATE_VALID or POFLR_STATE_INVALID
}poflr_counters;

typedef struct poflr_meters{
    uint32_t meter_num;
    pof_meter *meter;
    uint32_t *state;   // POFLR_STATE_VALID or POFLR_STATE_INVALID
}poflr_meters;

typedef struct pofec_error{
    uint16_t type;
    char type_str[POF_ERROR_STRING_MAX_LENGTH];
    uint16_t code;
    char error_str[POF_ERROR_STRING_MAX_LENGTH];
}pofec_error;

/* Switch ID. */
extern uint32_t g_poflr_dev_id;

/* Error message. */
extern pofec_error g_pofec_error;

/* Local_resource. */
extern uint32_t pof_localresource_init();
extern uint32_t poflr_set_config(uint16_t flags, uint16_t miss_send_len);
extern uint32_t poflr_reply_config();
extern uint32_t poflr_reply_feature_resource();
extern uint32_t poflr_clear_resource();
extern uint32_t poflr_get_switch_config(pof_switch_config **config_ptrptr);
extern uint32_t poflr_get_switch_feature(pof_switch_features **feature_ptrptr);

/* Port. */
extern uint32_t poflr_init_port();
extern uint32_t poflr_port_detect_task();
extern uint32_t poflr_reply_port_resource();
extern uint32_t poflr_get_hwaddr_by_ipaddr(uint8_t *hwaddr, char *ipaddr_stri, uint8_t *port_id_ptr);
extern uint32_t poflr_port_openflow_enable(uint8_t port_id,
                                           uint8_t ulFlowEnableFlg);
extern uint32_t poflr_disable_all_port();
extern uint32_t poflr_get_port(pof_port **port_ptrptr);
extern uint32_t poflr_get_port_number(uint16_t *port_number_ptr);
extern uint32_t poflr_get_port_number_max(uint16_t *port_number_ptr);
extern uint32_t poflr_set_port_task_id(task_t *tid, pof_port *p);
extern uint32_t poflr_del_port_task_id(task_t **tid, pof_port *p);

/* Flow table. */
extern uint32_t poflr_init_table_resource();
extern uint32_t poflr_free_table_resource();
extern uint32_t poflr_reply_table_resource();
extern uint32_t poflr_create_flow_table(uint8_t table_id, \
                                        uint8_t type, \
                                        uint16_t key_len, \
                                        uint32_t size, \
                                        char *name, \
										uint8_t match_field_num, \
										pof_match *match);
extern uint32_t poflr_delete_flow_table(uint8_t table_id, uint8_t type);
extern uint32_t poflr_init_flow_table();
extern uint32_t poflr_free_flow_table();
extern uint32_t poflr_empty_flow_table();
extern uint32_t poflr_table_ID_to_id (uint8_t table_ID, uint8_t *type_ptr, uint8_t *table_id_ptr);
extern uint32_t poflr_table_id_to_ID (uint8_t type, uint8_t id, uint8_t *ID_ptr);
extern uint32_t poflr_get_flow_table_resource(pof_flow_table_resource **flow_table_resource_ptrptr);
extern uint32_t poflr_get_flow_table(poflr_flow_table **flow_table_ptrptr, uint8_t table_type, uint8_t table_id);
extern uint32_t poflr_get_table_number(uint8_t **table_number_ptrptr);
extern uint32_t poflr_get_table_id_base(uint8_t **table_id_base_ptrptr);
extern uint32_t poflr_get_table_size(uint32_t *tabel_size_ptr);
extern uint32_t poflr_get_key_len(uint16_t **key_len_ptrptr);
extern uint32_t poflr_check_flow_table_exist(uint8_t ID);

extern uint32_t poflr_add_flow_entry(pof_flow_entry *flow_ptr);
extern uint32_t poflr_modify_flow_entry(pof_flow_entry *flow_ptr);
extern uint32_t poflr_delete_flow_entry(pof_flow_entry *flow_ptr);

/* Meter. */
extern uint32_t poflr_add_meter_entry(uint32_t meter_id, uint32_t rate);
extern uint32_t poflr_modify_meter_entry(uint32_t meter_id, uint32_t rate);
extern uint32_t poflr_delete_meter_entry(uint32_t meter_id, uint32_t rate);
extern uint32_t poflr_init_meter();
extern uint32_t poflr_free_meter();
extern uint32_t poflr_empty_meter();
extern uint32_t poflr_get_meter(poflr_meters **meter_ptrptr);
extern uint32_t poflr_get_meter_number(uint32_t *meter_number_ptr);

/* Group. */
extern uint32_t poflr_modify_group_entry(pof_group *group_ptr);
extern uint32_t poflr_add_group_entry(pof_group *group_ptr);
extern uint32_t poflr_delete_group_entry(pof_group *group_ptr);
extern uint32_t poflr_init_group();
extern uint32_t poflr_free_group();
extern uint32_t poflr_empty_group();
extern uint32_t poflr_get_group(poflr_groups **group_ptrptr);
extern uint32_t poflr_get_group_number(uint32_t *group_number_ptr);

/* Counter. */
extern uint32_t poflr_counter_init(uint32_t counter_id);
extern uint32_t poflr_counter_delete(uint32_t counter_id);
extern uint32_t poflr_counter_cleare(uint32_t counter_id);
extern uint32_t poflr_get_counter_value(uint32_t counter_id);
#ifdef POF_SD2N
extern uint32_t poflr_counter_increace(uint32_t counter_id, uint16_t byte_len);
#else // POF_SD2N
extern uint32_t poflr_counter_increace(uint32_t counter_id);
#endif // POF_SD2N
extern uint32_t poflr_init_counter();
extern uint32_t poflr_free_counter();
extern uint32_t poflr_empty_counter();
extern uint32_t poflr_get_counter(poflr_counters **counter_ptrptr);
extern uint32_t poflr_get_counter_number(uint32_t *counter_number_ptr);

/* Set MM table number. */
extern uint32_t poflr_set_MM_table_number(uint8_t MMTableNum);

/* Set LPM table number. */
extern uint32_t poflr_set_LPM_table_number(uint8_t LPMTableNum);

/* Set EM table number. */
extern uint32_t poflr_set_EM_table_number(uint8_t EMTableNum);

/* Set DT table number. */
extern uint32_t poflr_set_DT_table_number(uint8_t DTTableNum);

/* Set flow table size. */
extern uint32_t poflr_set_flow_table_size(uint32_t flow_table_size);

/* Set counter number. */
extern uint32_t poflr_set_counter_number(uint32_t counter_num);

/* Set group number. */
extern uint32_t poflr_set_group_number(uint32_t group_num);

/* Set the max number of ports. */
extern uint32_t poflr_set_port_number_max(uint32_t port_num_max);

/* Set the key length. */
extern uint32_t poflr_set_key_len(uint32_t key_len);

#endif // _POF_LOCALRESOURCE_H_
