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
#include "../include/pof_conn.h"
#include "../include/pof_local_resource.h"
#include "../include/pof_byte_transfer.h"
#include "../include/pof_log_print.h"

/* Xid in OpenFlow header received from Controller. */
uint32_t g_recv_xid = POF_INITIAL_XID;

/*******************************************************************************
 * Parse the OpenFlow message received from the Controller.
 * Form:     uint32_t  pof_parse_msg_from_controller(char* msg_ptr)
 * Input:    message length, message data
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function parses the OpenFlow message received from the Controller,
 *           and execute the response.
*******************************************************************************/
uint32_t  pof_parse_msg_from_controller(char* msg_ptr){
    pof_switch_config *config_ptr;
    pof_header        *header_ptr, head;
    pof_flow_entry    *flow_ptr;
    pof_counter       *counter_ptr;
    pof_flow_table    *table_ptr;
    pof_port          *port_ptr;
    pof_meter         *meter_ptr;
    pof_group         *group_ptr;
    uint32_t          ret = POF_OK;
    uint16_t          len;
    uint8_t           msg_type;

    header_ptr = (pof_header*)msg_ptr;
    len = POF_NTOHS(header_ptr->length);

    /* Print the parse information. */
#ifndef POF_DEBUG_PRINT_ECHO_ON
    if(header_ptr->type != POFT_ECHO_REPLY){
#endif
    POF_DEBUG_CPRINT_PACKET(msg_ptr,0,len);
#ifndef POF_DEBUG_PRINT_ECHO_ON
    }
#endif

    /* Parse the OpenFlow packet header. */
    pof_NtoH_transfer_header(header_ptr);
    msg_type = header_ptr->type;
    g_recv_xid = header_ptr->xid;

    /* Execute different responses according to the OpenFlow type. */
    switch(msg_type){
        case POFT_ECHO_REQUEST:
            if(POF_OK != pofec_reply_msg(POFT_ECHO_REPLY, g_recv_xid, 0, NULL)){
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
            }
            break;

        case POFT_SET_CONFIG:
            config_ptr = (pof_switch_config *)(msg_ptr + sizeof(pof_header));
            pof_HtoN_transfer_switch_config(config_ptr);

            ret = poflr_set_config(config_ptr->flags, config_ptr->miss_send_len);
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_GET_CONFIG_REQUEST:
            ret = poflr_reply_config();
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

            ret = poflr_reply_table_resource();
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

            ret = poflr_reply_port_resource();
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_PORT_MOD:
            port_ptr = (pof_port*)(msg_ptr + sizeof(pof_header) + sizeof(pof_port_status) - sizeof(pof_port));
            pof_NtoH_transfer_port(port_ptr);

            ret = poflr_port_openflow_enable(port_ptr->port_id, port_ptr->of_enable);
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_FEATURES_REQUEST:
            ret = poflr_reset_dev_id();
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

            ret = poflr_reply_feature_resource();
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_TABLE_MOD:
            table_ptr = (pof_flow_table*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_flow_table(table_ptr);

            if(table_ptr->command == POFTC_ADD){
                ret = poflr_create_flow_table(table_ptr->tid, \
                                              table_ptr->type, \
                                              table_ptr->key_len, \
                                              table_ptr->size, \
                                              table_ptr->table_name, \
											  table_ptr->match_field_num, \
											  table_ptr->match);
            }else if(table_ptr->command == POFTC_DELETE){
                ret = poflr_delete_flow_table(table_ptr->tid, table_ptr->type);
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_TABLE_MOD_FAILED, POFTMFC_BAD_COMMAND, g_recv_xid);
            }

            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_FLOW_MOD:
            flow_ptr = (pof_flow_entry*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_flow_entry(flow_ptr);

            if(flow_ptr->command == POFFC_ADD){
                ret = poflr_add_flow_entry(flow_ptr);
            }else if(flow_ptr->command == POFFC_DELETE){
                ret = poflr_delete_flow_entry(flow_ptr);
            }else if(flow_ptr->command == POFFC_MODIFY){
                ret = poflr_modify_flow_entry(flow_ptr);
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_FLOW_MOD_FAILED, POFFMFC_BAD_COMMAND, g_recv_xid);
            }
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
//            usr_cmd_tables();

            break;

         case POFT_METER_MOD:
            meter_ptr = (pof_meter*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_meter(meter_ptr);

            if(meter_ptr->command == POFMC_ADD){
                ret = poflr_add_meter_entry(meter_ptr->meter_id, meter_ptr->rate);
            }else if(meter_ptr->command == POFMC_MODIFY){
                ret = poflr_modify_meter_entry(meter_ptr->meter_id, meter_ptr->rate);
            }else if(meter_ptr->command == POFMC_DELETE){
                ret = poflr_delete_meter_entry(meter_ptr->meter_id, meter_ptr->rate);
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_METER_MOD_FAILED, POFMMFC_BAD_COMMAND, g_recv_xid);
            }

            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_GROUP_MOD:
            group_ptr = (pof_group*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_group(group_ptr);

            if(group_ptr->command == POFGC_ADD){
                ret = poflr_add_group_entry(group_ptr);
            }else if(group_ptr->command == POFGC_MODIFY){
                ret = poflr_modify_group_entry(group_ptr);
            }else if(group_ptr->command == POFGC_DELETE){
                ret = poflr_delete_group_entry(group_ptr);
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_GROUP_MOD_FAILED, POFGMFC_BAD_COMMAND, g_recv_xid);
            }

            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_COUNTER_MOD:
            counter_ptr = (pof_counter*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_counter(counter_ptr);

            if(counter_ptr->command == POFCC_CLEAR){
                ret = poflr_counter_clear(counter_ptr->counter_id);
            }else if(counter_ptr->command == POFCC_ADD){
                ret = poflr_counter_init(counter_ptr->counter_id);
            }else if(counter_ptr->command == POFCC_DELETE){
                ret = poflr_counter_delete(counter_ptr->counter_id);
            }else{
                POF_ERROR_HANDLE_RETURN_UPWARD(POFET_COUNTER_MOD_FAILED, POFCMFC_BAD_COMMAND, g_recv_xid);
            }

            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        case POFT_COUNTER_REQUEST:
            counter_ptr = (pof_counter*)(msg_ptr + sizeof(pof_header));
            pof_NtoH_transfer_counter(counter_ptr);

            ret = poflr_get_counter_value(counter_ptr->counter_id);
            POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
            break;

        default:
            POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_REQUEST, POFBRC_BAD_TYPE, g_recv_xid);
            break;
    }
    return ret;
}
