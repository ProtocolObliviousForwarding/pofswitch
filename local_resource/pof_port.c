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
#include "../include/pof_datapath.h"
#include "../include/pof_byte_transfer.h"
#include "../include/pof_log_print.h"
#include "string.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "string.h"
#include "net/if.h"
#include "sys/ioctl.h"
#include "arpa/inet.h"
#include "ifaddrs.h"

/* The max number of local physical ports. */
uint32_t poflr_device_port_num_max = POFLR_DEVICE_PORT_NUM_MAX;

/* Ports. */
pof_port *poflr_port;

/* Port name. */
char **poflr_port_name;

/* Port IP address. */
char **poflr_port_ipaddr_str;

/* Port number. */
uint16_t poflr_port_num = 0;

static uint32_t poflr_get_port_num_name_by_systemfile(char **name, uint16_t *num);
static uint32_t poflr_get_port_num_name_by_getifaddrs(char **name, uint16_t *num);
static uint32_t poflr_get_hwaddr_index_by_name(const char *name, \
                                               unsigned char *hwaddr, \
                                               uint32_t *index);
static uint32_t poflr_get_ip_by_name(const char *name, char *ip_str);
static uint32_t poflr_check_port_link(const char *name);
static uint32_t poflr_check_port_up(const char *name);
static uint32_t poflr_set_port(const char *name, pof_port *p);

/* Detect all of ports, and report to Controller if any change. */
static uint32_t poflr_port_detect_main(char **name_old, \
		                               uint16_t *num_old, \
		                               pof_port *p_old, \
									   char **name_new, \
									   uint16_t *num_new,\
									   pof_port *p_new, \
									   uint8_t *flag);
static uint32_t poflr_port_detect_info(pof_port *p, char **name, uint16_t *num);
static uint32_t poflr_port_compare_info(pof_port *p_old, \
		                                pof_port *p_new, \
										uint16_t num_new, \
										uint16_t num_old, \
										uint8_t *flag);
static uint32_t poflr_port_compare_info_single(pof_port *p_old, pof_port *p_new, uint16_t num_old, uint8_t *flag);
static uint32_t poflr_port_two_port_is_one(pof_port *p_old, pof_port *p_new);
static uint32_t poflr_port_detect_same(pof_port *p_old, pof_port *p_new, uint8_t *flag);
static uint32_t poflr_port_detect_modify(pof_port *p_old, pof_port *p_new, uint8_t *flag);
static uint32_t poflr_port_detect_add(pof_port *p_old, pof_port *p_new, uint8_t *flag);
static uint32_t poflr_port_compare_check_old(pof_port *p_old, uint16_t num_old, uint8_t *flag);
static uint32_t poflr_port_detect_delete(pof_port *p_old);
static uint32_t poflr_port_report(uint8_t reason, pof_port *p);
static uint32_t poflr_port_update(pof_port *p_old, pof_port *p_new, uint16_t *num_old, uint16_t *num_new);

enum poflr_port_compare_flag{
	POFPCF_DEFAULT = 0x00,
	POFPCF_SAME   = 0x01,
	POFPCF_MODIFY = 0x02,
	POFPCF_ADD    = 0x03,
};

typedef struct poflr_port_task_id{
	char name[MAX_IF_NAME_LENGTH];
	task_t *tid;
} poflr_port_task_id;

poflr_port_task_id *port_task_id;

pof_port *poflr_port_new;

static void poflr_port_print(pof_port *p){
    uint32_t i;

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

	POF_DEBUG_CPRINT(1,WHITE,"\n");
}

static void poflr_port_print_all(pof_port *p, uint16_t num){
	uint32_t i;

	for(i=0; i<num; i++){
		POF_DEBUG_CPRINT(1,PINK,"\nPort[%u]", i);
		poflr_port_print(p+i);
	}
}

/***********************************************************************
 * Get the HWaddr and the index of the local physical ports.
 * Form:     uint32_t poflr_get_ip_by_name(const char *name, char *ip_str)
 * Input:    port name string
 * Output:   HWaddr, port index, ip address
 * Return:   POF_OK or ERROR code
 * Discribe: This function will get the HWaddr, the index and the ip 
 *           address of the local physical ports by the port name string.
 ***********************************************************************/
