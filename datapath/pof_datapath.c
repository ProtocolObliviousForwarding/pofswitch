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
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#ifdef POF_DATAPATH_ON

/* Task id. */
task_t g_pofdp_main_task_id = 0;
task_t *g_pofdp_recv_raw_task_id_ptr;
task_t g_pofdp_send_raw_task_id = 0;
task_t g_pofdp_detect_port_task_id = 0;

/* Queue id. */
uint32_t g_pofdp_recv_q_id = POF_INVALID_QUEUEID;
uint32_t g_pofdp_send_q_id = POF_INVALID_QUEUEID;

static uint32_t pofdp_main_task(void *arg_ptr);
static uint32_t pofdp_forward(struct pofdp_packet *dp_packet, struct pof_instruction *first_ins);
static uint32_t pofdp_recv_raw_task(void *arg_ptr);
static uint32_t pofdp_send_raw_task(void *arg_ptr);

/* Malloc memery in struct pofdp_packet to store packet data. 
 * The memery should be free by free_packet_data(). */
static uint32_t malloc_packet_data(struct pofdp_packet *dpp, uint32_t len){
	dpp->buf = malloc(POFDP_PACKET_RAW_MAX_LEN);
    POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(dpp->buf, g_upward_xid++);

	memset(dpp->buf, 0, len);
	dpp->ori_len = len;

	dpp->left_len = dpp->ori_len;
	dpp->buf_offset = dpp->buf;

	return POF_OK;
}

/* Free memery in struct pofdp_packet which store packet data.
 * The memery is malloced by malloc_packet_data(). */
static void free_packet_data(struct pofdp_packet *dpp){
	if(dpp!=NULL && dpp->buf!=NULL){
		free(dpp->buf);
		dpp->buf = NULL;
		dpp->ori_len = 0;
	}
	return;
}

static uint32_t 
init_packet_metadata(struct pofdp_packet *dpp, struct pofdp_metadata *metadata, size_t len)
{
	if(len < sizeof *metadata){
		POF_ERROR_HANDLE_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_METADATA_LEN_ERROR);
	}

	memset(metadata, 0, len);
	metadata->len = POF_HTONS(dpp->ori_len);
	metadata->port_id = dpp->ori_port_id;

	dpp->metadata_len = len;
	dpp->metadata = metadata;

	return POF_OK;
}

/***********************************************************************
 * Initial the datapath module
 * Form:     pof_datapath_init()
 * Input:    NONE
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function initial the datapath module, creating the send
 *           and receive queues, and creating the necessary tasks. The tasks
 *           include datapath task, send task, and receive tasks which is
 *           corresponding to the local physical ports.
 ***********************************************************************/
