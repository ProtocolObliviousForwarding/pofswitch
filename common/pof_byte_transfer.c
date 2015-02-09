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
#include "../include/pof_byte_transfer.h"
#include "../include/pof_log_print.h"

static uint32_t match(void *ptr){
    pof_match *p = (pof_match *)ptr;

    POF_NTOHS_FUNC(p->field_id);
    POF_NTOHS_FUNC(p->offset);
    POF_NTOHS_FUNC(p->len);

    return POF_OK;
}

static uint32_t match_x(void *ptr){
    pof_match_x *p = (pof_match_x *)ptr;

    POF_NTOHS_FUNC(p->field_id);
    POF_NTOHS_FUNC(p->offset);
    POF_NTOHS_FUNC(p->len);

    return POF_OK;
}

static uint32_t
value(uint8_t type, void *ptr)
{
	union {
		uint32_t value;
		struct pof_match field;
	} *p = ptr;

	if(type == POFVT_IMMEDIATE_NUM){
		POF_NTOHL_FUNC(p->value);
	}else{
		match(&p->field);
	}
}

uint32_t pof_NtoH_transfer_error(void *ptr){
    pof_error *p = (pof_error *)ptr;

    POF_NTOHS_FUNC(p->type);
    POF_NTOHS_FUNC(p->code);
    POF_NTOHL_FUNC(p->device_id);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_packet_in(void *ptr){
    pof_packet_in *p = (pof_packet_in *)ptr;

    POF_NTOHL_FUNC(p->buffer_id);
    POF_NTOHS_FUNC(p->total_len);
    POF_NTOH64_FUNC(p->cookie);
    POF_NTOHL_FUNC(p->device_id);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_header(void *ptr){
    pof_header *head_p = (pof_header *)ptr;

    POF_NTOHS_FUNC(head_p->length);
    POF_NTOHL_FUNC(head_p->xid);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_header(void *ptr){
    pof_header *head_p = (pof_header *)ptr;

    POF_HTONS_FUNC(head_p->length);
    POF_HTONL_FUNC(head_p->xid);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_port(void *ptr){
    pof_port *p = (pof_port *)ptr;

    POF_NTOHL_FUNC(p->port_id);
    POF_NTOHL_FUNC(p->device_id);
    POF_NTOHL_FUNC(p->config);
    POF_NTOHL_FUNC(p->state);
    POF_NTOHL_FUNC(p->curr);
    POF_NTOHL_FUNC(p->advertised);
    POF_NTOHL_FUNC(p->supported);
    POF_NTOHL_FUNC(p->peer);
    POF_NTOHL_FUNC(p->curr_speed);
    POF_NTOHL_FUNC(p->max_speed);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_flow_table(void *ptr){
    pof_flow_table *p = (pof_flow_table *)ptr;
    uint32_t i;

    POF_NTOHL_FUNC(p->size);
    POF_NTOHS_FUNC(p->key_len);
    for(i=0; i<POF_MAX_MATCH_FIELD_NUM; i++)
        match((pof_match *)p->match + i);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_meter(void *ptr){
    pof_meter *p = (pof_meter *)ptr;

    POF_NTOHS_FUNC(p->rate);
    POF_NTOHL_FUNC(p->meter_id);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_counter(void *ptr){
    pof_counter *p = (pof_counter *)ptr;

    POF_NTOHL_FUNC(p->counter_id);
    POF_NTOH64_FUNC(p->value);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_packet_in(void *ptr){
    pof_packet_in *p = (pof_packet_in *)ptr;

    POF_HTONL_FUNC(p->buffer_id);
    POF_HTONS_FUNC(p->total_len);
    POF_HTON64_FUNC(p->cookie);
    POF_HTONL_FUNC(p->device_id);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_switch_features(void *ptr){
    pof_switch_features *p = (pof_switch_features *)ptr;

    POF_HTONL_FUNC(p->dev_id);
    POF_HTONS_FUNC(p->port_num);
    POF_HTONS_FUNC(p->table_num);
    POF_HTONL_FUNC(p->capabilities);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_table_resource_desc(void *ptr){
    pof_table_resource_desc *p = (pof_table_resource_desc *)ptr;

    POF_HTONL_FUNC(p->device_id);
    POF_HTONS_FUNC(p->key_len);
    POF_HTONL_FUNC(p->total_size);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_flow_table_resource(void *ptr){
    pof_flow_table_resource *p = (pof_flow_table_resource *)ptr;
    int i;

    POF_HTONL_FUNC(p->counter_num);
    POF_HTONL_FUNC(p->meter_num);
    POF_HTONL_FUNC(p->group_num);

    for(i=0;i<POF_MAX_TABLE_TYPE;i++)
        pof_HtoN_transfer_table_resource_desc((pof_table_resource_desc *)p->tbl_rsc_desc+i);

    return POF_OK;
}

static uint32_t port(void *ptr){
    pof_port *p = (pof_port *)ptr;

    POF_HTONL_FUNC(p->port_id);
    POF_HTONL_FUNC(p->device_id);
    POF_HTONL_FUNC(p->config);
    POF_HTONL_FUNC(p->state);
    POF_HTONL_FUNC(p->curr);
    POF_HTONL_FUNC(p->advertised);
    POF_HTONL_FUNC(p->supported);
    POF_HTONL_FUNC(p->peer);
    POF_HTONL_FUNC(p->curr_speed);
    POF_HTONL_FUNC(p->max_speed);

    return POF_OK;
}

uint32_t pof_HtoN_transfer_port_status(void *ptr){
    pof_port_status *p = (pof_port_status *)ptr;

    port((pof_port *)&(p->desc));

    return POF_OK;
}

uint32_t pof_HtoN_transfer_switch_config(void * ptr){
    pof_switch_config *p = (pof_switch_config *)ptr;

    POF_HTONS_FUNC(p->flags);
    POF_HTONS_FUNC(p->miss_send_len);

    return POF_OK;
}

static uint32_t action_OUTPUT(void *ptr){
    pof_action_output *p = (pof_action_output *)ptr;

    POF_NTOHL_FUNC(p->outputPortId);
    POF_NTOHS_FUNC(p->metadata_offset);
    POF_NTOHS_FUNC(p->metadata_len);
    POF_NTOHS_FUNC(p->packet_offset);

    return POF_OK;
}

static uint32_t action_ADD_FIELD(void *ptr){
    pof_action_add_field *p = (pof_action_add_field *)ptr;

    POF_NTOHS_FUNC(p->tag_id);
    POF_NTOHS_FUNC(p->tag_pos);
    POF_NTOHL_FUNC(p->tag_len);
    POF_NTOH64_FUNC(p->tag_value);

    return POF_OK;
}

static uint32_t action_DROP(void *ptr){
    pof_action_drop *p = (pof_action_drop *)ptr;

    POF_NTOHL_FUNC(p->reason_code);

    return POF_OK;
}

static uint32_t action_PACKET_IN(void *ptr){
    pof_action_packet_in *p = (pof_action_packet_in *)ptr;

    POF_NTOHL_FUNC(p->reason_code);

    return POF_OK;
}


static uint32_t action_DELETE_FIELD(void *ptr){
    pof_action_delete_field *p = (pof_action_delete_field *)ptr;

    POF_NTOHS_FUNC(p->tag_pos);
    POF_NTOHL_FUNC(p->tag_len);

    return POF_OK;
}

static uint32_t action_CALCULATE_CHECKSUM(void *ptr){
    pof_action_calculate_checksum *p = (pof_action_calculate_checksum *)ptr;

    POF_NTOHS_FUNC(p->checksum_pos);
    POF_NTOHS_FUNC(p->checksum_len);
    POF_NTOHS_FUNC(p->cal_startpos);
    POF_NTOHS_FUNC(p->cal_len);

    return POF_OK;
}

static uint32_t action_SET_FIELD(void *ptr){
    pof_action_set_field *p = (pof_action_set_field *)ptr;

    match_x(&p->field_setting);

    return POF_OK;
}

static uint32_t action_SET_FIELD_FROM_METADATA(void *ptr){
    pof_action_set_field_from_metadata *p = (pof_action_set_field_from_metadata *)ptr;

    match(&p->field_setting);
    POF_NTOHS_FUNC(p->metadata_offset);

    return POF_OK;
}

static uint32_t action_MODIFY_FIELD(void *ptr){
    pof_action_modify_field *p = (pof_action_modify_field *)ptr;

    match(&p->field);
    POF_NTOHL_FUNC(p->increment);

    return POF_OK;
}

static uint32_t action_COUNTER(void *ptr){
    pof_action_counter *p = (pof_action_counter *)ptr;

    POF_NTOHL_FUNC(p->counter_id);

    return POF_OK;
}

static uint32_t action_GROUP(void *ptr){
    pof_action_group *p = (pof_action_group *)ptr;

    POF_NTOHL_FUNC(p->group_id);

    return POF_OK;
}

static uint32_t 
action_EXPERIMENTER(void *ptr)
{
	POF_ERROR_CPRINT_FL_NO_LOCK(1,RED,"Unsupport action type.");
	return POF_ERROR;
}

static uint32_t action(void *ptr){
    pof_action *p = (pof_action *)ptr;

    POF_NTOHS_FUNC(p->type);
    POF_NTOHS_FUNC(p->len);

    switch(p->type){
#define ACTION(NAME,VALUE) case POFAT_##NAME: action_##NAME(p->action_data); break;
		ACTIONS
#undef ACTION
        default:
            POF_ERROR_CPRINT_FL_NO_LOCK(1,RED,"Unknow action type: %u", p->type);
            break;
    }
    return POF_OK;
}

/* Intructions. */
static uint32_t
instruction_WRITE_ACTIONS(void *ptr)
{
	POF_ERROR_CPRINT_FL_NO_LOCK(1,RED,"Unsupport instruction type.");
	return POF_ERROR;
}

static uint32_t
instruction_CLEAR_ACTIONS(void *ptr)
{
	POF_ERROR_CPRINT_FL_NO_LOCK(1,RED,"Unsupport instruction type.");
	return POF_ERROR;
}

static uint32_t
instruction_EXPERIMENTER(void *ptr)
{
	POF_ERROR_CPRINT_FL_NO_LOCK(1,RED,"Unsupport instruction type.");
	return POF_ERROR;
}

static uint32_t instruction_GOTO_DIRECT_TABLE(void *ptr){
    pof_instruction_goto_direct_table *p = (pof_instruction_goto_direct_table *)ptr;

    POF_NTOHL_FUNC(p->table_entry_index);
    POF_NTOHS_FUNC(p->packet_offset);

    return POF_OK;
}

static uint32_t instruction_METER(void *ptr){
    pof_instruction_meter *p = (pof_instruction_meter *)ptr;

    POF_NTOHL_FUNC(p->meter_id);

    return POF_OK;
}

static uint32_t instruction_WRITE_METADATA(void *ptr){
    pof_instruction_write_metadata *p = (pof_instruction_write_metadata *)ptr;

    POF_NTOHS_FUNC(p->metadata_offset);
    POF_NTOHS_FUNC(p->len);
    POF_NTOHL_FUNC(p->value);

    return POF_OK;
}

static uint32_t instruction_WRITE_METADATA_FROM_PACKET(void *ptr){
    pof_instruction_write_metadata_from_packet *p = \
                    (pof_instruction_write_metadata_from_packet *)ptr;

    POF_NTOHS_FUNC(p->metadata_offset);
    POF_NTOHS_FUNC(p->packet_offset);
    POF_NTOHS_FUNC(p->len);

    return POF_OK;
}

static uint32_t instruction_GOTO_TABLE(void *ptr){
    pof_instruction_goto_table *p = (pof_instruction_goto_table *)ptr;
    int i;

    POF_NTOHS_FUNC(p->packet_offset);
    for(i=0; i<POF_MAX_MATCH_FIELD_NUM; i++)
        match((pof_match *)p->match + i);

    return POF_OK;
}

static uint32_t instruction_APPLY_ACTIONS(void *ptr){
    pof_instruction_apply_actions *p = (pof_instruction_apply_actions *)ptr;
    int i;

    for(i=0; i<POF_MAX_ACTION_NUMBER_PER_INSTRUCTION; i++)
        action((pof_action *)p->action + i);

    return POF_OK;
}

static uint32_t instruction_HtoN(void *ptr){
    pof_instruction *p = (pof_instruction *)ptr;

    switch(p->type){
#define INSTRUCTION(NAME,VALUE) case POFIT_##NAME: instruction_##NAME(p->instruction_data); break;
		INSTRUCTIONS
#undef INSTRUCTION
        default:
            POF_ERROR_CPRINT_FL_NO_LOCK(1,RED,"Unknow Instruction type: %u", p->type);
            break;
    }

    POF_NTOHS_FUNC(p->type);
    POF_NTOHS_FUNC(p->len);
    return POF_OK;
}

static uint32_t instruction_NtoH(void *ptr){
    pof_instruction *p = (pof_instruction *)ptr;

    POF_NTOHS_FUNC(p->type);
    POF_NTOHS_FUNC(p->len);

    switch(p->type){
#define INSTRUCTION(NAME,VALUE) case POFIT_##NAME: instruction_##NAME(p->instruction_data); break;
		INSTRUCTIONS
#undef INSTRUCTION
        default:
            POF_ERROR_CPRINT_FL_NO_LOCK(1,RED,"Unknow Instruction type: %u", p->type);
            break;
    }
    return POF_OK;
}

uint32_t pof_HtoN_transfer_flow_entry(void *ptr){
    pof_flow_entry *p = (pof_flow_entry *)ptr;
    int i;

    POF_NTOHL_FUNC(p->counter_id);
    POF_NTOH64_FUNC(p->cookie);
    POF_NTOH64_FUNC(p->cookie_mask);
    POF_NTOHS_FUNC(p->idle_timeout);
    POF_NTOHS_FUNC(p->hard_timeout);
    POF_NTOHS_FUNC(p->priority);
    POF_NTOHL_FUNC(p->index);

    for(i=0;i<p->match_field_num;i++)
        match_x((pof_match_x *)p->match+i);
    for(i=0;i<p->instruction_num;i++)
        instruction_HtoN((pof_instruction *)p->instruction+i);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_flow_entry(void *ptr){
    pof_flow_entry *p = (pof_flow_entry *)ptr;
    int i;

    POF_NTOHL_FUNC(p->counter_id);
    POF_NTOH64_FUNC(p->cookie);
    POF_NTOH64_FUNC(p->cookie_mask);
    POF_NTOHS_FUNC(p->idle_timeout);
    POF_NTOHS_FUNC(p->hard_timeout);
    POF_NTOHS_FUNC(p->priority);
    POF_NTOHL_FUNC(p->index);

    for(i=0;i<p->match_field_num;i++)
        match_x((pof_match_x *)p->match+i);
    for(i=0;i<p->instruction_num;i++)
        instruction_NtoH((pof_instruction *)p->instruction+i);

    return POF_OK;
}

uint32_t pof_NtoH_transfer_group(void *ptr){
    pof_group *p = (pof_group *)ptr;
    int i;

    POF_NTOHL_FUNC(p->group_id);
    POF_NTOHL_FUNC(p->counter_id);
    for(i=0;i<POF_MAX_ACTION_NUMBER_PER_GROUP;i++)
        action((pof_action *)p->action+i);

    return POF_OK;
}