static uint32_t poflr_get_ip_by_name(const char *name, char *ip_str){
    struct sockaddr_in *sin;
    struct ifreq ifr; /* Interface request. */
    int    sock;

    if(-1 == (sock = socket(AF_INET, SOCK_STREAM, 0))){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE);
    }

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name, strlen(name)+1);

    /* Get IP address. */
	ioctl(sock, SIOCGIFADDR, &ifr);
	sin = (struct sockaddr_in *)&(ifr.ifr_addr);
	strcpy(ip_str, inet_ntoa(sin->sin_addr));

    close(sock);
    return POF_OK;
}

/***********************************************************************
 * Get the HWaddr and the index of the local physical ports.
 * Form:     uint32_t poflr_get_hwaddr_index_by_name(const char *name, \
 *                                                   unsigned char *hwaddr, \
 *                                                   uint32_t *index)
 * Input:    port name string
 * Output:   HWaddr, port index, ip address
 * Return:   POF_OK or ERROR code
 * Discribe: This function will get the HWaddr, the index and the ip 
 *           address of the local physical ports by the port name string.
 ***********************************************************************/
static uint32_t poflr_get_hwaddr_index_by_name(const char *name, \
                                               unsigned char *hwaddr, \
                                               uint32_t *index)
{
    struct ifreq ifr; /* Interface request. */
    int    sock;

    if(-1 == (sock = socket(AF_INET, SOCK_STREAM, 0))){
		perror("Socket ERROR:");
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE);
    }

    memset(hwaddr, 0, POF_ETH_ALEN);
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, name, strlen(name)+1);

    /* Get hardware address. */
    if(-1 == ioctl(sock, SIOCGIFHWADDR, &ifr)){
        close(sock);
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_GET_PORT_INFO_FAILURE);
    }else{
        memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, POF_ETH_ALEN);
    }

    /* Get port index. */
    if(-1 == ioctl(sock, SIOCGIFINDEX, &ifr)){
        close(sock);
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_GET_PORT_INFO_FAILURE);
    }else{
        *index = ifr.ifr_ifindex;
    }

    close(sock);
    return POF_OK;
}

/***********************************************************************
 * Get the number and the names of the local physical net ports.
 * Form:     static uint32_t poflr_get_port_num_name_by_systemfile(char *name\
                                                                   uint16_t num)
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function will get the number and the names of the local
 *           physical net ports. The number will be stored in poflr_port_num,
 *           and the names will be stored in poflr_port_name. 
 ***********************************************************************/
static uint32_t poflr_get_port_num_name_by_systemfile(char **name, uint16_t *num){
    uint16_t count = 0, s_len = 0;
	FILE *fp = NULL;
	char filename[] = "/proc/net/dev";
	char s_line[512] = "\0";

	if( (fp = fopen(filename, "r")) == NULL ){
		POF_ERROR_CPRINT_FL(1,RED, "Open /proc/net/dev failed.");
		exit(0);
	}

	if(fgets(s_line, sizeof(s_line), fp) == 0){
		POF_ERROR_CPRINT_FL(1,RED, "Open /proc/net/dev failed.");
        exit(0);
    }

	if(fgets(s_line, sizeof(s_line), fp) == 0){
		POF_ERROR_CPRINT_FL(1,RED, "Open /proc/net/dev failed.");
        exit(0);
    }

	while(fgets(s_line, sizeof(s_line), fp)){
		sscanf(s_line, "%*[^1-9a-zA-Z]%[^:]", name[count]);
		s_len = strlen(name[count]);
		if(s_len <= 0)
			continue;
		if(name[count][s_len-1] == ':')
			name[count][s_len-1] = 0;
		if(strcmp(name[count], "lo") == 0)
			continue;

		count ++;
		
		if(count >= poflr_device_port_num_max)
			break;
	}

	*num = count;

	fclose(fp);
    return POF_OK;
}

/* Thanks Sergio for reporting bug and offering the diff file.  */
static uint32_t 
poflr_get_port_num_name_by_getifaddrs(char **name, uint16_t *num)
{
    struct ifaddrs *addrs, *tmp;
    uint16_t count = 0;

    getifaddrs(&addrs);
    for(tmp=addrs; tmp!=NULL; tmp=tmp->ifa_next){
        if(tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_PACKET){
            if(strcmp(tmp->ifa_name, "lo")==0){
                continue;
            }
            strcpy(name[count], tmp->ifa_name);
            count++;
        }
    }
    *num = count;
    freeifaddrs(addrs);
    return POF_OK;
}

