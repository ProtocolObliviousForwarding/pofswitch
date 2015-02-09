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
#include "../include/pof_datapath.h"

#ifdef POF_DATAPATH_ON

static uint32_t pofdp_match_per_entry(uint8_t **key_ptr, uint8_t match_field_num, const pof_match_x *match);

/***********************************************************************
 * Lookup the matched flow entry in the table using the keys.
 * Form:     uint32_t pofdp_lookup_in_table(uint8_t **key_ptr,
 *                                          uint8_t match_field_num,
 *                                          poflr_flow_table table_vhal,
 *                                          pof_flow_entry **entry_ptrptr)
 * Input:    keys, match field number, flow table
 * Output:   matched flow entry
 * Return:   POF_OK or Error code
 * Discribe: This function lookup the flow entry matched the packet in
 *           the flow table using the match keys. If there is no matched
 *           flow entry, return POF_ERROR.
 ***********************************************************************/
uint32_t pofdp_lookup_in_table(uint8_t **key_ptr, \
                               uint8_t match_field_num, \
                               poflr_flow_table table_vhal, \
                               pof_flow_entry **entry_ptrptr)
{
    poflr_flow_entry *entry_ptr = table_vhal.entry_ptr;
    pof_instruction  *ins_ptr;
    uint32_t i, entry_num = table_vhal.entry_num;
    uint16_t *priority = NULL;

    *entry_ptrptr = NULL;

    /* Check the flow table state. */
    if( table_vhal.state == POFLR_STATE_INVALID ){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_INSTRUCTION, POFBIC_TABLE_UNEXIST, g_upward_xid++);
    }

    /* Match the key against every flow entry in the flow table. */
    for(i=0; i<entry_num; entry_ptr++){

        /* Check the flow entry state. If the flow entry is invalid, go to next flow entry. */
        if(entry_ptr->state == POFLR_STATE_INVALID)
            continue;

        /* Match the key against the flow entry. */
        if(pofdp_match_per_entry(key_ptr, match_field_num, entry_ptr->entry.match) == TRUE){
            if(priority == NULL){
                priority = &entry_ptr->entry.priority;
                *entry_ptrptr = &entry_ptr->entry;
            }else{
                if(*priority < entry_ptr->entry.priority){
                    priority = &entry_ptr->entry.priority;
                    *entry_ptrptr = &entry_ptr->entry;
                }
            }
        }
        i++;
    }

    /* Check whether the key matches any flow entry or not. */
    if(*entry_ptrptr == NULL){
        return POF_ERROR;
    }

    return POF_OK;
}

/***********************************************************************
 * Match the keys against the specified flow entry.
 * Form:     static uint32_t pofdp_match_per_entry(uint8_t **key_ptr, \
 *                                                 uint8_t match_field_num, \
 *                                                 const pof_match_x *match)
 * Input:    keys, match field number, match data
 * Output:   NONE
 * Return:   TRUE: match, FALSE: do not match
 * Discribe: This function matches the keys against the specified flow
 *           entry. If match successfully, return TRUE.
 ***********************************************************************/
static uint32_t pofdp_match_per_entry(uint8_t **key_ptr, uint8_t match_field_num, const pof_match_x *match){
    pof_match_x tmp_match;
    uint16_t    j, len_B;
    uint8_t     i, *value, *mask;

    for(i=0; i<match_field_num; i++){
        tmp_match = match[i];
        value = tmp_match.value;
        mask = tmp_match.mask;
        len_B = POF_BITNUM_TO_BYTENUM_CEIL(tmp_match.len);

        for(j=0; j<len_B; j++){
            if((mask[j] & key_ptr[i][j]) != (mask[j] & value[j]))
                return FALSE;
        }
    }
    return TRUE;
}

/***********************************************************************
 * Cover the piece of data using the specified value.
 * Form:     uint32_t pofdp_cover_bit(uint8_t *data_ori, \
 *                                    uint8_t *value, \
 *                                    uint16_t pos_b, \
 *                                    uint16_t len_b)
 * Input:    original data, value, position offset(bit unit), length(bit unit)
 * Output:   data
 * Return:   POF_OK or Error code
 * Discribe: This function covers the piece of data using the specified
 *           value. The value is not a number, but a data buffer. Caller
 *           should make sure that data_ori and value are not NULL.
 ***********************************************************************/
void pofdp_cover_bit(uint8_t *data_ori, uint8_t *value, uint16_t pos_b, uint16_t len_b){
    uint32_t process_len_b = 0;
    uint16_t pos_b_x, len_B, after_len_b_x;
    uint8_t *ptr;

    pos_b_x = pos_b % 8;
    len_B = (uint16_t)((len_b - 1) / 8 + 1);
    after_len_b_x = (len_b + pos_b - 1) % 8 + 1;
    ptr = data_ori + (uint16_t)(pos_b / 8);

    if(len_b <= (8 - pos_b_x)){
        *ptr = (*ptr & POF_MOVE_BIT_LEFT(0xff, (8-pos_b_x)) \
            | (POF_MOVE_BIT_RIGHT(*value, pos_b_x) & POF_MOVE_BIT_LEFT(0xff, 8-after_len_b_x)) \
            | (*ptr & POF_MOVE_BIT_RIGHT(0xff, after_len_b_x)));
        return;
    }

    *ptr = *ptr & POF_MOVE_BIT_LEFT(0xff, (8-pos_b_x)) \
        | POF_MOVE_BIT_RIGHT(*value, pos_b_x);

    process_len_b = 8 - pos_b_x;
    while(process_len_b < (len_b-8)){
        *(++ptr) = POF_MOVE_BIT_LEFT(*value, 8-pos_b_x) | POF_MOVE_BIT_RIGHT(*(++value), pos_b_x);
        process_len_b += 8;
    }

    *(ptr+1) = (POF_MOVE_BIT_LEFT(*value, 8-pos_b_x) | POF_MOVE_BIT_RIGHT(*(++value), pos_b_x) \
        & POF_MOVE_BIT_LEFT(0xff, 8 - (len_b - process_len_b))) \
        | (*(ptr+1) & POF_MOVE_BIT_RIGHT(0xff, len_b - process_len_b));

    return;
}

/***********************************************************************
 * Copy the piece of original data to the result data buffer.
 * Form:     uint32_t pofdp_copy_bit(uint8_t *data_ori, \
 *                                   uint8_t *data_res, \
 *                                   uint16_t offset_b, \
 *                                   uint16_t len_b)
 * Input:    original data, offset(bit unit), length(bit unit)
 * Output:   result data
 * Return:   POF_OK or Error code
 * Discribe: This function copies the piece of original data to the result
 *           data buffer.
 ***********************************************************************/
void pofdp_copy_bit(uint8_t *data_ori, uint8_t *data_res, uint16_t offset_b, uint16_t len_b){
    uint32_t process_len_b = 0, offset_b_x;
    uint16_t offset_B;
    uint8_t  *ptr;

	if(NULL==data_ori || NULL==data_res){
		return;
	}

    offset_B = (uint16_t)(offset_b / 8);
    offset_b_x = offset_b % 8;
    ptr = data_ori + offset_B;

    while(process_len_b < len_b){
        *(data_res++) = POF_MOVE_BIT_LEFT(*ptr, offset_b_x) \
            | POF_MOVE_BIT_RIGHT(*(++ptr), 8 - offset_b_x);
        process_len_b += 8;
    }

    data_res--;
    *data_res = *data_res & POF_MOVE_BIT_LEFT(0xff, process_len_b - len_b);

    return;
}

#endif // POF_DATAPATH_ON
