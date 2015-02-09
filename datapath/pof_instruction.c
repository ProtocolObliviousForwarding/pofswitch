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
#include "../include/pof_log_print.h"
#include "../include/pof_conn.h"
#include "../include/pof_datapath.h"
#include "../include/pof_byte_transfer.h"
#include <string.h>

#ifdef POF_DATAPATH_ON

static void pofdp_find_key(uint8_t *packet, uint8_t *metadata, uint8_t **key_ptr, uint8_t match_field_num, const pof_match *match);
static uint32_t pofdp_entry_nomatch(const struct pofdp_packet *dpp);

/* Update instruction pointer and number in dpp when one instruction
 * has been done. */
static void 
instruction_update(struct pofdp_packet *dpp)
{
	dpp->ins_done_num++;
	if(--dpp->ins_todo_num < 1){
		dpp->packet_done = TRUE;
		return;
	}
	dpp->ins++;
	return;
}

static uint32_t
move_packet_offset_forward(struct pofdp_packet *dpp, uint32_t offset)
{
	if(offset > dpp->left_len){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
	}
	dpp->left_len -= offset;
	dpp->offset += offset;
	dpp->buf_offset = dpp->buf + dpp->offset;
	return POF_OK;
}

static uint32_t
move_packet_offset_backward(struct pofdp_packet *dpp, uint32_t offset)
{
	if(offset > dpp->offset){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_BAD_OFFSET, g_upward_xid++);
	}
	dpp->left_len += offset;
	dpp->offset -= offset;
	dpp->buf_offset = dpp->buf + dpp->offset;
	return POF_OK;
}

/***********************************************************************
 * Find the packet key before forwarding to the table.
 * Form:     uint32_t pofdp_find_key(uint8_t *packet, \
 *                                   uint8_t *metadata, \
 *                                   uint8_t **key_ptr, \
 *                                   uint8_t match_field_num, \
 *                                   const pof_match *match)
 * Input:    packet, metadata, match field number, match data
 * Output:   key
 * Return:   POF_OK or Error code
 * Discribe: This function get the key of packet, according to the match
 *           data. This function will be called when the POFIT_GOTO_TABLE
 *           instruction is processing. We lookup the packet into flow
 *           table using the packet key builded in this function.
 ***********************************************************************/
static void pofdp_find_key(uint8_t *packet, uint8_t *metadata, uint8_t **key_ptr, uint8_t match_field_num, const pof_match *match){
    pof_match tmp_match;
    uint32_t  ret;
    uint8_t   i;

    for(i=0; i<match_field_num; i++){
        tmp_match = match[i];
        if(tmp_match.field_id != 0xFFFF){
            pofdp_copy_bit(packet, key_ptr[i], tmp_match.offset, tmp_match.len);
        }
        if(tmp_match.field_id == 0xFFFF){
            pofdp_copy_bit(metadata, key_ptr[i], tmp_match.offset, tmp_match.len);
        }
    }

    return;
}

/***********************************************************************
 * Processing the packet when no entry matche.
 * Form:     static uint32_t pofdp_entry_nomatch(uint8_t *packet, \
 *                                               uint16_t *len_B_ptr, \
 *                                               pof_flow_table *table_ptr)
 * Input:    packet, length of packet, current table info
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function handle the situation if there is no entry can
 *           match the pakcet key. Drop or send forward to the Controller.
 ***********************************************************************/
static uint32_t pofdp_entry_nomatch(const struct pofdp_packet *dpp){
    uint32_t ret;
    uint8_t  table_ID;

#if (POF_NOMATCH == POF_NOMATCH_PACKET_IN)
    POF_DEBUG_CPRINT_FL(1,YELLOW,"Send the packet which does NOT match " \
            "any entry in the table[%d][%d] to the controller", dpp->table_type, dpp->table_id);

    /* Current table ID. */
    ret = poflr_table_id_to_ID(dpp->table_type, dpp->table_id, &table_ID);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Send the no match packet upward to the Controller. */
    ret = pofdp_send_packet_in_to_controller(dpp->offset + dpp->left_len, \
			POFR_NO_MATCH, table_ID, 0, POF_FE_ID, dpp->buf);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

#elif (POF_NOMATCH == POF_NOMATCH_DROP)
    POF_DEBUG_CPRINT_FL(1,YELLOW,"Drop the packet which does NOT match " \
            "any entry in the table[%d][%d].", dpp->table_type, dpp->table_id);
#endif

    return POF_OK;
}

