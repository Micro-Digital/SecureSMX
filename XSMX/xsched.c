/*
* xsched.c                                                  Version 6.2.0
*
* smx scheduler and related functions.
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

#include "xsmx.h"

/* internal subroutines */
static bool    FixQCBFL(CB_PTR q);
static u32     GetCTRV(void);
static void    RepairRQ(void);
static void    smx_GetPoolStack(TCB_PTR task);
#if SMX_CFG_STACK_SCAN
static void    smx_StackScanB(void); /* scans a bound stack */
static void    smx_StackScanU(void); /* scans an unbound stack */
#endif

#if SMX_CFG_SSMX
#include "mparmm.h"
#endif

/*============================================================================
                                 CONSTANTS
============================================================================*/

/* This copyright must be retained in the binary image */
const char* smx_copyright1 = "SMX (R) v6.2.0 Copyright (c) 1988-2026";
const char* smx_copyright2 = "Micro Digital Inc. All rights reserved.";


/*============================================================================
                                 VARIABLES
============================================================================*/

extern u32 smx_sched_save;

TCB_PTR  smx_ctnew;     /* new current task. public for assembly macro access.*/
bool     smx_inssu;     /* in smx_StackScanU() */
u32      smx_psr;       /* copy of PSR */ 
RQCB_PTR rqnxt;


/*============================================================================
                           INITIALIZED VARIABLES
============================================================================*/

bool smx_eoos_once = true; /* out of stacks message once */

#if !SMX_CFG_SSMX
/* Abbreviated system service jump table if SecureSMX is not present */
u32 smx_sst[] = {
   (u32)smx_SchedAutoStop,
   (u32)smx_EM,
};
#endif

/*
*  smx_SchedRunLSRs()
*
*  LSR Scheduler is called from smx_PendSVHandler(), PSVH(), or from 
*  smx_SchedRunTasks() for LSR flybacks. smx_srnest > 0 prevents it and the LSR 
*  from being reentered. If lsr->flags.mode.trust == 1, the LSR is a trusted 
*  LSR that runs in hmode from here. If ...pmode == 1, the LSR is a safe LSR  
*  that runs in pmode. If ...umode == 1, the LSR is a safe LSR that runs in 
*  umode. For a safe LSR this function returns to PSVH(), which runs the LSR 
*  via an exception return. The LSR returns to PSVH() via an autostop, which
*  triggers a PSVH() exception call.
*/
bool smx_SchedRunLSRs(void)
{
   u32   par;

   while (smx_lqctr) /* smx interrupts must be disabled here */
   {
      sb_TM_START(&sb_ts1); /* beginning of LSR time measurements */

     #if SMX_CFG_DIAG
      /* update smx_lqhwm */
      if (smx_lqctr > smx_lqhwm)
         smx_lqhwm = smx_lqctr;
     #endif

      smx_lqctr--;

      sb_INT_ENABLE();

      /* get LSR and its parameter */
      smx_clsr = (LCB_PTR)smx_lqout->lsr;
      par = smx_lqout->par;

      /* update smx_lqout */
      smx_lqout++;
      if (smx_lqout > smx_lqx)
         smx_lqout = smx_lqi;

      if (smx_clsr->flags.mode.trust)
      {
         /* run trusted LSR */
        #if SMX_CFG_SSMX
         /* reload MPU for ct if an sLSR just ran */
         if (smx_psp_sav)
         {
            mp_MPULoad(true);
            smx_psp_sav = 0;
         }
        #endif

         smx_EVB_LOG_LSR(smx_clsr);
         smx_RTC_LSR_START();
         smx_clsr->fun(par);  /* run tLSR */
         sb_TM_LSR();         /* end of tLSR time measurement */
         smx_RTC_LSR_END();
         smx_EVB_LOG_LSR_RET(smx_clsr);
         smx_clsr = 0;
         sb_INT_DISABLE();
      }
     #if SMX_CFG_SSMX
      else
      {
         /* prepare to run safe LSR */
         mp_MPULoad(false);
         smx_StartSafeLSR(par);
         smx_EVB_LOG_LSR(smx_clsr);
         return true;         /* to smx_PendSV_Handler to run sLSR */
      }
     #endif
   }
   sb_INT_ENABLE();
   return false;
}

