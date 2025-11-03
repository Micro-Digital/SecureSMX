/*
* xglob.c                                                   Version 5.4.0
*
* smx globals
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

#include "xsmx.h"

FUNV_PTR       smx_autostop;           /* autostop function address */
PCB            smx_bcbs;               /* BCB pool */
LCB_PTR        smx_clsr;               /* current LSR */
TCB_PTR        smx_ct = (TCB_PTR)&smx_dtcb; /* current task */
bool           smx_ctstart;            /* current task start */
#if defined(SMX_DEBUG)
bool const     smx_debug_lib = true;   /* smx library compiled for debug */
#else
bool const     smx_debug_lib = false;  /* smx library not compiled for debug */
#endif
TCB            smx_dtcb;               /* dummy task control block <1> */
bool           smx_eben;               /* error buffer enable */
EREC_PTR       smx_ebi;                /* first record in error buffer */
EREC_PTR       smx_ebn;                /* next record in error buffer */
EREC_PTR       smx_ebx;                /* last record in error buffer */
PCB            smx_egcbs;              /* EGCB pool */
PCB            smx_eqcbs;              /* EQCB pool */
u32            smx_errctr;             /* total smx errors */
u8             smx_errctrs[SMX_NUM_ERRORS]; /* smx error counters */
SMX_ERRNO      smx_errno;              /* last smx error */
vu32           smx_etime;              /* elapsed time */
#if SMX_CFG_EVB
u32            smx_evben = 0;          /* event buffer flags to enable what to log */
u32*           smx_evbi;               /* first word in event buffer */
u32*           smx_evbn;               /* next word in event buffer */
u32*           smx_evbx;               /* last word in event buffer */
#endif
void*          smx_freestack;          /* free stack pointer */
HTREC_PTR      smx_hti;                /* first record in handle table */
HTREC_PTR      smx_htn;                /* next record in handle table */
HTREC_PTR      smx_htx;                /* last record in handle table */
bool           smx_htchg;              /* handle table changed (used by smxAware) */
bool           smx_htpres = false;     /* handle table present when true */
#if SMX_CFG_STACK_SCAN
bool           smx_idleup = false;     /* idle is running at > 0 priority */
#endif
bool           smx_init = false;       /* set when smx has been initialized */
PCB            smx_lcbs;               /* LCB pool */
u32            smx_lockctr;            /* scheduler lock nesting counter */
LQC_PTR        smx_lqi;                /* pointer to beginning of LSR queue */
LQC_PTR        smx_lqx;                /* pointer to end of LSR queue */
u32            smx_lqctr;              /* number of LSRs in lq */
u32            smx_lqhwm;              /* LSR queue high water mark */
LQC_PTR        smx_lqin;               /* pointer to next free position in lq */
LQC_PTR        smx_lqout;              /* pointer to next LSR to run */
PCB            smx_mcbs;               /* MCB pool */
#if SMX_CFG_SSMX
bool           smx_mpu_br_off;         /* background region was off in interrupted code */
#endif
u16            smx_mshwm;              /* main stack high water mark */
bool           smx_mshwmv = false;     /* main stack high water mark valid */
u32            smx_mstop;              /* main stack top -- used in xarmm_iar.s */
PCB            smx_mucbs;              /* MUCB pool */
PCB            smx_pcbs;               /* PCB pool */
PCB            smx_picbs;              /* PICB pool */
u32            smx_psp_sav;            /* psp stack pointer save */
RQCB           smx_rq[SMX_PRI_NUM];    /* ready queue */
RQCB_PTR       smx_rqx;                /* ready queue last level */
RQCB_PTR       smx_rqtop;              /* pointer to top priority level of smx_rq */
#if SMX_CFG_RTLIM
SCB_PTR        smx_rtlimsem;           /* runtime limit gate semaphore */
#endif
#if SMX_CFG_STACK_SCAN
void*          smx_scanstack = NULL;   /* stack scan pool pointer */
void*          smx_scanstack_end = &smx_scanstack; /* stack scan pool end pointer */
#endif
PCB            smx_scbs;               /* SCB pool */
SMX_SCHED      smx_sched;              /* scheduler flags */
u32*           smx_spmin;              /* start of stack pool */
u32            smx_srnest;             /* service routine nesting level */
#if SMX_CFG_SSMX
u32*           smx_sstp = smx_sst;     /* curent system service table pointer */
#endif
#if SMX_CFG_SSMX && SMX_CFG_DIAG
u32*           smx_sst_ctr;            /* system service call counters */
u32            smx_svc_ctr;            /* SVC call counter (total ss calls) */
#endif
vu32           smx_stime;              /* system time */
bool           smx_stkpl_init = false; /* set when the stack pool has been initialized */
#if SMX_CFG_STACK_SCAN
TCB_PTR        smx_tcbns;              /* next TCB to stack scan*/
#endif
PCB            smx_tcbs;                        /* TCB pool */
u32            smx_timeout[SMX_NUM_TASKS];  /* task timeout array */
PCB            smx_tmrcbs;                      /* TMRCB pool */
u32            smx_tmo_indx;                    /* index of minimum timeout */
u32            smx_tmo_min = SMX_TMO_INF;       /* minimum waiting timeout value */
CB             smx_tqcb;                        /* timer queue control block */
CB_PTR         smx_tq = &smx_tqcb;              /* timer queue */
PCB            smx_xcbs;                        /* XCB pool */

u32            smx_Version = SMX_VERSION;       /* smx version number (consulted by smxAware) */

/* system objects */
TCB_PTR        smx_Idle = NULL;        /* idle task */
LCB_PTR        smx_KeepTimeLSR;
#if SMX_CFG_PROFILE
LCB_PTR        smx_ProfileLSR;
#endif
LCB_PTR        smx_TaskDeleteLSR;
LCB_PTR        smx_TimeoutLSR;

/* Notes:
   1. smx_dtcb provides a place to write smx_ct->err and smx_ct->susploc
      for C++ initializers that call SSRs, before smx sets smx_ct to a task.
*/
