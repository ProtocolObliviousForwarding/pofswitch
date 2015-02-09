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
#include "../include/pof_log_print.h"
#include "../include/pof_byte_transfer.h"

static void
log_(char *cont){fprintf(g_log.log_fp, "%s", cont);}

static void
log_disable(char *cont){return;}

static void
textcolor(char *i, char *col) {printf("\033[%s;%sm", i, col);}

static void
textcolor_disable(char *o, char *col) {return;}

static void
overcolor() {printf("\033[0m");}

static void
overcolor_disable() {return;}

static void
print(char *cont, char *i, char *col)
{
    g_log.textcolor(i,col);
    printf("%s",cont);
    g_log.overcolor();
}

static void
print_disable(char *cont, char *i, char *col){return;}

void
poflp_debug_enable() 
{
	g_log._pDbg = print;
}

void
poflp_debug_disable() 
{
	g_log._pDbg = print_disable;
}

void
poflp_color_enable() 
{
	g_log.textcolor = textcolor;
	g_log.overcolor = overcolor;
}

void
poflp_color_disable() 
{
	g_log.textcolor = textcolor_disable;
	g_log.overcolor = overcolor_disable;
}

void
poflp_error_enable() 
{
	g_log._pErr = print;
}

void
poflp_error_disable() 
{
	g_log._pErr = print_disable;
}

void pof_open_log_file(char *filename){
	char *filename_ = NULL;
	if(filename){
		filename_ = filename;
	}else{
		filename_ = "/usr/local/var/log/pofswitch.log";
	}
    strncpy(g_states.log_file.cont, filename_, POF_STRING_PAIR_MAX_LEN-1);

	g_log.log_fp = fopen(filename_, "w");
	if(!g_log.log_fp){
		POF_ERROR_CPRINT_FL(1,RED,"Open %s failed!", filename_);
		exit(0);
	}
	g_log._pLog = log_;
	POF_DEBUG_CPRINT_FL(1,GREEN,"Create log file: %s", filename_);
	return;
}

void pof_close_log_file(){
	if(!g_log.log_fp){
		return;
	}
	fclose(g_log.log_fp);
	return;
}