/*
*  smx_SchedRunTasks()
*
*  Task Scheduler is called from smx_PendSV_Handler(), PSVH(). smx_srnest is 
*  set to 1 prior to this call, which allows interrupts to run, but blocks LSRs
*  from running. This allows interrupts to be enabled most of the time,
*  without interfering with task scheduling. LSR flybacks ensure that LSRs
*  run quickly after being invoked. If stack overflow is detected for a task
*  being suspended or stopped smx_EM() is called. One-shot tasks are given a
*  stack from the stack pool and a stack MPU region is generated. When a one-
*  shot task is stopped, its stack is returned to the stack pool. Callbacks are 
*  implemented for EXIT, STOP, ENTER, and START. Recovery from ready queue 
*  damage is implemented.
* 
*/
bool smx_SchedRunTasks(void)
{
   do
   {
      if (smx_ct->flags.stk_chk == 1)
      {
        #if SB_CPU_ARMM7 /*<8>*/
         /* check for stack pad overflow */
         if (((u32)smx_ct->sp <= (u32)smx_ct->spp) ||
            (smx_ct->shwm >= (u32)smx_ct->sbp - (u32)smx_ct->spp)) /*<4>*/
         {
            smx_EM(SMXE_STK_OVFL, 2);
         }
         else
        #endif
         if (smx_ct->flags.stk_ovfl == 0)
         {
            /* check for stack overflow -- report first time only */
            if (((u32)smx_ct->sp <= (u32)smx_ct->stp) || (smx_ct->shwm >= smx_ct->ssz)) /*<4>*/
            {
               smx_ct->flags.stk_ovfl = 1;
               smx_EM(SMXE_STK_OVFL, 0);
            }
         }
      }

      if (smx_sched & SMX_CT_TEST || smx_sched & SMX_CT_SUSP)  /* suspend ct */
      {
         if (smx_sched == SMX_CT_TEST)
            smx_ct->state = SMX_TASK_READY; /*<9>*/

         /* call hooked exit function */
         if (smx_ct->flags.hookd == 1)
            smx_ct->cbfun(SMX_CBF_EXIT, 0); 
      }
      else if (smx_sched & SMX_CT_STOP)   /* stop ct */
      {
         smx_ct->sp = NULL;               /* mark task as stopped */

         if (smx_ct->flags.stk_perm == 0)
            smx_RelPoolStack(smx_ct);     /* free shared stack */

         /* call hooked stop function */
         if (smx_ct->flags.hookd)
            smx_ct->cbfun(SMX_CBF_STOP, 0);
      }
      smx_lockctr = 0;
      smx_EVB_LOG_TASK_END();
      sb_TM_END(sb_ts1, &sb_te[3]); /* end of stop or suspend */

get_top_task:
      sb_TM_START(&sb_ts2);      /* beginning of resume or start */
      smx_sched = SMX_CT_NOP;
      smx_ctnew = (TCB_PTR)smx_rqtop->fl; /* get top task */

dispatch_next_task:
      if ((smx_ctnew < (TCB_PTR)smx_tcbs.pi) || (smx_ctnew > (TCB_PTR)smx_tcbs.px)
         || (smx_ctnew->cbtype != SMX_CB_TASK))
      {
         RepairRQ();
         sb_INT_DISABLE();
         if (smx_lqctr > 0)
            if (smx_SchedRunLSRs()) /* run waiting LSRs */
               return true;         /* run safe LSR */
         sb_INT_ENABLE();
         goto get_top_task;         /* no ready task loop */
      }
     #if SMX_CFG_RTLIM
      /* check if runtime limit has been reached */
      u32 rtlim = (smx_ctnew->parent == NULL ? smx_ctnew->rtlim : 
                                             *(u32*)smx_ctnew->rtlim); /*<5>*/
      if (rtlim > 0) /*<6>*/
      {
         u32 rtlimctr = (smx_ctnew->parent == NULL ? smx_ctnew->rtlimctr : 
                                          *(u32*)smx_ctnew->rtlimctr); /*<5>*/
         if (rtlimctr >= rtlim)
         {
            /* suspend smx_ctnew */
            smx_DQRQTask(smx_ctnew);
            smx_NQTask((CB_PTR)smx_rtlimsem, smx_ctnew);
            goto get_top_task;
         }
      }
     #endif

      if (smx_ctnew->sp != NULL)
      {
         /* resume leg */
         sb_INT_DISABLE();     /* make task + stack switch atomic */
         smx_ct = smx_ctnew;   /* switch to new task */
         smx_SWITCH_STACKS();
         sb_INT_ENABLE();

         /* call hooked enter function */
         if (smx_ct->flags.hookd == 1)
            smx_ct->cbfun(SMX_CBF_ENTER, 0);

         /* initialize task */
        #if SMX_CFG_SSMX
         #if defined(SMX_TSMX)
         smx_ct->sv = (u32)smx_ct->sp;  /* save sp for tsmx tests that use it */
         #endif
        #endif

         smx_ct->flags.stk_hwmv = 0;
         smx_ct->state = SMX_TASK_RUN;

        #if SMX_CFG_SSMX
         /* load MPU from MPA of ct */
         mp_MPULoad(true);
        #endif

         /* LSR flyback */
         sb_INT_DISABLE();
         if (smx_lqctr > 0)
         {
            /* run waiting LSRs */
            if (smx_SchedRunLSRs())
               return true;
            else
            {
               sb_INT_ENABLE();
               /* flyback if smx_ct not top task*/
               if ((TCB_PTR)(smx_rqtop->fl) != smx_ct)
                  continue;                              
               else
                  smx_sched = SMX_CT_NOP; /* clear smx_sched in case set */
            }        
         }
         smx_EVB_LOG_TASK_RESUME();
         return false;  /* go to PSVH() tail to resume task */
      }
      else /* start leg */
      {
         /* get stack if not bound */
         if (smx_ctnew->flags.stk_perm == 0)
         {
            if (smx_freestack != NULL)
            {
               smx_GetPoolStack(smx_ctnew);
            }
            else /* stack pool empty */
            {
              #if SMX_CFG_STACK_SCAN
               if (smx_scanstack != NULL)
               {
                  if (smx_inssu) /* finish smx_StackScanU() to free a stack */
                  {
                     smx_idleup = true;
                     smx_ctnew = smx_Idle;
                  }
                  else
                     smx_StackScanU(); /* scan and free next stack */
               }
               else
              #endif
               {
                  if (smx_eoos_once)
                  {
                     smx_EM(SMXE_OUT_OF_STKS, 0);
                     smx_eoos_once = false;
                  }

                  /* find next ready task that has a stack */
                  rqnxt = smx_rqtop;
                  while (smx_ctnew->stp == NULL)
                  {
                     if (smx_ctnew->fl != (CB_PTR)rqnxt) /* search this level */
                        smx_ctnew = (TCB_PTR)smx_ctnew->fl;
                     else
                     {
                        if (rqnxt > smx_rq)  /* move to a lower level */
                           rqnxt--;
                        while ((rqnxt->tq == 0) && (rqnxt > smx_rq))
                        {
                           rqnxt--;
                        }
                        if (rqnxt->tq == 0)  /* no tasks to run */
                        {
                           sb_INT_DISABLE();
                           if (smx_lqctr > 0)
                              if (smx_SchedRunLSRs()) /* run waiting LSRs */
                                 return true;         /* run safe LSR */
                           sb_INT_ENABLE();
                           goto get_top_task;         /* out of stacks loop */
                        }
                        else
                           smx_ctnew = (TCB_PTR)rqnxt->fl;
                     }
                  }
               }
               goto dispatch_next_task;   /* dispatch the task found */
            }
         }

         smx_ct = smx_ctnew;              /* switch to new task */

         /* select autostop function */
         #if SMX_CFG_SSMX
         if (smx_ct->flags.umode == 0)
            smx_autostop = &smx_SchedAutoStop;
         else
            smx_autostop = &smxu_SchedAutoStop;
         #else
            smx_autostop = &smx_SchedAutoStop;
         #endif

         /* call hooked start function <7> */
         if (smx_ct->flags.hookd)
            smx_ct->cbfun(SMX_CBF_START, 0);

         /* initialize new task */
         smx_ct->flags.stk_hwmv = 0;
         smx_ct->flags.stk_ovfl = 0;
         smx_ct->state = SMX_TASK_RUN;
         if (smx_ct->flags.strt_lockd == 1)
            smx_lockctr = 1;
         else
            smx_lockctr = 0;

        #if SMX_CFG_SSMX
         /* load MPU from smx_ct MPA */
         mp_MPULoad(true);
        #endif

         smx_SWITCH_TO_NEW_STACK();
         smx_MakeFrame();

         /* LSR flyback */
         sb_INT_DISABLE();
         if (smx_lqctr > 0)
         {
            /* run waiting LSRs */
            if (smx_SchedRunLSRs())
               return true;
            else
            {
               sb_INT_ENABLE();
               /* check if smx_ct still top task */
               if (smx_ct != (TCB_PTR)(smx_rqtop->fl))
               {
                  smx_sched = SMX_CT_STOP;   /* cause stack release */
                  smx_ct->sp = smx_ct->sbp;  /* prevent false stack overflow error */
                  continue;                  /* start flyback */
               }
               else
                  smx_sched = SMX_CT_NOP;    /* clear smx_sched in case it has been set */
            }        
         }
         return false;  /* go to PSVH() tail to start task */
      }
   } while (1);
}