/***********************************************************************
 * Handle the instruction with POFIT_METER type.
 * Form:     uint32_t pofdp_instruction_handle_meter(uint8_t *packet, \
 *                                                   uint8_t *ins_data, \
                                                     struct pofdp_metadata *metadata, \
 *                                                   uint32_t *packet_over, \
 *                                                   pof_meter *meter)
 * Input:    packet, length of packet, instruction data
 * Output:   packet over identifier, meter
 * Return:   POF_OK or Error code
 * Discribe: This function load the meter table, and copy the meter value,
 *           according to the meter_id given in the ins_data, to the meter.
 *           Rate-limiting is not supported yet.
 * NOTE:     If there is an ERROR, The packet_over identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_METER(POFDP_ARG)
{
    pof_instruction_meter *p = (pof_instruction_meter *)dpp->ins->instruction_data;
    poflr_meters *meter_ptr = NULL;
    uint32_t index;

    index = p->meter_id;
    poflr_get_meter(&meter_ptr);

    /* Check the meter id. */
    if(meter_ptr->state[index] == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_UNKNOWN_METER, g_upward_xid++);
    }

	dpp->rate = meter_ptr->meter[index].rate;
    POF_DEBUG_CPRINT_FL(1,GREEN,"instruction_meter has been DONE! meter_id = %u, rate = %u", \
			meter_ptr->meter[index].meter_id, meter_ptr->meter[index].rate);

	instruction_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the instruction with POFIT_WRITE_METADATA type.
 * Form:     uint32_t pofdp_instruction_handle_write_metadata(uint8_t *packet, \
 *                                                            uint8_t *ins_data, \
                                                              struct pofdp_metadata *metadata, \
 *                                                            uint32_t *packet_over)
 * Input:    instruction data
 * Output:   packet over identifier, metadata
 * Return:   POF_OK or Error code
 * Discribe: This function write the piece of metadata. The start position,
 *           the length, and the value is given in ins_data.
 * NOTE:     If there is an ERROR, The packet_over identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_WRITE_METADATA(POFDP_ARG)
{
    pof_instruction_write_metadata *p = \
			(pof_instruction_write_metadata *)dpp->ins->instruction_data;
    uint32_t value = p->value;
    uint32_t ret;
	pof_match pm = {POFDP_METADATA_FIELD_ID, p->metadata_offset, p->len};

	ret = pofdp_write_32value_to_field(value, &pm, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->metadata, POF_BITNUM_TO_BYTENUM_CEIL(p->metadata_offset+p->len),"instruction_write_metadata has been DONE! The metadata = ");

	instruction_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the instruction with POFIT_WRITE_METADATA_FROM_PACKET type.
 * Form:     uint32_t pofdp_instruction_handle_write_metadata_from_packet(uint8_t *packet, \
 *                                                                        uint8_t *ins_data, \
                                                                          struct pofdp_metadata *metadata, \
 *                                                                        uint32_t *packet_over)
 * Input:    packet, length of packet, instruction data
 * Output:   packet over identifier, metadata
 * Return:   POF_OK or Error code
 * Discribe: This function write the piece of metadata. The start position
 *           and the length of the piece is given in ins_data. The value
 *           is from the piece of paket, which is given in ins_data.
 * NOTE:     If there is an ERROR, The packet_over identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_WRITE_METADATA_FROM_PACKET(POFDP_ARG)
{
    pof_instruction_write_metadata_from_packet *p = \
                (pof_instruction_write_metadata_from_packet *)dpp->ins->instruction_data;
	struct pofdp_metadata *metadata = dpp->metadata;
    uint8_t  value[POFDP_METADATA_MAX_LEN];

    memset(value, 0, sizeof value);

    if(dpp->left_len * POF_BITNUM_IN_BYTE < p->len + p->packet_offset){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    pofdp_copy_bit(dpp->buf_offset, value, p->packet_offset, p->len);
    pofdp_cover_bit((uint8_t *)metadata, value, p->metadata_offset, p->len);

    POF_DEBUG_CPRINT_FL_0X(1,GREEN,metadata, POF_BITNUM_TO_BYTENUM_CEIL(p->metadata_offset+p->len),"instruction_write_metadata_from_packet has been DONE! The metadata = ");

	instruction_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the instruction with POFIT_GOTO_DIRECT_TABLE type.
 * Form:     uint32_t pofdp_instruction_handle_goto_direct_table(uint8_t *packet, \
 *                                                               uint8_t *ins_data, \
 *                                                               uint8_t *table_type, \
 *                                                               uint8_t *table_id, \
 *                                                               uint32_t *packet_over, \
 *                                                               pof_flow_entry **entry_ptrptr)
 * Input:    packet, length of packet, instruction data
 * Output:   table type, table id, packet over identifier, Entry
 * Return:   POF_OK or Error code
 * Discribe: This function forward the packet to the direct table. The
 *           type and id of the next table will be calculated according
 *           to the table_ID which is global number given in ins_data.
 * NOTE:     If there is an ERROR, The packet_over identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_GOTO_DIRECT_TABLE(POFDP_ARG)
{
    pof_instruction_goto_direct_table *p = \
			(pof_instruction_goto_direct_table *)dpp->ins->instruction_data;
    poflr_flow_table *tmp_table;
    poflr_flow_entry *tmp_entry;
    uint8_t *table_type = &dpp->table_type;
    uint8_t *table_id = &dpp->table_id;
    uint32_t i, ret, entry_index;

    p = (pof_instruction_goto_direct_table *)dpp->ins->instruction_data;

    /* The packet forward to the next table. */
	ret = move_packet_offset_forward(dpp, p->packet_offset);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Find the table type and id of the next direct table. */
    ret = poflr_table_ID_to_id(p->next_table_id, table_type, table_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	entry_index = p->table_entry_index;

    poflr_get_flow_table(&tmp_table, *table_type, *table_id);
    POF_DEBUG_CPRINT_FL(1,YELLOW,"Go to DT table[%d][%d][%d]!", *table_type, *table_id, entry_index);

    /* Check the table type. */
    if(*table_type != POF_LINEAR_TABLE){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBIC_BAD_TABLE_TYPE, g_upward_xid++);
    }

    /* Check the table state. */
    if(tmp_table->state == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBIC_TABLE_UNEXIST, g_upward_xid++);
    }

    tmp_entry = &(tmp_table->entry_ptr[entry_index]);
    /* Check the flow entry. */
    if(tmp_entry->state == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBIC_ENTRY_UNEXIST, g_upward_xid++);
    }

    /* Load the flow entry data. */
    dpp->flow_entry = &tmp_entry->entry;

    /* Increase the counter value. */
    ret = poflr_counter_increace(dpp->flow_entry->counter_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Update the instruction number and the instruction data corresponding to the
     * matched flow entry in the current flow table. */
    dpp->ins = dpp->flow_entry->instruction;
    dpp->ins_todo_num = dpp->flow_entry->instruction_num;
	dpp->ins_done_num = 0;

    POF_DEBUG_CPRINT_FL(1,GREEN,"Match entry! ");

    return POF_OK;
}

/***********************************************************************
 * Handle the instruction with POFIT_GOTO_TABLE type.
 * Form:     uint32_t pofdp_instruction_handle_goto_table(uint8_t **pp_Packet, \
 *                                                        uint8_t *ins_data, \
                                                          struct pofdp_metadata *metadata, \
 *                                                        uint8_t *table_type, \
 *                                                        uint8_t *table_id, \
 *                                                        uint32_t *packet_over, \
 *                                                        pof_flow_entry **entry_ptrptr)
 * Input:    packet, length of packet, instruction data
 * Output:   packet, length of packet, table type, table id,
 *           packet over identifier, Entry
 * Return:   POF_OK or Error code
 * Discribe: This function forward the packet to the next table. The
 *           type and id of the next table will be calculated according
 *           to the table_ID which is global number given in ins_data.
 *           Before forwarding the packet to the next table, the packet key
 *           will be calculated first. After forwarding, we lookup the
 *           matched flow entry in the table using the packet key. Then,
 *           the infomation of the matched flow entry will be write in
 *           the entry_ptrptr.
 * NOTE:     If there is an ERROR, The packet_over identifier will be TRUE.
 *           If there is no flow entry can match the packet key, the
 *           packet_over identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_GOTO_TABLE(POFDP_ARG)
{
    struct pof_instruction_goto_table *p = \
				(pof_instruction_goto_table *)dpp->ins->instruction_data;
    poflr_flow_table *table_vhal_ptr;
    uint32_t i, j, ret = POF_OK;
    uint8_t *table_type = &dpp->table_type;
    uint8_t *table_id = &dpp->table_id;
    uint8_t  **key_ptr;

    p = (pof_instruction_goto_table *)dpp->ins->instruction_data;

    /* The packet forward to the next table. */
	ret = move_packet_offset_forward(dpp, p->packet_offset);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* The table type and id. */
    ret = poflr_table_ID_to_id(p->next_table_id, table_type, table_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,YELLOW,"Go to table[%d][%d]!", *table_type, *table_id);

    poflr_get_flow_table(&table_vhal_ptr, *table_type, *table_id);

    /* Find the key corresponding to the next table infomation. */
    key_ptr = (uint8_t **)malloc(POF_MAX_MATCH_FIELD_NUM * sizeof(ptr_t));
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(key_ptr);
    for(i=0; i<POF_MAX_MATCH_FIELD_NUM;  i++){
        key_ptr[i] = (uint8_t *)malloc(POF_MAX_FIELD_LENGTH_IN_BYTE);
        if(key_ptr[i] == NULL){
            for(j=0; j<i; j++){
                free(key_ptr[j]);
            }
            free(key_ptr);
            POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
        }
    }
    pofdp_find_key(dpp->buf_offset, (uint8_t *)dpp->metadata, (uint8_t **)(key_ptr), 
			table_vhal_ptr->tbl_base_info.match_field_num, table_vhal_ptr->tbl_base_info.match);

    /* Lookup the flow entry which matches the packet in the next table. */
    if(pofdp_lookup_in_table((uint8_t **)key_ptr, table_vhal_ptr->tbl_base_info.match_field_num, \
             *table_vhal_ptr, &dpp->flow_entry) != POF_OK){

        /* No match. */
        POF_DEBUG_CPRINT_FL(1,RED,"Cannot find the right entry in table[%d][%d]!",*table_type,*table_id);

        ret = pofdp_entry_nomatch(dpp);
        POF_CHECK_RETVALUE_NO_RETURN_NO_UPWARD(ret);

        dpp->packet_done = TRUE;
    }else{

        /* Match. Increace the counter value. */
        ret = poflr_counter_increace(dpp->flow_entry->counter_id);
        POF_CHECK_RETVALUE_NO_RETURN_NO_UPWARD(ret);

        /* Update the instruction number and the instruction data corresponding to the
         * matched flow entry in the current flow table. */
        dpp->ins = dpp->flow_entry->instruction;
        dpp->ins_todo_num = dpp->flow_entry->instruction_num;
		dpp->ins_done_num = 0;
    }

    /* Free the memory. */
    for(i=0; i<POF_MAX_MATCH_FIELD_NUM; i++){
        free(key_ptr[i]);
    }
    free(key_ptr);

    return ret;
}

/***********************************************************************
 * Handle the instruction with POFIT_APPLY_ACTIONS type.
 * Form:     uint32_t pofdp_instruction_handle_apply_action(uint8_t *packet, \
 *                                                          uint8_t *ins_data, \
                                                            struct pofdp_metadata *metadata, \
 *                                                          uint32_t *packet_over, \
 *                                                          pof_flow_entry *entry_ptr)
 * Input:    packet, length of packet, instruction data, metadata,
 *           flow entry
 * Output:   packet, length of packet, packet over identifier
 * Return:   POF_OK or Error code
 * Discribe: This function apply some actions to the
 *           packet, such as ADD_TAG, OUTPUT, DROP and so on. The action
 *           data has been given in the ins_data. According to the different
 *           type of the action, the actions data have different format
 *           and different content.
 * NOTE:     If there is an ERROR, The packet_over identifier will be TRUE.
 *           During the process of applying action, the packet_over
 *           identifier might be changed to TRUE in some situation such
 *           as DROP, OUTPUT, TOCP action types, or an ERROR occurs in
 *           the process of applying action.
 ***********************************************************************/
static uint32_t execute_APPLY_ACTIONS(POFDP_ARG)
{
    pof_instruction_apply_actions *p = \
			(pof_instruction_apply_actions *)dpp->ins->instruction_data;
    uint32_t    ret;

    dpp->act = (pof_action *)p->action;
    dpp->act_num = p->action_num;

    ret = pofdp_action_execute(dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	instruction_update(dpp);
    return POF_OK;
}

/* Caller should make sure that len is less than 32, and the sum of offset and
 * len is less than the buf size. */
static void
write_32value_to_buf(uint8_t *buf, uint32_t value, \
						   uint16_t offset, uint16_t len)
{
	/* Transform the value format. */
	value = POF_MOVE_BIT_LEFT(value, 32 - len);
	POF_NTOHL_FUNC(value);

    pofdp_cover_bit(buf, (uint8_t *)&value, offset, len);
	return;
}

/* write value to packet/metadta field. */
uint32_t
pofdp_write_32value_to_field(uint32_t value, const struct pof_match *pm, \
							 struct pofdp_packet *dpp)
{
	uint8_t *dst;

	if(pm->len > 32){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_MATCH, POFBMC_BAD_LEN, g_upward_xid++);
	}

	/* Copy data from packet/metadata to value. */
	if(pm->field_id != POFDP_METADATA_FIELD_ID){
		if(pm->len + pm->offset > dpp->left_len * POF_BITNUM_IN_BYTE){
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
		}
		dst = dpp->buf_offset;
	}else{
		if(pm->len + pm->offset > dpp->metadata_len * POF_BITNUM_IN_BYTE){
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_METADATA_LEN_ERROR, g_upward_xid++);
		}
		dst = (uint8_t *)dpp->metadata;
	}

	write_32value_to_buf(dst, value, pm->offset, pm->len);

	return POF_OK;
}

