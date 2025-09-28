/*
* xevb.h                                                    Version 5.4.0
*
* smx Event Buffer macros and defines.
*
* Copyright (c) 2002-2025 Micro Digital Inc.
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
* Authors: Marty Cochran, David Moore, Ralph Moore
*
*****************************************************************************/

#ifndef SMX_XEVB_H
#define SMX_XEVB_H

/* pseudo handles for event buffer */
extern void* smx_InvokeH;
extern void* smx_TickISRH;

/* format of first word of EVB records:
      0x5555TTSS
      5555 = Recognition pattern
      TT = Type
      SS = Size(Words) 
*/

/* record types */
#define SMX_EVB_RT_ERR        0x00000100
#define SMX_EVB_RT_INVOKE     0x00000200
#define SMX_EVB_RT_ISR        0x00000300
#define SMX_EVB_RT_ISR_RET    0x00000400
#define SMX_EVB_RT_LSR        0x00000500
#define SMX_EVB_RT_LSR_RET    0x00000600
#define SMX_EVB_RT_SSR        0x00000700
#define SMX_EVB_RT_SSR_RET    0x00000800
#define SMX_EVB_RT_START      0x00000900
#define SMX_EVB_RT_RESUME     0x00000A00
#define SMX_EVB_RT_AUTOSTOP   0x00000B00
#define SMX_EVB_RT_PERR       0x00000C00
#define SMX_EVB_RT_USER       0x00001000
#define SMX_EVB_RT_PRINT      0x00001100
#define SMX_EVB_RT_PORTAL     0x00001200
#define SMX_EVB_RT_PORTAL_RET 0x00001300
#define SMX_EVB_RT_MASK       0x0000FF00

#define SMX_EVB_MAX_REC       11          /* maximum record size (words) */

/* log enable flags in smx_evben */
#define SMX_EVB_EN_TASK       0x00000001  /* log task events */
#define SMX_EVB_EN_LSR        0x00000004  /* log LSR events */
#define SMX_EVB_EN_ISR        0x00000008  /* log ISR events */
#define SMX_EVB_EN_ERR        0x00000010  /* log errors */
#define SMX_EVB_EN_USER       0x00000020  /* log user events */
#define SMX_EVB_EN_PORTAL     0x00000040  /* log portal events */
#define SMX_EVB_EN_PERR       0x00000080  /* log portal errors */
#define SMX_EVB_EN_SSR1       0x00010000  /* log SSR group 1 (see SSR IDs in xdef.h) */
#define SMX_EVB_EN_SSR2       0x00020000  /* log SSR group 2 */
#define SMX_EVB_EN_SSR3       0x00040000  /* log SSR group 3 */
#define SMX_EVB_EN_SSR4       0x00080000  /* log SSR group 4 */
#define SMX_EVB_EN_SSR5       0x00100000  /* log SSR group 5 */
#define SMX_EVB_EN_SSR6       0x00200000  /* log SSR group 6 */
#define SMX_EVB_EN_SSR7       0x00400000  /* log SSR group 7 */
#define SMX_EVB_EN_SSR8       0x00800000  /* log SSR group 8 */
#define SMX_EVB_EN_SSRS       0x00FF0000  /* log SSR groups 1-8 */
#define SMX_EVB_EN_ALL        0x00FF00FD  /* log all events */

#define  SMX_RESUMED          16          /* task resumed for task log */
#define  SMX_STARTED          8           /* task started for task log */

/*===========================================================================*
*                          EVENT BUFFER FUNCTIONS                            *
*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

bool smx_EVBInit(u32 flags);

/* for system events */
void smx_EVBLogISR(void* isr);
void smx_EVBLogISRRet(void* isr);
void smx_EVBLogLSR(void* lsr);
void smx_EVBLogLSRRet(void* lsr);
void smx_EVBLogError(u32 errno, void* h);
void smx_EVBLogInvoke(void* isr, LCB_PTR lsr, u32 par);
void smx_EVBLogSSRRet(u32 rv, u32 id);
void smx_EVBLogTaskAutoStop(void);
void smx_EVBLogTaskEnd(void);
void smx_EVBLogTaskResume(void);
void smx_EVBLogTaskStart(void);

/* for user events */
void smx_EVBLogUser0(void* h);
void smx_EVBLogUser1(void* h, u32 p1);
void smx_EVBLogUser2(void* h, u32 p1, u32 p2);
void smx_EVBLogUser3(void* h, u32 p1, u32 p2, u32 p3);
void smx_EVBLogUser4(void* h, u32 p1, u32 p2, u32 p3, u32 p4);
void smx_EVBLogUser5(void* h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5);
void smx_EVBLogUser6(void* h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6);
void smx_EVBLogUserPrint(u32 time, u32 index);

/* for smx_SSR_ENTERn() macros */
void smx_EVBLogSSR0(u32 id);
void smx_EVBLogSSR1(u32 id, u32 p1);
void smx_EVBLogSSR2(u32 id, u32 p1, u32 p2);
void smx_EVBLogSSR3(u32 id, u32 p1, u32 p2, u32 p3);
void smx_EVBLogSSR4(u32 id, u32 p1, u32 p2, u32 p3, u32 p4);
void smx_EVBLogSSR5(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5);
void smx_EVBLogSSR6(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6);
void smx_EVBLogSSR7(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6, u32 p7);

#ifdef __cplusplus
}
#endif

