/*
* main.h                                                    Version 5.4.0
*
* Application main header file.
*
* Copyright (c) 1989-2025 Micro Digital Inc.
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

#ifndef APP_MAIN_H
#define APP_MAIN_H

#include "bsp.h"           /* BSP defines */

#if SB_CFG_CON
extern TCB_PTR opcon;
#endif

#if CP_PORTAL
/* portal client structures for console portal */
extern FPCS cpcli_fpd;     /* fpdemo */
extern FPCS cpcli_fpu;     /* fpudemo */
extern FPCS cpcli_idle;    /* idle */
extern FPCS cpcli_opcon;   /* opcon */
#endif

/* definitions */
#define EXIT_ROW 21     /* uses next row too if error occurred */
#define Ctrl_D    4     /* Used to switch to debug message mode */
#define Ctrl_E    5     /* Used to show smx Error Buffer (if not empty) */
#define Esc      27     /* Escape */
#define ESC_COL  60     /* where Esc message displays */

#if SB_CPU_ARMM7
#define CP_SLOT_IDLE  4  /* pmsg slot in idle MPA */
#elif SB_CPU_ARMM8
#define CP_SLOT_IDLE  8  /* pmsg slot in idle MPA */
#endif

/* function prototypes */
#ifdef __cplusplus
extern "C" {
#endif
void  appl_init(void);
void  appl_exit(void);
void  display_protosystem_msgs(void);
void  mheap_init(void);
void  mw_modules_init(void);
void  mw_modules_exit(void);
#if (SB_CFG_CON)
void  opcon_init(void);
void  opcon_main(u32);
#endif
void  smx_EM(SMX_ERRNO errno, u8 sev); /* error manager */
void  smx_EMHook(SMX_ERRNO errno, void* h, u8 sev); /* error manager callback */
void  smx_HeapManager(u32 hn);
void  smx_IdleMain(u32);
void  smx_StackScan(void);             /* stack scan for high water mark */
#ifdef __cplusplus
}
#endif
#endif /* APP_MAIN_H */