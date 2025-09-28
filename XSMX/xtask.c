/*
* xtask.c                                                   Version 5.4.0
*
* smx Task Functions
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
* Authors: Ralph Moore, Alan Moore, David Moore
*
*****************************************************************************/

#include "xsmx.h"

/* internal subroutines */
/* smx_TaskDeleteLSRMain() in xsmx.h since shared */
static bool smx_StackPoolCreate(void);
static bool smx_TaskFreeAll(TCB_PTR task);
static bool smx_TaskSleep_F(u32 time);
static bool smx_TaskStart_F(TCB_PTR task, u8 pri);

/*
*  smx_TaskBump()   SSR
*
*  Set task priority to new_priority, unless it is SMX_PRI_NOCHG, in which
*  case it is unchanged. If task is in a queue, requeues it, even if its
*  priority is the same. This will move it after other tasks of the same
*  priority.
*/
bool smx_TaskBump(TCB_PTR task, u8 pri)
{
   bool pass;
   task = (task == SMX_CT ? smx_ct : task);
   smx_SSR_ENTER2(SMX_ID_TASK_BUMP, task, pri);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_BUMP, false);

   /* verify that task is valid and that current task has access permission */
   if (pass = smx_TCBTest(task, SMX_PRIV_HI))
   {
      smx_TASK_OP_PERMIT(task, SMX_ID_TASK_BUMP);

      /* change the priority to pri unless it is SMX_PRI_NOCHG */
      if (pri != (u8)SMX_PRI_NOCHG)
      {
         if (!smx_TEST_PRIORITY(pri))
            smx_ERROR_EXIT(SMXE_INV_PRI, false, 0, SMX_ID_TASK_BUMP);
         task->prinorm = pri;    /* Change normal priority. */

         /* raise priority, but lower it only if task does not own a mutex */
         if ((pri > task->pri) || (task->molp == NULL))
            task->pri = pri;
      }
      smx_ReQTask(task);
   }
   return((bool)smx_SSRExit((u32)pass, SMX_ID_TASK_BUMP));
}

