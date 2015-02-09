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
#include "../include/pof_log_print.h"
#include "../include/pof_local_resource.h"
#include "../include/pof_byte_transfer.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define POF_STRING_MAX_LEN (100)

struct pof_state g_states;

char pofc_cmd_input_file_name[POF_STRING_MAX_LEN] = "\0";
uint32_t pofc_cmd_set_ip_ret = FALSE;
uint32_t pofc_cmd_set_port_ret = FALSE;
uint32_t pofc_cmd_set_config_file = FALSE;
uint32_t pofc_cmd_auto_clear = TRUE;

#ifdef POF_CONN_CONFIG_COMMAND

#define COMMAND_STR_LEN (30)
#define HELP_STR_LEN    (50)

//  CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP)
#define CONFIG_CMDS \
    CONFIG_CMD('i',"i:","ip-addr",ip_addr,"IP address of the Controller.")                      \
    CONFIG_CMD('p',"p:","port",port,"Connection port number. Default is 6633.")              \
    CONFIG_CMD('f',"f:","file",file,"Set config file.")                                      \
    CONFIG_CMD('m',"m","man-clear",man_clear,"Don't auto clear the resource when disconnect.")    \
    CONFIG_CMD('l',"l","log-file",log_file,"Create log file: /usr/local/var/log/pofswitch.log.") \
    CONFIG_CMD('s',"s","state",state,"Print software state information.")                     \
    CONFIG_CMD('t',"t","test",test,"Test.")                     \
    CONFIG_CMD('h',"h","help",help,"Print help message.")                                    \
    CONFIG_CMD('v',"v","version",version,"Print the version number of POFSwitch.")

static uint32_t
start_cmd_ip_addr(char *optarg)
{
    pofsc_set_controller_ip(optarg);
    pofc_cmd_set_ip_ret = TRUE;
}

static uint32_t
start_cmd_port(char *optarg)
{
    pofsc_set_controller_port(atoi(optarg));
    pofc_cmd_set_port_ret = TRUE;
}

static uint32_t
start_cmd_file(char *optarg)
{
    strncpy(pofc_cmd_input_file_name, optarg, POF_STRING_MAX_LEN-1);
    pofc_cmd_set_config_file = TRUE;
}

