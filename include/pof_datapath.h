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

#ifndef _POF_DATAPATH_H_
#define _POF_DATAPATH_H_

#include <linux/if_packet.h>
#include "pof_global.h"
#include "pof_local_resource.h"
#include "pof_common.h"

/* Define bit number in a byte. */
#define POF_BITNUM_IN_BYTE                  (8)

/* Transfer the length from bit unit to byte unit. Take the ceil. */
#define POF_BITNUM_TO_BYTENUM_CEIL(len_b)   ((uint32_t)(((len_b)+7) / POF_BITNUM_IN_BYTE ))

/* Left shift. */
#define POF_MOVE_BIT_LEFT(x,n)              ((x) << (n))

/* Right shift. */
#define POF_MOVE_BIT_RIGHT(x,n)             ((x) >> (n))


#ifdef POF_DATAPATH_ON

/* Max length of the raw packet received by local physical port. */
#define POFDP_PACKET_RAW_MAX_LEN (2048)
/* Max length of the metadata. */
#define POFDP_METADATA_MAX_LEN (128)
/* The field offset of packet received port's ID infomation in metadata. */
#define POFDP_PORT_ID_FIELD_OFFSET_IN_METADATA_B (0)
/* The field length of packet received port's ID infomation in metadata. */
#define POFDP_PORT_ID_FIELD_LEN_IN_METADATA_B (1)
/* First table ID. */
#define POFDP_FIRST_TABLE_ID (0)
/* Define the metadata field id. */
#define POFDP_METADATA_FIELD_ID (0xFFFF)

/* Packet infomation including data, length, received port. */
struct pofdp_packet{
    /* Input information. */
    uint32_t ori_port_id;       /* The original packet input port index. */
    uint32_t ori_len;           /* The original packet length.
                                 * If packet length has been changed, such as
                                 * in add field action, the ori_len will NOT
                                 * change. The len in metadata will update
                                 * immediatley in this situation. */
    uint8_t *buf;               /* The memery which stores the whole packet. */

    /* Output. */
    uint32_t output_port_id;    /* The output port index. */
	uint16_t output_packet_len;			/* Byte unit. */
								/* The length of output packet. */
    uint16_t output_packet_offset;		/* Byte unit. */
								/* The start position of output. */
	uint16_t output_metadata_len;		/* Byte unit. */
	uint16_t output_metadata_offset;	/* Bit unit. */
								/* The output metadata length and offset. 
								 * Packet data output right behind the metadata. */
	uint16_t output_whole_len;  /* = output_packet_len + output_metadata_len. */
	uint8_t *buf_out;			/* The memery which store the output data. */

    /* Offset. */
    uint16_t offset;					/* Byte unit. */
								/* The base offset of table field and actions. */
    uint8_t *buf_offset;        /* The packet pointer shift offset. 
                                 * buf_offset = buf + offset */
    uint32_t left_len;          /* Length of left packet after shifting offset. 
                                 * left_len = metadata->len - offset. */

    /* Metadata. */
    struct pofdp_metadata *metadata;
                                /* The memery which stores the packet metadata. 
								 * The packet WHOLE length and input port index 
								 * has been stored in metadata. */
    uint16_t metadata_len;      /* The length of packet metadata in byte. */

    /* Flow. */
    uint8_t table_type;         /* Type of table which contains the packet now. */
    uint8_t table_id;           /* Index of table which contains the packet now. */
    pof_flow_entry *flow_entry; /* The flow entry which match the packet. */

    /* Instruction & Actions. */
    struct pof_instruction *ins;/* The memery which stores the instructions need
                                 * to be implemented. */
    uint8_t ins_todo_num;       /* Number of instructions need to be implemented. */
	uint8_t ins_done_num;       /* Number of instructions have been done. */
    struct pof_action *act;     /* Memery which stores the actions need to be
                                 * implemented. */
    uint8_t act_num;            /* Number of actions need to be implemented. */
    uint8_t packet_done;        /* Indicate whether the packet processing is
                                 * already done. 1 means done, 0 means not. */

	/* Meter. */
	uint16_t rate;				/* Rate. 0 means no limitation. */
};

/* Define Metadata structure. */
struct pofdp_metadata{
    uint16_t len;
    uint8_t port_id;
    uint8_t reserve;
    uint8_t data[];
};

/* Define datapath struction. */
struct pof_datapath{
    /* NONE promisc packet filter function. */
    uint32_t (*no_promisc)(uint8_t *packet, pof_port *port_ptr, struct sockaddr_ll sll);

    /* Promisc packet filter function. */
    uint32_t (*promisc)(uint8_t *packet, pof_port *port_ptr, struct sockaddr_ll sll);

    /* Set RAW packet filter function. */
    uint32_t (*filter)(uint8_t *packet, pof_port *port_ptr, struct sockaddr_ll sll);
};

extern struct pof_datapath dp;

#define POFDP_ARG	struct pofdp_packet *dpp

/* Task id in datapath module. */
extern task_t g_pofdp_main_task_id;
extern task_t *g_pofdp_recv_raw_task_id_ptr;
extern task_t g_pofdp_send_raw_task_id;

/* Queue id in datapath module. */
extern uint32_t g_pofdp_recv_q_id;
extern uint32_t g_pofdp_send_q_id;

extern uint32_t pof_datapath_init();
extern uint32_t pofdp_create_port_listen_task(task_t *tid, pof_port *p);
extern uint32_t pofdp_send_raw(struct pofdp_packet *dpp);
extern uint32_t pofdp_send_packet_in_to_controller(uint16_t len, \
                                                   uint8_t reason, \
                                                   uint8_t table_id, \
												   struct pof_flow_entry *pfe, \
                                                   uint32_t device_id, \
                                                   uint8_t *packet);
extern uint32_t pofdp_instruction_execute(POFDP_ARG);
extern uint32_t pofdp_action_execute(POFDP_ARG);

extern void pofdp_cover_bit(uint8_t *data_ori, uint8_t *value, uint16_t pos_b, uint16_t len_b);
extern void pofdp_copy_bit(uint8_t *data_ori, uint8_t *data_res, uint16_t offset_b, uint16_t len_b);
extern uint32_t pofdp_lookup_in_table(uint8_t **key_ptr, \
                                      uint8_t match_field_num, \
                                      poflr_flow_table table_vhal, \
                                      pof_flow_entry **entry_ptrptr);
extern uint32_t pofdp_write_32value_to_field(uint32_t value, const struct pof_match *pm, \
											 struct pofdp_packet *dpp);
extern uint32_t pofdp_get_32value(uint32_t *value, uint8_t type, void *u_, const struct pofdp_packet *dpp);

#endif // POF_DATAPATH_ON
#endif // _POF_DATAPATH_H_