/* Caller should make sure that len is less than 32, and the sum of offset and
 * len is less than the buf size. */
static void
pofdp_get_32value_from_buf(uint8_t *buf, uint32_t *value, \
						   uint16_t offset, uint16_t len)
{
	pofdp_copy_bit(buf, (uint8_t *)value, offset, len);

	/* Transform the value format. */
	POF_NTOHL_FUNC(*value);
	*value = POF_MOVE_BIT_RIGHT(*value, 32 - len);

	return;
}

/* get value from packet/metadta field. */
uint32_t
get_32value_from_field(uint32_t *value, const struct pof_match *pm, \
							 const struct pofdp_packet *dpp)
{
	uint8_t *src;
	if(pm->len > 32){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_MATCH, POFBMC_BAD_LEN, g_upward_xid);
	}

	/* Copy data from packet/metadata to value. */
	if(pm->field_id != POFDP_METADATA_FIELD_ID){
		if(pm->len + pm->offset > dpp->left_len * POF_BITNUM_IN_BYTE){
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid);
		}
		src = dpp->buf_offset;
	}else{
		if(pm->len + pm->offset > dpp->metadata_len * POF_BITNUM_IN_BYTE){
			POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_METADATA_LEN_ERROR, g_upward_xid);
		}
		src = (uint8_t *)dpp->metadata;
	}

	pofdp_get_32value_from_buf(src, value, pm->offset, pm->len);

	return POF_OK;
}

