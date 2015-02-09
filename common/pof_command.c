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
#include "../include/pof_command.h"
#include "../include/pof_datapath.h"
#include <string.h>
#include <stdio.h>

#ifdef POF_COMMAND_ON

/* The user commands. */
#define COMMANDS \
	COMMAND(help)				\
	COMMAND(quit)				\
	COMMAND(switch_config)		\
	COMMAND(switch_feature)		\
	COMMAND(table_resource)		\
	COMMAND(ports)				\
	COMMAND(tables)				\
	COMMAND(groups)				\
	COMMAND(meters)				\
	COMMAND(counters)			\
	COMMAND(version)			\
	COMMAND(state)			\
	COMMAND(clear_resource)		\
	COMMAND(enable_debug)		\
	COMMAND(disable_debug)		\
	COMMAND(enable_color)		\
	COMMAND(disable_color)		\
	COMMAND(enable_promisc)		\
	COMMAND(disable_promisc)	\
	COMMAND(test)

/* The user commands. */
enum pof_usr_cmd{
#define COMMAND(STRING) POFUC_##STRING,
	COMMANDS
#undef COMMAND

	POF_COMMAND_NUM
};

static void usr_cmd_clear_resource(){
	POF_COMMAND_PRINT_HEAD("clear_resource");
	POF_LOG_LOCK_OFF;
	poflr_clear_resource();
	POF_LOG_LOCK_ON;
	return;
}

static uint8_t usr_cmd_gets(char * cmd, size_t cmd_len){
    uint8_t i;
	if(fgets(cmd, cmd_len, stdin) == NULL){
		return 0xff;
	}
	cmd[strlen(cmd) - 1] = 0;

#define COMMAND(STRING) if(strcmp(cmd,#STRING)==0) {return POFUC_##STRING;}
	COMMANDS
#undef COMMAND

    return 0xff;
}