static void port(const unsigned char * ph){
    uint32_t i;
    pof_port *p;

    p = malloc(sizeof(pof_port));
    memcpy(p,ph,sizeof(pof_port));
    pof_NtoH_transfer_port(p);

    POF_DEBUG_CPRINT(1,CYAN,"port_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->port_id);
    POF_DEBUG_CPRINT(1,CYAN,"device_id=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p->device_id);
    POF_DEBUG_CPRINT(1,CYAN,"hw_addr=0x");
    for(i=0; i<POF_ETH_ALEN; i++){
        POF_DEBUG_CPRINT(1,WHITE,"%.2x", *(p->hw_addr+i));
    }
    POF_DEBUG_CPRINT(1,CYAN,"name=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p->name);
    POF_DEBUG_CPRINT(1,CYAN,"config=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->config);
    POF_DEBUG_CPRINT(1,CYAN,"state=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->state);
    POF_DEBUG_CPRINT(1,CYAN,"curr=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->curr);
    POF_DEBUG_CPRINT(1,CYAN,"advertised=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->advertised);
    POF_DEBUG_CPRINT(1,CYAN,"supported=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->supported);
    POF_DEBUG_CPRINT(1,CYAN,"peer=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->peer);
    POF_DEBUG_CPRINT(1,CYAN,"curr_speed=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->curr_speed);
    POF_DEBUG_CPRINT(1,CYAN,"max_speed=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->max_speed);
    POF_DEBUG_CPRINT(1,CYAN,"of_enable=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->of_enable);

    free(p);
}

static void port_status(const unsigned char *ph){
    pof_port_status *p;

    p = malloc(sizeof(pof_port_status));
    memcpy(p,ph,sizeof(pof_port_status));
    pof_HtoN_transfer_port_status(p);

    POF_DEBUG_CPRINT(1,CYAN,"reason=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->reason);

    POF_DEBUG_CPRINT(1,PINK,"[pof_port] ");

    port((unsigned char *)ph+sizeof(pof_port_status)- \
                sizeof(pof_port));

    free(p);
}

static void table_resource_desc(const unsigned char *ph){
    pof_table_resource_desc *p = \
			(pof_table_resource_desc *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"device_id=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p->device_id);
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->type);
    POF_DEBUG_CPRINT(1,CYAN,"tbl_num=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->tbl_num);
    POF_DEBUG_CPRINT(1,CYAN,"key_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->key_len);
    POF_DEBUG_CPRINT(1,CYAN,"total_size=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->total_size);
}

static void flow_table_resource(const unsigned char *ph){
    pof_flow_table_resource *p;
    int i;

    p = malloc(sizeof(pof_flow_table_resource));
    memcpy(p,ph,sizeof(pof_flow_table_resource));
    pof_HtoN_transfer_flow_table_resource(p);

    POF_DEBUG_CPRINT(1,CYAN,"resourceType=");
    POF_DEBUG_CPRINT(1,WHITE,"%.2x ",p->resourceType);
    POF_DEBUG_CPRINT(1,CYAN,"counter_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->counter_num);
    POF_DEBUG_CPRINT(1,CYAN,"meter_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->meter_num);
    POF_DEBUG_CPRINT(1,CYAN,"group_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->group_num);

    for(i=0;i<POF_MAX_TABLE_TYPE;i++){
        POF_DEBUG_CPRINT(1,PINK,"[pof_table_resource_desc%d] ",i);
        table_resource_desc((unsigned char *)p+sizeof(pof_flow_table_resource) \
                    -(POF_MAX_TABLE_TYPE-i)*sizeof(pof_table_resource_desc));
    }

    free(p);
}

static void switch_config(const unsigned char *ph){
    pof_switch_config *p;

    p = malloc(sizeof(pof_switch_config));
    memcpy(p,ph,sizeof(pof_switch_config));
    pof_HtoN_transfer_switch_config(p);

    POF_DEBUG_CPRINT(1,CYAN,"flags=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.2x ",p->flags);
    POF_DEBUG_CPRINT(1,CYAN,"miss_send_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->miss_send_len);

    free(p);
}

static void switch_features(const unsigned char *ph){
    pof_switch_features *p;

    p = malloc(sizeof(pof_switch_features));
    memcpy(p,ph,sizeof(pof_switch_features));
    pof_HtoN_transfer_switch_features(p);

    POF_DEBUG_CPRINT(1,CYAN,"dev_id=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p->dev_id);
    POF_DEBUG_CPRINT(1,CYAN,"port_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->port_num);
    POF_DEBUG_CPRINT(1,CYAN,"table_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->table_num);
    POF_DEBUG_CPRINT(1,CYAN,"capabilities=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->capabilities);
    POF_DEBUG_CPRINT(1,CYAN,"vendor_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p->vendor_id);
    POF_DEBUG_CPRINT(1,CYAN,"dev_fw_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p->dev_fw_id);
    POF_DEBUG_CPRINT(1,CYAN,"dev_lkup_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p->dev_lkup_id);

    free(p);
}

static void set_config(const unsigned char *ph){
    pof_switch_config *p;
    int i;

    p = malloc(sizeof(pof_switch_config));
    memcpy(p,ph,sizeof(pof_switch_config));
    pof_HtoN_transfer_switch_config(p);

    POF_DEBUG_CPRINT(1,CYAN,"flags=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p->flags);
    POF_DEBUG_CPRINT(1,CYAN,"miss_send_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%.4x ",p->miss_send_len);

    free(p);
}

static void match(const void *ph){
    pof_match *p = (pof_match *)ph;

    POF_DEBUG_CPRINT(1,PINK,"[match] ");
    POF_DEBUG_CPRINT(1,CYAN,"field_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->field_id);
    POF_DEBUG_CPRINT(1,CYAN,"offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->offset);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);
}

static void match_x(const void *ph){
    pof_match_x *p = (pof_match_x *)ph;

    POF_DEBUG_CPRINT(1,PINK,"[match_x] ");
    POF_DEBUG_CPRINT(1,CYAN,"field_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->field_id);
    POF_DEBUG_CPRINT(1,CYAN,"offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->offset);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);

    POF_DEBUG_CPRINT(1,CYAN,"value=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->value, POF_MAX_FIELD_LENGTH_IN_BYTE);
    POF_DEBUG_CPRINT(1,CYAN,"mask=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(p->mask, POF_MAX_FIELD_LENGTH_IN_BYTE);
}

static void
value(void *ph, uint8_t type, const char *union_name, const char *type_name)
{
	union {
		uint32_t value;
		struct pof_match field;
	} *p = ph;

	POF_DEBUG_CPRINT(1,CYAN,"%s=",type_name);
	if(type == POFVT_IMMEDIATE_NUM){
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_IMMEDIATE_NUM ");
		POF_DEBUG_CPRINT(1,CYAN,"%s=",union_name);
		POF_DEBUG_CPRINT(1,CYAN,"[value]",union_name);
		POF_DEBUG_CPRINT(1,WHITE,"%u ",p->value);
	}else{
		POF_DEBUG_CPRINT(1,WHITE,"POFVT_FIELD ");
		POF_DEBUG_CPRINT(1,CYAN,"%s=",union_name);
		match(&p->field);
	}
}

static void flow_table(const unsigned char *ph){
    pof_flow_table *p;
    uint32_t i;

    p = malloc(sizeof(pof_flow_table));
    memcpy(p,ph,sizeof(pof_flow_table));
    pof_NtoH_transfer_flow_table(p);

    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->command);
    POF_DEBUG_CPRINT(1,CYAN,"tid=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tid);
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->type);
    POF_DEBUG_CPRINT(1,CYAN,"match_field_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->match_field_num);
    POF_DEBUG_CPRINT(1,CYAN,"size=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->size);
    POF_DEBUG_CPRINT(1,CYAN,"key_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->key_len);
    POF_DEBUG_CPRINT(1,CYAN,"table_name=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p->table_name);

    for(i=0; i<p->match_field_num; i++){
        match(&p->match[i]);
    }

    free(p);
}

static void action_DROP(const void *ph){
    pof_action_drop *p = (pof_action_drop *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"reason_code=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->reason_code);
}

static void action_PACKET_IN(const void *ph){
    pof_action_packet_in *p = (pof_action_packet_in *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"reason_code=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->reason_code);
}

static void action_ADD_FIELD(const void *ph){
    pof_action_add_field *p = (pof_action_add_field *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"tag_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_id);
    POF_DEBUG_CPRINT(1,CYAN,"tag_pos=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_pos);
    POF_DEBUG_CPRINT(1,CYAN,"tag_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_len);
    POF_DEBUG_CPRINT(1,CYAN,"tag_value=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.16llx ",p->tag_value);
}

static void action_DELETE_FIELD(const void *ph){
    pof_action_delete_field *p = (pof_action_delete_field *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"tag_pos=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_pos);
    POF_DEBUG_CPRINT(1,CYAN,"tag_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->tag_len);
}

static void action_OUTPUT(const void *ph){
    pof_action_output *p = (pof_action_output *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"outputPortId=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->outputPortId);
    POF_DEBUG_CPRINT(1,CYAN,"metadata_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_offset);
    POF_DEBUG_CPRINT(1,CYAN,"metadata_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_len);
    POF_DEBUG_CPRINT(1,CYAN,"packet_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->packet_offset);
}

static void action_CALCULATE_CHECKSUM(const void *ph){
    pof_action_calculate_checksum *p = (pof_action_calculate_checksum *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"checksum_pos=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->checksum_pos);
    POF_DEBUG_CPRINT(1,CYAN,"checksum_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->checksum_len);
    POF_DEBUG_CPRINT(1,CYAN,"cal_startpos=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->cal_startpos);
    POF_DEBUG_CPRINT(1,CYAN,"cal_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->cal_len);
}

static void action_COUNTER(const void *ph){
    pof_action_counter *p = (pof_action_counter *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"counter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->counter_id);
}

static void action_GROUP(const void *ph){
    pof_action_group *p = (pof_action_group *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"group_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->group_id);
}

static void action_SET_FIELD(const void *ph){
    pof_action_set_field *p = (pof_action_set_field *)ph;

    match_x(&p->field_setting);
}

static void action_SET_FIELD_FROM_METADATA(const void *ph){
    pof_action_set_field_from_metadata *p = (pof_action_set_field_from_metadata *)ph;

    match(&p->field_setting);
    POF_DEBUG_CPRINT(1,CYAN,"metadata_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_offset);
}

static void action_MODIFY_FIELD(const void *ph){
    pof_action_modify_field *p = (pof_action_modify_field *)ph;

    match(&p->field);
    POF_DEBUG_CPRINT(1,CYAN,"increment=");
    POF_DEBUG_CPRINT(1,WHITE,"%d ",p->increment);
}

static void
action_EXPERIMENTER(const void *ph)
{
	POF_DEBUG_CPRINT(1,RED,"Unsupport action type.");
	return;
}

void poflp_action(const void *ph){
    pof_action *p = (pof_action *)ph;

    POF_DEBUG_CPRINT(1,PINK,"[action] ");
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->type);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);

    switch(p->type){
#define ACTION(NAME,VALUE) \
		case POFAT_##NAME:							\
			POF_DEBUG_CPRINT(1,PINK,"["#NAME"] ");	\
			action_##NAME(p->action_data);			\
			break;

		ACTIONS
#undef ACTION
        default:
            POF_DEBUG_CPRINT(1,RED,"Unknow action type");
			break;
    }
}

static void instruction_APPLY_ACTIONS(const void *ph){
    pof_instruction_apply_actions *p = (pof_instruction_apply_actions *)ph;
    uint32_t i;

    POF_DEBUG_CPRINT(1,CYAN,"action_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->action_num);

    for(i=0; i<p->action_num; i++){
        poflp_action(&p->action[i]);
    }
}

static void instruction_GOTO_TABLE(const void *ph){
    pof_instruction_goto_table *p = (pof_instruction_goto_table *)ph;
    uint32_t i;

    POF_DEBUG_CPRINT(1,CYAN,"next_table_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->next_table_id);
    POF_DEBUG_CPRINT(1,CYAN,"match_field_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->match_field_num);
    POF_DEBUG_CPRINT(1,CYAN,"packet_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->packet_offset);

    for(i=0; i<p->match_field_num; i++){
        match(&p->match[i]);
    }
}

static void instruction_GOTO_DIRECT_TABLE(const void *ph){
    pof_instruction_goto_direct_table *p = (pof_instruction_goto_direct_table *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"next_table_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->next_table_id);
    POF_DEBUG_CPRINT(1,CYAN,"table_entry_index=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->table_entry_index);
    POF_DEBUG_CPRINT(1,CYAN,"packet_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->packet_offset);
}

static void instruction_METER(const void *ph){
    pof_instruction_meter *p = (pof_instruction_meter *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"meter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->meter_id);
}

static void instruction_WRITE_METADATA(const void *ph){
    pof_instruction_write_metadata *p = (pof_instruction_write_metadata *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"metadata_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_offset);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);
    POF_DEBUG_CPRINT(1,CYAN,"value=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.8x ",p->value);
}

static void instruction_WRITE_METADATA_FROM_PACKET(const void *ph){
    pof_instruction_write_metadata_from_packet *p = \
                        (pof_instruction_write_metadata_from_packet *)ph;

    POF_DEBUG_CPRINT(1,CYAN,"metadata_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->metadata_offset);
    POF_DEBUG_CPRINT(1,CYAN,"packet_offset=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->packet_offset);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);
}

static void
instruction_WRITE_ACTIONS(const void *ph)
{
	POF_DEBUG_CPRINT(1,RED,"Unsupport instruction type.");
	return;
}

static void
instruction_CLEAR_ACTIONS(const void *ph)
{
	POF_DEBUG_CPRINT(1,RED,"Unsupport instruction type.");
	return;
}

static void
instruction_EXPERIMENTER(const void *ph)
{
	POF_DEBUG_CPRINT(1,RED,"Unsupport instruction type.");
	return;
}

static void
direction(uint8_t direction, const char *direction_name)
{
	POF_DEBUG_CPRINT(1,CYAN,"%s=", direction_name);
	if(direction == POFD_FORWARD){
		POF_DEBUG_CPRINT(1,WHITE,"POFD_FORWARD ");
	}else{
		POF_DEBUG_CPRINT(1,WHITE,"POFD_BACKWARD ");
	}
}

static void instruction(const void *ph){
    pof_instruction *p = (pof_instruction *)ph;

    POF_DEBUG_CPRINT(1,PINK,"[instruction] ");
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->type);
    POF_DEBUG_CPRINT(1,CYAN,"len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->len);

    switch(p->type){
#define INSTRUCTION(NAME,VALUE)							\
		case POFIT_##NAME:								\
			POF_DEBUG_CPRINT(1,PINK,"["#NAME"] ");		\
			instruction_##NAME(p->instruction_data);	\
			break;

		INSTRUCTIONS
#undef INSTRUCTION
        default:
            POF_DEBUG_CPRINT(1,RED,"Unknown instruction ");
			break;
    }
}

void 
poflp_flow_entry(const void *ph)
{
    pof_flow_entry *p = (pof_flow_entry *)ph;
    int i;

    POF_DEBUG_CPRINT(1,PINK,"<entry[%d][%d][%d]> ",p->table_type,
            p->table_id, p->index);
    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->command);
    POF_DEBUG_CPRINT(1,CYAN,"match_field_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->match_field_num);
    POF_DEBUG_CPRINT(1,CYAN,"instruction_num=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->instruction_num);
    POF_DEBUG_CPRINT(1,CYAN,"counter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->counter_id);
    POF_DEBUG_CPRINT(1,CYAN,"cookie=");
    POF_DEBUG_CPRINT(1,WHITE,"%llu ",p->cookie);
    POF_DEBUG_CPRINT(1,CYAN,"cookie_mask=");
    POF_DEBUG_CPRINT(1,WHITE,"%llu ",p->cookie_mask);
    POF_DEBUG_CPRINT(1,CYAN,"table_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->table_id);
    POF_DEBUG_CPRINT(1,CYAN,"table_type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->table_type);
    POF_DEBUG_CPRINT(1,CYAN,"idle_timeout=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->idle_timeout);
    POF_DEBUG_CPRINT(1,CYAN,"hard_timeout=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->hard_timeout);
    POF_DEBUG_CPRINT(1,CYAN,"priority=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->priority);
    POF_DEBUG_CPRINT(1,CYAN,"index=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->index);

    for(i=0; i<p->match_field_num; i++){
        match_x(&p->match[i]);
    }

    for(i=0; i<p->instruction_num; i++){
        instruction(&p->instruction[i]);
    }

	return;
}

static void flow_entry(const unsigned char *ph){
    pof_flow_entry *p;

    p = malloc(sizeof(pof_flow_entry));
    memcpy(p,ph,sizeof(pof_flow_entry));
    pof_NtoH_transfer_flow_entry(p);

//	poflp_flow_entry(p);

    free(p);
}

static void meter(const unsigned char *ph){
    pof_meter *p;

    p = malloc(sizeof(pof_meter));
    memcpy(p,ph,sizeof(pof_meter));
    pof_NtoH_transfer_meter(p);

    POF_DEBUG_CPRINT_0X_NO_ENTER(ph, sizeof(pof_meter));

    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->command);
    POF_DEBUG_CPRINT(1,CYAN,"rate=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->rate);
    POF_DEBUG_CPRINT(1,CYAN,"meter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->meter_id);

    free(p);
}

static void group(const unsigned char *ph){
    pof_group *p;
    uint32_t len = sizeof(pof_group), i;

    p = malloc(len);
    memcpy(p,ph,len);
    pof_NtoH_transfer_group(p);

    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->command);
    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->type);
    POF_DEBUG_CPRINT(1,CYAN,"action_number=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->action_number);
    POF_DEBUG_CPRINT(1,CYAN,"group_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->group_id);
    POF_DEBUG_CPRINT(1,CYAN,"counter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->counter_id);

    for(i=0; i<p->action_number; i++){
        poflp_action(&p->action[i]);
    }

    free(p);
}

static void counter(const unsigned char *ph){
    pof_counter *p;
    uint32_t len = sizeof(pof_counter);

    p = malloc(len);
    memcpy(p,ph,len);
    pof_NtoH_transfer_counter(p);

    POF_DEBUG_CPRINT(1,CYAN,"command=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->command);
    POF_DEBUG_CPRINT(1,CYAN,"counter_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->counter_id);
    POF_DEBUG_CPRINT(1,CYAN,"value=");
    POF_DEBUG_CPRINT(1,WHITE,"%llu ",p->value);

    free(p);
}

static void packet_in(const unsigned char *ph){
    pof_packet_in *p;
    uint32_t len = sizeof(pof_packet_in);

    p = malloc(len);
    memcpy(p,ph,len);
    pof_NtoH_transfer_packet_in(p);

    POF_DEBUG_CPRINT(1,CYAN,"buffer_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->buffer_id);
    POF_DEBUG_CPRINT(1,CYAN,"total_len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->total_len);
    POF_DEBUG_CPRINT(1,CYAN,"reason=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->reason);
    POF_DEBUG_CPRINT(1,CYAN,"table_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->table_id);
    POF_DEBUG_CPRINT(1,CYAN,"cookie=");
    POF_DEBUG_CPRINT(1,WHITE,"%llu ",p->cookie);
    POF_DEBUG_CPRINT(1,CYAN,"device_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->device_id);

    POF_DEBUG_CPRINT_0X_NO_ENTER(p->data, p->total_len);

    free(p);
}

static void error(const unsigned char *ph){
    pof_error *p;
    uint32_t len = sizeof(pof_error);

    p = malloc(len);
    memcpy(p,ph,len);
    pof_NtoH_transfer_error(p);

    POF_DEBUG_CPRINT(1,CYAN,"type=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->type);
    POF_DEBUG_CPRINT(1,CYAN,"code=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%.4x ",p->code);
    POF_DEBUG_CPRINT(1,CYAN,"device_id=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",p->device_id);
    POF_DEBUG_CPRINT(1,CYAN,"err_str=");
    POF_DEBUG_CPRINT(1,WHITE,"%s ",p->err_str);

    free(p);
}

static void packet_raw(const void *ph, pof_header *header_ptr){
    POF_DEBUG_CPRINT(1,BLUE,"[Header:] ");
    POF_DEBUG_CPRINT(1,CYAN,"Ver=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%x ",header_ptr->version);
    POF_DEBUG_CPRINT(1,CYAN,"Type=");
    POF_DEBUG_CPRINT(1,WHITE,"0x%x ",header_ptr->type);
    POF_DEBUG_CPRINT(1,CYAN,"Len=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",header_ptr->length);
    POF_DEBUG_CPRINT(1,CYAN,"Xid=");
    POF_DEBUG_CPRINT(1,WHITE,"%u ",header_ptr->xid);

	POF_DEBUG_CPRINT(1,BLUE,"[PACKET_RAW] ");
    POF_DEBUG_CPRINT(1,CYAN,"pofHead=");
    POF_DEBUG_CPRINT_0X_NO_ENTER(ph, sizeof(pof_header));
    POF_DEBUG_CPRINT(1,CYAN," pofBody=");

	if(header_ptr->length < sizeof(pof_header)){
		return;
	}
    POF_DEBUG_CPRINT_0X_NO_ENTER(ph + sizeof(pof_header), header_ptr->length - sizeof(pof_header));

	return;
}

void pof_debug_cprint_packet(const void * ph, uint32_t flag, int len){
    pof_header *header_ptr;
    pof_type type;

    header_ptr = malloc(sizeof(pof_header));
    memcpy(header_ptr, ph, sizeof(pof_header));
    pof_NtoH_transfer_header(header_ptr);

    if( flag == 0 ){
        POF_DEBUG_CPRINT(1,PINK,"Recv ");
    }
    else if( flag == 1 ){
        POF_DEBUG_CPRINT(1,PINK,"Send ");
    }

    POF_DEBUG_CPRINT(1,PINK,"[%d] ", len);

    type = header_ptr->type;
    switch(type){
        case POFT_HELLO:
            POF_DEBUG_CPRINT(1,PINK,"[HELLO:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_ECHO_REQUEST:
            POF_DEBUG_CPRINT(1,PINK,"[ECHO_REQUEST:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_ECHO_REPLY:
            POF_DEBUG_CPRINT(1,PINK,"[ECHO_REPLY:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_FEATURES_REQUEST:
            POF_DEBUG_CPRINT(1,PINK,"[FEATURES_REQUEST:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_GET_CONFIG_REQUEST:
            POF_DEBUG_CPRINT(1,PINK,"[GET_CONFIG_REQUEST:]");
			packet_raw(ph, header_ptr);
            break;
        case POFT_GET_CONFIG_REPLY:
            POF_DEBUG_CPRINT(1,PINK,"[GET_CONFIG_REPLY:]");
            switch_config((unsigned char *)ph+sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_SET_CONFIG:
            POF_DEBUG_CPRINT(1,PINK,"[SET_CONFIG:]");
            set_config((uint8_t *)ph+sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_FEATURES_REPLY:
            POF_DEBUG_CPRINT(1,PINK,"[FEATURES_REPLY:] ");
            switch_features((unsigned char *)ph+sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_RESOURCE_REPORT:
            POF_DEBUG_CPRINT(1,PINK,"[RESOURCE_REPORT:] ");
            flow_table_resource((unsigned char *)ph+sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_PORT_STATUS:
            POF_DEBUG_CPRINT(1,PINK,"[PORT_STATUS:] ");
            port_status((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_TABLE_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[TABLE_MOD:] ");
            flow_table((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_FLOW_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[FLOW_MOD:] ");
            flow_entry((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_METER_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[METER_MOD:] ");
            meter((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_GROUP_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[GROUP_MOD:] ");
            group((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_COUNTER_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[COUNTER_MOD:] ");
            counter((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_PORT_MOD:
            POF_DEBUG_CPRINT(1,PINK,"[PORT_MOD:] ");
            port_status((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_COUNTER_REQUEST:
            POF_DEBUG_CPRINT(1,PINK,"[COUNTER_REQUEST:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_COUNTER_REPLY:
            POF_DEBUG_CPRINT(1,PINK,"[COUNTER_REPLY:] ");
            counter((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_PACKET_IN:
            POF_DEBUG_CPRINT(1,PINK,"[PACKET_IN:] ");
            packet_in((unsigned char *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        case POFT_PACKET_OUT:
            POF_DEBUG_CPRINT(1,PINK,"[PACKET_OUT:] ");
			packet_raw(ph, header_ptr);
            break;
        case POFT_ERROR:
            POF_DEBUG_CPRINT(1,PINK,"[ERROR:] ");
            error((uint8_t *)ph + sizeof(pof_header));
			packet_raw(ph, header_ptr);
            break;
        default:
            POF_DEBUG_CPRINT(1,RED,"Unknown POF type. ");
            break;
    }
    POF_DEBUG_CPRINT(1,PINK,"\n");

    free(header_ptr);
}

struct log_util g_log = {
	NULL,
	PTHREAD_MUTEX_INITIALIZER,
	0,
	{0},

#ifdef POF_DEBUG_COLOR_ON
	textcolor,
	overcolor,
#else // POF_DEBUG_COLOR_ON
	textcolor_disable,
	overcolor_disable,
#endif // POF_DEBUG_COLOR_ON

#ifdef POF_DEBUG_ON
	print,
#else // POF_DEBUG_ON
	print_disable,
#endif // POF_DEBUG_ON

	print,					/* Command print function. */
	print,					/* Error print function. */
	print,					/* Print function. Can not disable. */
	log_disable,			/* Log function. */
};