/*
*  smx_TaskCreate()   SSR
*
*  Create a task with a preallocated stack, a stack from a heap or a stack
*  to be allocated from the stack pool when the task is dispatched. Allows task 
*  local storage area at the bottom of stack block and stack pad at top of
*  stack block. If stack is preallocated, sets task->flags.stk_preall flag so 
*  smx_TaskDelete() will not release stack. 
*/
TCB_PTR smx_TaskCreate(FUN_PTR fun, u8 pri, u32 tlssz_ssz, u32 fl_hn, 
                                       const char* name, u8* bp, TCB_PTR* thp)
{
   u32      flags;   /* task create flags */
   u32      hn;      /* heap number for stack block */
   u8*      sbp;     /* stack block pointer */
   u32      sbsz;    /* stack block size */
   u32      stksz;   /* stack size */
   TCB_PTR  task;    /* TCP pointer for task being created */
   u32      tlssz;   /* task local storage size */

   smx_SSR_ENTER7(SMX_ID_TASK_CREATE, fun, pri, tlssz_ssz, fl_hn, name, bp, thp);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_CREATE, NULL);

   /* block multiple creates and verify current task has create permission */
   if ((task = (TCB_PTR)smx_ObjectCreateTestH((u32*)thp)) && !smx_errno)
   {
      if (!smx_TEST_PRIORITY(pri))
         smx_ERROR_EXIT(SMXE_INV_PRI, NULL, 0, SMX_ID_TASK_CREATE);

      /* get flags and test <11> */
      flags = fl_hn & 0xFFFFFFF0;
     #if SMX_CFG_SSMX
      if ((flags & SMX_FL_UMODE) && (flags & SMX_FL_CHILD) && (smx_ct->flags.umode == 0))
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_TASK_CREATE);
     #endif
      
      /* allocate stack pool from heap <5> if first time called */
      if (smx_stkpl_init == false && !smx_StackPoolCreate())
         return((TCB_PTR)smx_SSRExit(NULL, SMX_ID_TASK_CREATE));

      /* get a task control block */
      if ((task = (TCB_PTR)sb_BlockGet(&smx_tcbs, 4)) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_TCBS, NULL, 0, SMX_ID_TASK_CREATE);

      if (name && *name)
         task->name = name;

      stksz = tlssz_ssz & 0xFFFF;
      tlssz = tlssz_ssz >> 16;
      hn    = fl_hn & 0x0000000F;

     #if SMX_CFG_SSMX
      if (flags & SMX_FL_UMODE || smx_ct->flags.umode == 1) /*<11>*/
         task->flags.umode = 1;
     #endif

      if (stksz)
      {
         if (bp)
         {
            /* use preallocated stack block and determine stksz */
            sbp = bp;
            stksz -= (SMX_SIZE_STACK_PAD + SMX_RSA_SIZE + tlssz);
            task->flags.stk_preall = 1;
         }
         else
         {
            sbsz  = SMX_SIZE_STACK_PAD + stksz + SMX_RSA_SIZE + tlssz;
 
          #if SMX_CFG_SSMX

            u32 nsbsz = sbsz; /* new stack block size */
   
           #if SB_CPU_ARMM7

            /* allocate stack block from heap <5> */
            task->mpap = (MPR*)&task->rv; /* to save rbar in rv, rasr in sv <3> */
            sbp = mp_RegionGetHeapT(task, sbsz, 0, MP_DATARW, 0, hn); /*<4>*/

            /* determine new stack block size */
            u32 rasr = task->sv;
            u32 rx = ((rasr >> 1) & 0x1F) + 1;     /* region exponent */
            nsbsz = 1 << rx;                       /* nsbsz = region size */
            u32 srsz = nsbsz/8;                    /* subregion size */
            u32 sds = rasr >> 8;                   /* subregion disables */
            /* deduct disabled subregions */
            for (u32 i = 8 ; i > 0; i--, sds = sds >> 1)
            {
               if (sds & 1)
                  nsbsz -= srsz;
            }     
               
           #elif SB_CPU_ARMM8

            /* allocate stack block from heap <5> */
            if (task->flags.umode == 1)   /* umode can use any heap */
            {
               task->mpap = (MPR*)&task->rv; /* to save rbar in rv, rlar in sv <3> */
               sbp = mp_RegionGetHeapT(task, sbsz, 0, MP_DATARW, 0, hn); /*<4>*/
            }
            else  /* pmode */
            { 
               sbp = (u8*)smx_HeapMalloc(sbsz, 5, hn);
               if (hn != 0) /*<17>*/
               {
                  /* make stack region and save rbar in rv, rlar in sv <3> */
                  mp_RegionMakeR((MPR*)&task->rv, sbp, sbsz, 0, MP_DATARW);
               }
            }

            /* determine new stack block size if hn != 0 */
            if ((hn != 0) || (task->flags.umode == 1))
            {
               u32 rbar = task->rv;
               u32 rlar = task->sv;
               nsbsz = ((rlar >> 5) - (rbar >> 5) + 1)*32;
            }
           #endif /* ARMM7/ARMM8 */

            /* increase stksz by the amount that sbsize increased */
            stksz += nsbsz - sbsz;

          #else   /* !SSMX */

            sbp = (u8*)smx_HeapMalloc(sbsz, 3, hn);

          #endif  /* SSMX */

            if (sbp == NULL)
            {
               /* stack allocation failed */
               sb_BlockRel(&smx_tcbs, (u8*)task, 0); /* return TCB */
               return((TCB_PTR)smx_SSRExit(NULL, SMX_ID_TASK_CREATE));
            }
         }

         /* initialize TCB stack fields */
         task->spp  = sbp;
         task->stp  = sbp + SMX_SIZE_STACK_PAD;
         task->sbp  = (u8 *)((u32)(task->stp + stksz) & (u32)(0 - SB_STACK_ALIGN)); /*<1>*/
         task->ssz  = (u16)((u32)task->sbp - (u32)task->stp);
         task->flags.stk_perm = 1;
         task->hn = hn;

        #if SMX_CFG_STACK_SCAN
         /* fill stack pad and stack with fill value <7> */
         memset(task->spp, SB_STK_FILL_VAL, SMX_SIZE_STACK_PAD + task->ssz);
        #endif
      }

     #if SMX_CFG_SSMX
      if (smx_ct->flags.umode == 1)
      {
         /* smx_ct is parent utask and task is its child utask */
         task->parent = smx_ct;
         task->irq = smx_ct->irq;
      }
      else if (flags & SMX_FL_CHILD || smx_ct->parent != NULL)
      {
         /* smx_ct is ptask parent and task is its ptask child */ 
         task->parent = smx_ct;
      }
      else
      {
         /* task is a normal ptask */
         task->parent = NULL;
      }
     #endif

     #if SMX_CFG_RTLIM
      if (task->parent) /*<8>*/
      {
         /* set task->rtlim and rtlimctr to point to top parent rtlim and rtlimctr */
         if (smx_ct->parent == NULL)
         {
            task->rtlim  = (u32)&smx_ct->rtlim; /*<9>*/
            task->rtlimctr = (u32)&smx_ct->rtlimctr;
         }
         else
         {
            task->rtlim  = smx_ct->rtlim; /*<10>*/
            task->rtlimctr = smx_ct->rtlimctr;
         }
      }
     #endif

     #if SMX_CFG_TOKENS
      if (task->parent) /*<14>*/
      {
         /* set task->tap to point to top parent tap */
         if (smx_ct->parent == NULL)
            task->tap  = (u32*)&smx_ct->tap;
         else
            task->tap  = smx_ct->tap;
      }
      else
      {
         task->tap = NULL;
      }
     #endif
      task->thp = thp;

     #if SMX_CFG_SSMX
      /* limit child priority to parent priority */
      pri = ((task->parent && (pri > smx_ct->pri)) ? smx_ct->pri : pri);
     #endif

      /* initialize remaining task TCB fields */
      task->indx = (u8)(task - (TCB_PTR)smx_tcbs.pi);
      task->fun = fun;
      task->pri = pri;
      task->prinorm = pri;
      task->pritmo = pri;
      task->cbtype = SMX_CB_TASK;
      task->state = SMX_TASK_WAIT;
      task->shwm = 0;
      task->flags.stk_hwmv = 1;
      task->flags.stk_chk  = 1;
    #if SMX_CFG_SSMX
      task->mpap  = (MPR*)mpa_dflt;    /* select default MPA */
     #if SB_CPU_ARMM8
      task->mpasz = MP_MPU_ACTVSZ + 1; /* allow for AUX region for v8 */
     #else
      task->mpasz = MP_MPU_ACTVSZ;
     #endif
    #endif
      if (flags & SMX_FL_STRT_LOCKD)
         task->flags.strt_lockd = 1;
      /* load task handle */
      if (thp)
         *thp = task;
   }
   return((TCB_PTR)smx_SSRExit((u32)task, SMX_ID_TASK_CREATE));
}