/*===========================================================================*
*                            EVENT BUFFER MACROS                             *
*===========================================================================*/
#if SMX_CFG_EVB
/* for system events */
#define smx_EVB_LOG_ISR(isr)              smx_EVBLogISR(isr)
#define smx_EVB_LOG_ISR_RET(isr)          smx_EVBLogISRRet(isr)
#define smx_EVB_LOG_LSR(lsr)              smx_EVBLogLSR(lsr)
#define smx_EVB_LOG_LSR_RET(lsr)          smx_EVBLogLSRRet(lsr)
#define smx_EVB_LOG_ERROR(errno, h)       smx_EVBLogError(errno, h)
#define smx_EVB_LOG_INVOKE(isr, lsr, par) smx_EVBLogInvoke(isr, lsr, par)
#define smx_EVB_LOG_SSR_RET(rv, id)       smx_EVBLogSSRRet(rv, id)
#define smx_EVB_LOG_TASK_AUTOSTOP()       smx_EVBLogTaskAutoStop()
#define smx_EVB_LOG_TASK_END()            smx_EVBLogTaskEnd()
#define smx_EVB_LOG_TASK_RESUME()         smx_EVBLogTaskResume()
#define smx_EVB_LOG_TASK_START()          smx_EVBLogTaskStart()

/* for smx_SSR_ENTERn() macros */
#define smx_EVB_LOG_SSR0(id)                          smx_EVBLogSSR0(id)
#define smx_EVB_LOG_SSR1(id, p1)                      smx_EVBLogSSR1(id, p1)
#define smx_EVB_LOG_SSR2(id, p1, p2)                  smx_EVBLogSSR2(id, p1, p2)
#define smx_EVB_LOG_SSR3(id, p1, p2, p3)              smx_EVBLogSSR3(id, p1, p2, p3)
#define smx_EVB_LOG_SSR4(id, p1, p2, p3, p4)          smx_EVBLogSSR4(id, p1, p2, p3, p4)
#define smx_EVB_LOG_SSR5(id, p1, p2, p3, p4, p5)      smx_EVBLogSSR5(id, p1, p2, p3, p4, p5)
#define smx_EVB_LOG_SSR6(id, p1, p2, p3, p4, p5, p6)  smx_EVBLogSSR6(id, p1, p2, p3, p4, p5, p6)
#define smx_EVB_LOG_SSR7(id, p1, p2, p3, p4, p5, p6, p7)  smx_EVBLogSSR7(id, p1, p2, p3, p4, p5, p6, p7)

/* for user events */
#define smx_EVB_LOG_USER0(h)                          smx_EVBLogUser0(h);
#define smx_EVB_LOG_USER1(h, p1)                      smx_EVBLogUser1(h, p1);
#define smx_EVB_LOG_USER2(h, p1, p2)                  smx_EVBLogUser2(h, p1, p2);
#define smx_EVB_LOG_USER3(h, p1, p2, p3)              smx_EVBLogUser3(h, p1, p2, p3);
#define smx_EVB_LOG_USER4(h, p1, p2, p3, p4)          smx_EVBLogUser4(h, p1, p2, p3, p4);
#define smx_EVB_LOG_USER5(h, p1, p2, p3, p4, p5)      smx_EVBLogUser5(h, p1, p2, p3, p4, p5);
#define smx_EVB_LOG_USER6(h, p1, p2, p3, p4, p5, p6)  smx_EVBLogUser6(h, p1, p2, p3, p4, p5, p6);
#define smx_EVB_LOG_USER_PRINT(time, index)           smx_EVBLogUserPrint(time, index);

#else  /* for event logging disabled */

#define smx_EVB_LOG_ISR(isr)
#define smx_EVB_LOG_ISR_RET(isr)
#define smx_EVB_LOG_LSR(lsr)
#define smx_EVB_LOG_LSR_RET(lsr)
#define smx_EVB_LOG_ERROR(errno, hndl)
#define smx_EVB_LOG_INVOKE(isr, lsr, par)
#define smx_EVB_LOG_SSR_RET(rv, id)
#define smx_EVB_LOG_TASK_AUTOSTOP()
#define smx_EVB_LOG_TASK_END()
#define smx_EVB_LOG_TASK_RESUME()
#define smx_EVB_LOG_TASK_START()
#define smx_EVB_LOG_SSR0(id)
#define smx_EVB_LOG_SSR1(id, p1)
#define smx_EVB_LOG_SSR2(id, p1, p2)
#define smx_EVB_LOG_SSR3(id, p1, p2, p3)
#define smx_EVB_LOG_SSR4(id, p1, p2, p3, p4)
#define smx_EVB_LOG_SSR5(id, p1, p2, p3, p4, p5)
#define smx_EVB_LOG_SSR6(id, p1, p2, p3, p4, p5, p6)
#define smx_EVB_LOG_SSR7(id, p1, p2, p3, p4, p5, p6, p7)
#define smx_EVB_LOG_USER0(h)
#define smx_EVB_LOG_USER1(h, p1)
#define smx_EVB_LOG_USER2(h, p1, p2)
#define smx_EVB_LOG_USER3(h, p1, p2, p3)
#define smx_EVB_LOG_USER4(h, p1, p2, p3, p4)
#define smx_EVB_LOG_USER5(h, p1, p2, p3, p4, p5)
#define smx_EVB_LOG_USER6(h, p1, p2, p3, p4, p5, p6)
#define smx_EVB_LOG_USER_PRINT(time, index)
#endif  /* SMX_CFG_EVB */
#endif  /* SMX_XEVB_H */