uint32_t pof_datapath_init(){
    pof_port *port_ptr = NULL;
    uint32_t i, ret;
    uint16_t port_number = 0, port_number_max = 0;

    /* Create message queues to store send or receive message. */
    ret = pofbf_queue_create(&g_pofdp_recv_q_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    ret = pofbf_queue_create(&g_pofdp_send_q_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    /* Create datapath task. */
    ret = pofbf_task_create(NULL, (void *)pofdp_main_task, &g_pofdp_main_task_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    POF_DEBUG_CPRINT_FL(1,BLUE,"Start datapatch task!");

    /* Create task to send raw packet. */
    ret = pofbf_task_create(NULL, (void *)pofdp_send_raw_task, &g_pofdp_send_raw_task_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    POF_DEBUG_CPRINT_FL(1,BLUE,"Start send_raw task!");

    /* Create task to receive raw packet. */
    poflr_get_port_number(&port_number);
    poflr_get_port_number_max(&port_number_max);
    poflr_get_port(&port_ptr);
    g_pofdp_recv_raw_task_id_ptr = (task_t *)malloc(port_number_max * sizeof(task_t));
    POF_MALLOC_ERROR_HANDLE_RETURN_NO_UPWARD(g_pofdp_recv_raw_task_id_ptr);
    for(i=0; i<port_number; i++){
		ret = pofdp_create_port_listen_task(g_pofdp_recv_raw_task_id_ptr + i, port_ptr + i);
        if(POF_OK != ret){
            POF_DEBUG_CPRINT_ERR();
            free(g_pofdp_recv_raw_task_id_ptr);
            return ret;
        }
    }

    /* Create task to detect the ports. */
    ret = pofbf_task_create(NULL, (void *)poflr_port_detect_task, &g_pofdp_detect_port_task_id);
    POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

    return POF_OK;
}

uint32_t pofdp_create_port_listen_task(task_t *tid, pof_port *p){
	uint32_t ret = POF_OK;

	ret = pofbf_task_create(p, (void *)pofdp_recv_raw_task, tid);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    POF_DEBUG_CPRINT_FL(1,BLUE,"Port %s: Start recv_raw task!", p->name);

	ret = poflr_set_port_task_id(tid, p);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	return POF_OK;
}

static void set_goto_first_table_instruction(struct pof_instruction *p)
{
	struct pof_instruction_goto_table *pigt = \
			(struct pof_instruction_goto_table *)p->instruction_data;

	p->type = POFIT_GOTO_TABLE;
	p->len = sizeof(pigt);
	pigt->next_table_id = POFDP_FIRST_TABLE_ID;
	return;
}

/***********************************************************************
 * Receive RAW packet function
 * Form:     uint32_t pofdp_recv_raw(uint8_t *buf, uint32_t *plen, uint32_t *port_id_ptr)
 * Input:    NONE
 * Output:   receive data, length of receive data
 * Return:   POF_OK or Error code
 * Discribe: This function read the receive queue to get the RAW packet
 *           data. Then copy to the buf.
 ***********************************************************************/
static uint32_t pofdp_recv_raw(struct pofdp_packet *dpp){
    uint32_t queue_id = g_pofdp_recv_q_id;

    /* Read the packet from the send queue. */
    if(pofbf_queue_read(queue_id, (uint8_t *)dpp, sizeof *dpp, POF_WAIT_FOREVER) != POF_OK){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_READ_MSG_QUEUE_FAILURE, g_upward_xid++);
    }

    return POF_OK;
}

/***********************************************************************
 * The task function of the datapath task
 * Form:     static void pofdp_main_task(void *arg_ptr)
 * Input:    NONE
 * Output:   NONE
 * Return:   VOID
 * Discribe: This is the main function of the datapath task, which is
 *           infinite loop running. It receive the RAW packet, and forward.
 *           The next RAW packet will be not proceeded until the forward
 *           process of the last packet is over.
 * NOTE:     The datapath task will be terminated if some ERROR occurs
 *           during the receive process and the datapath process.
 ***********************************************************************/
static uint32_t pofdp_main_task(void *arg_ptr){
    uint8_t recv_buf[POFDP_PACKET_RAW_MAX_LEN] = {0};
	struct pofdp_packet dpp[1] = {0};
	struct pof_instruction first_ins[1] = {0};
    uint32_t len_B = 0, port_id = 0;
    uint32_t ret;

	/* Set GOTO_TABLE instruction to go to the first flow table. */
	set_goto_first_table_instruction(first_ins);

    while(1){
        /* Receive raw packet through local physical OpenFlow-enabled ports. */
        ret = pofdp_recv_raw(dpp);
        if(ret != POF_OK){
            POF_CHECK_RETVALUE_NO_RETURN_NO_UPWARD(ret);
            /* Delay 0.1s to send error message upward to the Controller. */
            pofbf_task_delay(100);
            terminate_handler();
        }

        /* Check the packet length. */
        if(dpp->ori_len > POFDP_PACKET_RAW_MAX_LEN){
			free_packet_data(dpp);
            POF_ERROR_HANDLE_NO_RETURN_NO_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR);
            continue;
        }

		/* Check whether the first flow table exist. */
		if(POF_OK != poflr_check_flow_table_exist(POFDP_FIRST_TABLE_ID)){
			POF_DEBUG_CPRINT_FL(1,RED,"Received a packet, but the first flow table does NOT exist.");
			free_packet_data(dpp);
			continue;
		}

        /* Forward the packet. */
        ret = pofdp_forward(dpp, first_ins);
        POF_CHECK_RETVALUE_NO_RETURN_NO_UPWARD(ret);

		free_packet_data(dpp);
        POF_DEBUG_CPRINT_FL(1,GREEN,"one packet_raw has been processed!\n");
    }
    return POF_OK;
}

/***********************************************************************
 * Forward function
 * Form:     static uint32_t pofdp_forward(uint8_t *packet, \
 *                                         uint32_t len, \
 *                                         uint32_t port_id)
 * Input:    packet, length of packet
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function forwards the packet between the flow tables.
 *           The new packet will be send into the MM0 table, which is
 *           head flow table. Then according to the matched flow entry,
 *           the packet will be forwarded between the other flow tables
 *           or execute the instruction and action corresponding to the
 *           matched flow entry.
 * NOTE:     This function will be over in these situations: 1, All of
 *           the instruction has been executed. 2, The packet has been
 *           droped, send upward to the Controller, or output through
 *           the specified local physical port. 3, Any ERROR has occurred
 *           during the process.
 ***********************************************************************/
static uint32_t pofdp_forward(struct pofdp_packet *dpp, struct pof_instruction *first_ins)
{
	uint8_t metadata[POFDP_METADATA_MAX_LEN] = {0};
	uint32_t ret;

	POF_DEBUG_CPRINT(1,BLUE,"\n");
	POF_DEBUG_CPRINT_FL(1,BLUE,"Receive a raw packet! len_B = %d, port id = %u", \
			dpp->ori_len, dpp->ori_port_id);
	POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf,dpp->left_len,"Input packet data is ");

	/* Initialize the metadata. */
	ret = init_packet_metadata(dpp, (struct pofdp_metadata *)metadata, sizeof(metadata));
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);

	/* Set the first instruction to the Datapath packet. */
	dpp->ins = first_ins;
	dpp->ins_todo_num = 1;

	ret = pofdp_instruction_execute(dpp);
	POF_CHECK_RETVALUE_RETURN_NO_UPWARD(ret);
    return POF_OK;
}