/*
*  smx_TaskCurrent()   Function
*
*  Returns handle of current task.
*/
TCB_PTR smx_TaskCurrent(void)
{
   return smx_ct;
}

/*
*  smx_TaskDelete()   SSR
*  
*  Deletes task and releases all objects its owns. If task is deleting itself 
*  invokes smx_TaskDeleteLSR to finish deletion. Otherwise frees task stack, 
*  frees its TCB and clears it, and sets its handle to NULL.
*/
bool smx_TaskDelete(TCB_PTR* thp)
{
   u32*     p;
   bool     pass;
   TCB_PTR  task = (thp ? *thp : NULL);

   smx_SSR_ENTER1(SMX_ID_TASK_DELETE, task);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_DELETE, false);

   /* verify that task is valid and that current task has access permission */
   if (pass = smx_TCBTest(task, SMX_PRIV_HI))
   {
      if (thp == &smx_ct) /* thp must point to task handle */
         smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_TASK_DELETE)
      smx_TASK_OP_PERMIT(task, SMX_ID_TASK_DELETE);

      /* remove task from any queue it is in */
      if (task->fl)
         smx_DQTask(task);
      task->state = SMX_TASK_DEL; /* for FRPort but may be generally useful */

      /* release all objects owned by task */
      if (task->cbfun)
         task->cbfun(SMX_CBF_DELETE, (u32)task);
      pass = smx_TaskFreeAll(task);

      if (pass)
      {
         /* self-delete */
         if (task == smx_ct)
         {
            /* invoke LSR to finish task self delete */
            smx_LSRInvoke(smx_TaskDeleteLSR, (u32)thp);
            pass = false; /*<13>*/
         }
         else
         {
            /* free task stack if not preallocated */
            if (task->flags.stk_preall == 0)
            {
               if (task->flags.stk_perm == 1)
               {
                  /* release permanent stack <5> */
                  pass &= smx_HeapFree((void*)(task->spp), task->hn); 
               }
               else
               {
                  /* release shared stack */
                  smx_RelPoolStack(task); /* shared stack */
                  #if SMX_CFG_STACK_SCAN
                  /* clear task handle from stack block owner fields <6> */
                  for (p = (u32*)smx_scanstack; p != NULL; p = (u32*)*p)
                     if (*(p + 1) == (u32)task)
                        *(p + 1) = NULL;
                  #endif
               }
            }
            if (pass)
            {
               /* free TCB, clear it, and clear task handle */
               sb_BlockRel(&smx_tcbs, (u8*)task, sizeof(TCB));
               if (thp)
                  *thp = NULL;
            }
         }
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_TASK_DELETE));
}

/*
*  smx_TaskLocate()   SSR
*
*  If the task forward link is NULL, aborts and returns NULL. Otherwise follows
*  the forward links until a non-TCB is found. If in rq, finds the top level
*  and returns it. Tests for a broken queue and reports SMXE_BROKEN_Q, if
*  found.
*/
void* smx_TaskLocate(const TCB_PTR task)
{
   CB_PTR qb;

   smx_SSR_ENTER1(SMX_ID_TASK_LOCATE, task);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_LOCATE, NULL);

   /* verify that task is valid and that current task has access permission */
   if (qb = (CB_PTR)smx_TCBTest(task, SMX_PRIV_LO))
   {
      smx_TASK_OP_PERMIT(task, SMX_ID_TASK_LOCATE);

      if (task->fl == NULL)
         return((void*)smx_SSRExit((u32)NULL, SMX_ID_TASK_LOCATE));

      /* find queue control block */
      for (qb = task->fl; qb != (CB_PTR)task; qb = qb->fl)
         if (qb->cbtype != SMX_CB_TASK)
            break;

      /* if in rq, search for the top level */
      while (qb != NULL && qb->cbtype == SMX_CB_SUBLV)
      {
         qb = (CB_PTR)((u32)qb - sizeof(RQCB));
      }

      if (!smx_TEST_BRKNQ(qb))
         smx_ERROR_EXIT(SMXE_BROKEN_Q, NULL, 0, SMX_ID_TASK_LOCATE);
   }
   return((void*)smx_SSRExit((u32)qb, SMX_ID_TASK_LOCATE));
}

/*
*  smx_TaskLock()   Function
*
*  Increments lock counter if < limit.
*/
bool smx_TaskLock(void)
{
  #if SMX_CFG_EVB
   smx_EVBLogSSR0(SMX_ID_TASK_LOCK);
  #endif
   /* pmode only so no PERMIT check */
   smx_ct->err = SMXE_OK;
   if (smx_lockctr < SMX_LOCK_NEST_LIMIT)
   {
      smx_lockctr++;
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(true, SMX_ID_TASK_LOCK);
     #endif
      return true;
   }
   else
   {
      smx_lockctr = 1; /* so sb_ConWriteString() works */
      smx_ERROR(SMXE_EXCESS_LOCKS, 0);
      smx_lockctr = SMX_LOCK_NEST_LIMIT; /* restore wrong count */
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(false, SMX_ID_TASK_LOCK);
     #endif
      return false;
   }
}

