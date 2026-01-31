/*
* xglob.h                                                   Version 6.0.0
*
* smx globals
*
* Copyright (c) 1989-2026 Micro Digital Inc.
* All rights reserved. www.smxrtos.com
*
* SPDX-License-Identifier: GPL-2.0-only OR LicenseRef-MDI-Commercial
*
* This software, documentation, and accompanying materials are made available
* under a dual license, either GPLv2 or Commercial. You may not use this file
* except in compliance with either License. GPLv2 is at www.gnu.org/licenses.
* It does not permit the incorporation of this code into proprietary programs.
*
* Commercial license and support services are available from Micro Digital.
* Inquire at support@smxrtos.com.
*
* This Work embodies patents listed in smx.h. A patent license is hereby
* granted to use these patents in this Work and Derivative Works, except in
* another RTOS or OS.
*
* This entire comment block must be preserved in all copies of this file.
*
* Author: Ralph Moore
*
*****************************************************************************/

#ifndef SMX_XGLOB_H
#define SMX_XGLOB_H

extern FUNV_PTR   smx_autostop;     /* autostop function address */
extern PCB        smx_bcbs;         /* BCB pool */
extern LCB_PTR    smx_clsr;         /* current LSR */
#if SMX_CFG_PROFILE
extern CPS        smx_cpa;          /* coarse profile accumulator */
extern CPS        smx_cpd;          /* coarse profile display */
#endif
extern TCB_PTR    smx_ct;           /* current task */
extern bool       smx_ctstart;      /* current task start */
extern TCB        smx_dtcb;         /* dummy task control block */
extern bool const smx_debug_lib;    /* smx library debug version */
extern bool       smx_eben;         /* smx error buffer enabled */
extern EREC_PTR   smx_ebi;          /* first record in error buffer */
extern EREC_PTR   smx_ebn;          /* next record in error buffer */
extern EREC_PTR   smx_ebx;          /* last record in error buffer */
extern PCB        smx_egcbs;        /* EGCB pool */
extern PCB        smx_eqcbs;        /* EQCB pool */
extern u32        smx_errctr;       /* total smx errors */
extern u8         smx_errctrs[SMX_NUM_ERRORS]; /* smx error counters */
extern SMX_ERRNO  smx_errno;        /* last smx error */
extern const char* const smx_errmsgs[]; /* error message table */
extern vu32       smx_etime;        /* elapsed time */
#if SMX_CFG_EVB
extern u32        smx_evben;        /* event buffer flags to enable what to log */
extern u32*       smx_evbi;         /* first word in event buffer */
extern u32*       smx_evbn;         /* next word in event buffer */
extern u32*       smx_evbx;         /* last word in event buffer */
#endif
extern void*      smx_freestack;    /* free stack pointer */
extern MUCB_PTR   smx_hmtx[EH_NUM_HEAPS]; /* heap mutex pointer array */
extern bool       smx_hmng;         /* run HeapManager */
extern u32        smx_htmo;         /* heap mutex timeout */
extern HTREC_PTR  smx_hti;          /* first record in handle table */
extern HTREC_PTR  smx_htn;          /* next record in handle table */
extern HTREC_PTR  smx_htx;          /* last record in handle table */
extern bool       smx_htchg;        /* handle table changed (used by smxAware) */
extern bool       smx_htpres;       /* handle table present when true */
extern u32        smx_i_rtc;        /* captured ISR rtc at end of frame */
#if SMX_CFG_STACK_SCAN
extern bool       smx_idleup;       /* idle is running at > 0 priority */
#endif
extern bool       smx_init;         /* set when smx has been initialized */
extern bool       smx_inssu;        /* in smx_StackScanU() */
extern u32        smx_isr_rtc;      /* ISR runtime counter */
extern u32        smx_l_rtc;        /* captured LSR rtc at end of frame */
extern PCB        smx_lcbs;         /* LCB pool */
extern u32        smx_lockctr;      /* scheduler lock nesting counter */
extern LQC_PTR    smx_lqi;          /* pointer to beginning of LSR queue */
extern LQC_PTR    smx_lqx;          /* pointer to end of LSR queue */
extern u32        smx_lqctr;        /* number of LSRs in lq */
extern u32        smx_lqhwm;        /* LSR queue high water mark */
extern LQC_PTR    smx_lqin;         /* pointer to next free position in lq */
extern LQC_PTR    smx_lqout;        /* pointer to next LSR to run */
extern u32        smx_lsr_rtc;      /* LSR runtime counter */
extern PCB        smx_mcbs;         /* MCB pool */
#if SMX_CFG_SSMX
extern bool       smx_mpu_br_off;   /* background region was off in interrupted code */
#endif
extern u16        smx_mshwm;        /* main stack high water mark */
extern bool       smx_mshwmv;       /* main stack high water mark valid */
extern u32        smx_mstop;        /* main stack top -- used in xarmm_iar.s */
extern PCB        smx_mucbs;        /* MUCB pool */
extern PCB        smx_pcbs;         /* PCB pool */
extern PCB        smx_picbs;        /* PICB pool */
#if SMX_CFG_PROFILE
extern u32        smx_pidle;        /* % of time idle */
extern u32        smx_povhd;        /* % of time in smx */
extern u32        smx_pwork;        /* % of time doing useful work */
#endif
extern u32        smx_psp_sav;      /* psp stack pointer save */
extern RQCB       smx_rq[SMX_PRI_NUM]; /* ready queue */
extern RQCB_PTR   smx_rqx;             /* ready queue last level */
extern RQCB_PTR   smx_rqtop;           /* pointer to top priority level of smx_rq */
extern u32*       smx_rtcbi;           /* start of smx_rtcb[][] */
extern volatile u32  smx_rtc_frame_ctr;   /* determines rtc frame */
#if SMX_CFG_RTLIM
extern SCB_PTR    smx_rtlimsem;        /* runtime limit gate semaphore */
#endif
#if SMX_CFG_STACK_SCAN
extern void*      smx_scanstack;       /* stack scan pool pointer */
extern void*      smx_scanstack_end;   /* stack scan pool end pointer */
#endif
extern PCB        smx_scbs;         /* SCB pool */
extern SMX_SCHED  smx_sched;        /* scheduler flags */
extern u32*       smx_spmin;        /* start of stack pool */
extern u32        smx_srnest;       /* service routine nesting level */
#if SMX_CFG_SSMX
extern u32        smx_sst[];        /* main system service table */
extern u32*       smx_sstp;         /* curent system service table pointer */
#endif
#if SMX_CFG_SSMX && SMX_CFG_DIAG
extern u32*       smx_sst_ctr;      /* system service call counters */
extern u32        smx_svc_ctr;      /* SVC call counter (total ss calls) */
#endif
extern vu32       smx_stime;        /* system time */
extern bool       smx_stkpl_init;   /* set when the stack pool has been initialized */
extern TCB_PTR    smx_tcbns;        /* next TCB to stack scan*/
extern PCB        smx_tcbs;         /* TCB pool */
extern u32        smx_tick_ctr;     /* counter used to time seconds */
extern u32        smx_timeout[SMX_NUM_TASKS]; /* task timeout array */
extern PCB        smx_tmrcbs;       /* TMRCB pool */
extern u32        smx_tmo_indx;     /* index of minimum timeout */
extern u32        smx_tmo_min;      /* minimum waiting timeout value */
extern CB         smx_tqcb;         /* timer queue control block */
extern CB_PTR     smx_tq;           /* timer queue */
extern u32        smx_Version;      /* smx version number (consulted by smxAware) */
extern PCB        smx_xcbs;         /* XCB pool */

/* system objects */
extern TCB_PTR    smx_Idle;         /* idle task */
extern LCB_PTR    smx_KeepTimeLSR;
#if SMX_CFG_PROFILE
extern LCB_PTR    smx_ProfileLSR;
#endif
extern LCB_PTR    smx_TaskDeleteLSR;
extern LCB_PTR    smx_TimeoutLSR;
#endif /* SMX_XGLOB_H */