static void
usr_cmd_enable_debug()
{
	POF_COMMAND_PRINT_HEAD("enable_debug");
	poflp_debug_enable();
    strncpy(g_states.debug_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
}

static void
usr_cmd_disable_debug()
{
	POF_COMMAND_PRINT_HEAD("disable_debug");
	poflp_debug_disable();
    strncpy(g_states.debug_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
}

static void
usr_cmd_enable_color()
{
	POF_COMMAND_PRINT_HEAD("enable_color");
	poflp_color_enable();
    strncpy(g_states.debug_color_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
}

static void
usr_cmd_disable_color()
{
	POF_COMMAND_PRINT_HEAD("disable_color");
	poflp_color_disable();
    strncpy(g_states.debug_color_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
}

static void
usr_cmd_state()
{
	POF_COMMAND_PRINT_HEAD("state");
    pof_states_print();
}

static void
usr_cmd_test()
{
	POF_COMMAND_PRINT_HEAD("test");
	return;
}

static void usr_cmd_version(){
	POF_COMMAND_PRINT_HEAD("version");
	POF_COMMAND_PRINT(1,PINK,"%s\n", POFSWITCH_VERSION);
	return;
}

static void usr_cmd_help(){
    int i;
	POF_COMMAND_PRINT_HEAD("help");
#define COMMAND(STRING) POF_COMMAND_PRINT(1,PINK,#STRING"\n");
	COMMANDS
#undef COMMAND
    return;
}

static void usr_cmd_switch_config(){
    pof_switch_config *pofsc_ptr = NULL;
    poflr_get_switch_config(&pofsc_ptr);

    POF_COMMAND_PRINT_HEAD("switch_config");

    POF_COMMAND_PRINT(1,CYAN,"flags=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",pofsc_ptr->flags);
    POF_COMMAND_PRINT(1,CYAN,"miss_send_len=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",pofsc_ptr->miss_send_len);
	POF_COMMAND_PRINT(1,WHITE,"\n");
    return;
}

static void usr_cmd_switch_feature(){
    pof_switch_features *p = NULL;
    poflr_get_switch_feature(&p);
    POF_COMMAND_PRINT_HEAD("switch_feature");

    POF_COMMAND_PRINT(1,CYAN,"dev_id=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.4x ",p->dev_id);
    POF_COMMAND_PRINT(1,CYAN,"port_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->port_num);
    POF_COMMAND_PRINT(1,CYAN,"table_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->table_num);
    POF_COMMAND_PRINT(1,CYAN,"capabilities=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->capabilities);
    POF_COMMAND_PRINT(1,CYAN,"vendor_id=");
    POF_COMMAND_PRINT(1,WHITE,"%s ",p->vendor_id);
    POF_COMMAND_PRINT(1,CYAN,"dev_fw_id=");
    POF_COMMAND_PRINT(1,WHITE,"%s ",p->dev_fw_id);
    POF_COMMAND_PRINT(1,CYAN,"dev_lkup_id=");
    POF_COMMAND_PRINT(1,WHITE,"%s ",p->dev_lkup_id);
	POF_COMMAND_PRINT(1,WHITE,"\n");
    return;
}

static void table_resource_desc(pof_table_resource_desc *p){

    POF_COMMAND_PRINT(1,CYAN,"device_id=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.4x ",p->device_id);
    POF_COMMAND_PRINT(1,CYAN,"type=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->type);
    POF_COMMAND_PRINT(1,CYAN,"tbl_num=");
    POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->tbl_num);
    POF_COMMAND_PRINT(1,CYAN,"key_len=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->key_len);
    POF_COMMAND_PRINT(1,CYAN,"total_size=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->total_size);
	POF_COMMAND_PRINT(1,WHITE,"\n");
    return;
}

static void usr_cmd_table_resource(){
    pof_flow_table_resource *p = NULL;
    int i;
    poflr_get_flow_table_resource(&p);
    POF_COMMAND_PRINT_HEAD("table_resource");

    POF_COMMAND_PRINT(1,CYAN,"resourceType=");
    POF_COMMAND_PRINT(1,WHITE,"%.2x ",p->resourceType);
    POF_COMMAND_PRINT(1,CYAN,"counter_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->counter_num);
    POF_COMMAND_PRINT(1,CYAN,"meter_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->meter_num);
    POF_COMMAND_PRINT(1,CYAN,"group_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",p->group_num);
	POF_COMMAND_PRINT(1,WHITE,"\n");

    for(i=0;i<POF_MAX_TABLE_TYPE;i++){
        POF_COMMAND_PRINT(1,PINK,"[pof_table_resource_desc %d] ",i);
        table_resource_desc(&p->tbl_rsc_desc[i]);
		POF_COMMAND_PRINT(1,WHITE,"\n");
    }
    return;
}

static void match(pof_match *p){
    POF_COMMAND_PRINT(1,CYAN,"field_id=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", p->field_id);
    POF_COMMAND_PRINT(1,CYAN,"offset=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", p->offset);
    POF_COMMAND_PRINT(1,CYAN,"len=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", p->len);
}

static void flow_entry_baseinfo(pof_flow_entry *p){
    int i;

	void (*tmp)(char *,char *,char *) = g_log._pDbg;
	poflp_debug_enable();

	poflp_flow_entry(p);

	g_log._pDbg = tmp;

    POF_COMMAND_PRINT(1,WHITE,"\n");
    return;
}

static void flow_entry(poflr_flow_table *poflrft_ptr){
    int entry_num = poflrft_ptr->entry_num;
    poflr_flow_entry *tmp_vhal_entry_ptr = poflrft_ptr->entry_ptr;
    int entry_id, sum = 0;
    for(entry_id=0; sum<entry_num; entry_id++){
        if(tmp_vhal_entry_ptr[entry_id].state == POFLR_STATE_INVALID)
            continue;
        flow_entry_baseinfo(&tmp_vhal_entry_ptr[entry_id].entry);
        sum++;
    }
}

static void flow_table_baseinfo(poflr_flow_table *tmp_vhal_flow_table){
    char ctype[POF_MAX_TABLE_TYPE][5] = {
                            "MM","LPM","EM","DT",};
    uint32_t type = tmp_vhal_flow_table->tbl_base_info.type;
    uint32_t table_id = tmp_vhal_flow_table->tbl_base_info.tid;
    uint32_t i;

    POF_COMMAND_PRINT(1,PINK,"\n[Flow table] [%s %d] ", ctype[type], table_id);

    pof_flow_table t = tmp_vhal_flow_table->tbl_base_info;
    if(tmp_vhal_flow_table->state == POFLR_STATE_INVALID){
        POF_COMMAND_PRINT(1,CYAN,"state=");
        POF_COMMAND_PRINT(1,WHITE,"INVALID ");
        return;
    }

    POF_COMMAND_PRINT(1,CYAN,"state=");
    POF_COMMAND_PRINT(1,WHITE,"VALID ");

    POF_COMMAND_PRINT(1,CYAN,"entry_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", tmp_vhal_flow_table->entry_num);
    POF_COMMAND_PRINT(1,CYAN,"table_id=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", t.tid);
    POF_COMMAND_PRINT(1,CYAN,"type=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", t.type);
    POF_COMMAND_PRINT(1,CYAN,"match_field_num=");
    POF_COMMAND_PRINT(1,WHITE,"%u ",t.match_field_num);
    POF_COMMAND_PRINT(1,CYAN,"size=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", t.size);
    POF_COMMAND_PRINT(1,CYAN,"key_len=");
    POF_COMMAND_PRINT(1,WHITE,"%u ", t.key_len);
    POF_COMMAND_PRINT(1,CYAN,"table_name=");
    POF_COMMAND_PRINT(1,WHITE,"%s ", t.table_name);
    for(i=0;i<t.match_field_num;i++){
        POF_COMMAND_PRINT(1,PINK,"<match %d> ", i);
        match(&t.match[i]);
    }
	POF_COMMAND_PRINT(1,WHITE,"\n");
    return;
}

static void flow_table_single(poflr_flow_table table_vhal){
    flow_table_baseinfo(&table_vhal);
    flow_entry(&table_vhal);
}

static void flow_table(){
    poflr_flow_table *p = NULL;
    uint8_t *table_num = NULL;
    int type, table_id, entry_id;

    poflr_get_table_number(&table_num);

    for(type=0; type<POF_MAX_TABLE_TYPE; type++){
        for(table_id=0; table_id<table_num[type]; table_id++){
            poflr_get_flow_table(&p, type, table_id);
            if(p->state == POFLR_STATE_INVALID){
                continue;
            }
            flow_table_single(*p);
        }
    }
}

static void usr_cmd_groups(){
    pof_flow_table_resource *flow_table_resource_ptr = NULL;
    poflr_groups *group_ptr = NULL;
    uint32_t i, j, count, num;
    pof_group *p = NULL;

    poflr_get_group(&group_ptr);
    poflr_get_flow_table_resource(&flow_table_resource_ptr);
    num = group_ptr->group_num;

    POF_COMMAND_PRINT_HEAD("groups");
    for(i=0, count=0; i<flow_table_resource_ptr->group_num || count<num; i++){
        if(group_ptr->state[i] == POFLR_STATE_INVALID){
            continue;
        }
        p = &group_ptr->group[i];
        POF_COMMAND_PRINT(1,CYAN,"type=");
        POF_COMMAND_PRINT(1,WHITE,"%u ", p->type);
        POF_COMMAND_PRINT(1,CYAN,"action_number=");
        POF_COMMAND_PRINT(1,WHITE,"%u ", p->action_number);
        POF_COMMAND_PRINT(1,CYAN,"group_id=");
        POF_COMMAND_PRINT(1,WHITE,"%u ", p->group_id);
        POF_COMMAND_PRINT(1,CYAN,"counter_id=");
        POF_COMMAND_PRINT(1,WHITE,"%u ", p->counter_id);

		void (*tmp)(char *,char *,char *) = g_log._pDbg;
		poflp_debug_enable();

        for(j=0;j<p->action_number;j++){
            poflp_action(&p->action[j]);
        }
		g_log._pDbg = tmp;

        POF_COMMAND_PRINT(1,CYAN,"\n");
        count++;
    }
}

static void usr_cmd_meters(){
    pof_flow_table_resource *flow_table_resource_ptr = NULL;
    poflr_meters *meter_ptr = NULL;
    uint32_t i, j, count, num;
    pof_meter *p = NULL;

    poflr_get_meter(&meter_ptr);
    poflr_get_flow_table_resource(&flow_table_resource_ptr);
    num = meter_ptr->meter_num;

    POF_COMMAND_PRINT_HEAD("meters");
    for(i=0, count=0; i<flow_table_resource_ptr->meter_num || count<num; i++){
        if(meter_ptr->state[i] == POFLR_STATE_INVALID){
            continue;
        }
        p = &meter_ptr->meter[i];
        POF_COMMAND_PRINT(1,CYAN,"rate=");
        POF_COMMAND_PRINT(1,WHITE,"%u ", p->rate);
        POF_COMMAND_PRINT(1,CYAN,"meter_id=");
        POF_COMMAND_PRINT(1,WHITE,"%u ", p->meter_id);
        POF_COMMAND_PRINT(1,CYAN,"\n");
        count++;
    }
}

static void usr_cmd_counters(){
    pof_flow_table_resource *flow_table_resource_ptr = NULL;
    poflr_counters *counter_ptr = NULL;
    uint32_t i, j, count, num;
    pof_counter *p = NULL;

    poflr_get_counter(&counter_ptr);
    poflr_get_flow_table_resource(&flow_table_resource_ptr);
    num = counter_ptr->counter_num;

    POF_COMMAND_PRINT_HEAD("counter");
    for(i=0, count=0; i<flow_table_resource_ptr->counter_num || count<num; i++){
        if(counter_ptr->state[i] == POFLR_STATE_INVALID){
            continue;
        }
        p = &counter_ptr->counter[i];
        POF_COMMAND_PRINT(1,CYAN,"counter_id=");
        POF_COMMAND_PRINT(1,WHITE,"%u ", p->counter_id);
        POF_COMMAND_PRINT(1,CYAN,"value=");
        POF_COMMAND_PRINT(1,WHITE,"%llu ", p->value);
        POF_COMMAND_PRINT(1,CYAN,"\n");
        count++;
    }
}

void usr_cmd_tables(){
	POF_COMMAND_PRINT_HEAD("tables");
    flow_table();
}

static void usr_cmd_ports(){
    pof_port *p = NULL;
    uint16_t port_num = 0;
    int i;

    POF_COMMAND_PRINT_HEAD("ports");
    poflr_get_port(&p);
    poflr_get_port_number(&port_num);

    for(i=0;i<port_num;i++){
        POF_COMMAND_PRINT(1,PINK,"[Port %d] ", i);

        POF_COMMAND_PRINT(1,CYAN,"port_id=");
        POF_COMMAND_PRINT(1,WHITE,"%u ",p->port_id);
        POF_COMMAND_PRINT(1,CYAN,"device_id=");
        POF_COMMAND_PRINT(1,WHITE,"0x%.4x ",p->device_id);
        POF_COMMAND_PRINT(1,CYAN,"hw_addr=");
        POF_COMMAND_PRINT_0X_NO_ENTER(p->hw_addr, POF_ETH_ALEN);
        POF_COMMAND_PRINT(1,CYAN,"name=");
        POF_COMMAND_PRINT(1,WHITE,"%s ",p->name);
        POF_COMMAND_PRINT(1,CYAN,"config=");
        POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->config);
        POF_COMMAND_PRINT(1,CYAN,"state=");
        POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->state);
        POF_COMMAND_PRINT(1,CYAN,"curr=");
        POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->curr);
        POF_COMMAND_PRINT(1,CYAN,"advertised=");
        POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->advertised);
        POF_COMMAND_PRINT(1,CYAN,"supported=");
        POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->supported);
        POF_COMMAND_PRINT(1,CYAN,"peer=");
        POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->peer);
        POF_COMMAND_PRINT(1,CYAN,"curr_speed=");
        POF_COMMAND_PRINT(1,WHITE,"%u ",p->curr_speed);
        POF_COMMAND_PRINT(1,CYAN,"max_speed=");
        POF_COMMAND_PRINT(1,WHITE,"%u ",p->max_speed);
        POF_COMMAND_PRINT(1,CYAN,"of_enable=");
        POF_COMMAND_PRINT(1,WHITE,"0x%.2x ",p->of_enable);
        p++;
		POF_COMMAND_PRINT(1,WHITE,"\n");
    }
    return;
}

static void
usr_cmd_quit()
{
	POF_COMMAND_PRINT_HEAD("quit");
	return;
}

static void
usr_cmd_enable_promisc()
{
    dp.filter = dp.promisc;
	POF_COMMAND_PRINT_HEAD("Enable PROMISC.");
    strncpy(g_states.promisc_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
	return;
}

static void
usr_cmd_disable_promisc()
{
    dp.filter = dp.no_promisc;
	POF_COMMAND_PRINT_HEAD("Disable PROMISC.");
    strncpy(g_states.promisc_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
	return;
}

void pof_runing_command(char *cmd, size_t cmd_len){
	uint8_t usr_cmd_type;
    while(1){
        POF_COMMAND_PRINT_NO_LOGFILE(1,BLUE,"pof_command >>");

		usr_cmd_type = usr_cmd_gets(cmd, cmd_len);
		if(usr_cmd_type == POFUC_quit){
			return;
		}

        switch(usr_cmd_type){
#define COMMAND(STRING) \
			case POFUC_##STRING:	\
				POF_LOG_LOCK_ON;	\
				usr_cmd_##STRING(); \
				POF_LOG_LOCK_OFF;	\
				break;

			COMMANDS
#undef COMMAND
            default:
				if(cmd[0] == 0){
					break;
				}
				POF_COMMAND_PRINT_HEAD("wrong command: %u", usr_cmd_type);
                POF_COMMAND_PRINT(1,RED,"Input 'help' to find commands!\n");
                break;
        }
    }
}

#endif // POF_COMMAND_ON