/* 
   SSR Enter Functions.  
*/
void smx_SSREnter0(u32 id)
{ 
   smx_srnest++;
   smx_EVB_LOG_SSR0(id);
   smx_ct->err = SMXE_OK; 
}

void smx_SSREnter1(u32 id, u32 p1)
{
   smx_srnest++;
   smx_EVB_LOG_SSR1(id, p1);
   smx_ct->err = SMXE_OK; 
}

void smx_SSREnter2(u32 id, u32 p1, u32 p2)
{
   smx_srnest++;
   smx_EVB_LOG_SSR2(id, p1, p2);
   smx_ct->err = SMXE_OK; 
}

void smx_SSREnter3(u32 id, u32 p1, u32 p2, u32 p3)
{
   smx_srnest++;
   smx_EVB_LOG_SSR3(id, p1, p2, p3);
   smx_ct->err = SMXE_OK; 
}

void smx_SSREnter4(u32 id, u32 p1, u32 p2, u32 p3, u32 p4)
{
   smx_srnest++;
   smx_EVB_LOG_SSR4(id, p1, p2, p3, p4);
   smx_ct->err = SMXE_OK; 
}

void smx_SSREnter5(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5)
{
   smx_srnest++;
   smx_EVB_LOG_SSR5(id, p1, p2, p3, p4, p5);
   smx_ct->err = SMXE_OK;
}