/*
*  smx_TaskLockClear()   SSR
*
*  SSR that clears lock counter and tests preemption.
*/
#if !defined(SMX_FRPORT)
bool smx_TaskLockClear(void)
{
   smx_SSR_ENTER0(SMX_ID_TASK_LOCK_CLEAR);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_LOCK_CLEAR, false);
   /* pmode only so no PERMIT check */
   if (smx_lockctr > 0)
   {
      if (smx_lockctr > 1)
         smx_ERROR(SMXE_INSUFF_UNLOCKS, 0);
      smx_lockctr = 0;
      smx_DO_CTTEST();
   }
   return((bool)smx_SSRExit(true, SMX_ID_TASK_LOCK_CLEAR));
}
#else
/* FRPort difference: detect excess unlocks */
bool smx_TaskLockClear(void)
{
   smx_SSR_ENTER0(SMX_ID_TASK_LOCK_CLEAR);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_LOCK_CLEAR, false);
   /* pmode only so no PERMIT check */
   if (smx_lockctr > 1)
      smx_ERROR(SMXE_INSUFF_UNLOCKS, 0);
   if (smx_lockctr == 0)
      smx_ERROR(SMXE_EXCESS_UNLOCKS, 0);
   smx_lockctr = 0;
   smx_DO_CTTEST();
   return((bool)smx_SSRExit(true, SMX_ID_TASK_LOCK_CLEAR));
}
#endif

/*
*  smx_TaskPeek()   SSR
*
*  Permits looking at task parameters.
*/
u32 smx_TaskPeek(TCB_PTR task, SMX_PK_PAR par)
{
   u32 err, val = 0;

   task = (task == SMX_CT ? smx_ct : task);

   /* smx_SSR_ENTER2() without clearing smx_ct->err */
   smx_srnest++;
   smx_EVB_LOG_SSR2(SMX_ID_TASK_PEEK, (u32)task, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_PEEK, 0);

   /* verify that task is valid and that current task has access permission */
   if (smx_TCBTest(task, SMX_PRIV_LO))
   {
      smx_TASK_OP_PERMIT(task, SMX_ID_TASK_PEEK);
      err = task->err;
      smx_ct->err = SMXE_OK;
      switch (par)
      {
         case SMX_PK_ERROR:
            val = err;
            break;
         case SMX_PK_FUN:
            val = (u32)task->fun;
            break;
         case SMX_PK_HN:
            val = (u32)task->hn;
            break;
         case SMX_PK_INDEX:
            val = (u32)task->indx;
            break;
         case SMX_PK_LOCK:
            val = smx_lockctr;
            break;
         case SMX_PK_MTX:
            val = (u32)task->molp;
            break;
         case SMX_PK_NAME:
            val = (u32)task->name;
            break;
         case SMX_PK_NEXT:
            val = (u32)task->fl;
            break;
        #if SMX_CFG_SSMX
         case SMX_PK_PARENT:
            val = (u32)task->parent;
            break;
        #endif
         case SMX_PK_PREV:
            val = (u32)task->bl;
            break;
         case SMX_PK_PRI:
            val = (u32)task->pri;
            break;
         case SMX_PK_PRINORM:
            val = (u32)task->prinorm;
            break;
         case SMX_PK_RTC:
            val = task->rtc;
            break;
        #if SMX_CFG_SSMX
         case SMX_PK_RTLIM:
            if (task->parent)
               val = *(u32*)(task->rtlim);
            else
               val = task->rtlim;
            break;
         case SMX_PK_RTLIMCTR:
            if (task->parent)
               val = *(u32*)(task->rtlimctr);
            else
               val = task->rtlimctr;
            break;
        #endif
         case SMX_PK_STATE:
            val = (u32)task->state;
            break;
         case SMX_PK_TLSP:
            val = (u32)task->sbp + SMX_RSA_SIZE;
            break;
         case SMX_PK_TMO:
            val = smx_timeout[task->indx];
            if (val != SMX_TMO_INF)
               val -= smx_etime;
            break;
        #if SMX_CFG_SSMX
         case SMX_PK_PRIV:
            if (task->parent)
               val = task->parent->priv;
            else
               val = task->priv;
            break;
         case SMX_PK_UMODE:
            val = (u32)task->flags.umode;
            break;
        #endif
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_TASK_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_TASK_PEEK);
}

/*
*  smx_TaskResume()   SSR
*
*  Dequeue task from any queue it may be in and set task->rv = 0. Make tast
*  ready to run. Does nothing if task already in rq. Note: ENTER callback 
*  function is called by scheduler.
*/
bool smx_TaskResume(TCB_PTR task)
{
   bool  pass;
   task = (task == SMX_CT ? smx_ct : task);
   smx_SSR_ENTER1(SMX_ID_TASK_RESUME, task);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_RESUME, false);

   /* verify that task is valid and that current task has access permission */
   if (pass = smx_TCBTest(task, SMX_PRIV_HI))
   {
      smx_TASK_OP_PERMIT(task, SMX_ID_TASK_RESUME);

      if (task->fl)
      {
         smx_DQTask(task);
         task->rv = 0;
      }
      /* make task ready to run */
      smx_NQRQTask(task);
      smx_timeout[task->indx] = SMX_TMO_INF;
      smx_DO_CTTEST(); /*<15>*/
   }
   return((bool)smx_SSRExit(pass, SMX_ID_TASK_RESUME));
}

