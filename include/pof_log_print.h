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

#ifndef _POF_LOG_PRINT_H_
#define _POF_LOG_PRINT_H_

#include <stdio.h>
#include <pthread.h>
#include "pof_common.h"

#define BLACK   "30"
#define RED     "31"
#define GREEN   "32"
#define YELLOW  "33"
#define BLUE    "34"
#define PINK    "35"
#define CYAN    "36"
#define WHITE	""

#define POF_LOG_STRING_MAX_LEN (512)

struct log_util{
	FILE *log_fp;
	pthread_mutex_t mutex;
	uint32_t counter;
	char str[POF_LOG_STRING_MAX_LEN];

	/* Start color funtion. */
	void (*textcolor)(char *i, char *col);

	/* End color function. */
	void (*overcolor)(void);

	/* Log print function. */
	void (*_pDbg)(char *cont, char *i, char *col);
	
	/* Command print function. */
	void (*_pCmd)(char *cont, char *i, char *col);

	/* Error print function. */
	void (*_pErr)(char *cont, char *i, char *col);

	/* Print function. Can not disable. */
	void (*_print)(char *cont, char *i, char *col);

	/* Log file. */
	void (*_pLog)(char *cont);
};

extern struct log_util g_log;

/* flag = 0: receive, flag = 1: send */
extern void pof_debug_cprint_packet(const void *ph, uint32_t flag, int len);
extern void poflp_flow_entry(const void *ph);
extern void poflp_action(const void *ph);
extern void pof_open_log_file(char *filename);
extern void pof_close_log_file();
extern void poflp_debug_enable();

#define POF_LOG_LOCK_ON			pthread_mutex_lock(&g_log.mutex)
#define POF_LOG_LOCK_OFF		pthread_mutex_unlock(&g_log.mutex)

#define POF_TEXTCOLOR(i,col)    g_log.textcolor(#i,col)
#define POF_OVERCOLOR()         g_log.overcolor()

#define POF_FILE_PRINT(cont) g_log._pLog(cont)
/*
#define POF_CPRINT(i,col,type,cont,...) \
            POF_TEXTCOLOR(i,col);											 \
			snprintf(g_log.str,POF_LOG_STRING_MAX_LEN-1,cont,##__VA_ARGS__); \
            POF_FILE_PRINT(g_log.str);										 \
			g_log._##type(g_log.str);										 \
            POF_OVERCOLOR()
            */

#define POF_CPRINT(i,col,type,cont,...) \
			snprintf(g_log.str,POF_LOG_STRING_MAX_LEN-1,cont,##__VA_ARGS__);    \
            POF_FILE_PRINT(g_log.str);										    \
			g_log._##type(g_log.str,#i,col)

#define POF_CPRINT_NO_LOGFILE(i,col,type,cont,...) \
			snprintf(g_log.str,POF_LOG_STRING_MAX_LEN-1,cont,##__VA_ARGS__);    \
			g_log._##type (g_log.str,#i,col)

#define POF_DEBUG_CPRINT(i,col,cont,...)        POF_CPRINT(i,col,pDbg,cont, ##__VA_ARGS__)

#define POF_DEBUG_CPRINT_HEAD(i,col)            POF_DEBUG_CPRINT(i,col,"%s|%s|%05u|INFO|%s|%d: ", __TIME__, __DATE__, g_log.counter++, __FILE__, __LINE__)
#define POF_DEBUG_CPRINT_FL(i,col,cont,...) \
			POF_LOG_LOCK_ON;								\
            POF_DEBUG_CPRINT_HEAD(i,col);					\
            POF_DEBUG_CPRINT(i,col,cont"\n",##__VA_ARGS__); \
			POF_LOG_LOCK_OFF

#define POF_DEBUG_CPRINT_FL_NO_ENTER(i,col,cont,...) \
            POF_DEBUG_CPRINT_HEAD(i,col); \
            POF_DEBUG_CPRINT(i,col,cont,##__VA_ARGS__); \

/* flag = 0: receive, flag = 1: send */
#define POF_DEBUG_CPRINT_PACKET(pheader,flag,len) \
			POF_LOG_LOCK_ON;							\
            POF_DEBUG_CPRINT_HEAD(1,GREEN);				\
            pof_debug_cprint_packet(pheader,flag,len);	\
			POF_LOG_LOCK_OFF

#define POF_DEBUG_CPRINT_0X_NO_ENTER(x, len) \
            { \
                POF_DEBUG_CPRINT(1,WHITE,"0x"); \
                uint32_t i; \
                for(i=0; i<len; i++){   \
                    POF_DEBUG_CPRINT(1,WHITE,"%.2x ", *((uint8_t *)x + i)); \
                } \
            }

#define POF_DEBUG_CPRINT_0X(x, len) \
            POF_DEBUG_CPRINT_0X_NO_ENTER(x, len); \
            POF_DEBUG_CPRINT(1,WHITE,"\n")

#define POF_DEBUG_CPRINT_FL_0X(i,col,x,len,cont,...) \
			POF_LOG_LOCK_ON;										\
            POF_DEBUG_CPRINT_FL_NO_ENTER(i,col,cont,##__VA_ARGS__); \
            POF_DEBUG_CPRINT_0X(x,len);								\
			POF_LOG_LOCK_OFF

#define POF_DEBUG_SLEEP(i) \
            POF_DEBUG_CPRINT_FL(1,WHITE,"sleep start! [%ds]\n", i); \
            sleep(i); \
            POF_DEBUG_CPRINT_FL(1,WHITE,"sleep over! [%ds]\n", i)

#ifdef POF_ERROR_PRINT_ON
#define POF_ERROR_CPRINT(i,col,cont,...)		POF_CPRINT(i,col,pErr,cont,##__VA_ARGS__);
#else // POF_ERROR_PRINT_ON
#define POF_ERROR_CPRINT(i,col,cont,...)
#endif // POF_ERROR_PRINT_ON

#define POF_ERROR_CPRINT_HEAD(i,col)            POF_ERROR_CPRINT(i,col,"%s|%s|ERROR|%s|%d: ", __TIME__, __DATE__, __FILE__, __LINE__)
#define POF_ERROR_CPRINT_FL(i,col,cont,...)             \
			POF_LOG_LOCK_ON;							\
            POF_ERROR_CPRINT_HEAD(i,col);               \
            POF_ERROR_CPRINT(i,col,cont"\n", ##__VA_ARGS__);  \
			POF_LOG_LOCK_OFF; \
            perror("System ERROR information: ")

#define POF_ERROR_CPRINT_FL_NO_LOCK(i,col,cont,...)     \
            POF_ERROR_CPRINT(i,col,"\n");                     \
            POF_ERROR_CPRINT_HEAD(i,col);               \
            POF_ERROR_CPRINT(i,col,cont"\n", ##__VA_ARGS__);

#define POF_PRINT(i,col,cont,...)	POF_CPRINT(i,col,print,cont,##__VA_ARGS__)


#endif // _POF_LOG_PRINT_H_