void smx_SSREnter6(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6)
{
   smx_srnest++;
   smx_EVB_LOG_SSR6(id, p1, p2, p3, p4, p5, p6);
   smx_ct->err = SMXE_OK; 
}

void smx_SSREnter7(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6, u32 p7)
{
   smx_srnest++;
   smx_EVB_LOG_SSR7(id, p1, p2, p3, p4, p5, p6, p7);
   smx_ct->err = SMXE_OK;
}

/* 
   SSR Exit -- used to exit SSRs that can cause a task switch  
*/
u32 smx_SSRExit(u32 rv, u32 id)
{
   smx_EVB_LOG_SSR_RET(rv, id);
   if (smx_srnest == 1)
   {
      smx_RTC_TASK_END();
      sb_INT_DISABLE();
      if ((smx_sched > 0) || (smx_lqctr > 0))
      {
         smx_ct->rv = rv;     /* save in case smx_ct suspended */
         if ((smx_GetPSR() & 0x1FF) != 0xE) /* if not in PendSV handler <3> */
         {
            smx_PENDSVH();    /* PSVH runs here, unless in SVC <1>. Enables int. */
         }
         else
         {
            smx_srnest = 0;   /* necessary for deferred action function */ 
         }
         return(GetCTRV());   /* ct->rv != rv if ct changed <1> */
      }
      smx_srnest = 0;
      sb_INT_ENABLE();
      smx_CLEAR_SUSPLOC();
      smx_RTC_TASK_START();
   }
   else
   {
      if (smx_srnest > 0)
         smx_srnest--;
      sb_INT_ENABLE();
   }
   return rv; /* continue smx_ct, LSR, or SSR */
}