/***********************************************************************
 * The task function of receive task
 * Form:     static void pofdp_recv_raw_task(void *arg_ptr)
 * Input:    port infomation
 * Output:   NONE
 * Return:   VOID
 * Discribe: This is the task function of receive task, which is infinite
 *           loop running. It receives RAW packet by binding socket to the
 *           local physical net port spicified in the port infomation.
 *           After filtering, the packet data and port infomation will be
 *           assembled with format of struct pofdp_packet, and be send
 *           into the receive queue. The only parameter arg_ptr is the
 *           pointer of the local physical net port infomation which has
 *           been assembled with format of struct pof_port.
 * NOTE:     This task will be terminated if any ERRORs occur.
 *           If the openflow function of this physical port is disable,
 *           it will be still loop running but nothing will be received.
 ***********************************************************************/
static uint32_t pofdp_recv_raw_task(void *arg_ptr){
    pof_port *port_ptr = (pof_port *)arg_ptr;
    struct pofdp_packet *dpp = malloc(sizeof *dpp);
    struct   sockaddr_ll sockadr, from;
    uint32_t from_len, len_B;
    uint8_t  buf[POFDP_PACKET_RAW_MAX_LEN];
    int      sock;

    from_len = sizeof(struct sockaddr_ll);

    /* Create socket, and bind it to the specific port. */
    if((sock = socket(AF_PACKET, SOCK_RAW, POF_HTONS(ETH_P_ALL))) == -1){
        POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE, g_upward_xid++);
        /* Delay 0.1s to send error message upward to the Controller. */
        pofbf_task_delay(100);
        terminate_handler();
    }

    sockadr.sll_family = AF_PACKET;
    sockadr.sll_protocol = POF_HTONS(ETH_P_ALL);
    sockadr.sll_ifindex = port_ptr->port_id;

    if(bind(sock, (struct sockaddr *)&sockadr, sizeof(struct sockaddr_ll)) != 0){
       POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_BIND_SOCKET_FAILURE, g_upward_xid++);
        /* Delay 0.1s to send error message upward to the Controller. */
        pofbf_task_delay(100);
        terminate_handler();
    }

    /* Receive the raw packet through the specific port. */
    while(1){
		pthread_testcancel();

        /* Receive the raw packet. */
        if((len_B = recvfrom(sock, buf, POFDP_PACKET_RAW_MAX_LEN, 0, (struct sockaddr *)&from, &from_len)) <=0){
            POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_RECEIVE_MSG_FAILURE, g_upward_xid++);
            continue;
        }

        /* Check whether the OpenFlow-enabled of the port is on or not. */
        if(port_ptr->of_enable == POFLR_PORT_DISABLE || from.sll_pkttype == PACKET_OUTGOING){
            continue;
        }

        /* Check the packet length. */
        if(len_B > POF_MTU_LENGTH){
            POF_DEBUG_CPRINT_FL(1,RED,"The packet received is longer than MTU. DROP!");
            continue;
        }

        /* Filter the received raw packet by some rules. */
        if(dp.filter(buf, port_ptr, from) != POF_OK){
            continue;
        }
		
        /* Store packet data, length, received port infomation into the message queue. */
		memset(dpp, 0, sizeof *dpp);
        dpp->ori_port_id = port_ptr->port_id;
		malloc_packet_data(dpp, len_B);
        memcpy(dpp->buf, buf, len_B);

        if(pofbf_queue_write(g_pofdp_recv_q_id, dpp, sizeof *dpp, POF_WAIT_FOREVER) != POF_OK){
            POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_upward_xid++);
			free_packet_data(dpp);

            pofbf_task_delay(100);
            terminate_handler();
        }
    }

    close(sock);
    return POF_OK;
}

