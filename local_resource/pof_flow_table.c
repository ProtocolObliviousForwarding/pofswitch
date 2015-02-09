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

#include "../include/pof_common.h"
#include "../include/pof_type.h"
#include "../include/pof_global.h"
#include "../include/pof_local_resource.h"
#include "../include/pof_conn.h"
#include "../include/pof_byte_transfer.h"
#include "../include/pof_log_print.h"
#include "../include/pof_datapath.h"
#include "string.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "string.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "arpa/inet.h"

/* The table number of each type. */
uint8_t poflr_mm_tbl_num = POFLR_MM_TBL_NUM;
uint8_t poflr_lpm_tbl_num = POFLR_LPM_TBL_NUM;
uint8_t poflr_em_tbl_num = POFLR_EM_TBL_NUM;
uint8_t poflr_dt_tbl_num = POFLR_DT_TBL_NUM;

/* Flow table size. */
uint32_t poflr_flow_table_size = POFLR_FLOW_TABLE_SIZE;

/* Flow tables. */
poflr_flow_table *poflr_table_ptr[POF_MAX_TABLE_TYPE];

/* Key length of each type flow table. */
uint16_t poflr_key_len_each_type[POF_MAX_TABLE_TYPE];

/* Table number of each type flow table. */
uint8_t poflr_table_num_each_type[POF_MAX_TABLE_TYPE];

/* Table ID's base value of all types. */
uint8_t poflr_key_tid_base_each_type[POF_MAX_TABLE_TYPE];

/* Key length. */
uint32_t poflr_key_len = POFLR_KEY_LEN;

static uint32_t poflr_compare_two_flow(pof_flow_entry *p1, pof_flow_entry *p2);
static uint32_t poflr_check_flow_in_table(pof_flow_entry *flow_ptr, poflr_flow_table *table_ptr);

/***********************************************************************
 * Compare the two flow entry in the same table.
 * Form:     static uint32_t poflr_compare_two_flow(pof_flow_entry *p1, pof_flow_entry *p2)
 * Input:    flow entry 1, flow entry 2
 * Output:   NONE
 * Return:   POF_OK means they are not same. POF_ERROR means they are same.
 * Discribe: This function will compare the two flow entry in the same
 *           table. It will be called when a new flow entry will be added.
 *           If an exactly same flow entry is already exist in the table,
 *           the new flow entry will be added specially.
 ***********************************************************************/
static uint32_t poflr_compare_two_flow(pof_flow_entry *p1, pof_flow_entry *p2){
    uint32_t i,j;

    if(p1->match_field_num != p2->match_field_num){
        return POF_OK;
    }
    if(p1->priority != p2->priority){
        return POF_OK;
    }

    for(i=0; i<p1->match_field_num; i++){
        if(memcmp(&(p1->match[i]), &(p2->match[i]), \
            sizeof(pof_match_x) - 2*POF_MAX_FIELD_LENGTH_IN_BYTE) != 0){
            return POF_OK;
        }
        for(j=0; j<POF_MAX_FIELD_LENGTH_IN_BYTE; j++){
            if((p1->match[i].value[j] & p1->match[i].mask[j]) \
                    != (p2->match[i].value[j] & p2->match[i]. mask[j])){
                return POF_OK;
            }
        }
    }

    return POF_ERROR;
}

/***********************************************************************
 * Check flow entry in the table.
 * Form:     static uint32_t poflr_check_flow_in_table(pof_flow_entry *flow_ptr, \
 *                                                     poflr_flow_table *table_ptr)
 * Input:    flow entry, flow table
 * Output:   NONE
 * Return:   POF_OK means the new flow entry is OK.
 * Discribe: This function will check if an exactly flow entry is already
 *           exist in the table when a new flow entry need to be added.
 *           If there is a same flow entry already, the task will be
 *           terminated.
 ***********************************************************************/
static uint32_t poflr_check_flow_in_table(pof_flow_entry *flow_ptr, poflr_flow_table *table_ptr){
    poflr_flow_entry *entry_ptr;
    uint32_t num=0, count=0;

    num = table_ptr->entry_num;
    entry_ptr = table_ptr->entry_ptr;

    for(count=0; count<num; entry_ptr++){
        if(entry_ptr->state == POFLR_STATE_INVALID){
            continue;
        }

        count++;
		if(entry_ptr->entry.index == flow_ptr->index){
			continue;
		}

        if(poflr_compare_two_flow(flow_ptr, &entry_ptr->entry) != POF_OK){
            POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_ENTRY_EXIST, g_recv_xid);
        }
    }

    return POF_OK;
}

