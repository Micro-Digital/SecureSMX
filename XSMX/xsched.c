/*
* xsched.c                                                  Version 5.4.0
*
* smx scheduler and related functions.
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

/* internal subroutines */
static bool    FixQCBFL(CB_PTR q);
static u32     GetCTRV(void);
static void    RepairRQ(void);
static void    smx_GetPoolStack(TCB_PTR task);
static void    smx_StackScanB(void); /* scans a bound stack */
static void    smx_StackScanU(void); /* scans an unbound stack */

#if SMX_CFG_SSMX
#include "mparmm.h"
#endif

/*============================================================================
                                 CONSTANTS
============================================================================*/

/* This copyright must be retained in the binary image */
const char* smx_copyright1 = "SMX (R) v5.4.0 Copyright (c) 1988-2025";
const char* smx_copyright2 = "Micro Digital Inc. All rights reserved.";


/*============================================================================
                                 VARIABLES
============================================================================*/

extern u32 smx_sched_save;

TCB_PTR smx_ctnew;      /* new current task. public for assembly macro access.*/
bool    smx_inssu;      /* in smx_StackScanU() */
u32     smx_psr;        /* copy of PSR */
static RQCB_PTR rqnxt;


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
*  LSR Scheduler. This function is called from smx_PendSVHandler() and from 
*  smx_SchedRunTasks() for LSR flybacks. smx_srnest > 0, set prior to call,
*  prevents smx_SchedRunLSRs() and lsr from being reentered. Hence, interrupts
*  can be enabled except when updating smx_lqhwm. If lsr->flag.sys == 1, lsr
*  runs in hmode and is called a trusted LSR, tLSR. If lsr->flags.trust == 0
*  and lsr->flags.umode == 0, lsr runs in pmode and is called a pLSR. If 
*  lsr->flags.trust == 0 and lsr->flags.umode == 1, lsr runs in umode and is
*  called a uLSR.
*/
bool smx_SchedRunLSRs(u32 reload)
{
   u32   par;

   while (smx_lqctr) /* Note: smx interrupts must be disabled here */
   {
      sb_TM_START(&sb_ts1); /* beginning of LSR time measurements */

      /* update smx_lqhwm and decrement smx_lqctr */
      if (smx_lqctr > smx_lqhwm)
         smx_lqhwm = smx_lqctr;
      smx_lqctr--;

      sb_INT_ENABLE();
      /* get LSR and its parameter */
      smx_clsr = (LCB_PTR)smx_lqout->lsr;
      par = smx_lqout->par;

      /* update smx_lqout */
      smx_lqout++;
      if (smx_lqout > smx_lqx)
         smx_lqout = smx_lqi;

      /* call LSR */
      if (smx_clsr->flags.trust)
      {
        #if SMX_CFG_SSMX
         /* reload MPU for ct if a safe LSR (uLSR or pLSR) just ran */
         if (reload)
            mp_MPULoad(true);
        #endif
         /* run trusted LSR (tLSR) */
         smx_EVB_LOG_LSR(smx_clsr);
         smx_RTC_LSR_START();
         smx_clsr->fun(par);  /* run LSR */
         sb_TM_LSR();         /* end of tLSR time measurement */
         smx_RTC_LSR_END();
         smx_EVB_LOG_LSR_RET(smx_clsr);
         smx_clsr = 0;
         sb_INT_DISABLE();
      }
      else
      {
        #if SMX_CFG_SSMX
         /* prepare to run safe LSR */
         mp_MPULoad(false);
         smx_StartSafeLSR(par);
         smx_EVB_LOG_LSR(smx_clsr);
         sb_INT_DISABLE();
         return true;         /* run LSR (return to smx_PendSV_Handler) */
        #endif
      }
   }
   return false;
}