/*
*  smx_TaskSet()   SSR
*
*  Sets a specified task parameter to a specified value. Not permitted in umode.
*/
bool smx_TaskSet(TCB_PTR task, SMX_ST_PAR par, u32 v1, u32 v2)
{
   bool  pass;
   task = (task == SMX_CT ? smx_ct : task);
   smx_SSR_ENTER4(SMX_ID_TASK_SET, task, par, v1, v2);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_SET, false);

   /* verify that task is valid and that current task has access permission */
   if (pass = smx_TCBTest(task, SMX_PRIV_HI))
   {
      #if SMX_CFG_SSMX
      /* a utask cannot make task changes */
      if (smx_ct->flags.umode == 1)
         smx_ERROR_EXIT(SMXE_PRIV_VIOL, false, 0, SMX_ID_TASK_SET);
      #endif

      /* perform set operation on task */
      switch (par)
      {
         case SMX_ST_CBFUN:
            task->cbfun = (CBF2_PTR)v1;
            if (v1 != 0 && v2 > 0)
               task->flags.hookd = 1;
            else
               task->flags.hookd = 0;
            break;
         case SMX_ST_FUN:
            task->fun = (FUN_PTR)v1;
            break;
        #if SMX_CFG_SSMX
         case SMX_ST_IRQ:
            task->irq = (IRQ_PERM*)v1;
            break;
        #endif
        #if SMX_CFG_RTLIM
         case SMX_ST_RTLIM:
            if (task->parent == NULL)
            {
               task->rtlim = v1;
               task->rtlimctr = 0;
            }
            break;
        #endif
         case SMX_ST_STK_CK:
            if (v1 > 0)
               v1 = 1;
            task->flags.stk_chk = v1;
            break;
         case SMX_ST_STRT_LOCKD:
            task->flags.strt_lockd = v1;
            break;
         case SMX_ST_PRITMO:
            task->pritmo = v1;
            if (v2 != 0)
               *(u32*)v2 = task->prinorm;
            break;
        #if SMX_CFG_SSMX
         case SMX_ST_PRIV:
            if (!task->parent) /* only top parent priv can be set */
            {
               task->priv = v1;
               task->flags.priv_fixed = v2;
            }
            else
               smx_ERROR_EXIT(SMXE_OP_NOT_ALLOWED, false, 0, SMX_ID_TASK_SET);
            break;
         case SMX_ST_TAP:
            if (!task->parent) /* only top parent tap can be set */
               task->tap = (u32*)v1;
            else
               smx_ERROR_EXIT(SMXE_OP_NOT_ALLOWED, false, 0, SMX_ID_TASK_SET);
            break;
         case SMX_ST_UMODE:
            if (!task->parent) /* a child ptask cannot be put into umode */
               task->flags.umode = 1;
            break;
        #endif
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_TASK_SET);
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_TASK_SET));
}

/*
*  smx_TaskSleep()   SSR
*
*  Aborts if called from an LSR and reports OP_NOT_ALLOWED, else sets smx_sched 
*  = SUSPEND and calls smx_TaskSleep_F(). No permit check because operates 
*  only on smx_ct.
*/
bool smx_TaskSleep(u32 time)
{
   bool pass;

   if (smx_clsr)
      smx_ERROR_RET(SMXE_OP_NOT_ALLOWED, false, 0);

   smx_SSR_ENTER1(SMX_ID_TASK_SLEEP, time);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_SLEEP, false);
   smx_sched = SMX_CT_SUSP;
   pass = smx_TaskSleep_F(time);
   return((bool)smx_SSRExit(pass, SMX_ID_TASK_SLEEP));
}

/*
*  smx_TaskSleepStop()   SSR
*
*  Aborts if called from an LSR and reports OP_NOT_ALLOWED, else sets smx_sched 
*  = STOP and calls smx_TaskSleep_F(). No permit check because operates only on 
*  smx_ct.
*/
void smx_TaskSleepStop(u32 time)
{
   bool pass;

   if (smx_clsr)
      smx_ERROR_RET_VOID(SMXE_OP_NOT_ALLOWED, 0);
   smx_RET_IF_IN_ISR_VOID();
   smx_SSR_ENTER1(SMX_ID_TASK_SLEEP_STOP, time);
   smx_sched = SMX_CT_STOP;
   pass = smx_TaskSleep_F(time);
   smx_SSRExit(pass, SMX_ID_TASK_SLEEP_STOP);
}

/*
*  smx_TaskStart()   SSR
*
*  Starts task and passes parameter to task->rv, if successful. If task starts
*  itself, passes par to smx_SSRExit(), which loads it into task->rv.
*/
bool smx_TaskStart(TCB_PTR task, u32 par)
{
   u8 pass;
   task = (task == SMX_CT ? smx_ct : task);
   smx_SSR_ENTER2(SMX_ID_TASK_START, task, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_START, false);

   /* verify that task is valid and that current task has access permission */
   if (pass = smx_TCBTest(task, SMX_PRIV_HI))
   {
      smx_TASK_OP_PERMIT(task, SMX_ID_TASK_START);
      if (pass = smx_TaskStart_F(task, task->pri))
      {
         if (task == smx_ct)
            pass = par;
         else
            task->rv = par; /*<2>*/
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_TASK_START));
}

/*
*  smx_TaskStartNew()   SSR
*
*  Starts task with new main function, priority, and parameter. Passes parameter 
*  to task->rv, if successful. If task starts itself, passes par to 
*  smx_SSRExit(), which loads it into task->rv.
*/
bool smx_TaskStartNew(TCB_PTR task, u32 par, u8 pri, FUN_PTR fun)
{
   u8 pass;

   task = (task == SMX_CT ? smx_ct : task);
   smx_SSR_ENTER4(SMX_ID_TASK_START_NEW, task, par, pri, fun);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_START_NEW, false);
   if (pass = smx_TCBTest(task, SMX_PRIV_HI))
   {
      smx_TASK_OP_PERMIT(task, SMX_ID_TASK_START_NEW);

      if (!smx_TEST_PRIORITY(pri))
         smx_ERROR_EXIT(SMXE_INV_PRI, false, 0, SMX_ID_TASK_START_NEW);

      if (pass = smx_TaskStart_F(task, pri))
      {
         task->fun = fun;
         if (task == smx_ct)
            pass = par;
         else
            task->rv = par; /*<2>*/
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_TASK_START_NEW));
}