/***********************************************************************
 * Get the hardware address of the local physical net ports by IP address.
 * Form:     uint32_t poflr_get_hwaddr_by_ipaddr(uint8_t *hwaddr, 
 *                                               char *ipaddr_str, 
 *                                               uint8_t *port_id_ptr)
 * Input:    ipaddr_str
 * Output:   hwaddr
 * Return:   POF_OK or Error code
 * Discribe: This function will get the hardware address of the local
 *           physical net ports by IP address.
 ***********************************************************************/
uint32_t poflr_get_hwaddr_by_ipaddr(uint8_t *hwaddr, char *ipaddr_str, uint8_t *port_id_ptr){
    uint32_t i;

    /* Traverse all of the ports to find the one matching the 'ipaddr_str'. */
    for(i=0; i<poflr_port_num; i++){
        if(strcmp(ipaddr_str, poflr_port_ipaddr_str[i]) == POF_OK){
            memcpy(hwaddr, poflr_port[i].hw_addr, POF_ETH_ALEN);
			*port_id_ptr = poflr_port[i].port_id;
            return POF_OK;
        }
    }

    /* Error occurred and returned if there is no port matching the 'ipaddr_str'. */
    POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_GET_PORT_INFO_FAILURE);
}

/***********************************************************************
 * Initialize the local physical net port infomation.
 * Form:     uint32_t poflr_init_port()
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will initialize the local physical net port
 *           infomation, including port name, port index, port MAC address
 *           and so on.
 ***********************************************************************/
uint32_t poflr_init_port(){
    uint32_t i, j, ret = POF_OK;
    uint16_t ulPort;

    poflr_port_num = 0;

	port_task_id = (poflr_port_task_id *)malloc(poflr_device_port_num_max * sizeof(poflr_port_task_id));
	POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(port_task_id);
	memset(port_task_id, 0, poflr_device_port_num_max * sizeof(poflr_port_task_id));

	poflr_port_name = (char **)malloc(poflr_device_port_num_max * sizeof(ptr_t));
	POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(poflr_port_name);
	for(i=0; i<poflr_device_port_num_max; i++){
		poflr_port_name[i] = (char *)malloc(MAX_IF_NAME_LENGTH);
		if(NULL == poflr_port_name[i]){
			for(j=0; j<i; j++)
				free(poflr_port_name[j]);
			free(poflr_port_name);
			POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
		}
	}

    ret = poflr_get_port_num_name_by_getifaddrs(poflr_port_name, &poflr_port_num);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    poflr_port_ipaddr_str = (char **)malloc(poflr_port_num * sizeof(ptr_t));
	POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(poflr_port_ipaddr_str);

    poflr_port = (pof_port *)malloc(poflr_port_num * sizeof(pof_port));
	if(NULL == poflr_port){
		free(poflr_port_ipaddr_str);
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
	}
	memset(poflr_port, 0, poflr_port_num * sizeof(pof_port));

    /* Traverse all of the ports. */
    for (ulPort = 0; ulPort < poflr_port_num; ulPort++){

        /* Get port's IP address. */
        poflr_port_ipaddr_str[ulPort] = (char *)malloc(POF_IP_ADDRESS_STRING_LEN);
		if(NULL == poflr_port_ipaddr_str[ulPort]){
			for(i=0; i<ulPort; i++){
				free(poflr_port_ipaddr_str[ulPort]);
			}
			free(poflr_port_ipaddr_str);
			free(poflr_port);
			POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_ALLOCATE_RESOURCE_FAILURE);
		}

        ret = poflr_get_ip_by_name(poflr_port_name[ulPort], poflr_port_ipaddr_str[ulPort]);
		ret |= poflr_set_port(poflr_port_name[ulPort], &poflr_port[ulPort]);
		if(POF_OK != ret){
			POF_DEBUG_CPRINT_ERR();
			for(i=0; i<=ulPort; i++){
				free(poflr_port_ipaddr_str[ulPort]);
			}
			free(poflr_port_ipaddr_str);
			free(poflr_port);
			return ret;
		}
    }

    return POF_OK;
}