/***********************************************************************
 * The task function of send task
 * Form:     static void pofdp_send_raw_task(void *arg)
 * Input:    NONE
 * Output:   NONE
 * Return:   VOID
 * Discribe: This is the task function of send task, which is infinite
 *           loop running. It reads the send queue to get the packet data
 *           and the sending port infomation. Then it sends the packet
 *           out by binding the socket to the local physical net port
 *           spicified in the port infomation.
 * NOTE:     This task will be terminated if any ERRORs occur.
 ***********************************************************************/
static uint32_t pofdp_send_raw_task(void *arg){
    struct pofdp_packet *dpp = malloc(sizeof *dpp);
    struct   sockaddr_ll sll;
    int      sock;

    /* Create socket. */
    if((sock = socket(PF_PACKET, SOCK_RAW, POF_HTONS(ETH_P_ALL))) == -1){
        POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_CREATE_SOCKET_FAILURE, g_upward_xid++);

        /* Delay 0.1s to send error message upward to the Controller. */
        pofbf_task_delay(100);
        terminate_handler();
    }
    memset(&sll, 0, sizeof(struct sockaddr_ll));

    while(1){

        /* Read the message from the queue. */
        if(pofbf_queue_read(g_pofdp_send_q_id, dpp, sizeof *dpp, POF_WAIT_FOREVER) != POF_OK){
            POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_READ_MSG_QUEUE_FAILURE, g_upward_xid++);

            /* Delay 0.1s to send error message upward to the Controller. */
            pofbf_task_delay(100);
            terminate_handler();
        }

        /* Send the packet data out through the port. */
        memset(&sll, 0, sizeof sll);
        sll.sll_family = AF_PACKET;
        sll.sll_ifindex = dpp->output_port_id;
        sll.sll_protocol = POF_HTONS(ETH_P_ALL);

        if(sendto(sock, dpp->buf_out, dpp->output_whole_len, 0, (struct sockaddr *)&sll, sizeof(sll)) == -1){
			free(dpp->buf_out);
            POF_ERROR_HANDLE_NO_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_SEND_MSG_FAILURE, g_upward_xid++);

            /* Delay 0.1s to send error message upward to the Controller. */
            pofbf_task_delay(100);
            terminate_handler();
        }
		free(dpp->buf_out);
    }

    close(sock);
    return POF_OK;
}