/*
*  smx_TaskStop()   SSR
*
*  Dequeues task from any queue it may be in. If task is smx_ct, sets sched = 
*  CT_STOP so the scheduler will stop it. Otherwise releases unbound stack and 
*  clears task->sp so sched will restart task. If timeout > 0, sets task
*  timeout, else makes task ready to run. Note: STOP callback function is 
*  called by the scheduler.
*/
bool smx_TaskStop(TCB_PTR task, u32 timeout)
{
   bool pass;
   task = (task == SMX_CT ? smx_ct : task);
   smx_SSR_ENTER2(SMX_ID_TASK_STOP, task, timeout);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_STOP, false);

   /* verify that task is valid and that current task has access permission */
   if (pass = smx_TCBTest(task, SMX_PRIV_HI))
   {
      smx_TASK_OP_PERMIT(task, SMX_ID_TASK_STOP);

      /* stop task */
      if (task->fl)
      {
         smx_DQTask(task);
         task->rv = 0;
      }
      if (task == smx_ct)
      {
         smx_sched = SMX_CT_STOP;
      }
      else 
      {
         if (task->flags.stk_perm == 0)
         {
            /* free unbound stack */
            sb_INT_DISABLE();
            smx_RelPoolStack(task);
            sb_INT_ENABLE();
         }
         task->sp = NULL;  /* so sched will restart task */       
      }

      /* restart task */
      if (timeout)
      {
         smx_TimeoutSet(task, timeout);
      }
      else
      {
         /* make task ready to run */
         smx_timeout[task->indx] = SMX_TMO_INF;
         smx_NQRQTask(task);
         if (task != smx_ct)
         {
            smx_DO_CTTEST();  /*<15>*/
         }
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_TASK_STOP));
}

/*
*  smx_TaskSuspend   SSR
*
*  Removes task from any queue it may be in and clears task->rv. If task is 
*  smx_ct, sets smx_sched = SMX_CT_SUSP to suspend it. If timeout > 0, sets 
*  task timeout, else makes task ready to run. Note: EXIT callback function
*  is called by scheduler.
*/
bool smx_TaskSuspend(TCB_PTR task, u32 timeout)
{
   bool pass;
   task = (task == SMX_CT ? smx_ct : task);
   smx_SSR_ENTER2(SMX_ID_TASK_SUSPEND, task, timeout);
   smx_EXIT_IF_IN_ISR(SMX_ID_TASK_SUSPEND, false);

   /* verify that task is valid and that current task has access permission */
   if (pass = smx_TCBTest(task, SMX_PRIV_HI))
   {
      smx_TASK_OP_PERMIT(task, SMX_ID_TASK_SUSPEND);

      /* suspend smx_ct */
      if (task->fl)
      {
         smx_DQTask(task);
         task->rv = 0;
      }

      /* resume task */
      if (task == smx_ct)
      {
         smx_sched = SMX_CT_SUSP;
      }
      if (timeout)
      {
         smx_TimeoutSet(task, timeout);
      }
      else
      {
         /* make task ready to run */
         smx_NQRQTask(task);
         smx_timeout[task->indx] = SMX_TMO_INF;
         smx_DO_CTTEST(); /*<15>*/
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_TASK_SUSPEND));
}

/*
*  smx_TaskUnlock()   Function (calls SSR)
*
*  Decrements lock nesting counter if > 1; else, calls smx_TaskLockClear().
*/
#if !defined(SMX_FRPORT)
bool smx_TaskUnlock(void)
{
  #if SMX_CFG_EVB
   smx_EVBLogSSR0(SMX_ID_TASK_UNLOCK);
  #endif
   /* pmode only so no PERMIT check */
   smx_ct->err = SMXE_OK;
   if (smx_lockctr > 1)
   {
      smx_lockctr--;
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(true, SMX_ID_TASK_UNLOCK);
     #endif
      return true;
   }
   else if (smx_lockctr == 1)
   {
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(true, SMX_ID_TASK_UNLOCK);
     #endif
      return smx_TaskLockClear();
   }
   else
   {
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(false, SMX_ID_TASK_UNLOCK);
     #endif
      smx_ERROR_RET(SMXE_EXCESS_UNLOCKS, false, 0);
   }
}
#else
/* FRPort difference: reverses returns */
bool smx_TaskUnlock(void)
{
  #if SMX_CFG_EVB
   smx_EVBLogSSR0(SMX_ID_TASK_UNLOCK);
  #endif
   /* pmode only so no PERMIT check */
   smx_ct->err = SMXE_OK;
   if (smx_lockctr > 1)
   {
      smx_lockctr--;
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(false, SMX_ID_TASK_UNLOCK);
     #endif
      return false;
   }
   else
   {
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(true, SMX_ID_TASK_UNLOCK);
     #endif
      return smx_TaskLockClear();
   }
}
#endif