/***********************************************************************
 * Create a flow table.
 * Form:     uint32_t poflr_create_flow_table(uint8_t table_id,
 *                                            uint8_t type,
 *                                            uint16_t key_len,
 *                                            uint32_t size,
 *                                            char *name, 
 *                                            uint8_t match_field_num, 
 *                                            pof_match *match)
 * Input:    table id, table type, key length, table size, table name,
 *           match field number, match data
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will create a flow table in local resource.
 *           The new table is empty.
 ***********************************************************************/
uint32_t poflr_create_flow_table(uint8_t table_id, \
                                 uint8_t type, \
                                 uint16_t key_len, \
                                 uint32_t size, \
								 char *name, \
								 uint8_t match_field_num, \
								 pof_match *match)
{
    poflr_flow_table *tmp_tbl_ptr;

    /* Check type. */
    if(type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(table_id >= poflr_table_num_each_type[type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_ID, g_recv_xid);
    }

    /* Check whether the table have already existed. */
    if(poflr_table_ptr[type][table_id].state != POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_TABLE_EXIST, g_recv_xid);
    }

    /* Check table_size. */
    if(size > poflr_flow_table_size){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_SIZE, g_recv_xid);
    }

    /* Check table_key_length. */
    if(key_len > poflr_key_len_each_type[type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_KEY_LEN, g_recv_xid);
    }

    /* Initialize the table. */
    tmp_tbl_ptr = &poflr_table_ptr[type][table_id];

    tmp_tbl_ptr->entry_ptr = (poflr_flow_entry *)malloc(size * sizeof(poflr_flow_entry));
    POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(tmp_tbl_ptr->entry_ptr, g_recv_xid);
    memset(tmp_tbl_ptr->entry_ptr, 0, size * sizeof(poflr_flow_entry));

    tmp_tbl_ptr->entry_num = 0;
    tmp_tbl_ptr->state = POFLR_STATE_VALID;
    tmp_tbl_ptr->tbl_base_info.key_len = key_len;
    tmp_tbl_ptr->tbl_base_info.size = size;
    strcpy(tmp_tbl_ptr->tbl_base_info.table_name, name);
    tmp_tbl_ptr->tbl_base_info.tid = table_id;
    tmp_tbl_ptr->tbl_base_info.type = type;
	tmp_tbl_ptr->tbl_base_info.match_field_num = match_field_num;
	memcpy(tmp_tbl_ptr->tbl_base_info.match, match, match_field_num * sizeof(pof_match));

    POF_DEBUG_CPRINT_FL(1,GREEN,"Create flow table SUC!");
    return POF_OK;
}

/***********************************************************************
 * Delete a flow table.
 * Form:     uint32_t poflr_delete_flow_table(uint8_t table_id, \
 *                                            uint8_t type)
 * Input:    table id, table type
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will delete the flow table corresponding to the
 *           table id and the table type.
 ***********************************************************************/
uint32_t poflr_delete_flow_table(uint8_t table_id, uint8_t type){
    poflr_flow_table *tmp_tbl_ptr;

    /* Check type. */
    if(type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(table_id >= poflr_table_num_each_type[type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_TABLE_ID, g_recv_xid);
    }

    tmp_tbl_ptr = & poflr_table_ptr[type][table_id];
    /* Check whether the table have already existed. */
    if(tmp_tbl_ptr->state != POFLR_STATE_VALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_TABLE_UNEXIST, g_recv_xid);
    }

    /* Check whether the table is empty. */
    if(tmp_tbl_ptr->entry_num != 0){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_TABLE_UNEMPTY, g_recv_xid);
    }

    /* Free the memory of the entry in the table. */
    free(tmp_tbl_ptr->entry_ptr);

    /* Initialize the table. */
    memset(tmp_tbl_ptr,0,sizeof(poflr_flow_table));

    POF_DEBUG_CPRINT_FL(1,GREEN,"Delete flow table SUC!");
    return POF_OK;
}

/***********************************************************************
 * Add a flow entry.
 * Form:     uint32_t poflr_add_flow_entry(pof_flow_entry *flow_ptr)
 * Input:    flow entry
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will add a new flow entry in the table. If a
 *           same flow entry is already exist in this table, ERROR.
 ***********************************************************************/
