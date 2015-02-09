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

/* Update action pointer and number in dpp when one action
 * has been done. */
static void action_update(struct pofdp_packet *dpp){
    if(--dpp->act_num < 1){
        return;
    }
    dpp->act++;
    return;
}

/**********************************************************************
 * Make the piece of packet to be zero.
 * Form:     pofdp_bzero_bit(uint8_t *data, uint16_t pos_b, uint16_t len_b)
 * Input:    packet, start position(bit), length(bit)
 * Output:   packet
 * Return:   POF_OK or Error code
 * Discribe: This function makes the piece of packet with pos_b and len_b
 *           to be zero. The units of pos_b and len_b both are BIT.
 *********************************************************************/
static uint32_t bzero_bit(uint8_t *data, uint16_t pos_b, uint16_t len_b){
    uint32_t ret;
    uint16_t len_B;
    uint8_t  *zero;

    if((pos_b % POF_BITNUM_IN_BYTE == 0) && (len_b % POF_BITNUM_IN_BYTE == 0)){
        memset(data + pos_b / POF_BITNUM_IN_BYTE, 0, len_b / POF_BITNUM_IN_BYTE);
        return POF_OK;
    }

    len_B = POF_BITNUM_TO_BYTENUM_CEIL(len_b);

    zero = (uint8_t *)malloc(len_B);
    POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(zero,g_upward_xid++);
    memset(zero, 0, len_B);

    pofdp_cover_bit(data, zero, pos_b, len_b);

    free(zero);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_COUNTER type.
 * Form:     uint32_t pofdp_action_handle_counter(uint8_t *packet, \
 *                                                uint8_t *action_data, \
                                                  struct pofdp_metadata *metadata, \
 *                                                uint32_t *packet_over)
 * Input:    action data
 * Output:   packet over identifier
 * Return:   POF_OK or Error code
 * Discribe: This function handles the action with POFAT_COUNTER type. The
 *           counter value corresponding the counter_id given by
 *           action_data will increase by 1.
 * Note:     If there is an ERROR, The packet_over identifier will be TRUE
 ***********************************************************************/
static uint32_t execute_COUNTER(POFDP_ARG)
{
    pof_action_counter *p = (pof_action_counter *)dpp->act->action_data;
    uint32_t ret;
    ret = poflr_counter_increace(p->counter_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_counter has been done!");
    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_GROUP type.
 * Form:     uint32_t pofdp_action_handle_group(uint8_t *packet, \
 *                                              uint8_t *action_data, \
                                                struct pofdp_metadata *metadata, \
 *                                              uint32_t *packet_over)
 * Input:    packet, packet length, action data, metadata
 * Output:   packet, packet length, packet over identifier
 * Return:   POF_OK or Error code
 * Discribe: This function handles the action with POFAT_GROUP type. The
 *           packet will be forward to the group table. The group id is
 *           given in action_data. The instructions corresponding to the
 *           group will be all executed what ever packet_over is TRUE or
 *           not. In this version, we consider all group type is all.
 * Note:     If there is an ERROR, The packet_over identifier will be TRUE
 ***********************************************************************/
static uint32_t execute_GROUP(POFDP_ARG)
{
    pof_action_group *p = (pof_action_group *)dpp->act->action_data;
    poflr_groups *group_lr_ptr = NULL;
    pof_group  *group_ptr;
    uint32_t   group_id, ret;

    group_id = p->group_id;
    poflr_get_group(&group_lr_ptr);

    if(group_lr_ptr->state[group_id] == POFLR_STATE_INVALID){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_UNKNOWN_GROUP, g_upward_xid++);
    }

    group_ptr = &group_lr_ptr->group[group_id];

    POF_DEBUG_CPRINT_FL(1,YELLOW,"Go to Group[%u]", group_id);

    ret = poflr_counter_increace(group_ptr->counter_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	dpp->act = group_ptr->action;
	dpp->act_num = group_ptr->action_number;

    ret = pofdp_action_execute(dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_group has been DONE!");
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_TOCP type.
 * Form:     uint32_t pofdp_action_handle_tocp(uint8_t *packet, \
 *                                             uint8_t *action_data, \
                                               struct pofdp_metadata *metadata, \
 *                                             pof_flow_entry *entry_ptr, \
 *                                             uint32_t *packet_over)
 * Input:    packet, length of packet, action data, matched flow entry
 * Output:   packet over identifier
 * Return:   POF_OK or Error code
 * Discribe: This function send the packet upward to the Controller through
 *           the OpenFlow Channel.
 * Note:     If there is an ERROR, The packet_over identifier will be TRUE.
 *           If the packet has been send successfully, The packet_over
 *           identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_PACKET_IN(POFDP_ARG)
{
    pof_action_packet_in *p = (pof_action_packet_in *)dpp->act->action_data;
    uint32_t reason, ret;
    uint8_t  table_ID;

    ret = poflr_table_id_to_ID(dpp->table_type, dpp->table_id, &table_ID);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    ret = pofdp_send_packet_in_to_controller(dpp->offset + dpp->left_len, \
            p->reason_code, table_ID, dpp->flow_entry, POF_FE_ID, dpp->buf);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,YELLOW,"action_packet_in has been done! The packet in reason is %d.", p->reason_code);
	POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf, dpp->offset + dpp->left_len, "The packet in data is ");

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_DROP type.
 * Form:     uint32_t pofdp_action_handle_drop(uint8_t *packet, \
 *                                             uint8_t *action_data, \
                                               struct pofdp_metadata *metadata, \
 *                                             uint32_t *packet_over)
 * Input:    action data
 * Output:   packet over identifier
 * Return:   POF_OK or Error code
 * Discribe: This function drops the packet out of the switch.
 * Note:     If there is an ERROR, The packet_over identifier will be TRUE.
 *           If the packet has been droped successfully, The packet_over
 *           identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_DROP(POFDP_ARG)
{
    pof_action_drop *p = (pof_action_drop *)dpp->act->action_data;

    POF_DEBUG_CPRINT_FL(1,YELLOW,"action_drop has been done! The drop reason is %d\n.", p->reason_code);

    dpp->packet_done = TRUE;

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_SET_FIELD type.
 * Form:     uint32_t pofdp_action_handle_set_field(uint8_t *packet, \
 *                                                  uint8_t *action_data, \
                                                    struct pofdp_metadata *metadata, \
 *                                                  uint32_t *packet_over)
 * Input:    packet, length of packet, action data
 * Output:   packet over identifier, packet
 * Return:   POF_OK or Error code
 * Discribe: This function will set the field of the packet by the value
 *           given by action_data
 * Note:     If there is an ERROR, The packet_over identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_SET_FIELD(POFDP_ARG)
{
    pof_action_set_field *p = (pof_action_set_field *)dpp->act->action_data;
    uint32_t i, ret;
    uint16_t offset_b, len_b;
    uint8_t  value[POF_MAX_FIELD_LENGTH_IN_BYTE];

    offset_b = p->field_setting.offset;
    len_b = p->field_setting.len;

    /* Check packet length. */
    if(dpp->left_len * POF_BITNUM_IN_BYTE < offset_b + len_b){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    for(i=0; i<POF_MAX_FIELD_LENGTH_IN_BYTE; i++){
        value[i] = p->field_setting.value[i] & p->field_setting.mask[i];
    }

    pofdp_cover_bit(dpp->buf_offset, value, offset_b, len_b);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_set_field has been DONE");
	POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The packet is ");

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_SET_FIELD_FROM_METADATA type.
 * Form:     uint32_t pofdp_action_handle_set_field_from_metadata(uint8_t *packet, \
 *                                                                uint8_t *action_data, \
                                                                  struct pofdp_metadata *metadata, \
 *                                                                uint32_t *packet_over)
 * Input:    packet, length of packet, action data, metadata
 * Output:   packet over identifier, packet
 * Return:   POF_OK or Error code
 * Discribe: This function will set the field of the packet from the piece
 *           of metadata.
 * Note:     If there is an ERROR, The packet_over identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_SET_FIELD_FROM_METADATA(POFDP_ARG)
{
    pof_action_set_field_from_metadata *p = \
            (pof_action_set_field_from_metadata *)dpp->act->action_data;
    uint32_t ret;
    uint16_t offset_b, len_b, metadata_offset_b;
    uint8_t  value[POF_MAX_FIELD_LENGTH_IN_BYTE];

    offset_b = p->field_setting.offset;
    len_b = p->field_setting.len;
    metadata_offset_b = p->metadata_offset;

    /* Check packet length. */
    if(dpp->left_len * POF_BITNUM_IN_BYTE < offset_b + len_b){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    pofdp_copy_bit((uint8_t *)dpp->metadata, value, metadata_offset_b, len_b);

    pofdp_cover_bit(dpp->buf_offset, value, offset_b, len_b);

	POF_DEBUG_CPRINT_FL_0X(1,GREEN,value,POF_BITNUM_TO_BYTENUM_CEIL(len_b), \
			"Set_field_from_metadata has been done! The metadata is :");
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The packet is :");

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_MODIFY_FIELD type.
 * Form:     uint32_t pofdp_action_handle_modify_field(uint8_t *packet, \
 *                                                     uint8_t *action_data, \
                                                       struct pofdp_metadata *metadata, \
 *                                                     uint32_t *packet_over)
 * Input:    packet, length of packet, action data, metadata
 * Output:   packet over identifier, packet
 * Return:   POF_OK or Error code
 * Discribe: This function will modify the field of the packet. The value
 *           will be increase by the number given by action_data.
 * Note:     If there is an ERROR, The packet_over identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_MODIFY_FIELD(POFDP_ARG)
{
    pof_action_modify_field *p = (pof_action_modify_field *)dpp->act->action_data;
    uint32_t value, ret;

	ret = pofdp_get_32value(&value, POFVT_FIELD, &p->field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    value += p->increment;

	ret = pofdp_write_32value_to_field(value, &p->field, dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_modeify_field has been done! The increment is %d", p->increment);
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The packet is ");

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Calculate the checksum.
 * Form:     static uint32_t pofdp_checksum(uint8_t *data, \
 *                                          uint64_t *value, \
 *                                          uint16_t pos_b, \
 *                                          uint16_t len_b, \
 *                                          uint16_t cs_len_b)
 * Input:    data, start position(bit), length(bit), checksum len(bit)
 * Output:   calculate result
 * Return:   POF_OK or Error code
 * Discribe: This function calculates the checksum of the piece of data
 *           with pos_b and length. The length of checksum is cs_len_b.
 *           The result will be stored in value. The units of pos_b,
 *           len_b, cs_len_b all are BIT.
 * NOTE:     The max of length of checksum is 64bits.
 ***********************************************************************/
static uint32_t checksum(uint8_t *data, \
                         uint64_t *value, \
                         uint16_t pos_b, \
                         uint16_t len_b, \
                         uint16_t cs_len_b)
{
    uint64_t *value_temp, threshold = 1;
    uint32_t ret;
    uint16_t cal_num, i;
    uint8_t  data_temp[8] = {0};

    cal_num = len_b / cs_len_b;
    value_temp = (uint64_t *)data_temp;

    for(i=0; i<cs_len_b; i++){
        threshold *= 2;
    }

    for(i=0; i<cal_num; i++){
        memset(data_temp, 0, sizeof(data_temp));
        pofdp_copy_bit(data, data_temp, pos_b + i*cs_len_b, cs_len_b);

        POF_NTOH64_FUNC(*value_temp);
        *value_temp = ~*value_temp;
        *value_temp = POF_MOVE_BIT_RIGHT(*value_temp, 64 - cs_len_b);

        *value += *(value_temp);
        if(*value >= threshold){
            *value = *value - threshold + 1;
        }
    }

    *value = POF_MOVE_BIT_LEFT(*value, 64 - cs_len_b);
    POF_NTOH64_FUNC(*value);

    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_CALCULATE_CHECKSUM type.
 * Form:     uint32_t pofdp_action_handle_calculate_checksum(uint8_t *data, \
 *                                                           uint8_t *action_data, \
                                                             struct pofdp_metadata *metadata, \
 *                                                           uint32_t *packet_over)
 * Input:    packet, length of packet, action data
 * Output:   packet over identifier, packet
 * Return:   POF_OK or Error code
 * Discribe: This function will calculate the checksum of the piece of
 *           packet specified in action_data. The calculate result will be
 *           stored in the specified piece of packet.
 * Note:     If there is an ERROR, The packet_over identifier will be TRUE.
 *           The max of the length of checksum is 64bits.
 ***********************************************************************/
static uint32_t execute_CALCULATE_CHECKSUM(POFDP_ARG)
{
    pof_action_calculate_checksum *p = \
            (pof_action_calculate_checksum *)dpp->act->action_data;
    uint64_t checksum_value = 0;
    uint32_t ret;
    uint16_t cal_pos_b, cal_len_b, cs_pos_b, cs_len_b = p->checksum_len;

    cal_pos_b = p->cal_startpos;
    cal_len_b = p->cal_len;
    cs_pos_b = p->checksum_pos;
    cs_len_b = p->checksum_len;

    if(cs_len_b > 64){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBAC_BAD_LEN, g_upward_xid++);
    }
/*
    if(((cal_pos_b+cal_len_b) > (dpp->left_len * 8)) || (cs_pos_b+cs_len_b) > (dpp->left_len * 8)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }
    if(((cal_len_b % cs_len_b) != 0) || (((cs_pos_b - cal_pos_b) % cs_len_b) != 0)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBAC_BAD_LEN, g_upward_xid++);
    }
*/

    ret = bzero_bit(dpp->buf_offset, cs_pos_b, cs_len_b);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    ret = checksum(dpp->buf_offset, &checksum_value, cal_pos_b, cal_len_b, cs_len_b);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    pofdp_cover_bit(dpp->buf_offset, (uint8_t *)&checksum_value, cs_pos_b, cs_len_b);

    POF_DEBUG_CPRINT_FL_0X(1,GREEN,&checksum_value,8,"action_calculate_checksum has been done! The checksum_value = ");

    action_update(dpp);
    return POF_OK;
}


/***********************************************************************
 * Handle the action with POFAT_OUTPUT type.
 * Form:     uint32_t pofdp_action_handle_output(uint8_t *packet, \
 *                                               uint8_t *action_data, \
                                                 struct pofdp_metadata *metadata, \
 *                                               uint32_t *packet_over)
 * Input:    packet, length of packet, action data, metadata
 * Output:   packet over identifier
 * Return:   POF_OK or Error code
 * Discribe: This function will send the packet out through the physical
 *           port specified in action_data. The specified piece of metadata
 *           should be send before the packet. The metadata and packet
 *           consist a new packet.
 * Note:     If there is an ERROR, the packet_over identifier will be TRUE.
 *           If the packet has been send successfully, the packet_over
 *           identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_OUTPUT(POFDP_ARG)
{
    pof_action_output *p = (pof_action_output *)dpp->act->action_data;
    uint32_t ret;

    if(p->packet_offset > dpp->offset + dpp->left_len){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    if(POF_BITNUM_TO_BYTENUM_CEIL(p->metadata_len + p->metadata_offset) > dpp->metadata_len){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_METADATA_LEN_ERROR, g_upward_xid++);
    }

    dpp->output_port_id = p->outputPortId;

	ret = poflr_check_port_index(dpp->output_port_id);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    dpp->output_packet_offset = p->packet_offset;   /* Byte unit. */
    dpp->output_packet_len = dpp->offset + dpp->left_len - dpp->output_packet_offset;   /* Byte unit. */
    dpp->output_metadata_len = POF_BITNUM_TO_BYTENUM_CEIL(p->metadata_len); /* Byte unit. */
    dpp->output_metadata_offset = p->metadata_offset;   /* Bit unit. */
    dpp->output_whole_len = dpp->output_packet_len + dpp->output_metadata_len;  /* Byte unit. */

    ret = pofdp_send_raw(dpp);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_ADD_TAG type.
 * Form:     uint32_t pofdp_action_handle_add_field(uint8_t *packet, \
 *                                                uint8_t *action_data, \
                                                  struct pofdp_metadata *metadata, \
 *                                                uint32_t *packet_over)
 * Input:    packet, length of packet, action data
 * Output:   packet over identifier, packet
 * Return:   POF_OK or Error code
 * Discribe: This function will add tag into the packet. The position and
 *           the value has been given by action_data. The length of packet
 *           will be changed after adding tag.
 * Note:     If there is an ERROR, the packet_over identifier will be TRUE.
 *           The max bit number of tag_value is 64bits.
 ***********************************************************************/
static uint32_t execute_ADD_FIELD(POFDP_ARG)
{
    pof_action_add_field *p = (pof_action_add_field *)dpp->act->action_data;
    uint64_t value;
    uint32_t tag_len_b, len_b_behindtag, ret;
    uint16_t tag_pos_b;
    uint8_t  *tag_value;
    uint8_t  buf_behindtag[POFDP_PACKET_RAW_MAX_LEN];

    tag_pos_b = p->tag_pos;
    tag_len_b = p->tag_len;
    value = p->tag_value;
    tag_value = (uint8_t *)&value;
    len_b_behindtag = dpp->left_len * 8 - tag_pos_b;

    if((dpp->offset + dpp->left_len + POF_BITNUM_TO_BYTENUM_CEIL(tag_len_b)) > POFDP_PACKET_RAW_MAX_LEN \
            || len_b_behindtag < 0){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

	if(tag_len_b > 64){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_MATCH, POFBMC_BAD_TAG, g_upward_xid++);
	}

    value = POF_MOVE_BIT_LEFT(value, 64 - tag_len_b);
    value = POF_HTON64(value);

    pofdp_copy_bit(dpp->buf_offset, buf_behindtag, tag_pos_b, len_b_behindtag);
    pofdp_cover_bit(dpp->buf_offset, tag_value, tag_pos_b, tag_len_b);
    pofdp_cover_bit(dpp->buf_offset, buf_behindtag, tag_pos_b+tag_len_b, len_b_behindtag);

    dpp->left_len += POF_BITNUM_TO_BYTENUM_CEIL(tag_len_b);
    dpp->metadata->len = POF_HTONS(POF_HTONS(dpp->metadata->len) + POF_BITNUM_TO_BYTENUM_CEIL(tag_len_b));

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_add_field has been done!");
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The new packet = ");

    action_update(dpp);
    return POF_OK;
}

/***********************************************************************
 * Handle the action with POFAT_DELETE_TAG type.
 * Form:     uint32_t pofdp_action_handle_delete_tag(uint8_t *packet, \
 *                                                   uint8_t *action_data, \
                                                     struct pofdp_metadata *metadata, \
 *                                                   uint32_t *packet_over)
 * Input:    packet, length of packet, action data
 * Output:   packet over identifier, packet
 * Return:   POF_OK or Error code
 * Discribe: This function will delete tag of packet. The position and the
 *           length of the tag has been given by action_data. The length
 *           of the packet will be changed after deleting tag.
 * Note:     If there is an ERROR, the packet_over identifier will be TRUE.
 ***********************************************************************/
static uint32_t execute_DELETE_FIELD(POFDP_ARG)
{
    pof_action_delete_field *p = (pof_action_delete_field *)dpp->act->action_data;
    uint32_t ret, tag_len_b;
    uint16_t tag_pos_b, tag_len_b_x, len_b_behindtag;
    uint8_t  buf_temp[POFDP_PACKET_RAW_MAX_LEN] = {0};

    tag_pos_b = p->tag_pos;
    tag_len_b = p->tag_len;
    tag_len_b_x = tag_len_b % POF_BITNUM_IN_BYTE;
    len_b_behindtag = dpp->left_len * POF_BITNUM_IN_BYTE - (tag_pos_b + tag_len_b);

    if((int16_t)len_b_behindtag < 0){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    pofdp_copy_bit(dpp->buf_offset, buf_temp, tag_pos_b+tag_len_b, len_b_behindtag);
    pofdp_cover_bit(dpp->buf_offset, buf_temp, tag_pos_b, len_b_behindtag);

    dpp->left_len = POF_BITNUM_TO_BYTENUM_CEIL(dpp->left_len * POF_BITNUM_IN_BYTE - tag_len_b);
    dpp->metadata->len = POF_HTONS(dpp->left_len + dpp->offset);

    if(tag_len_b_x != 0){
        *(dpp->buf_offset + dpp->left_len - 1) &= POF_MOVE_BIT_LEFT(0xff, tag_len_b_x);
    }

    POF_DEBUG_CPRINT_FL(1,GREEN,"action_delete_field has been done!");
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_offset,dpp->left_len,"The new packet = ");

    action_update(dpp);
    return POF_OK;
}

static uint32_t
execute_EXPERIMENTER(POFDP_ARG)
{
	// TODO
	POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBAC_BAD_TYPE, g_upward_xid++);
}

/***********************************************************************
 * Execute actions
 * Form:     uint32_t pofdp_action_handle(uint8_t *packet, \
 *                                        pof_action *action_ptr, \
 *                                        uint8_t action_num, \
                                          struct pofdp_metadata *metadata, \
 *                                        uint8_t *metadata, \
 *                                        uint32_t *packet_over, \
 *                                        pof_flow_entry *entry_ptr, \
 *                                        uint8_t execute_all)
 * Input:    packet, packet length, actions, action number, metadata,
 *           entry_ptr, execute_all
 * Output:   packet over identifier
 * Return:   POF_OK or Error code
 * Discribe: This function executes the actions. If the execute_all is
 *           FALSE, this function will return as soon as the packet_over
 *           identifier is TRUE. If the execute_all is TRUE, this
 *           function will not stop until all of the actions have been
 *           executed, and the packet_over parameter is useless.
 ***********************************************************************/
uint32_t pofdp_action_execute(POFDP_ARG)
{
    uint32_t ret;
    uint8_t *packet, action_num, execute_all;
    pof_action *action_ptr;
    struct pofdp_metadata *metadata;
    uint32_t *packet_over;
    pof_flow_entry *entry_ptr;

    while(dpp->packet_done == FALSE && dpp->act_num > 0){
		/* Execute the actions. */
        switch(dpp->act->type){
#define ACTION(NAME,VALUE) case POFAT_##NAME: ret = execute_##NAME(dpp); break;
			ACTIONS
#undef ACTION

            default:
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBAC_BAD_TYPE, g_upward_xid++);
                break;
        }
        POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    }

    return POF_OK;
}

#endif // POF_DATAPATH_ON