uint32_t
pofdp_get_32value(uint32_t *value, uint8_t type, void *u_, const struct pofdp_packet *dpp)
{
	union {
		uint32_t value;
		struct pof_match field;
	} *u = u_;
	uint32_t ret;

	if(type == POFVT_IMMEDIATE_NUM){
		*value = u->value;
		return POF_OK;
	}

	ret = get_32value_from_field(value, &u->field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	return POF_OK;
}

static uint32_t 
execute_WRITE_ACTIONS(POFDP_ARG)
{
	// TODO
    POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_UNSUP_INST, g_upward_xid++);
}

static uint32_t 
execute_CLEAR_ACTIONS(POFDP_ARG)
{
	// TODO
    POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_UNSUP_INST, g_upward_xid++);
}

static uint32_t 
execute_EXPERIMENTER(POFDP_ARG)
{
	// TODO
    POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_UNSUP_INST, g_upward_xid++);
}

uint32_t pofdp_instruction_execute(POFDP_ARG)
{
	uint32_t ret = POF_OK;
    /* Forward the packet via executing the instructions until packet_over is TRUE or all
     * instructions have been done. */
    while(dpp->packet_done == FALSE){
        /* Execute the instructions. */
        switch(dpp->ins->type){
#define INSTRUCTION(NAME,VALUE) case POFIT_##NAME: ret = execute_##NAME(dpp); break;
			INSTRUCTIONS
#undef INSTRUCTION
            default:
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_UNKNOWN_INST, g_upward_xid++);
				break;
        }
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }
	return POF_OK;
}

#endif // POF_DATAPATH_ON