static uint32_t poflr_set_port(const char *name, pof_port *p){
	uint32_t ret = POF_OK;

	ret = poflr_get_hwaddr_index_by_name(name, p->hw_addr, &p->port_id);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	if( (POF_OK == poflr_check_port_up(name)) && \
			(POF_OK == poflr_check_port_link(name)) ){
		p->state = POFPS_LIVE;
	}
	else{
		p->state = POFPS_LINK_DOWN;
	}


	/* Fill the port's other infomation. */
	p->config = 0;
	p->curr = POFPF_10MB_HD | POFPF_10MB_FD;
	p->of_enable = POFLR_PORT_DISABLE;
	p->device_id = g_poflr_dev_id;
	p->supported = 0xffffffff;
	p->advertised = POFPF_10MB_FD | POFPF_100MB_FD;
	p->peer = POFPF_10MB_FD | POFPF_100MB_FD;
	strcpy((char *)p->name, name);

	return POF_OK;
}

/***********************************************************************
 * Modify the OpenFlow function enable flag of the port.
 * Form:     uint32_t poflr_port_openflow_enable(uint8_t port_id, uint8_t ulFlowEnableFlg)
 * Input:    port id, OpenFlow enable flag
 * Output:   NONE
 * Return:   POF_OK or ERROR code
 * Discribe: This function will modify the OpenFlow function flag of the
 *           port.
 ***********************************************************************/
uint32_t poflr_port_openflow_enable(uint8_t port_id, uint8_t ulFlowEnableFlg){
    pof_port *tmp_port;
    uint32_t ret = POF_ERROR, i;

    /* Find the port with 'port_id' to change the flag 'of_enable'. */
    for(i=0; i<poflr_port_num; i++){
        tmp_port = &poflr_port[i];
        if(tmp_port->port_id == port_id){
            tmp_port->of_enable = ulFlowEnableFlg;
            ret = POF_OK;
        }
    }

    /* Error occurred and returned if there is no port with 'port_id'. */
    if(ret == POF_ERROR){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_PORT_MOD_FAILED, POFPMFC_BAD_PORT_ID, g_recv_xid);
    }

    POF_DEBUG_CPRINT_FL(1,GREEN,"Port openflow enable MOD SUC!");
    return POF_OK;
}

/* Get ports. */
uint32_t poflr_get_port(pof_port **port_ptrptr){
	*port_ptrptr = poflr_port;
	return POF_OK;
}

/* Get port number. */
uint32_t poflr_get_port_number(uint16_t *port_number_ptr){
	*port_number_ptr = poflr_port_num;
	return POF_OK;
}

/* Get port number max. */
uint32_t poflr_get_port_number_max(uint16_t *port_number_ptr){
	*port_number_ptr = poflr_device_port_num_max;
	return POF_OK;
}

/* Set the max number of ports. */
uint32_t poflr_set_port_number_max(uint32_t port_num_max){
	poflr_device_port_num_max = port_num_max;
	return POF_OK;
}

/* Disable all port OpenFlow forwarding. */
uint32_t poflr_disable_all_port(){
	uint32_t i;

    for(i = 0; i < poflr_port_num; i++){
        poflr_port[i].of_enable = POFLR_PORT_DISABLE;
    }

	return POF_OK;
}

static uint32_t poflr_check_port_link(const char *name){
	struct ifreq ifr;
	int sock_fd;

    if(-1 == (sock_fd = socket(AF_INET, SOCK_DGRAM, 0))){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE);
    }

	strcpy(ifr.ifr_name, name);

	if(ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0){
		close(sock_fd);
		return POF_ERROR;
	}

	if((ifr.ifr_flags & IFF_RUNNING)){
		close(sock_fd);
		return POF_OK;
	}
	else{
		close(sock_fd);
		return POF_ERROR;
	}
}

static uint32_t poflr_check_port_up(const char *name){
	struct ifreq ifr;
	int sock_fd;

    if(-1 == (sock_fd = socket(AF_INET, SOCK_DGRAM, 0))){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE);
    }

	strcpy(ifr.ifr_name, name);

	if(ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0){
		close(sock_fd);
		return POF_ERROR;
	}

	if(ifr.ifr_flags & IFF_UP){
		close(sock_fd);
		return POF_OK;
	}
	else{
		close(sock_fd);
		return POF_ERROR;
	}
}