/* 
   SSR Exit Internal Function -- used by smx_MutexGetF() and smx_MutexRelF()
   to wait inside of SSR calling them.
*/
u32 smx_SSRExitIF(u32 rv)
{
   if (smx_clsr == 0)
   {
      sb_INT_DISABLE();
      if ((smx_sched > 0) || ((smx_srnest == 1) && (smx_lqctr > 0)))
      {
         smx_RTC_TASK_END();
         smx_ct->rv = rv;     /* save in case smx_ct suspended */
         smx_ct->srnest = smx_srnest;
         smx_srnest = 1;
         if ((smx_GetPSR() & 0x1FF) != 0xE) /* verify not in PSVH() <3> */
         {
            smx_PENDSVH();    /* PSVH runs here, unless in SVC <1> */
         }
         sb_INT_ENABLE();         
         /* ct resumes here */
         rv = smx_ct->rv;
         smx_srnest = smx_ct->srnest;
         smx_RTC_TASK_START();
      }
      sb_INT_ENABLE();
      smx_srnest--;
   }
   return rv;
}

#pragma diag_default=Pe940  /* restore warning */

/*
*  smx_SchedAutoStop for ARMM does the same as the normal autostop code above,
*  except that instead of continuing from within the scheduler, it triggers
*  smx_PendSV_Handler(), which calls the scheduler in handler mode. For a ptask, 
*  it is called directly by return from the last task main }. For a utask, 
*  smxu_SchedAutoStop() is called, instead. It invokes svc AS, which switches 
*  the processor to handler mode and jumps here via smx_sst[AS]. Calling of 
*  smx_ or smxu_ is determined by smx_MakeFrame() in xarmm_iar.s.
*
*  smx_SchedAutoStopLSR() is defined in assembly in xarmm_iar.s to avoid a
*  compiler code generation issue.
*/
void smx_SchedAutoStop(void)
{
   smx_RTC_TASK_END();
   smx_EVB_LOG_TASK_AUTOSTOP();
   smx_srnest = 1;
   smx_DQRQTask(smx_ct);
   smx_sched = SMX_CT_STOP;
   smx_PENDSVH();  /* trigger smx_PendSV_Handler() */
}