uint32_t poflr_add_flow_entry(pof_flow_entry *flow_ptr){
    poflr_flow_entry *tmp_vhal_entry_ptr;
    poflr_flow_table *tmp_tbl_ptr;
    uint32_t index = flow_ptr->index, ret;
    uint8_t  table_id = flow_ptr->table_id;
    uint8_t  table_type = flow_ptr->table_type;

    /* Check type. */
    if(table_type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(table_id >= poflr_table_num_each_type[table_type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    tmp_tbl_ptr = & poflr_table_ptr[table_type][table_id];
    /* Check whether the table have already existed. */
    if(tmp_tbl_ptr->state != POFLR_STATE_VALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    /* Check the index. */
    if(index >= tmp_tbl_ptr->tbl_base_info.size){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_ENTRY_ID, g_recv_xid);
    }

    tmp_vhal_entry_ptr = &tmp_tbl_ptr->entry_ptr[index];
    /* Check whether the index have already existed. */
    if(tmp_vhal_entry_ptr->state != POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_ENTRY_EXIST, g_recv_xid);
    }

#ifdef POF_ADD_ENTRY_CHECK_ON
    /* Check whether a same flow entry have already exsited in the table. */
	if(table_type != POF_LINEAR_TABLE){
		ret = poflr_check_flow_in_table(flow_ptr, tmp_tbl_ptr);
		POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
	}
#endif // POF_ADD_ENTRY_CHECK_ON

    /* Initialize the counter_id. */
	ret = poflr_counter_init(flow_ptr->counter_id);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Create entry. */
    memcpy(&tmp_vhal_entry_ptr->entry, flow_ptr, sizeof(pof_flow_entry));
    tmp_vhal_entry_ptr->state = POFLR_STATE_VALID;
    tmp_tbl_ptr->entry_num++;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Add flow entry SUC! Totally %d entries in this table.",
            tmp_tbl_ptr->entry_num);
	POF_LOG_LOCK_ON;
    poflp_flow_entry(flow_ptr);
    POF_DEBUG_CPRINT(1,WHITE,"\n");
	POF_LOG_LOCK_OFF;
    return POF_OK;
}

/***********************************************************************
 * Modify a flow entry.
 * Form:     uint32_t poflr_modify_flow_entry(pof_flow_entry *flow_ptr)
 * Input:    flow entry
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will modify a flow entry.
 ***********************************************************************/
uint32_t poflr_modify_flow_entry(pof_flow_entry *flow_ptr){
    poflr_flow_entry *tmp_vhal_entry_ptr;
    poflr_flow_table *tmp_tbl_ptr;
    uint32_t index = flow_ptr->index, ret;
    uint8_t  table_id = flow_ptr->table_id;
    uint8_t  table_type = flow_ptr->table_type;

    /* Check type. */
    if(table_type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(table_id >= poflr_table_num_each_type[table_type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    tmp_tbl_ptr = & poflr_table_ptr[table_type][table_id];
    /* Check whether the table have already existed. */
    if(tmp_tbl_ptr->state != POFLR_STATE_VALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    /* Check index. */
    if( index >= tmp_tbl_ptr->tbl_base_info.size ){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_ENTRY_ID, g_recv_xid);
    }

    tmp_vhal_entry_ptr = &tmp_tbl_ptr->entry_ptr[index];
    /* Check whether the index have already existed. */
    if( tmp_vhal_entry_ptr->state != POFLR_STATE_VALID ){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_ENTRY_UNEXIST, g_recv_xid);
    }

#ifdef POF_ADD_ENTRY_CHECK_ON
    /* Check whether a same flow entry have already exsited in the table. */
    ret = poflr_check_flow_in_table(flow_ptr, tmp_tbl_ptr);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#endif // POF_ADD_ENTRY_CHECK_ON

    /* Check the counter id in the flow entry. */
    if(tmp_vhal_entry_ptr->entry.counter_id != flow_ptr->counter_id){
        /* Initialize the counter_id. */
        ret = poflr_counter_init(flow_ptr->counter_id);
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }

    /* Modify entry. */
    memcpy(&tmp_vhal_entry_ptr->entry, flow_ptr, sizeof(pof_flow_entry));

    POF_DEBUG_CPRINT_FL(1,GREEN,"Modify flow entry SUC!");
    return POF_OK;
}

/***********************************************************************
 * Delete a flow entry.
 * Form:     uint32_t poflr_delete_flow_entry(pof_flow_entry *flow_ptr)
 * Input:    flow entry
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will delete a flow entry in the flow table.
 ***********************************************************************/
uint32_t poflr_delete_flow_entry(pof_flow_entry *flow_ptr){
    poflr_flow_entry *tmp_vhal_entry_ptr;
    poflr_flow_table *tmp_tbl_ptr;
    uint32_t index = flow_ptr->index, ret;
    uint8_t  table_id = flow_ptr->table_id;
    uint8_t  table_type = flow_ptr->table_type;

    /* Check type. */
    if(table_type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_TYPE, g_recv_xid);
    }

    /* Check table_id. */
    if(table_id >= poflr_table_num_each_type[table_type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    tmp_tbl_ptr = & poflr_table_ptr[table_type][table_id];
    /* Check whether the table have already existed. */
    if(tmp_tbl_ptr->state != POFLR_STATE_VALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_TABLE_ID, g_recv_xid);
    }

    /* Check index. */
    if( index >= tmp_tbl_ptr->tbl_base_info.size ){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_ENTRY_ID, g_recv_xid);
    }

    tmp_vhal_entry_ptr = &tmp_tbl_ptr->entry_ptr[index];
    /* Check whether the index have already existed. */
    if( tmp_vhal_entry_ptr->state != POFLR_STATE_VALID ){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_ENTRY_UNEXIST, g_recv_xid);
    }

    /* Delete the counter. */
    ret = poflr_counter_delete(tmp_vhal_entry_ptr->entry.counter_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Delete and initialize the flow entry. */
    memset(tmp_vhal_entry_ptr, 0, sizeof(poflr_flow_entry));
    tmp_tbl_ptr->entry_num--;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Delete flow entry SUC!");
    return POF_OK;
}

/* Initialize flow table resource. */
uint32_t poflr_init_flow_table(){
    uint32_t i;

    /* Initialize the key length of each table type. */
    for(i=0; i<POF_MAX_TABLE_TYPE; i++){
        poflr_key_len_each_type[i] = poflr_key_len;
    }

    /* Initialize the table number of each type. */
    poflr_table_num_each_type[0] = poflr_mm_tbl_num;
    poflr_table_num_each_type[1] = poflr_lpm_tbl_num;
    poflr_table_num_each_type[2] = poflr_em_tbl_num;
    poflr_table_num_each_type[3] = poflr_dt_tbl_num;

    /* Initialize the table id base value of all types. */
    poflr_key_tid_base_each_type[0] = 0;
    poflr_key_tid_base_each_type[1] = poflr_mm_tbl_num;
    poflr_key_tid_base_each_type[2] = poflr_mm_tbl_num + poflr_lpm_tbl_num;
    poflr_key_tid_base_each_type[3] = poflr_mm_tbl_num + poflr_lpm_tbl_num + poflr_em_tbl_num;

    /* Initialize the flow table resource. */
    for(i = 0; i < POF_MAX_TABLE_TYPE; i++){
        poflr_table_ptr[i] = (poflr_flow_table*)malloc(poflr_table_num_each_type[i] * sizeof(poflr_flow_table));
		if(poflr_table_ptr[i] == NULL){
			poflr_free_table_resource();
			POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
		}
        memset((poflr_table_ptr[i]), 0, poflr_table_num_each_type[i] * sizeof(poflr_flow_table));
    }

	return POF_OK;
}

/* Free the flow table resource. */
uint32_t poflr_free_flow_table(){
	uint32_t i, j;

	for(i=0; i<POF_MAX_TABLE_TYPE; i++){
		if(NULL != poflr_table_ptr[i]){
			for(j=0; j<poflr_table_num_each_type[i]; j++){
				free(poflr_table_ptr[j][i].entry_ptr);
			}
		}
		free(poflr_table_ptr[i]);
	}
	return POF_OK;
}

/* Empty flow table. */
uint32_t poflr_empty_flow_table(){
    poflr_flow_table *tmp_tbl_ptr;
    uint32_t type, table_id;

    /* Empty the flow entry, and flow tabel. */
    for(type=0; type<POF_MAX_TABLE_TYPE; type++){
        for(table_id=0; table_id<poflr_table_num_each_type[type]; table_id++){
            tmp_tbl_ptr = &poflr_table_ptr[type][table_id];
            memset(tmp_tbl_ptr->entry_ptr, 0, sizeof(poflr_flow_entry)*tmp_tbl_ptr->tbl_base_info.size);
            free(tmp_tbl_ptr->entry_ptr);
            memset(tmp_tbl_ptr, 0, sizeof(poflr_flow_table));
        }
    }

	return POF_OK;
}

/***********************************************************************
 * Mapping the table id each type to global table ID.
 * Form:     uint32_t poflr_table_id_to_ID (uint8_t type, uint8_t id, uint8_t *ID_ptr)
 * Input:    table type, table id
 * Output:   global table ID.
 * Return:   POF_OK or Error code
 * Discribe: This function calculate the global table ID according to the
 *           table id each table type.
 ***********************************************************************/
uint32_t poflr_table_id_to_ID (uint8_t type, uint8_t id, uint8_t *ID_ptr){

    /* Ckeck the table type. */
    if(type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_BAD_TABLE_TYPE, g_upward_xid++);
    }

    /* Check the table id. */
    if(id >= poflr_table_num_each_type[type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_BAD_TABLE_ID, g_upward_xid++);
    }

    /* Calculate the global table ID */
    *ID_ptr = id + poflr_key_tid_base_each_type[type];
    return POF_OK;
}

/***********************************************************************
 * Mapping the global table ID to the table id each type.
 * Form:     uint32_t poflr_table_ID_to_id (uint8_t table_ID, uint8_t *type_ptr, uint8_t *table_id_ptr){
 * Input:    global table ID
 * Output:   table type, table id
 * Return:   POF_OK or Error code
 * Discribe: This function calculate the table type and the table id
 *           according to the global table ID.
 ***********************************************************************/
uint32_t poflr_table_ID_to_id (uint8_t table_ID, uint8_t *type_ptr, uint8_t *table_id_ptr){
    uint8_t type = 0xff, table_id, tmp_table_num = 0, i;

    for(i=0; i<POF_MAX_TABLE_TYPE; i++){
        tmp_table_num += poflr_table_num_each_type[i];
        if(table_ID < tmp_table_num){
            type = i;
            table_id = table_ID - (tmp_table_num - poflr_table_num_each_type[i]);
            break;
        }
    }

    /* No mapping. */
    if(type == 0xff){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_BAD_TABLE_ID, g_upward_xid++);

    }

    *type_ptr = type;
    *table_id_ptr = table_id;
    return POF_OK;
}