/* Detect all of ports, and report to Controller if any change. */
uint32_t poflr_port_detect_task(){
	uint32_t ret = POF_OK, i;
	char **name_new;
	uint16_t num_new = 0;
	uint8_t *flag;
	/* TODO */

	flag = (uint8_t *)malloc(poflr_device_port_num_max * sizeof(uint8_t));
	if(NULL == flag){
		terminate_handler();
	}

	poflr_port_new = (pof_port *)malloc(poflr_device_port_num_max * sizeof(pof_port));
	if((NULL == poflr_port_new)){
		terminate_handler();
	}

	name_new = (char **)malloc(poflr_device_port_num_max * sizeof(ptr_t));
	if(NULL == name_new){
		terminate_handler();
	}

	for(i=0; i<poflr_device_port_num_max; i++){
		name_new[i] = (char *)malloc(MAX_IF_NAME_LENGTH);
		if(NULL == name_new[i]){
			terminate_handler;
		}
	}

	while(1){
		sleep(5);
		memset(poflr_port_new, 0, poflr_device_port_num_max * sizeof(pof_port));
		memset(flag, 0, poflr_device_port_num_max * sizeof(uint8_t));
		for(i=0; i<poflr_device_port_num_max; i++){
			memset(name_new[i], 0, MAX_IF_NAME_LENGTH);
		}
		num_new = 0;

		ret = poflr_port_detect_main(poflr_port_name, &poflr_port_num, poflr_port, \
				                     name_new, &num_new, poflr_port_new, flag);
		POF_CHECK_RETVALUE_TERMINATE(ret);

		ret = poflr_port_update(poflr_port, poflr_port_new, &poflr_port_num, &num_new);
	}
}

static uint32_t poflr_port_detect_main(char **name_old, \
		                               uint16_t *num_old, \
		                               pof_port *p_old, \
									   char **name_new, \
									   uint16_t *num_new, \
									   pof_port *p_new, \
									   uint8_t *flag){
	uint32_t ret = POF_OK;

	/* Detect all of ports infomation right now. */
	ret = poflr_port_detect_info(p_new, name_new, num_new);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	/* Compare the ports infomation to  */
	ret = poflr_port_compare_info(p_old, p_new, *num_new, *num_old, flag);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	ret = poflr_port_compare_check_old(p_old, *num_old, flag);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	return POF_OK;
}

static uint32_t poflr_port_detect_info(pof_port *p, char **name, uint16_t *num){
	uint32_t ret = POF_OK, i;

	ret = poflr_get_port_num_name_by_getifaddrs(name, num);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	for(i=0; i<*num; i++){
		poflr_set_port(name[i], p+i);
	}
	
	return POF_OK;
}

static uint32_t poflr_port_compare_info(pof_port *p_old, \
		                                pof_port *p_new, \
										uint16_t num_new, \
										uint16_t num_old, \
										uint8_t *flag)
{
	uint32_t ret = POF_OK, i, j;

	for(i=0; i<num_new; i++){
		/* Check the port in the old ports. */
		ret = poflr_port_compare_info_single(p_old, p_new+i, num_old, flag);
		if(POF_OK == ret){
			/* The port is already have. */
			continue;
		}
		/* ret != POF_OK means the port is a new port, need to report to Controller. */
		ret = poflr_port_detect_add(p_old, p_new+i, flag);
		POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
	}


	return POF_OK;
}

static uint32_t poflr_port_compare_info_single(pof_port *p_old, pof_port *p_new, uint16_t num_old, uint8_t *flag){
	uint32_t ret = POF_OK, i;
	pof_port *p;
	
	for(i=0; i<num_old; i++){
		p = (pof_port *)(p_old + i);
		if(POF_OK != poflr_port_two_port_is_one(p, p_new)){
			continue;
		}
		p_new->of_enable = p->of_enable;
		if(POF_OK == (ret = memcmp(p, p_new, sizeof(pof_port)))){
			ret = poflr_port_detect_same(p, p_new, flag + i);
		}else{
			ret = poflr_port_detect_modify(p, p_new, flag + i);
		}
		POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
		return POF_OK;
	}

	return POF_ERROR;
}

static uint32_t poflr_port_two_port_is_one(pof_port *p_old, pof_port *p_new){
	uint32_t ret;

	/* Two ports is one, if they have same name. return POF_OK */
	ret = strncmp(p_old->name, p_new->name, MAX_IF_NAME_LENGTH);
	if(POF_OK == ret){
		return POF_OK;
	}
	return POF_ERROR;
}

static uint32_t poflr_port_detect_same(pof_port *p_old, pof_port *p_new, uint8_t *flag){
	*flag = POFPCF_SAME;
	return POF_OK;
}

static uint32_t poflr_port_detect_modify(pof_port *p_old, pof_port *p_new, uint8_t *flag){
	uint32_t ret = POF_OK;

	ret = poflr_port_report(POFPR_MODIFY, p_new);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	*flag = POFPCF_MODIFY;
	return POF_OK;
}