/*
*  Stack Scan
*
*  Called from the idle task, once per pass of idle. If a stack is found in the
*  scanstack pool, smx_StackScanU() is called to scan it. If the scanstack pool
*  is empty, smx_StackScanB() is called to scan the next bound stack. Unbound
*  stacks are given precedence to speed up one-shot task dispatching. Tests
*  smx_mshwm for sufficient headroom during debug and for overflow during run.
*  Periodically scans the main stack instead of a task stack.
*/
void smx_StackScan(void)
{
  #if SMX_CFG_STACK_SCAN
   /* scan main stack */
   if (smx_mshwmv == false)
   {
      smx_mshwm = sb_MS_GET_SIZE() - sb_MSScan();
      smx_mshwmv = true;
      if (smx_mshwm >= sb_MS_GET_SIZE())
         smx_ERROR(SMXE_MSTK_OVFL, 2);
      return;
   }

   /* scan next task stack */
   if (smx_scanstack != NULL)
   {
      smx_inssu = true;
      smx_StackScanU();
      smx_inssu = false;
   }
   else
      smx_StackScanB();
  #endif
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  Fix QCB Forward Link.
*
*  Assumes q->fl != NULL. Searches backward until it finds the break, then
*  joins last CB to the QCB and returns true. Aborts and returns false if
*  queue cannot be fixed.
*/
bool FixQCBFL(CB_PTR q)  /* smx_srnest must be > 0 */
{
   CB_PTR  cb;

   /* abort if can't be fixed */
   if (q->bl < (CB_PTR)smx_tcbs.pi || q->bl > (CB_PTR)smx_tcbs.px
      || q->bl->cbtype != SMX_CB_TASK)
      return(false);

   /* search backward for the break */
   cb = (CB_PTR)q;
   while (cb->bl->fl == cb)
   {
      cb = cb->bl;
      if (cb < (CB_PTR)smx_tcbs.pi || cb > (CB_PTR)smx_tcbs.px
         || cb->cbtype != SMX_CB_TASK  || cb->bl == cb)
         return(false);
   }

   /* mend the break */
   q->fl = cb;
   cb->bl = (CB_PTR)q;
   return(true);
}

u32 GetCTRV(void)  /* function necessary to avoid a code generation problem */
{
   return smx_ct->rv;
}

/*
*  Repair smx_rq
*
*  Assumes smx_rqtop may be damaged, so searches from the top of smx_rq for an
*  occupied level. If it finds one, it checks that the forward link is valid.
*  If not, it reports SMXE_BROKEN_Q, calls FixQCBFL() to attempt to fix the forward
*  link, then reports SMXE_Q_FIXED, if successful. Note that if an smx_rq level
*  cannot be fixed, it is set to empty. Hence, there will be only one SMXE_BROKEN_Q
*  error reported per broken smx_rq level.
*/
void RepairRQ(void)  /* smx_srnest must be > 0 */
{
   RQCB_PTR q;

   /* reset smx_rqtop */
   smx_rqtop = smx_rq + SMX_PRI_NUM-1;
   while ((smx_rqtop->tq == 0) && (smx_rqtop > smx_rq))
      smx_rqtop--;

   if (smx_rqtop->tq == 0) /* no ready task found */
      return;

   /* test for broken queue */
   if ((smx_rqtop->fl < (TCB_PTR)smx_tcbs.pi) || (smx_rqtop->fl > (TCB_PTR)smx_tcbs.px)
                                   || (smx_rqtop->fl->cbtype != SMX_CB_TASK))
   {
      smx_EM(SMXE_RQ_ERROR, 0);
      if (FixQCBFL((CB_PTR)smx_rqtop))
      {
         smx_EM(SMXE_Q_FIXED, 0);
      }
      else
      {
         q = smx_rqtop;
         q->tq = 0;  /* mark as empty if can't be fixed */
         q->fl = NULL;
      }
   }
}

/*
*  smx_GetPoolStack()
*
*  Gets a stack from the stack pool, loads its TCB stack pointers and size, and
*  sets smx_eoos_once. If SMX_CFG_SSMX, also loads the stack region into the 
*  task's MPA. Stack pool stacks are 8-byte aligned.
*/
void smx_GetPoolStack(TCB_PTR task)
{
   /* get stack and load TCB fields */
   task->spp = (u8*)smx_freestack;
   task->stp = (u8*)((u32)task->spp + SMX_SIZE_STACK_PAD);
   task->sbp = (u8*)((u32)task->stp + SMX_SIZE_STACK);
   task->ssz = SMX_SIZE_STACK;
   smx_eoos_once = true;

  #if SMX_CFG_SSMX
   if (task->mpap != mpa_dflt)
   {
      u32* mp = mp_MPA_PTR(task, (MP_MPU_ACTVSZ - 1));
      u32  bp = (u32)smx_freestack;
      u32  sz = SMX_SIZE_STACK_BLK;

      /* load task stack region into the task's MPA */
     #if SB_CPU_ARMM7
      *mp++ = bp | 0x10 | (MP_MPU_SZ - 1);
      *mp   = 0x13020000 | ((30-__CLZ(sz)) << 1) | 1;
      #if MP_MPA_DEV
      *++mp = (u32)"stack";
      #endif
     #elif SB_CPU_ARMM8
      if (task->flags.umode)
      {
         *mp++ = bp | 0x3;
         *mp   = ((bp + sz - 1) & 0xFFFFFFE0) | 1;
         #if MP_MPA_DEV
         *++mp = (u32)"stack";
         #endif
      }
     #endif /* SB_CPU_ARMM8*/
   }
  #endif  /* SMX_CFG_SSMX */

   smx_freestack = *(void**)smx_freestack;
   *(u32*)task->spp = SB_STK_FILL_VAL;
}

#if SMX_CFG_STACK_SCAN
/*
*  Stack Scan Bound
*
*  The next task to be scanned in the TCB table, tcbns, is tested for a stack.
*  If it has one, the stack is scanned, as above, and tcbns->shwm is updated
*  and tcbns->flags.stk_hwmv flag is set, unless the stack just scanned is SS.
*  Note: it is possible that tcbns has run again and used more stack. tcbns is
*  incremented cyclically for the next pass.
*/
void smx_StackScanB(void)
{
   TCB_PTR  onr; /* current owner of stack */
   u32  *p, *ep;
   u8   *stp;

   if ((smx_tcbns->stp != NULL) && (smx_tcbns->flags.stk_hwmv == 0))
   {
      smx_TaskLock(); /* prevent tcbns fields from being changed */
      onr = smx_tcbns;
      p = (u32*)onr->spp;
      ep = (u32*)onr->sbp;
      stp = onr->stp;
      smx_TaskUnlock();

      /* update shwm */
      for (; p < ep && *p == SB_STK_FILL_VAL; p++) {}
      if (onr->stp != NULL && onr->stp == stp)
      {
         /* avoid erroneous report if released <2> */
         smx_LSRsOff();
         onr->shwm = (ep - p)*4;
         onr->flags.stk_hwmv = 1;
         smx_LSRsOn();
      }
   }

   /* update tcbns for next pass */
   if (smx_tcbns == (TCB_PTR)smx_tcbs.px)
      smx_tcbns = (TCB_PTR)smx_tcbs.pi;
   else
      smx_tcbns++;
}

/*
*  Stack Scan Unbound
*
*  A stack in the scanstack pool is scanned from the stack pad down to the end of
*  the pattern, then its previous owner's shwm is updated, even if it now has
*  another stack. Its stk_hwmv flag is set only if it is stopped. Then the rest
*  of the stack is pattern-filled and it is put into the freestack pool for
*  reuse.
*
*  If the scheduler cannot run the top task because it needs a stack and there
*  is a stack in the scanstack pool, this function will be called to scan it
*  and move it to the freestack pool. There are conditions under which this
*  can be called even though the scanstack pool is empty, so it must be
*  tested.
*/
void smx_StackScanU(void)
{
   TCB_PTR  ponr;  /* previous owner of stack or NULL if owner deleted */
   u32  *p, *ep;

   if (smx_scanstack != NULL)
   {
      p = (u32*)smx_scanstack;
      ep = p + (SMX_SIZE_STACK + SMX_SIZE_STACK_PAD)/4;
      p++;
      ponr = (TCB_PTR)*p;
      *p = SB_STK_FILL_VAL;

      /* search for new shwm and load if larger */
      for ( ; *p == SB_STK_FILL_VAL; p++) {}
      if (ponr != NULL)
      {
         smx_LSRsOff();
         if (((u32)((ep - p)*4) > ponr->shwm) && p < ep)
            ponr->shwm = (ep - p)*4;
         if (ponr->stp == NULL)   /* set shwm valid flag only if ponr stopped */
            ponr->flags.stk_hwmv = 1;
         smx_LSRsOn();
      }
      for ( ; p < ep ; p++)     /* fill rest of stack to its end */
         *p = SB_STK_FILL_VAL;

      /* move stack from scan list to free list atomically */
      p = (u32*)smx_scanstack;  /* reset p to the top of stack */
      smx_LSRsOff();
      smx_scanstack = *(void**)p;
      if (smx_scanstack == NULL)
         smx_scanstack_end = &smx_scanstack;
      *(void**)p = smx_freestack;
      smx_freestack = (void*)p;
      smx_LSRsOn();
   }
}
#endif /* SMX_CFG_STACK_SCAN */

/*
   Notes:
      Abbreviations:
      PSVH()   smx_PendSV_Handler()
      SVCH()   smx_SVC_Handler()
   1. PSVH() normally runs at the point of trigger in smx_SSRExit(), but not
      when the SSR is run in SVCH(). In that case, it pends, SVCH() completes,
      and tail-chains to PSVH(). In this case, GetCTRV() cannot return the 
      final return value because the task switch and the complementary SSR 
      have yet to run. So, PSVH() passes the final return value to the 
      suspended task via the R0 position in the exception frame.
   2. If a one-shot task's stack was about to be scanned by smx_StackScanB(),
      but then that task preempted the scan (Idle) and ran to completion,
      it would release the stack to the scan pool, changing its top 2 words.
      Returning here, the scan would start and see those changed words
      and erroneously report overflow. This check ensures that the same stack
      is still assigned to the task. Note that the stack could not be
      released and reassigned to the same task (which would void the check)
      because it stays in the scan pool until smx_StackScanU() runs.
   3. Invoking PSVH() while in PSVH -> Hard Fault.
   4. <= and >= are necessary to avoid damage above or below the stack. 
   5. A child task uses its top parent task's rtlim and rtlimctr.
   6. If rtlim == 0, smx_ctnew has no runtime limit. Note: Use smx_TaskSet
      (task, SMXE_ST_RTLIM, limit) to set a task's rtlim before starting it.
   7. Must occur after autostop loaded so cbfun(START) can change it to a 
      custom autostop function, if necessary.
   8. For ARMM8, PSPLIM detects stack pad overflow.
   9. If SMX_CT_SUSP, task state is already SMX_TASK_WAIT and task is not in rq.
*/ 

