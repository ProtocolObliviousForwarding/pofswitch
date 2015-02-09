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

/* The number of meter. */
uint32_t poflr_meter_number = POFLR_METER_NUMBER;

/* Meter table. */
poflr_meters *poflr_meter;

/***********************************************************************
 * Add the meter.
 * Form:     uint32_t poflr_mod_meter_entry(uint32_t meter_id, uint32_t rate)
 * Input:    device id, meter id, rate
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will add the meter in the meter table.
 ***********************************************************************/
uint32_t poflr_add_meter_entry(uint32_t meter_id, uint32_t rate){
    pof_meter *meter;

    /* Check meter_id. */
    if(meter_id >= poflr_meter_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_INVALID_METER, g_recv_xid);
    }
    if(poflr_meter->state[meter_id] == POFLR_STATE_VALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_METER_EXISTS, g_recv_xid);
    }

    meter = &(poflr_meter->meter[meter_id]);
    meter->meter_id = meter_id;
    meter->rate = rate;

    poflr_meter->meter_num++;
    poflr_meter->state[meter_id] = POFLR_STATE_VALID;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Add meter SUC!");
    return POF_OK;
}

/***********************************************************************
 * Modify the meter.
 * Form:     uint32_t poflr_mod_meter_entry(uint32_t meter_id, uint32_t rate)
 * Input:    device id, meter id, rate
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will modify the meter in the meter table.
 ***********************************************************************/
uint32_t poflr_modify_meter_entry(uint32_t meter_id, uint32_t rate){
    pof_meter *meter;

    /* Check meter_id. */
    if(meter_id >= poflr_meter_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_INVALID_METER, g_recv_xid);
    }
    if(poflr_meter->state[meter_id] == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_UNKNOWN_METER, g_recv_xid);
    }

    meter = &(poflr_meter->meter[meter_id]);
    meter->meter_id = meter_id;
    meter->rate = rate;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Modify meter SUC!");
    return POF_OK;
}

/***********************************************************************
 * Delete the meter.
 * Form:     uint32_t poflr_mod_meter_entry(uint32_t meter_id, uint32_t rate)
 * Input:    device id, meter id, rate
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will add the meter in the meter table.
 ***********************************************************************/
uint32_t poflr_delete_meter_entry(uint32_t meter_id, uint32_t rate){
    pof_meter *meter;

    /* Check meter_id. */
    if(meter_id >= poflr_meter_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_INVALID_METER, g_recv_xid);
    }
    if(poflr_meter->state[meter_id] == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_UNKNOWN_METER, g_recv_xid);
    }

    meter = &(poflr_meter->meter[meter_id]);
    meter->meter_id = 0;
    meter->rate = 0;

    poflr_meter->meter_num--;
    poflr_meter->state[meter_id] = POFLR_STATE_INVALID;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Delete meter SUC!");
    return POF_OK;
}

/* Initialize meter resource. */
uint32_t poflr_init_meter(){

    /* Initialize meter table. */
    poflr_meter = (poflr_meters *)malloc(sizeof(poflr_meters));
	if(poflr_meter == NULL){
		poflr_free_table_resource();
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
    memset(poflr_meter, 0, sizeof(poflr_meters));

    poflr_meter->meter = (pof_meter *)malloc(sizeof(pof_meter) * poflr_meter_number);
	if(poflr_meter->meter == NULL){
		poflr_free_table_resource();
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
    memset(poflr_meter->meter, 0, sizeof(pof_meter) * poflr_meter_number);

    poflr_meter->state = (uint32_t *)malloc(sizeof(uint32_t) * poflr_meter_number);
	if(poflr_meter->state == NULL){
		poflr_free_table_resource();
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
    memset(poflr_meter->state, 0, sizeof(uint32_t) * poflr_meter_number);

	return POF_OK;
}

/* Free meter resource. */
uint32_t poflr_free_meter(){
	if(NULL != poflr_meter){
		free(poflr_meter->meter);
		free(poflr_meter->state);
		free(poflr_meter);
	}

	return POF_OK;
}

/* Empty meter. */
uint32_t poflr_empty_meter(){
    memset(poflr_meter->meter, 0, sizeof(pof_meter) * poflr_meter_number);
    memset(poflr_meter->state, 0, sizeof(uint32_t) * poflr_meter_number);
    poflr_meter->meter_num = 0;

	return POF_OK;
}

/* Get meter table. */
uint32_t poflr_get_meter(poflr_meters **meter_ptrptr){
	*meter_ptrptr = poflr_meter;
	return POF_OK;
}

/* Get meter number. */
uint32_t poflr_get_meter_number(uint32_t *meter_number_ptr){
	*meter_number_ptr = poflr_meter_number;
	return POF_OK;
}

/* Set meter number. */
uint32_t poflr_set_meter_number(uint32_t meter_num){
	poflr_meter_number = meter_num;
	return POF_OK;
}