/***********************************************************************
 * Send packet out function
 * Form:     uint32_t pofdp_send_raw(uint8_t *buf, uint32_t len, uint32_t port_id)
 * Input:    packet data, packet length, output port id
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function send the packet data out through the port
 *           corresponding the port_id. The length of packet data is len.
 *           It assembles the packet data, the packet length
 *           and the output port id with format of struct pofdp_packet,
 *           and write it to the send queue. Caller should make sure that
 *           output_packet_offset plus output_packet_len is less than the
 *           whole packet_len, and that output_metadata_offset plus 
 *           output_metadata_len is less than the whole metadata_len.
 ***********************************************************************/
uint32_t pofdp_send_raw(struct pofdp_packet *dpp){
	uint8_t *data = malloc(dpp->output_whole_len);

	/* Malloc the output data memery which will be freed in 
	 * pofdp_send_raw_task. */
	data = malloc(dpp->output_whole_len);
    POF_MALLOC_ERROR_HANDLE_RETURN_UPWARD(data, g_upward_xid++);
    memset(data, 0, dpp->output_whole_len);

	/* Copy metadata to output buffer. */
    pofdp_copy_bit((uint8_t *)dpp->metadata, data, dpp->output_metadata_offset, \
			dpp->output_metadata_len * POF_BITNUM_IN_BYTE);
	/* Copy packet to output buffer right behind metadata. */
    memcpy(data + dpp->output_metadata_len, dpp->buf + dpp->output_packet_offset, dpp->output_packet_len);
	dpp->buf_out = data;

    POF_DEBUG_CPRINT_FL(1,GREEN,"One packet is about to be sent out! port_id = %d, packet_len = %u, metadata_len = %u, total_len = %u", \
			            dpp->output_port_id, dpp->output_packet_len, dpp->output_metadata_len, dpp->output_whole_len);
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf + dpp->output_packet_offset, dpp->output_packet_len, \
			"The packet is ");
    POF_DEBUG_CPRINT_FL_0X(1,GREEN,dpp->buf_out,dpp->output_metadata_len,"The metatada is ");
    POF_DEBUG_CPRINT_FL_0X(1,YELLOW,dpp->buf_out, dpp->output_whole_len,"The whole output packet is ");

    /* Check the packet lenght. */
    if(dpp->output_whole_len > POF_MTU_LENGTH){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    /* Write the packet to the send queue. */
    if(pofbf_queue_write(g_pofdp_send_q_id, dpp, sizeof *dpp, POF_WAIT_FOREVER) != POF_OK){
		free(data);
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_upward_xid++);
    }

    return POF_OK;
}

/***********************************************************************
 * Send packet upward to the Controller
 * Form:     uint32_t pofdp_send_packet_in_to_controller(uint16_t len, \
 *                                                       uint8_t reason, \
 *                                                       uint8_t table_id, \
 *                                                       uint64_t cookie, \
 *                                                       uint32_t device_id, \
 *                                                       uint8_t *packet)
 * Input:    packet length, upward reason, current table id, cookie,
 *           device id, packet data
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function send the packet data upward to the controller.
 *           It assembles the packet data, length, reason, table id,
 *           cookie and device id with format of struct pof_packet_in,
 *           and encapsulate to a new openflow packet. Then the new packet
 *           will be send to the mpu module in order to send upward to the
 *           Controller.
 ***********************************************************************/
uint32_t pofdp_send_packet_in_to_controller(uint16_t len, \
                                            uint8_t reason, \
                                            uint8_t table_id, \
											struct pof_flow_entry *pfe, \
                                            uint32_t device_id, \
                                            uint8_t *packet)
{
    pof_packet_in packetin = {0};
    uint32_t      packet_in_len;

    /* The length of the packet in data upward to the Controller is the real length
     * instead of the max length of the packet_in. */
    packet_in_len = sizeof(pof_packet_in) - POF_PACKET_IN_MAX_LENGTH + len;

    /* Check the packet length. */
    if(len > POF_PACKET_IN_MAX_LENGTH){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_PACKET_LEN_ERROR, g_upward_xid++);
    }

    packetin.buffer_id = 0xffffffff;
    packetin.total_len = len;
    packetin.reason = reason;
    packetin.table_id = table_id;
	if(NULL != pfe){
		packetin.cookie = pfe->cookie & pfe->cookie_mask;
	}else{
		packetin.cookie = 0;
	}
    packetin.device_id = device_id;
    memcpy(packetin.data, packet, len);
    pof_HtoN_transfer_packet_in(&packetin);

    if(POF_OK != pofec_reply_msg(POFT_PACKET_IN, g_upward_xid++, packet_in_len, (uint8_t *)&packetin)){
        POF_ERROR_HANDLE_RETURN_UPWARD(POFET_SOFTWARE_FAILED, POF_WRITE_MSG_QUEUE_FAILURE, g_recv_xid);
    }

    return POF_OK;
}