/* Get flow tables. */
uint32_t poflr_get_flow_table(poflr_flow_table **flow_table_ptrptr, uint8_t table_type, uint8_t table_id){

    /* Ckeck the table type. */
    if(table_type >= POF_MAX_TABLE_TYPE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_BAD_TABLE_TYPE, g_upward_xid++);
    }

    /* Check the table id. */
    if(table_id >= poflr_table_num_each_type[table_type]){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_BAD_TABLE_ID, g_upward_xid++);
    }

	*flow_table_ptrptr = &(poflr_table_ptr[table_type][table_id]);
	return POF_OK;
}

/* Check flow table if exist. */
uint32_t poflr_check_flow_table_exist(uint8_t ID){
	uint8_t table_id = 0xff, table_type = 0xff;

	pthread_testcancel();

    poflr_table_ID_to_id(ID, &table_type, &table_id);

	if(poflr_table_ptr[table_type][table_id].state == POFLR_STATE_INVALID){
		return POF_ERROR;
	}
	
	return POF_OK;
}

/* Get table number. */
uint32_t poflr_get_table_number(uint8_t **table_number_ptrptr){
	*table_number_ptrptr = poflr_table_num_each_type;
	return POF_OK;
}

/* Get table ID's base value. */
uint32_t poflr_get_table_id_base(uint8_t **table_id_base_ptrptr){
	*table_id_base_ptrptr = poflr_key_tid_base_each_type;
	return POF_OK;
}

