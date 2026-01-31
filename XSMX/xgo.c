/*
* xgo.c                                                     Version 6.0.0
*
* smx_Go() initializes smx and starts the task scheduler.
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
* Authors: Alan Moore, David Moore, Ralph Moore
*
*****************************************************************************/

#include "xsmx.h"

/* internal subroutines */
static void smx_RQInit(void);

/*
*  smx_Go()   Function
*
*  Initialize smx and start task mode with idle at PRI_SYS and ainit() main
*  function.
*/
void smx_Go(void)
{
   u32 size;

   sb_TM_INIT();        /* initialize precise time measurement routines */
   sb_StimeSet();       /* initialize smx_stime */

   if (!smx_EMInit())   /* initialize error manager */
      smx_ERROR(SMXE_SMX_INIT_FAIL, 2);

  #if SMX_CFG_EVB
   if (!smx_EVBInit(SMX_EVB_EN_ALL & 0xFF7FFFFF)) /* initialize event logging <2> */
      smx_ERROR(SMXE_SMX_INIT_FAIL, 2);
  #endif

  #if !defined(SMX_DEBUG)
   sb_handler_en = true;  /* enable fault handlers <1> */
  #endif

   #if SMX_CFG_STACK_SCAN
   smx_tcbns = (TCB_PTR)smx_tcbs.pi;  /* start stack scan with first task */
   #endif

   /* allocate space for the LSR queue and initialize it */
   size = sizeof(LQC)*SMX_SIZE_LQ;
   smx_lqi = (LQC_PTR)smx_HeapMalloc(size, sizeof(LQC));
   if (smx_lqi)
   {
      memset(smx_lqi, 0, size);
      smx_lqin = smx_lqi;
      smx_lqout = smx_lqi;
      smx_lqx = smx_lqi + SMX_SIZE_LQ - 1;
   }
   else
   {
      smx_EM(SMXE_INSUFF_HEAP, 0);
      smx_EM(SMXE_SMX_INIT_FAIL, 2);
   }

   /* initialize ready queue */
   smx_RQInit();
   smx_HT_ADD(smx_rq, "smx_rq");

   /* initialize timer queue */
   smx_tq->cbtype = SMX_CB_TQ;
   smx_tq->name = "smx_tq";
   smx_HT_ADD(smx_tq, "smx_tq");

   /* initialize smx_timeout array */
   for (u32 i = 0; i < SMX_NUM_TASKS; i++)
      smx_timeout[i] = SMX_TMO_INF;

   smx_HT_ADD(&smx_dtcb, "dummy (init)");  /* register dummy TCB */

   /* create smx trusted LSRs */
   smx_KeepTimeLSR = smx_LSRCreate(smx_KeepTimeLSRMain, SMX_FL_TRUST, "smx_KeepTimeLSR");
   smx_TaskDeleteLSR = smx_LSRCreate(smx_TaskDeleteLSRMain, SMX_FL_TRUST, "smx_TaskDeleteLSR");
   smx_TimeoutLSR    = smx_LSRCreate(smx_TimeoutLSRMain, SMX_FL_TRUST, "smx_TimeoutLSR");

  #if SMX_CFG_PROFILE
   smx_ProfileLSR = smx_LSRCreate(smx_ProfileLSRMain, SMX_FL_TRUST, "smx_ProfileLSR");
  #endif

  #if SMX_CFG_RTLIM
   /* create runtime limit gate semaphore */
   smx_rtlimsem = smx_SemCreate(SMX_SEM_GATE, 1, "rtlimsem"); 
  #endif

  #if SMX_CFG_SSMX && SMX_CFG_DIAG
   /* allocate system service counter array and clear it <3> */
   smx_sst_ctr = (u32*)smx_HeapMalloc(smx_sstp[0]*sizeof(u32));
   memset(smx_sst_ctr, 0, 4*smx_sstp[0]);
  #endif

  #if defined(SMXAWARE)
   smxaware_init();
  #endif

   /* create smx_Idle with ainit() as its initial code and maximum priority */
   smx_Idle = smx_TaskCreate(ainit, PRI_SYS, SMX_SIZE_STACK_IDLE, 0, "idle");
   if (!smx_Idle)
      smx_EM(SMXE_SMX_INIT_FAIL, 2);

 #if SMX_CFG_SSMX
   /* create MPA for smx_Idle */
  #ifdef SMX_TSMX
   #if SB_CPU_ARMM8
   mp_MPACreate(smx_Idle, &mpa_tmplt_tinit, 0x7F, MP_MPU_ACTVSZ+2);
   #else
   mp_MPACreate(smx_Idle, &mpa_tmplt_tinit, 0xF, MP_MPU_ACTVSZ+2);
   #endif
  #else
   #if SB_CPU_ARMM8
   mp_MPACreate(smx_Idle, &mpa_tmplt_init, 0x7, 9);
   #else
   mp_MPACreate(smx_Idle, &mpa_tmplt_init, 0xF, 8);
   #endif
  #endif
 #endif

   smx_init = true;

   /* start smx_Idle */
   smx_srnest = 1; /* prevent premature start */
   smx_sched = SMX_CT_STOP;
   smx_TaskStart(smx_Idle);
   smx_SwitchToPSP();
   smx_PENDSVH();  /* trigger smx_PendSV_Handler() -- does not return */
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

#define    RQ_NAMES_LEN  6  /* length names up to 99 */
#define    RQ_NAMES_MAX 15  /* number of names in rqln[] */

/* ready queue level names ("rq[n]\0") */
const char rqln[] = "rq[0]\0rq[1]\0rq[2]\0rq[3]\0rq[4]\0rq[5]\0rq[6]\0rq[7]\0"
                    "rq[8]\0rq[9]\0rq[10]\0rq[11]\0rq[12]\0rq[13]\0rq[14]\0";

/*
*  smx_RQInit()
*
*  Creates the ready queue as a num_level array of XCBs. Initializes XCBs to be
*  the queue heads for priority levels beginning at 0. smx_rqtop is set to
*  point to the lowest priority level since rq is empty. Loads names into RQCBs.
*/
static void smx_RQInit(void)
{
   int      i;
   RQCB_PTR rq;
   int      sz;
   const char *p = rqln;

   /* initialize rq levels from last to first */
   smx_rqx = smx_rq + SMX_PRI_NUM - 1;
   for (rq = smx_rqx, i = (SMX_PRI_NUM-1); i > 0; i--, rq--)
   {
      rq->cbtype = SMX_CB_SUBLV;
      rq->tplim = (u16)i;
   }
   rq->cbtype = SMX_CB_RQ;
   smx_rqtop = rq;

   /* load names for each rq level up to RQ_NAMES_MAX into RQCBs */
   sz = RQ_NAMES_LEN;
   for (i = 0; i < SMX_PRI_NUM && i < RQ_NAMES_MAX; i++, p += sz)
   {
      (rq + i)->name = p;

      /* adjust for larger sizes after levels 9 and 99 */
      if (i == 10 || i == 100)
         sz += 1;
   }
}

/* Notes:
   1. sb_handler_en = false results in halting on a fault in order to enable 
      using the call stack to find its cause. During normal operation the 
      fault handler runs, instead -- see vectors.c.
   2. Except group 8 SSRs -- see xdef.h.
   3. sst[] is the system service table defined in svc.c for use by the SVC
      handler. sst[0] = size of sst[]. The smx_sst_ctr array keeps track of
      the number of times each system service is used from umode.
*/