static uint32_t pofdp_promisc(uint8_t *packet, pof_port *port_ptr, struct sockaddr_ll sll){
    uint32_t *daddr, ret = POF_OK;
    uint16_t *ether_type;
    uint8_t  *ip_protocol, *eth_daddr;

#ifdef POF_RECVRAW_ANYPACKET
	return POF_OK;
#endif // POF_RECVRAW_ANYPACKET

#ifdef POF_RECVRAW_TESTPACKET
    /* Any packet which source MAC address is equal to the match_buf
     * will be received. */
    uint8_t match_buf[POF_ETH_ALEN] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06
    };
    if(memcmp((uint8_t *)packet + POF_ETH_ALEN, (uint8_t *)match_buf, POF_ETH_ALEN) == 0){
        return POF_OK;
    }else{
		return POF_ERROR;
	}
#endif // POF_TEST_ON

#ifdef POF_RECVRAW_ETHTYPE_IP
    /* The ether type have to be IP. */
    ether_type = (uint16_t *)(packet + 2 * POF_ETH_ALEN);
    if(POF_NTOHS(*ether_type) != ETHERTYPE_IP){
//    if(POF_NTOHS(*ether_type) != 0x0888){
        return POF_ERROR;
    }
#endif // POF_RECVRAW_ETHTYPE_IP

#ifdef POF_RECVRAW_IPPROTO_ICMP
    /* The IP protocol have to be ICMP. */
    ip_protocol = packet + (2*POF_ETH_ALEN+2) + (4*2+1);
    if(*ip_protocol != IPPROTO_ICMP){
        return POF_ERROR;
    }
#endif // POF_RECVRAW_IPPROTO_ICMP

//    return POF_OK;
    return POF_OK;
}

/***********************************************************************
 * NONE promisc mode packet filter
 * Form:     static uint32_t pofdp_no_promisc(uint8_t *packet, pof_port *port_ptr, struct sockaddr *sll)
 * Input:    packet data, port infomation
 * Output:   NONE
 * Return:   POF_OK or Error code
 * Discribe: This function filter the RAW packet received by the local
 *           physical net port.
 ***********************************************************************/
static uint32_t pofdp_no_promisc(uint8_t *packet, pof_port *port_ptr, struct sockaddr_ll sll){
    uint32_t *daddr, ret = POF_OK;
    uint16_t *ether_type;
    uint8_t  *ip_protocol, *eth_daddr;
    uint8_t broadcast[POF_ETH_ALEN] = {
        0xff,0xff,0xff,0xff,0xff,0xff
    };

#ifdef POF_RECVRAW_DHWADDR_LOCAL
    eth_daddr = (uint8_t *)packet;
    if(memcmp(eth_daddr, port_ptr->hw_addr, POF_ETH_ALEN) != 0 && \
            memcmp(eth_daddr, broadcast, POF_ETH_ALEN) != 0){
		return POF_ERROR;
    }
#endif // POF_RECVRAW_DHWADDR_LOCAL

    if(sll.sll_pkttype == PACKET_OTHERHOST){
        return POF_ERROR;
    }

    return POF_OK;
}

/* Global datapath structure. */
struct pof_datapath dp = {
    pofdp_no_promisc,
    pofdp_promisc,
#ifdef POF_PROMISC_ON
    pofdp_promisc,
#else // POF_PROMISC_ON
    pofdp_no_promisc,
#endif // POF_PROMISC_ON
};


#endif // POF_DATAPATH_ON
