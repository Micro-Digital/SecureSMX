/*
* cprtl.h                                                   Version 5.4.0
*
* Console Portal Header File.
*
* Copyright (c) 2019-2025 Micro Digital Inc.
* All rights reserved. www.smxrtos.com
*
* This software, documentation, and accompanying materials are made available
* under the Apache License, Version 2.0. You may not use this file except in
* compliance with the License. http://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
*
* This Work is protected by patents listed in smx.h. A patent license is
* granted according to the License above. This entire comment block must be
* preserved in all copies of this file.
*
* Support services are offered by MDI. Inquire at support@smxrtos.com.
*
* Author: Ralph Moore
*
*****************************************************************************/

#ifndef SB_CPRTL_H
#define SB_CPRTL_H

#if CP_PORTAL

/* configuration */
#if SB_CPU_ARMM7
#define CPS_SLOTS    4             /* console portal server MPA slot */
#elif SB_CPU_ARMM8
#define CPS_SLOTS    0x80 + 4      /* console portal server MPA slot */
#endif
#define CPS_SLOTX    5             /* console portal slot for deleting portal 
                                      (free slot in mpa_tmplt_init) */

/* function IDs */
typedef enum {CP_CLR_SCREEN, CP_CLR_SCREEN_UNP, CP_DBG_MODE, CP_DBG_MODE_SET,
       CP_PUT_CHAR, CP_WRITE_CHAR, CP_WRITE_STRING, CP_WRITE_STRING_NUM} CP_IDS;

typedef struct CPSH {      /* CONSOLE PORTAL SERVICE HEADER */
   u32         fid;           /* function ID */
   u32         p1;            /* parameter 1 */
   u32         p2;            /* parameter 2 */
   u32         dp;            /* data pointer */
   u32         ret;           /* return value */
   void*       caller;        /* caller addr (debug) */
} CPSH;

/*===========================================================================*
*                       Console Portal Client API                            *
*===========================================================================*/

#define cp_ConPortalClose(cpch, xsn)   mp_FPortalClose(cpch, xsn)

#ifdef __cplusplus
extern "C" {
#endif

bool  cp_init(u32 ssn);
bool  cp_exit(u32 csn);

/* shell functions in cpcli.c  */
void  sbp_ConClearScreen(FPCS* pch);
void  sbp_ConClearScreenUnp(FPCS* pch);
bool  sbp_ConDbgMsgMode(FPCS* pch);
void  sbp_ConDbgMsgModeSet(bool enable, FPCS* pch);
int   sbp_ConPutChar(char ch, FPCS* pch);
void  sbp_ConWriteChar(u32 col, u32 row, u32 F_color, u32 B_color, u32 blink, 
                                                             char ch, FPCS* pch);
void  sbp_ConWriteString(u32 col, u32 row, u32 F_color, u32 B_color,
                                    u32 blink, const char *in_string, FPCS* pch);
void  sbp_ConWriteStringNum(u32 col, u32 row, u32 F_color, u32 B_color,
                           u32 blink, const char *in_string, u32 num, FPCS* pch);
#ifdef __cplusplus
}
#endif

#endif /* CP_PORTAL */
#endif /* SB_CPRTL_H */