/* Get key len. */
uint32_t poflr_get_key_len(uint16_t **key_len_ptrptr){
	*key_len_ptrptr = poflr_key_len_each_type;
	return POF_OK;
}

/* Get flow table size. */
uint32_t poflr_get_table_size(uint32_t *table_size_ptr){
	*table_size_ptr = poflr_flow_table_size;
	return POF_OK;
}

/* Set MM table number. */
uint32_t poflr_set_MM_table_number(uint8_t MMTableNum){
	poflr_mm_tbl_num = MMTableNum;
	return POF_OK;
}

/* Set LPM table number. */
uint32_t poflr_set_LPM_table_number(uint8_t LPMTableNum){
	poflr_lpm_tbl_num = LPMTableNum;
	return POF_OK;
}

/* Set EM table number. */
uint32_t poflr_set_EM_table_number(uint8_t EMTableNum){
	poflr_em_tbl_num = EMTableNum;
	return POF_OK;
}

/* Set DT table number. */
uint32_t poflr_set_DT_table_number(uint8_t DTTableNum){
	poflr_dt_tbl_num = DTTableNum;
	return POF_OK;
}

/* Set flow table size. */
uint32_t poflr_set_flow_table_size(uint32_t flow_table_size){
	poflr_flow_table_size = flow_table_size;
	return POF_OK;
}

/* Set the key length. */
uint32_t poflr_set_key_len(uint32_t key_len){
	poflr_key_len = key_len;
	return POF_OK;
}