/*
*  smx_SchedRunTasks()
*
*  Task Scheduler. This function is called from smx_ISRExit(), smx_SSRExit(),
*  and smx_Go(). smx_srnest is set to 1 prior to these calls and ensures
*  that ISRs return to the point of interrupt within the scheduler. Hence,
*  interrupts can be enabled during most parts of this scheduler without
*  interfering with the task scheduling process. LSR flybacks ensure that if an
*  LSR becomes ready to run due to an interrupt, it will run ahead of the task
*  being scheduled. Then a flyback occurs in case a higher priority task has
*  become ready to run. Normally the idle task should always be ready to run.
*  However, if not, the scheduler tries to recover if smx_rqtop or smx_rq have
*  been damaged, then waits for an ISR to invoke an LSR that starts or resumes
*  a task. Prior to stopping, suspending, or continuing a task, a test is made
*  to determine if there is or has been a stack overflow. If so, smx_EM() is
*  called, which may permanently stop the task. Runs in System Stack, SS.
*/
void smx_SchedRunTasks(void)
{
   do /* Note: smx interrupts must be disabled or masked here */
   {
      if (smx_ct->flags.stk_chk == 1)
      {
        #if SB_CPU_ARMM7
         /* check for stack pad overflow <10> */
         if (((u32)smx_ct->sp <= (u32)smx_ct->spp) ||
            (smx_ct->shwm >= (u32)smx_ct->sbp - (u32)smx_ct->spp)) /*<6>*/
         {
            smx_EM(SMXE_STK_OVFL, 2);
         }
         else
        #endif
         if (smx_ct->flags.stk_ovfl == 0)
         {
            /* check for stack overflow -- report first time only */
            if (((u32)smx_ct->sp <= (u32)smx_ct->stp) || (smx_ct->shwm >= smx_ct->ssz)) /*<6>*/
            {
               smx_ct->flags.stk_ovfl = 1;
               smx_EM(SMXE_STK_OVFL, 0);
            }
         }
      }

      /* check whether to suspend or stop ct */
      if (smx_sched & SMX_CT_TEST || smx_sched & SMX_CT_SUSP)
      {
         sb_INT_ENABLE();
         /* if ct is being preempted but staying in rq change state to READY */
         if (smx_sched == SMX_CT_TEST)
            smx_ct->state = SMX_TASK_READY;

         /* call hooked exit function */
         if (smx_ct->flags.hookd == 1)
            smx_ct->cbfun(SMX_CBF_EXIT, 0); 
      }
      /* check whether to stop ct */
      else if (smx_sched & SMX_CT_STOP)
      {
         sb_INT_ENABLE();
         smx_ct->sp = NULL; /* mark task as stopped */

         if (smx_ct->flags.stk_perm == 0) /* free shared stack */
         {
            sb_INT_DISABLE(); /* make stack + task switch atomic */
            smx_RelPoolStack(smx_ct);
            sb_INT_ENABLE();
         }
         /* call hooked stop function */
         if (smx_ct->flags.hookd)
            smx_ct->cbfun(SMX_CBF_STOP, 0);
      }
      smx_lockctr = 0;   /* clear lock counter */
      smx_EVB_LOG_TASK_END();
      sb_TM_END(sb_ts1, &sb_te[0]); /* end of stop or suspend */

get_top_task:
      sb_TM_START(&sb_ts2);   /* beginning of resume or start */
      sb_INT_ENABLE();        /* make sure ISRs can run if looping here */
      smx_sched = SMX_CT_NOP;
      smx_ctnew = (TCB_PTR)smx_rqtop->fl; /* get top task */

dispatch_next_task:
      if ((smx_ctnew < (TCB_PTR)smx_tcbs.pi) || (smx_ctnew > (TCB_PTR)smx_tcbs.px)
         || (smx_ctnew->cbtype != SMX_CB_TASK))
      {
         RepairRQ();
         sb_INT_DISABLE();
         if (smx_lqctr > 0)
            smx_SchedRunLSRs(0); /* run waiting LSRs */
         goto get_top_task;      /* no-task-to-run loop */
      }

     #if SMX_CFG_RTLIM
      /* check if runtime limit has been reached */
      u32 rtlim = (smx_ctnew->parent == NULL ? smx_ctnew->rtlim : *(u32*)smx_ctnew->rtlim); /*<7>*/
      if (rtlim > 0) /*<8>*/
      {
         u32 rtlimctr = (smx_ctnew->parent == NULL ? smx_ctnew->rtlimctr : *(u32*)smx_ctnew->rtlimctr); /*<7>*/
         if (rtlimctr >= rtlim)
         {
            /* suspend smx_ctnew */
            smx_DQRQTask(smx_ctnew);
            smx_NQTask((CB_PTR)smx_rtlimsem, smx_ctnew);
            goto get_top_task;
         }
      }
     #endif

      if (smx_ctnew->sp != NULL) /* resume ctnew */
      {
         /* resume smx_ctnew */
         sb_INT_DISABLE();     /* make task + stack switch atomic */
         smx_ct = smx_ctnew;   /* switch to new task */
         smx_SWITCH_STACKS();
         sb_INT_ENABLE();
         smx_EVB_LOG_TASK_RESUME();

         /* call hooked enter function */
         if (smx_ct->flags.hookd == 1)
            smx_ct->cbfun(SMX_CBF_ENTER, 0);

         /* LSR flyback */
         sb_INT_DISABLE();
         if (smx_lqctr > 0)
         {
            smx_SchedRunLSRs(0);                    /* run waiting LSRs */
            if ((TCB_PTR)(smx_rqtop->fl) != smx_ct) /* check if smx_ct still top */
               continue;                            /* resume flyback */
            else
               smx_sched = SMX_CT_NOP;    /* clear smx_sched in case it has been set */
         }

         /* continue -- no flyback, interrupts disabled */
        #if SMX_CFG_SSMX
         #if defined(SMX_TSMX)
         smx_ct->sv = (u32)smx_ct->sp;  /* save sp for a few TSMX tem tests that use it */
         #endif
         smx_ct->sp = NULL;
        #endif

         smx_ct->flags.stk_hwmv = 0;
         smx_ct->state = SMX_TASK_RUN;

        #if SMX_CFG_SSMX
         /* load MPU from MPA of ctnew */
         mp_MPULoad(true);
        #endif

         return;  /* resume ctnew */
      }
      else /* start smx_ctnew */
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
                        if (rqnxt->tq == 0) /* no tasks to run */
                        {
                           sb_INT_DISABLE();
                           if (smx_lqctr > 0)
                              smx_SchedRunLSRs(0); /* run waiting LSRs */
                           goto get_top_task;      /* out of stacks loop */
                        }
                        else
                           smx_ctnew = (TCB_PTR)rqnxt->fl;
                     }
                  }
               }
               goto dispatch_next_task;  /* dispatch the task found */
            }
         }
         /* continue -- have stack */
         smx_ct = smx_ctnew;        /* switch to new task */

         /* select autostop function */
         #if SMX_CFG_SSMX
         if (smx_ct->flags.umode == 0)
            smx_autostop = &smx_SchedAutoStop;
         else
            smx_autostop = &smxu_SchedAutoStop;
         #else
            smx_autostop = &smx_SchedAutoStop;
         #endif

         /* call hooked start function <9> */
         if (smx_ct->flags.hookd)
            smx_ct->cbfun(SMX_CBF_START, 0);

         /* LSR flyback */
         sb_INT_DISABLE();
         if (smx_lqctr > 0)         /* check for flyback */
         {
            smx_SchedRunLSRs(0);                     /* run waiting LSRs */
            if ((TCB_PTR)(smx_rqtop->fl) != smx_ct)  /* check if smx_ct still top */
            {
               smx_sched = SMX_CT_STOP;   /* cause stack release */
               smx_ct->sp = smx_ct->sbp;  /* prevent false stack overflow error */
               continue;                  /* start flyback */
            }
            else
               smx_sched = SMX_CT_NOP;    /* clear smx_sched in case it has been set */
         }

         /* continue -- no flyback, interrupts are disabled */
         smx_ct->flags.stk_hwmv = 0;
         smx_ct->flags.stk_ovfl = 0;
         smx_ct->state = SMX_TASK_RUN;
         smx_ctstart = true;
         if (smx_ct->flags.strt_lockd == 1)
            smx_lockctr = 1;
         else
            smx_lockctr = 0;
         smx_EVB_LOG_TASK_START();

        #if SMX_CFG_SSMX
         /* load MPU from smx_ct MPA */
         mp_MPULoad(true);
        #endif

         smx_SWITCH_TO_NEW_STACK();
         smx_MakeFrame();
         return;  /* go to PSVH() tail to start task */
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
u32 smx_SSRExit(u32 SsrReturnValue, u32 id)
{
   smx_EVB_LOG_SSR_RET(SsrReturnValue, id);
   if (smx_srnest == 1)
   {
      smx_RTC_TASK_END();
      sb_INT_DISABLE();
      if ((smx_sched > 0) || (smx_lqctr > 0))
      {
         smx_ct->rv = SsrReturnValue; /* save in case smx_ct suspended */
         if ((smx_GetPSR() & 0x1FF) != 0xE) /* if not in PendSV handler <5> */
         {
            smx_PENDSVH();  /* PSVH runs here, unless in SVC <3> */
         }
         return(GetCTRV()); /* (ct->rv may != SsrReturnValue) <3>. */
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
   return SsrReturnValue; /* continue smx_ct, LSR, or SSR */
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
         if ((smx_GetPSR() & 0x1FF) != 0xE) /* verify not in PendSV handler <5> */
         {
            smx_PENDSVH();    /* PSVH runs here, unless in SVC <3> */
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
   sb_INT_ENABLE();   /* in case auto stop is called with interrupts disabled */
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
      smx_mshwmv = true;
      if (sb_MSScan() >= sb_MS_GET_SIZE())
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
      if (onr->stp != NULL && onr->stp == stp)  /* avoid erroneous report if released <4> */
      {
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
   1. (deleted)
   2. Newer versions of GNU have added an optimization that saves the
      address of smx_ct on the stack and references it rather than the
      literal pool, but the reference is done after smx_SWITCH_STACKS()
      in smx_SchedRunTasks(), causing failure. We could not isolate
      which optimization causes this. -O0, -O1, -Os ok; -O2, -O3 not.
   3. smx_PendSVHandler normally runs at the point of trigger in
      smx_SSRExit(), but not when the SSR is run using a SVC exception.
      In that case, it pends until SVC handler exits and tail-chains to it.
      The GetCTRV() operation is useless, since the task switch and
      complementary SSR have not yet run. The complementary SSR passes
      the return value to the suspended task via smx_PUT_RV_IN_EXR0(),
      which puts it in the R0 position in its stack.
   4. If a one-shot task's stack was about to be scanned by smx_StackScanB(),
      but then that task preempted the scan (Idle) and ran to completion,
      it would release the stack to the scan pool, changing its top 2 words.
      Returning here, the scan would start and see those changed words
      and erroneously report overflow. This check ensures that the same stack
      is still assigned to the task. Note that the stack could not be
      released and reassigned to the same task (which would void the check)
      because it stays in the scan pool until smx_StackScanU() runs.
   5. Invoking PendSV while in PendSV_Handler() -> Hard Fault.
   6. <= and >= are necessary to avoid damage above the stack. If SMX_CFG_SSMX
      stack overflow may be reported twice due to an MMF followed by detection 
      here. However, the MMF does not indicate its cause.
   7. A child task uses its top parent task's rtlim and rtlimctr.
   8. If rtlim == 0, smx_ctnew has no runtime limit. Note: Use smx_TaskSet
      (task, SMXE_ST_RTLIM, limit) to set task's rtlim before starting it.
   9. Must occur after autostop loaded so cbfun(START) can change it to a 
      custom autostop function.
   10. PSPLIM detects stack pad overflow for ARMM8.
*/ 