/*
*  smx_TaskUnlockQuick()   Function
*
*  Decrements lock nesting counter if >= 1; else signals error.
*/
#if !defined(SMX_FRPORT)
bool smx_TaskUnlockQuick(void)
{
  #if SMX_CFG_EVB
   smx_EVBLogSSR0(SMX_ID_TASK_UNLOCK_QUICK);
  #endif
   /* pmode only so no PERMIT check */
   smx_ct->err = SMXE_OK;
   if (smx_lockctr >= 1)
   {
      smx_lockctr--;
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(true, SMX_ID_TASK_UNLOCK_QUICK);
     #endif
      return true;
   }
   else
   {
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(false, SMX_ID_TASK_UNLOCK_QUICK);
     #endif
      smx_ERROR_RET(SMXE_EXCESS_UNLOCKS, false, 0);
   }
}
#else
/* FRPort difference: reverses returns */
bool smx_TaskUnlockQuick(void)
{
  #if SMX_CFG_EVB
   smx_EVBLogSSR0(SMX_ID_TASK_UNLOCK_QUICK);
  #endif
   /* pmode only so no PERMIT check */
   smx_ct->err = SMXE_OK;
   if (smx_lockctr >= 1)
      smx_lockctr--;
   if (smx_lockctr > 0)
   {
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(false, SMX_ID_TASK_UNLOCK_QUICK);
     #endif
      return false;
   }
   else
   {
     #if SMX_CFG_EVB
      smx_EVBLogSSRRet(true, SMX_ID_TASK_UNLOCK_QUICK);
     #endif
      return true;
   }
}
#endif

#if SMX_CFG_SSMX
/*
*  smx_TaskYield()   SSR
*
*  Requeues task at the end of its level in rq to allow others to run.
*  Special case of smx_TaskBump() allowed from umode.
*  Must be function not macro for MPU, since utask caller can't access smx_ct.
*/
bool smx_TaskYield(void)
{
   /* self only so no PERMIT check */
   return(smx_TaskBump(smx_ct, SMX_PRI_NOCHG));
}
#endif /* SMX_CFG_SSMX */

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/* create stack pool if STK_BLK_SIZE and NUM_STACKS are not 0 */
bool smx_StackPoolCreate(void)
{
 #if (SMX_NUM_STACKS * SMX_SIZE_STACK_BLK)
   u32 num = SMX_NUM_STACKS;
   u32 sz  = SMX_SIZE_STACK_BLK;
   u32 i, *sp;
  #if SMX_CFG_SSMX

   #if SB_CPU_ARMM7
   u32 an;
   /* get stack pool of v7-region blocks from mheap */
   for (an = 5, sz >>= 5; sz > 1; sz >>= 1, an++) {}
   sz = 1 << an;
   sp = (u32*)smx_HeapMalloc(num*sz, an);

   #elif SB_CPU_ARMM8
   /* get stack pool of v8-region blocks from mheap */
   sp = (u32*)smx_HeapMalloc(num*sz, 5);
   #endif

  #else /* !SSMX */
   /* get stack pool of 8-byte-aligned blocks from mheap */
   sp = (u32*)smx_HeapMalloc(num*sz, 3);
  #endif

   if (sp == NULL)
      return false;

   /* fill stacks with scan pattern */
   #if SMX_CFG_STACK_SCAN
   (void)memset(sp, SB_STK_FILL_VAL, num*sz);
   #endif

   /* initialize globals and link stacks to smx_freestack */
   smx_spmin = sp;
   smx_freestack = sp;
   for (i = 0; i < num - 1; i++)
   {
      *sp = (u32)(sp + sz/4);
      sp += sz/4;
   }
   *sp = NULL; /* end of linked list */
 #endif /* SMX_NUM_STACKS * SMX_SIZE_STACK_BLK */

   smx_stkpl_init = true;
   return true;
}

/* 
*  smx_TaskDeleteLSRMain()
*
*  Completes task self-deletion. 
*/
void smx_TaskDeleteLSRMain(u32 taskp)
{
   u32*     p;
   bool     pass = true;
   TCB_PTR  task = *(TCB_PTR*)taskp;

   /* check for stack overflow */
   if ((task->flags.stk_chk == 1 && (u32)task->sp != 0) && 
      ((u32)task->sp < (u32)task->stp || (task->shwm >= task->ssz))) /*<12>*/
      smx_EM(SMXE_STK_OVFL, 1);

   /* free task stack if not preallocated */
   if (task->flags.stk_preall == 0)
   {
      if (task->flags.stk_perm == 1)
      {
         /* release permanent stack <5> */
         pass = smx_HeapFree((void *)(task->spp), task->hn); 
      }
      else
      {
         /* release shared stack */
         smx_RelPoolStack(task); /* shared stack */
         #if SMX_CFG_STACK_SCAN
         /* clear task handle from stack block owner fields <6> */
         for (p = (u32*)smx_scanstack; p != NULL; p = (u32*)*p)
            if (*(p + 1) == (u32)task)
               *(p + 1) = NULL;
         #endif
      }
   }
   if (pass)
   {
      /* free TCB and clear task handle so it cannot be reused */
      sb_BlockRel(&smx_tcbs, (u8*)task, sizeof(TCB));
      *(u32*)taskp = NULL;
      smx_sched = SMX_CT_DELETE;
   }
   else
      smx_sched = SMX_CT_NOP;
}

/*
*  smx_TaskFreeAll()
*
*  Releases all owned timers, frees task's MPA, deactivates task timeout, 
*  and releases all owned blocks, messages, and mutexes.
*/
bool smx_TaskFreeAll(TCB_PTR task)
{
   TMRCB_PTR tmr;
   bool  pass = true;

   /* search for and release all owned timers */
   if (smx_tmrcbs.pi)
      for (tmr = (TMRCB_PTR)smx_tmrcbs.pi; tmr <= (TMRCB_PTR)smx_tmrcbs.px; tmr++)
         if (tmr->onr == task)
            smx_TimerStop(tmr, NULL);

   #if SMX_CFG_SSMX
   /* free MPA block back to heap */
   if (task->mpap != (MPR*)mpa_dflt)
      pass &= smx_HeapFree(task->mpap);
   #endif

   /* deactivate task timeout */
   smx_timeout[task->indx] = SMX_TMO_INF;

   /* release all owned blocks and msgs */
   smx_BlockRelAll_F(task);
   smx_MsgRelAll_F(task);

   /* search for and free all owned mutexes */
   while (task->molp != NULL)
      pass &= smx_MutexFree(task->molp);
   return pass;
}

