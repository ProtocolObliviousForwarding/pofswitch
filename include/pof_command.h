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

#ifndef _POF_COMMAND_H_
#define _POF_COMMAND_H_

#include "pof_global.h"
#include "pof_log_print.h"
#include "pof_local_resource.h"
#include "pof_common.h"

#ifdef POF_COMMAND_ON
/* Print in user command mode with an enter at the end. */
#define POF_COMMAND_PRINT_NO_LOGFILE(i,col,cont,...)       POF_CPRINT_NO_LOGFILE(i,col,pCmd,cont, ##__VA_ARGS__)
#define POF_COMMAND_PRINT(i,col,cont,...)       POF_CPRINT(i,col,pCmd,cont, ##__VA_ARGS__)
#define POF_COMMAND_PRINT_HEAD(cont,...)		POF_COMMAND_PRINT(1,PINK,"%s|%s|POF_COMMAND: "cont"\n",__TIME__,__DATE__,##__VA_ARGS__)

/* Print data x in byte unit with hexadeimal format in user command mode with no enter at the end. */
#define POF_COMMAND_PRINT_0X_NO_ENTER(x, len) \
    {int i;																\
        POF_COMMAND_PRINT(1,WHITE,"0x");                                \
        for(i=0; i<len; i++){											\
            POF_COMMAND_PRINT(1,WHITE,"%.2x ", *((uint8_t *)x + i));	\
        }                                                               \
    }
/* Go into user command mode. */
void pof_runing_command(char * comm, size_t comm_len);
/* Print all tables, entries information. */
void usr_cmd_tables();

#endif // POF_COMMAND_ON
#endif // _POF_COMMAND_H_