static uint32_t
start_cmd_man_clear(char *optarg)
{
    pofc_cmd_auto_clear = FALSE;
    strncpy(g_states.autoclear_after_disconn.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
}

static uint32_t
start_cmd_log_file(char *optarg)
{
    if(POF_OK != pofsc_check_root()){
        exit(0);
    }
    pof_open_log_file(NULL);
}

static uint32_t
start_cmd_state(char *optarg)
{
    pof_states_print();
    exit(0);
}

static uint32_t
start_cmd_test(char *optarg)
{
    exit(0);
}

static uint32_t
start_cmd_help(char *optarg)
{
	printf("Usage: pofswitch [options] [target] ...\n");
	printf("Options:\n");

#define CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP) \
    printf("  -%c, --%-24s%-50s\n",OPT,LONG,HELP);
    CONFIG_CMDS
#undef CONFIG_CMD

	printf("\nReport bugs to <yujingzhou@huawei.com>\n");
    exit(0);
}

static uint32_t
start_cmd_version(char *optarg)
{
    printf("Version: %s\n", POFSWITCH_VERSION);
    exit(0);
}


/***********************************************************************
 * Initialize the config by command.
 * Form:     static uint32_t pof_set_init_config_by_command(int argc, char *argv[])
 * Input:    command arg numbers, command args.
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function initialize the config of Soft Switch by user's 
 *           command arguments. 
 *           -i: set the IP address of the Controller.
 *           -p: set the port to connect to the Controller.
 ***********************************************************************/
static uint32_t pof_set_init_config_by_command(int argc, char *argv[]){
	uint32_t ret = POF_OK;
#define OPTSTRING_LEN (50)
	char optstring[OPTSTRING_LEN];
#undef OPTSTRING_LEN
 	struct option long_options[] = {
#define CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP) {LONG,0,NULL,OPT},
        CONFIG_CMDS
#undef CONFIG_CMD
		{NULL,0,NULL,0}
	};
	int ch;

#define CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP) strncat(optstring,OPTSTR,COMMAND_STR_LEN);
    CONFIG_CMDS
#undef CONFIG_CMD

	while((ch=getopt_long(argc, argv, optstring, long_options, NULL)) != -1){
		switch(ch){
			case '?':
				exit(0);
				break;
#define CONFIG_CMD(OPT,OPTSTR,LONG,FUNC,HELP)   \
            case OPT:                           \
                start_cmd_##FUNC(optarg);       \
                break;

            CONFIG_CMDS
#undef CONFIG_CMD
			default:
				break;
		}
	}
}

uint32_t pof_auto_clear(){
	return pofc_cmd_auto_clear;
}

#endif // POF_CONN_CONFIG_COMMAND

#ifdef POF_CONN_CONFIG_FILE

enum pof_init_config_type{
	POFICT_CONTROLLER_IP	= 0,
	POFICT_CONNECTION_PORT  = 1,
	POFICT_MM_TABLE_NUMBER = 2,
	POFICT_LPM_TABLE_NUMBER = 3,
	POFICT_EM_TABLE_NUMBER  = 4,
	POFICT_DT_TABLE_NUMBER  = 5,
	POFICT_FLOW_TABLE_SIZE  = 6,
	POFICT_FLOW_TABLE_KEY_LENGTH = 7,
	POFICT_METER_NUMBER     = 8,
	POFICT_COUNTER_NUMBER   = 9,
	POFICT_GROUP_NUMBER     = 10,
	POFICT_DEVICE_PORT_NUMBER_MAX = 11,

	POFICT_CONFIG_TYPE_MAX,
};

char *config_type_str[POFICT_CONFIG_TYPE_MAX] = {
	"Controller_IP", "Connection_port", 
	"MM_table_number", "LPM_table_number", "EM_table_number", "DT_table_number",
	"Flow_table_size", "Flow_table_key_length", 
	"Meter_number", "Counter_number", "Group_number", 
	"Device_port_number_max"
};

static uint8_t pofsic_get_config_type(char *str){
	uint8_t i;

	for(i=0; i<POFICT_CONFIG_TYPE_MAX; i++){
		if(strcmp(str, config_type_str[i]) == 0){
			return i;
		}
	}

	return 0xff;
}

static uint32_t pofsic_get_config_data(FILE *fp, uint32_t *ret_p){
	char str[POF_STRING_MAX_LEN];

	if(fscanf(fp, "%s", str) != 1){
		*ret_p = POF_ERROR;
		return POF_ERROR;
	}else{
		return atoi(str);
	}
}

/***********************************************************************
 * Initialize the config by file.
 * Form:     static uint32_t pof_set_init_config_by_file()
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function initialize the config of Soft Switch by "config"
 *           file. The config's content:
 *			 "Controller_IP", "Connection_port", 
 *			 "MM_table_number", "LPM_table_number", "EM_table_number", "DT_table_number",
 *			 "Flow_table_size", "Flow_table_key_length", 
 *			 "Meter_number", "Counter_number", "Group_number", 
 *			 "Device_port_number_max"
 ***********************************************************************/
static uint32_t pof_set_init_config_by_file(){
	uint32_t ret = POF_OK, data = 0;
	char     filename_relative[] = "./pofswitch_config.conf";
	char     filename_absolute[] = "/etc/pofswitch/pofswitch_config.conf";
	char     filename_final[POF_STRING_MAX_LEN] = "\0";
	char     str[POF_STRING_MAX_LEN] = "\0";
	char     ip_str[POF_STRING_MAX_LEN] = "\0";
	FILE     *fp = NULL;
	uint8_t  config_type = 0;

	if((fp = fopen(pofc_cmd_input_file_name, "r")) == NULL){
		if(pofc_cmd_set_config_file != FALSE){
			POF_DEBUG_CPRINT_FL(1,RED,"The file %s is not exist.", pofc_cmd_input_file_name);
		}
		if((fp = fopen(filename_relative, "r")) == NULL){
			POF_DEBUG_CPRINT_FL(1,RED,"The file %s is not exist.", filename_relative);
			if((fp = fopen(filename_absolute, "r")) == NULL){
				POF_DEBUG_CPRINT_FL(1,RED,"The file %s is not exist.", filename_absolute);
				return POF_ERROR;
			}else{
				strncpy(filename_final, filename_absolute, POF_STRING_MAX_LEN-1);
			}
		}else{
			strncpy(filename_final, filename_relative, POF_STRING_MAX_LEN-1);
		}
	}else{
        strncpy(filename_final, pofc_cmd_input_file_name, POF_STRING_MAX_LEN-1);
	}

    POF_DEBUG_CPRINT_FL(1,GREEN,"The config file %s have been loaded.", filename_final);
    strncpy(g_states.config_file.cont, filename_final, POF_STRING_PAIR_MAX_LEN-1);
	
	while(fscanf(fp, "%s", str) == 1){
		config_type = pofsic_get_config_type(str);
		if(config_type == POFICT_CONTROLLER_IP){
			if(fscanf(fp, "%s", ip_str) != 1){
				ret = POF_ERROR;
			}else{
				if(pofc_cmd_set_ip_ret == FALSE){
					pofsc_set_controller_ip(ip_str);
				}
			}
		}else{
			data = pofsic_get_config_data(fp, &ret);
			switch(config_type){
				case POFICT_CONNECTION_PORT:
					if(pofc_cmd_set_port_ret == FALSE){
	                    pofsc_set_controller_port(data);
					}
					break;
				case POFICT_MM_TABLE_NUMBER:
					poflr_set_MM_table_number(data);
					break;
				case POFICT_LPM_TABLE_NUMBER:
					poflr_set_LPM_table_number(data);
					break;
				case POFICT_EM_TABLE_NUMBER:
					poflr_set_EM_table_number(data);
					break;
				case POFICT_DT_TABLE_NUMBER:
					poflr_set_DT_table_number(data);
					break;
				case POFICT_FLOW_TABLE_SIZE:
					poflr_set_flow_table_size(data);
					break;
				case POFICT_FLOW_TABLE_KEY_LENGTH:
					poflr_set_key_len(data);
					break;
				case POFICT_METER_NUMBER:
					poflr_set_meter_number(data);
					break;
				case POFICT_COUNTER_NUMBER:
					poflr_set_counter_number(data);
					break;
				case POFICT_GROUP_NUMBER:
					poflr_set_group_number(data);
					break;
				case POFICT_DEVICE_PORT_NUMBER_MAX:
					poflr_set_port_number_max(data);
					break;
				default:
					ret = POF_ERROR;
					break;
			}
		}

		if(ret != POF_OK){
			POF_ERROR_CPRINT_FL(1,RED,"Can't load config file: Wrong config format.");
			exit(0);
		}
	}

	fclose(fp);
	return ret;
}
#endif // POF_CONN_CONFIG_FILE

static void pof_set_init_states(){
    
    strncpy(g_states.version.item, "VERSION", POF_STRING_PAIR_MAX_LEN - 1);
    strncpy(g_states.version.cont, POFSWITCH_VERSION, POF_STRING_PAIR_MAX_LEN-1);

    strncpy(g_states.os_bit.item, "OS BIT", POF_STRING_PAIR_MAX_LEN - 1);
#if (POF_OS_BIT == POF_OS_32)
    strncpy(g_states.os_bit.cont, "32", POF_STRING_PAIR_MAX_LEN-1);
#elif (POF_OS_BIT == POF_OS_64)
    strncpy(g_states.os_bit.cont, "64", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_OS_BIT

    strncpy(g_states.byte_order.item, "BYTE ORDER", POF_STRING_PAIR_MAX_LEN-1);
#if (POF_BYTE_ORDER == POF_BIG_ENDIAN)
    strncpy(g_states.byte_order.cont, "BIG ENDIAN", POF_STRING_PAIR_MAX_LEN-1);
#elif (POF_BYTE_ORDER == POF_LITTLE_ENDIAN)
    strncpy(g_states.byte_order.cont, "LITTLE ENDIAN", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_BYTE_ORDER

    strncpy(g_states.configure_arg.item, "CONFIGURE ARG", POF_STRING_PAIR_MAX_LEN-1);
#ifdef ENABLE_CONFIGURE_ARG
    strncpy(g_states.configure_arg.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.configure_arg.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif // ENABLE_CONFIGURE_ARG

    strncpy(g_states.debug_on.item, "DEBUG", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_DEBUG_ON
    strncpy(g_states.debug_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.debug_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_DEBUG_ON

    strncpy(g_states.debug_color_on.item, "COLOR", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_DEBUG_COLOR_ON
    strncpy(g_states.debug_color_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.debug_color_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_DEBUG_COLOR_ON

    strncpy(g_states.debug_print_echo_on.item, "ECHO PRINT", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_DEBUG_PRINT_ECHO_ON
    strncpy(g_states.debug_print_echo_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.debug_print_echo_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_DEBUG_PRINT_ECHO_ON

    strncpy(g_states.datapath_on.item, "DATAPATH", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_DATAPATH_ON
    strncpy(g_states.datapath_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.datapath_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_DATAPATH_ON

    strncpy(g_states.command_on.item, "POF COMMAND", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_COMMAND_ON
    strncpy(g_states.command_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.command_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_COMMAND_ON

    strncpy(g_states.error_print_on.item, "ERROR PRINT", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_ERROR_PRINT_ON
    strncpy(g_states.error_print_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.error_print_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_ERROR_PRINT_ON

    strncpy(g_states.config_file.item, "CONFIG FILE", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_CONN_CONFIG_FILE
    strncpy(g_states.config_file.cont, "./pofswitch_config.conf", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.config_file.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_CONN_CONFIG_FILE
    
    strncpy(g_states.config_cmd.item, "CONFIG CMD", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_CONN_CONFIG_COMMAND
    strncpy(g_states.config_cmd.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.config_cmd.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_CONN_CONFIG_COMMAND

    strncpy(g_states.perform_after_ctrl_disconn.item, "AFTER DISCONNECTION", POF_STRING_PAIR_MAX_LEN-1);
#if (POF_PERFORM_AFTER_CTRL_DISCONN == POF_AFTER_CTRL_DISCONN_RECONN)
    strncpy(g_states.perform_after_ctrl_disconn.cont, "RE CONNECT", POF_STRING_PAIR_MAX_LEN-1);
#elif (POF_PERFORM_AFTER_CTRL_DISCONN == POF_AFTER_CTRL_DISCONN_SHUT_DOWN)
    strncpy(g_states.perform_after_ctrl_disconn.cont, "SHUT DOWN", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_PERFORM_AFTER_CTRL_DISCONN

    strncpy(g_states.nomatch.item, "NO MATCH", POF_STRING_PAIR_MAX_LEN-1);
#if (POF_NOMATCH == POF_NOMATCH_DROP)
    strncpy(g_states.nomatch.cont, "DROP", POF_STRING_PAIR_MAX_LEN-1);
#elif (POF_NOMATCH == POF_NOMATCH_PACKET_IN)
    strncpy(g_states.nomatch.cont, "PACKET IN", POF_STRING_PAIR_MAX_LEN-1);
#endif // POF_NOMATCH

    strncpy(g_states.sd2n.item, "SD2N", POF_STRING_PAIR_MAX_LEN-1);
    strncpy(g_states.sd2n.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);

    strncpy(g_states.promisc_on.item, "PROMISC", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_PROMISC_ON
    strncpy(g_states.promisc_on.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.promisc_on.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif

    strncpy(g_states.ctrl_ip.item, "CONTROLLER IP", POF_STRING_PAIR_MAX_LEN-1);
    strncpy(g_states.ctrl_ip.cont, pofsc_controller_ip_addr, POF_STRING_PAIR_MAX_LEN-1);

    strncpy(g_states.conn_port.item, "CONNECTION PORT", POF_STRING_PAIR_MAX_LEN-1);
    sprintf(g_states.conn_port.cont, "%u", pofsc_controller_port);

    strncpy(g_states.autoclear_after_disconn.item, "AUTO CLEAR AFTER DISCONNECTION", POF_STRING_PAIR_MAX_LEN-1);
#ifdef POF_AUTOCLEAR
    strncpy(g_states.autoclear_after_disconn.cont, "ON", POF_STRING_PAIR_MAX_LEN-1);
#else
    strncpy(g_states.autoclear_after_disconn.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
#endif

    strncpy(g_states.log_file.item, "LOG FILE", POF_STRING_PAIR_MAX_LEN-1);
    strncpy(g_states.log_file.cont, "OFF", POF_STRING_PAIR_MAX_LEN-1);
}

void pof_states_print(){
    struct pof_str_pair *str_pair = NULL;
    int num = 0, i;
    
    num = sizeof(struct pof_state) / sizeof(struct pof_str_pair);
    for(i=0;i<num;i++){
        str_pair = (struct pof_str_pair *)&g_states + i;
        POF_DEBUG_CPRINT(1,CYAN,"  %-30s| %s\n",str_pair->item,str_pair->cont);
    }

    return;
}

/***********************************************************************
 * Initialize the config by file or command.
 * Form:     uint32_t pof_set_init_config(int argc, char *argv[])
 * Input:    number of arg, args
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function initialize the config of Soft Switch by "config"
 *           file or command arguments.
 ***********************************************************************/
uint32_t pof_set_init_config(int argc, char *argv[]){
	uint32_t ret;

    pof_set_init_states();

#ifdef POF_CONN_CONFIG_COMMAND
	ret = pof_set_init_config_by_command(argc, argv);
#endif
#ifdef POF_CONN_CONFIG_FILE
	ret = pof_set_init_config_by_file();
#endif // POF_CONN_CONFIG_FILE
    
    POF_DEBUG_CPRINT_FL(1,GREEN,"STATE:");
    pof_states_print();

	return POF_OK;
}

#undef POF_STRING_MAX_LEN
