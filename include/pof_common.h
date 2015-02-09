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

#ifndef POF_COMMON_H_
#define POF_COMMON_H_

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <endian.h>
#include "stdio.h"
#include "string.h"
#include "time.h"

/* The macro for enabling Openflow function. */
#define OPENFLOW_ENABLE

/* Define the wordsize of the system. */
#define POF_OS_32 (0) // 32bit system
#define POF_OS_64 (1) // 64bit system
#if (__WORDSIZE == 64)
#define POF_OS_BIT POF_OS_64
#else // __WORDSIZE
#define POF_OS_BIT POF_OS_32
#endif // __WORDSIZE

/* Define the byte order of the system. */
#define POF_LITTLE_ENDIAN (0)
#define POF_BIG_ENDIAN (1)
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define POF_BYTE_ORDER POF_LITTLE_ENDIAN
#else // __BYTE_ORDER
#define POF_BYTE_ORDER POF_BIG_ENDIAN
#endif // __BYTE_ORDER

/* As a standard GNU software, you can turn on or turn off some fucntions
 * using the arguments of "./configure" before "make". */
#define ENABLE_CONFIGURE_ARG



#ifndef ENABLE_CONFIGURE_ARG

/* Output debug log. */
#define POF_DEBUG_ON

/* Color print. Only for command line. */
#define POF_DEBUG_COLOR_ON

/* Print echo request and reply. */
//#define POF_DEBUG_PRINT_ECHO_ON

/* Start datapath module. */
#define POF_DATAPATH_ON

/* Start user command module. */
#define POF_COMMAND_ON

/* Enable promisc mode. */
//#define POF_PROMISC_ON

#endif // ENABLE_CONFIGURE_ARG


/* Output ERROR log. */
#define POF_ERROR_PRINT_ON

/* Check if the flow entry have already existed when add flow entry. */
//#define POF_ADD_ENTRY_CHECK_ON

/* Modes for setting initial config of connection: from "config" file, from command arg. */
#define POF_CONN_CONFIG_FILE
#define POF_CONN_CONFIG_COMMAND

/* Soft Switch performance after Controller disconnection. */
/* The Soft Switch will try to reconnect the Controller after Controller disconnection. */
#define POF_AFTER_CTRL_DISCONN_RECONN    (1)
/* The Soft Switch will be shut down after Controller disconnection. */
#define POF_AFTER_CTRL_DISCONN_SHUT_DOWN (2)
#define POF_PERFORM_AFTER_CTRL_DISCONN  POF_AFTER_CTRL_DISCONN_RECONN
//#define POF_PERFORM_AFTER_CTRL_DISCONN  POF_AFTER_CTRL_DISCONN_SHUT_DOWN

#define POF_AUTOCLEAR

/* Soft Switch performance when there is no flow entry matches the packet. */
/* The no match packet will be send upward to the Controller. */
#define POF_NOMATCH_PACKET_IN (1)
/* The no match packet will be drop. */
#define POF_NOMATCH_DROP      (2)
//#define POF_NOMATCH POF_NOMATCH_PACKET_IN
#define POF_NOMATCH POF_NOMATCH_DROP

/* Soft Switch's filter of received raw packet in datapath module. If the raw packet
 * received by local physical port fit the filter condition, it will be forwarded.
 * On the contrary, the packet will be droped. If you want to set a complex filter, 
 * you should rewrite the function "pofdp_packet_raw_filter" in ../datapath/pof_datapath.c*/
/* Receive any pacekt. */
//#define POF_RECVRAW_ANYPACKET
/* Receive the test pacekt. */
//#define POF_RECVRAW_TESTPACKET
/* Drop the packet which destination hwaddr is not local. */
//#define POF_RECVRAW_DHWADDR_LOCAL
/* Drop the packet which ethernet type is not IP. */
//define POF_RECVRAW_ETHTYPE_IP
/* Drop the packet which ip protocol is not ICMP. */
//#define POF_RECVRAW_IPPROTO_ICMP

//#define POF_TEST_SEND_ORDER



/* Define NULL. */
#ifndef NULL
#define NULL (0)
#endif

/* Define TRUE and FALSE. */
#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

/* Define the invalid and valid id based on system. */
#define POF_INVALID_QUEUEID (-1)
#define POF_VALID_QUEUEID (1)
#define POF_INVALID_TIMERID (0)
#define POF_VALID_TIMERID (1)
#define POF_INVALID_TASKID (0)

/* Define waiting mode. */
#define POF_NO_WAIT IPC_NOWAIT
#define POF_WAIT_FOREVER (0)

/* Define message size. */
#define POF_MESSAGE_SIZE (2560)

/* Define message type. */
#define POF_MSGTYPE_ANY (0)
#define POF_MSGTYPE     (1)

/* Initialize the Xid value. */
#define POF_INITIAL_XID (0)

#define POF_STRING_PAIR_MAX_LEN (64)
struct pof_str_pair{
    char item[POF_STRING_PAIR_MAX_LEN];
    char cont[POF_STRING_PAIR_MAX_LEN];
};

struct pof_state {
    struct pof_str_pair version;
    struct pof_str_pair os_bit;
    struct pof_str_pair byte_order;
    struct pof_str_pair configure_arg;
    struct pof_str_pair debug_on;
    struct pof_str_pair debug_color_on;
    struct pof_str_pair debug_print_echo_on;
    struct pof_str_pair datapath_on;
    struct pof_str_pair command_on;
    struct pof_str_pair error_print_on;
    struct pof_str_pair config_file;
    struct pof_str_pair config_cmd;
    struct pof_str_pair perform_after_ctrl_disconn;
    struct pof_str_pair nomatch;
    struct pof_str_pair sd2n;
    struct pof_str_pair promisc_on;
    struct pof_str_pair ctrl_ip;
    struct pof_str_pair conn_port;
    struct pof_str_pair autoclear_after_disconn;
    struct pof_str_pair log_file;
};

extern struct pof_state g_states;

#endif


