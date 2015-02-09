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

/* The number of counter. */
uint32_t poflr_counter_number = POFLR_COUNTER_NUMBER;

/* Counter table. */
poflr_counters *poflr_counter;

/***********************************************************************
 * Initialize the counter corresponding the counter_id.
 * Form:     uint32_t poflr_counter_init(uint32_t counter_id)
 * Input:    counter id
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function will initialize the counter corresponding the
 *           counter_id. The initial counter value is zero.
 ***********************************************************************/
uint32_t poflr_counter_init(uint32_t counter_id){
    pof_counter *p;

	if(!counter_id){
		POF_DEBUG_CPRINT_FL(1,GREEN,"The counter_id 0 means that counter is no need.");
		return POF_OK;
	}

    /* Check counter id. */
    if(counter_id >= poflr_counter_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_recv_xid);
    }
    if(poflr_counter->state[counter_id] == POFLR_STATE_INVALID){
        poflr_counter->state[counter_id] = POFLR_STATE_VALID;
        poflr_counter->counter_num++;
        p = &poflr_counter->counter[counter_id];
        p->counter_id = counter_id;
        p->value = 0;
    }

    POF_DEBUG_CPRINT_FL(1,GREEN,"The counter[%u] has been initialized!", counter_id);
    return POF_OK;
}

/***********************************************************************
 * Delete the counter corresponding the counter_id.
 * Form:     uint32_t poflr_counter_delete(uint32_t counter_id)
 * Input:    counter id
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function will delete the counter.
 ***********************************************************************/
uint32_t poflr_counter_delete(uint32_t counter_id){
    pof_counter *p;

    if(!counter_id){
        return POF_OK;
    }

    /* Check counter id. */
    if(counter_id >= poflr_counter_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_recv_xid);
    }
    if(poflr_counter->state[counter_id] == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_COUNTER_UNEXIST, g_recv_xid);
    }

    poflr_counter->state[counter_id] = POFLR_STATE_INVALID;
    poflr_counter->counter_num--;
    p = &poflr_counter->counter[counter_id];
    p->counter_id = 0;
    p->value = 0;

    POF_DEBUG_CPRINT_FL(1,GREEN,"The counter[%u] has been deleted!", counter_id);
    return POF_OK;
}

/***********************************************************************
 * Cleare counter value.
 * Form:     uint32_t poflr_counter_clear(uint32_t counter_id)
 * Input:    device id, counter id
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will make the counter value corresponding to
 *           counter_id to be zero.
 ***********************************************************************/
uint32_t poflr_counter_clear(uint32_t counter_id){
    if(!counter_id){
        return POF_OK;
    }

    /* Check counter_id. */
    if(counter_id >= poflr_counter_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_recv_xid);
    }
    if(poflr_counter->state[counter_id] == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_COUNTER_UNEXIST, g_recv_xid);
    }

    /* Initialize the counter value. */
    pof_counter * tmp_counter_ptr = & (poflr_counter->counter[counter_id]);
    tmp_counter_ptr->value = 0;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Clear counter value SUC!");
    return POF_OK;
}

/***********************************************************************
 * Get counter value.
 * Form:     uint32_t poflr_get_counter_value(uint32_t counter_id)
 * Input:    device id, counter id
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will get the counter value corresponding to
 *           tht counter_id and send it to the Controller as reply
 *           through the OpenFlow channel.
 ***********************************************************************/
uint32_t poflr_get_counter_value(uint32_t counter_id){
    pof_counter counter = {0};

    /* Check counter_id. */
    if(!counter_id || counter_id >= poflr_counter_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_recv_xid);
    }
    if(counter_id && poflr_counter->state[counter_id] != POFLR_STATE_VALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_COUNTER_UNEXIST, g_recv_xid);
    }

    counter.command = POFCC_REQUEST;
    counter.counter_id = counter_id;
    counter.value = (poflr_counter->counter[counter_id]).value;
    pof_NtoH_transfer_counter(&counter);

	/* Delay 0.1s. */
	pofbf_task_delay(100);

    if(POF_OK != pofec_reply_msg(POFT_COUNTER_REPLY, g_recv_xid, sizeof(pof_counter), (uint8_t *)&counter)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }

    POF_DEBUG_CPRINT_FL(1,GREEN,"Get counter value SUC! counter id = %u, counter value = %llu", \
                        counter_id, counter.value);
    return POF_OK;
}

/***********************************************************************
 * Increace the counter
 * Form:     uint32_t poflr_counter_increace(uint32_t counter_id)
 * Input:    counter_id
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function increase the counter value corresponding to
 *           the counter_id by one.
 ***********************************************************************/
uint32_t poflr_counter_increace(uint32_t counter_id){
    pof_counter *p;

    if(!counter_id){
        return POF_OK;
    }

    /* Check the counter id. */
    if(counter_id >= poflr_counter_number){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COUNTER_ID, g_upward_xid++);
    }
    if(poflr_counter->state[counter_id] == POFLR_STATE_INVALID){
		poflr_counter_init(counter_id);
    }

    p = &poflr_counter->counter[counter_id];
    p->value++;

    POF_DEBUG_CPRINT_FL(1,GREEN,"The counter %d has increased, value = %llu", counter_id, p->value);
    return POF_OK;
}

/* Initialize counter resource. */
uint32_t poflr_init_counter(){

    /* Initialize counter table. */
    poflr_counter = (poflr_counters *)malloc(sizeof(poflr_counters));
	if(poflr_counter == NULL){
		poflr_free_table_resource();
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
    memset(poflr_counter, 0, sizeof(poflr_counters));

    poflr_counter->counter = (pof_counter *)malloc(sizeof(pof_counter) * poflr_counter_number);
	if(poflr_counter->counter == NULL){
		poflr_free_table_resource();
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
    memset(poflr_counter->counter, 0, sizeof(pof_counter) * poflr_counter_number);

    poflr_counter->state = (uint32_t *)malloc(sizeof(uint32_t) * poflr_counter_number);
	if(poflr_counter->state == NULL){
		poflr_free_table_resource();
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
    memset(poflr_counter->state, 0, sizeof(uint32_t) * poflr_counter_number);

	return POF_OK;
}

/* Free counter resource. */
uint32_t poflr_free_counter(){
	if(NULL != poflr_counter){
		free(poflr_counter->counter);
		free(poflr_counter->state);
		free(poflr_counter);
	}

	return POF_OK;
}

/* Empty counter. */
uint32_t poflr_empty_counter(){
    memset(poflr_counter->counter, 0, sizeof(pof_counter) * poflr_counter_number);
    memset(poflr_counter->state, 0, sizeof(uint32_t) * poflr_counter_number);
    poflr_counter->counter_num = 0;
	
	return POF_OK;
}

/* Get counter table. */
uint32_t poflr_get_counter(poflr_counters **counter_ptrptr){
	*counter_ptrptr = poflr_counter;
	return POF_OK;
}

/* Get counter number. */
uint32_t poflr_get_counter_number(uint32_t *counter_number_ptr){
	*counter_number_ptr = poflr_counter_number;
	return POF_OK;
}

/* Set counter number. */
uint32_t poflr_set_counter_number(uint32_t counter_num){
	poflr_counter_number = counter_num;
	return POF_OK;
}