#if SMX_CFG_SSMX
/* Verify operation is permitted on task. */
bool smx_TaskOpPermit(TCB_PTR task)
{
   /* permit ptask or LSR to operate on any task */
   if (smx_ct->flags.umode != 1 || smx_clsr != 0)
      return(true);

   /* umode: permit task to operate on its children or itself */
   if (task->parent == smx_ct || task == smx_ct)
      return(true);

   /* everything else is not permitted, such as child operating on parent */
   return(false);
}
#endif /* SMX_CFG_SSMX */

/*
*  smx_TaskSleep_F()
*
*  If time has not already passed, convert it from system time in seconds to 
*  ticks from now and set smx_ct timeout.
*/
static bool smx_TaskSleep_F(u32 time)
{
   u32     tmo;

   if (time <= smx_SysPeek(SMX_PK_STIME))
      smx_ERROR_RET(SMXE_INV_TIME, false, 0);
   smx_DQRQTask(smx_ct);
   tmo = (time - smx_SysPeek(SMX_PK_STIME))*SMX_TICKS_PER_SEC;
   smx_TimeoutSet(smx_ct, tmo);
   return true;
}

/*
*  smx_TaskStart_F()
*
*  Dequeues task from any queue it may be in. If task is smx_ct, sets sched = 
*  SMX_CT_STOP so the scheduler will stop task. Otherwise, releases unbound 
*  stack, clears task->sp so sched will restart task, and calls smx_DO_CTTEST(). 
*  Then makes task ready to run and calls INIT callback function, if any.
*
*  Called from smx_TaskStart() and smx_TaskStartNew(). 
*/
static bool smx_TaskStart_F(TCB_PTR task, u8 pri)
{
   /* stop task */
   if (task->fl)
   {
      smx_DQTask(task);
   }
   if (task == smx_ct)
   {
      smx_sched = SMX_CT_STOP;
   }
   else
   {
      if (task->flags.stk_perm == 0)
      {
         /* free unbound stack */
         sb_INT_DISABLE();
         smx_RelPoolStack(task);
         sb_INT_ENABLE();        
      }
      task->sp = NULL;  /* so sched will restart task */
      smx_DO_CTTEST();  /*<15>*/
   }

   /* make task ready to run */
   if (pri != SMX_PRI_NOCHG)  /*<16>*/
   {
      task->pri = pri;
      task->prinorm = pri;
      task->pritmo = pri;
   }
   smx_NQRQTask(task);
   smx_timeout[task->indx] = SMX_TMO_INF;
   if (task->cbfun)
      task->cbfun(SMX_CBF_INIT, (u32)task);
   return true;
}

/*
Notes:
   1. task->sbp is 8-byte aligned so that when task->fun is entered, sp is 
      8-byte aligned to conform to AAPCS. This is done as a precaution.
      Whether it is actually necessary requires further study. If task->sbp is
      moved up one word, task->ssz will be 4 bytes less than expected.
   2. ARMM: Don't use smx_PUT_RV_IN_EXR0() in smx_TaskStart() calls, since
      smx_MakeFrame() will put rv into task stack after it is assigned
      during task dispatch in the scheduler.
   3. If SMX_CFG_SSMX, task->rv and sv temporarily store rbar and rasr/rlar, 
      which are loaded into the task's MPA[stack_slot] by mp_MPACreate() 
      after smx_TaskCreate(). If mp_MPACreate() is not called, the task can
      operate only if its stack is in an mpa_dflt region. 
   4. hn must be 0 to load rbar and rasr/rlar in rv and sv, also ACTVSZ is an 
      application parameter.
   5. Current task will be suspended for up to smx_htmo ticks if heap hn is
      busy. Operation aborts with smx Heap Timeout error if timeout occurs.
   6. When a task is deleted, its TCB pointer must be removed from the pointer
      fields in stack blocks to be scanned, else when the stack block is 
      scanned, shwm and flags.stk_hwm will be changed in the TCB, which may now 
      belong to another task.
   7. The stack pad must also be scanned because a stack overflow may skip over
      the top word in the stack and not change it.
   8. For a non-child task, its rtlim must be set with smx_TaskSet(task, 
      SMXE_ST_RTLIM, limit). This also clears its rtlimctr.
   9. A child task uses its parent's runtime limit and counter, so pointers to 
      its parent's rtlim and rtlimctr are copied to its rtlim and rtlimctr.
   10. If smx_ct is a child task, its rtlim and rtlimctr, which are pointers to 
      the top parent's rtlim and rtlimctr, are copied to the child's rtlim and 
      rtlimctr.
   11. A ptask cannot create a child utask, and a utask can create only a child 
       utask.
   12. >= is necessary when there is no stack pad.
   13. Return false because LSR may fail to delete task. No code should follow 
       smx_TaskDelete(self). Success can be determined by task = NULL
       in another task.
   14. If tokens are enabled, a child task uses its top parent task's token array.
   15. Do not preempt if smx_ct is locked.
   16. Must be here because smx_NQRQTask() follows.
   17. If hn == 0 and umode == 0 cannot create a stack region. Stack must be in
       task's data region.
*/
