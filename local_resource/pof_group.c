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
#include "string.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "string.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "arpa/inet.h"

/* The number of group. */
uint32_t poflr_group_number = POFLR_GROUP_NUMBER;

/* Group table. */
poflr_groups *poflr_group;

/***********************************************************************
 * Add the group entry.
 * Form:     uint32_t poflr_modify_group_entry(pof_group *group_ptr)
 * Input:    device id, group id, counter id, group type, action number,
 *           action data
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will add the group entry in the group table.
 ***********************************************************************/
uint32_t poflr_add_group_entry(pof_group *group_ptr){
    uint32_t ret;

    /* Check group_id. */
    if(group_ptr->group_id >= poflr_group_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_INVALID_GROUP, g_recv_xid);
    }
    if(poflr_group->state[group_ptr->group_id] == POFLR_STATE_VALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_GROUP_EXISTS, g_recv_xid);
    }

    /* Initialize the counter_id. */
    ret = poflr_counter_init(group_ptr->counter_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    memcpy(&poflr_group->group[group_ptr->group_id], group_ptr,  sizeof(pof_group));
    poflr_group->group_num ++;
    poflr_group->state[group_ptr->group_id] = POFLR_STATE_VALID;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Add group entry SUC!");
    return POF_OK;
}

/***********************************************************************
 * Modify the group entry.
 * Form:     uint32_t poflr_modify_group_entry(pof_group *group_ptr)
 * Input:    device id, group id, counter id, group type, action number,
 *           action data
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will modify the group entry in the group table.
 ***********************************************************************/
uint32_t poflr_modify_group_entry(pof_group *group_ptr){

    /* Check group_id. */
    if(group_ptr->group_id >= poflr_group_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_INVALID_GROUP, g_recv_xid);
    }
    if(poflr_group->state[group_ptr->group_id] == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_UNKNOWN_GROUP, g_recv_xid);
    }

    /* Check the counter_id. */
    if(group_ptr->counter_id != poflr_group->group[group_ptr->group_id].counter_id){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_BAD_COUNTER_ID, g_recv_xid);
    }

    memcpy(&poflr_group->group[group_ptr->group_id], group_ptr,  sizeof(pof_group));

    POF_DEBUG_CPRINT_FL(1,GREEN,"Modify group entry SUC!");
    return POF_OK;
}

/***********************************************************************
 * Delete the group entry.
 * Form:     uint32_t poflr_modify_group_entry(pof_group *group_ptr)
 * Input:    device id, group id, counter id, group type, action number,
 *           action data
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will delete the group entry in the group table.
 ***********************************************************************/
uint32_t poflr_delete_group_entry(pof_group *group_ptr){
    uint32_t ret;

    /* Check group_id. */
    if(group_ptr->group_id >= poflr_group_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_INVALID_GROUP, g_recv_xid);
    }
    if(poflr_group->state[group_ptr->group_id] == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_UNKNOWN_GROUP, g_recv_xid);
    }

    /* Delete the counter_id. */
    ret = poflr_counter_delete(group_ptr->counter_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    poflr_group->group_num --;
    poflr_group->state[group_ptr->group_id] = POFLR_STATE_INVALID;
    memset(&poflr_group->group[group_ptr->group_id], 0, sizeof(pof_group));

    POF_DEBUG_CPRINT_FL(1,GREEN,"Delete group entry SUC!");
    return POF_OK;
}

/* Initialize group resource. */
uint32_t poflr_init_group(){

    /* Initialize group table. */
    poflr_group = (poflr_groups *)malloc(sizeof(poflr_groups));
	if(poflr_group == NULL){
		poflr_free_table_resource();
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
    memset(poflr_group, 0, sizeof(poflr_groups));

    poflr_group->group = (pof_group *)malloc(sizeof(pof_group) * poflr_group_number);
	if(poflr_group->group == NULL){
		poflr_free_table_resource();
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
    memset(poflr_group->group, 0, sizeof(pof_group)*poflr_group_number);

    poflr_group->state = (uint32_t *)malloc(sizeof(uint32_t) * poflr_group_number);
	if(poflr_group->state == NULL){
		poflr_free_table_resource();
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
    memset(poflr_group->state, 0, sizeof(uint32_t) * poflr_group_number);

	return POF_OK;
}

/* Free group resource. */
uint32_t poflr_free_group(){
	if(NULL != poflr_group){
		free(poflr_group->group);
		free(poflr_group->state);
		free(poflr_group);
	}

	return POF_OK;
}

/* Empty group. */
uint32_t poflr_empty_group(){
    memset(poflr_group->group, 0, sizeof(pof_group) * poflr_group_number);
    memset(poflr_group->state, 0, sizeof(uint32_t) * poflr_group_number);
    poflr_group->group_num = 0;

	return POF_OK;
}

/* Get group table. */
uint32_t poflr_get_group(poflr_groups **group_ptrptr){
	*group_ptrptr = poflr_group;
	return POF_OK;
}

/* Get group number. */
uint32_t poflr_get_group_number(uint32_t *group_number_ptr){
	*group_number_ptr = poflr_group_number;
	return POF_OK;
}

/* Set group number. */
uint32_t poflr_set_group_number(uint32_t group_num){
	poflr_group_number = group_num;
	return POF_OK;
}