static uint32_t poflr_port_detect_add(pof_port *p_old, pof_port *p_new, uint8_t *flag){
	uint32_t ret = POF_OK, i;
	task_t *tid = NULL;

	ret = poflr_port_report(POFPR_ADD, p_new);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

#ifdef POF_DATAPATH_ON
	for(i=0; i<poflr_device_port_num_max; i++){
		if(*(g_pofdp_recv_raw_task_id_ptr+i) != POF_INVALID_TASKID)
			continue;
		tid = g_pofdp_recv_raw_task_id_ptr + i;
		break;
	}

	if(NULL == tid){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_CREATE_FAIL);
	}

	ret = pofdp_create_port_listen_task(tid, p_new);
	if(POF_OK != ret){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_CREATE_FAIL);
	}
#endif // POF_DATAPATH_ON

	return POF_OK;
}

static uint32_t poflr_port_compare_check_old(pof_port *p_old, uint16_t num_old, uint8_t *flag){
	uint32_t ret = POF_OK, i;

	for(i=0; i<num_old; i++){
		if(*(flag+i) != POFPCF_DEFAULT){
			continue;
		}
		ret = poflr_port_detect_delete(p_old + i);
		POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
	}

	return POF_OK;
}

static uint32_t poflr_port_detect_delete(pof_port *p_old){
	uint32_t ret = POF_OK;
	task_t *tid = NULL;

	ret = poflr_port_report(POFPR_DELETE, p_old);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

#ifdef POF_DATAPATH_ON
	ret = poflr_del_port_task_id(&tid, p_old);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	ret = pofbf_task_delete(tid);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
#endif // POF_DATAPATH_ON

	return POF_OK;
}

static uint32_t poflr_port_report(uint8_t reason, pof_port *p){
    pof_port_status port_status = {0};

	port_status.reason = reason;
	memcpy(&port_status.desc, (uint8_t *)p, sizeof(pof_port));
	pof_HtoN_transfer_port_status(&port_status);

	if(POF_OK != pofec_reply_msg(POFT_PORT_STATUS, g_upward_xid, sizeof(pof_port_status), (uint8_t *)&port_status)){
		POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
	}

    return POF_OK;
}

static uint32_t poflr_port_update(pof_port *p_old, pof_port *p_new, uint16_t *num_old, uint16_t *num_new){
	uint32_t i;

	for(i=0; i<*num_new; i++){
		memcpy(p_old+i, p_new+i, sizeof(pof_port));
	}
	*num_old = *num_new;

	return POF_OK;
}

uint32_t poflr_set_port_task_id(task_t *tid, pof_port *p){
	poflr_port_task_id *ptask = NULL;
	uint32_t i;

	for(i=0; i<poflr_device_port_num_max; i++){
		if(port_task_id[i].tid != NULL){
			continue;
		}
		
		ptask = port_task_id + i;
		break;
	}

	if(ptask == NULL){
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_TASK_CREATE_FAIL);
	}

	strcpy(ptask->name, p->name);
	ptask->tid = tid;

	return POF_OK;
}

uint32_t poflr_del_port_task_id(task_t **tid, pof_port *p){
	uint32_t i;
	poflr_port_task_id *ptask = NULL;
	task_t *tid_t = NULL;

	for(i=0; i<poflr_device_port_num_max; i++){

		ptask = (poflr_port_task_id *)(port_task_id + i);

		if(POF_OK != strcmp(ptask->name, p->name)){
			continue;
		}

		tid_t = ptask->tid;
		memset(ptask, 0, sizeof(poflr_port_task_id));
	}

	if(tid_t == NULL){
		/* There is no thread id matched to the port name. */
        POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_PORT_DELETE_FAIL);
	}

	*tid = tid_t;

	return POF_OK;
}

uint32_t
poflr_check_port_index(uint32_t id)
{
	uint32_t i;

	for(i=0; i<poflr_port_num; i++){
		if(id == poflr_port[i].port_id){
			return POF_OK;
		}
	}

	POF_DEBUG_CPRINT_FL(1,RED,"Output port with index %u is invalid.", id);
	POF_ERROR_HANDLE_RETURN_UPWARD(POFET_BAD_ACTION, POFBAC_BAD_OUT_PORT, g_upward_xid++);
}
