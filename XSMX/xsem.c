/*
* xsem.c                                                    Version 5.4.0
*
* smx Semaphore Services
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
static bool smx_SemClear_F(SCB_PTR sem);
static bool smx_SemTest_F(SCB_PTR sem, u32 timeout);

/*
*  smx_SemClear()   SSR
*
*  Clears a semaphore by resuming all waiting tasks with NULL return values,
*  and sets count = 0 for EVENT, THRES, & GATE, and = lim for RSRC semaphores.
*  Returns true, if successful.
*/
bool smx_SemClear(SCB_PTR sem)
{
   bool pass;

   smx_SSR_ENTER1(SMX_ID_SEM_CLEAR, sem);
   smx_EXIT_IF_IN_ISR(SMX_ID_SEM_CLEAR, false);

   /* clear task queue */
   if (pass = smx_SemClear_F(sem))
   {
      /* reset count */
      if (sem->mode == SMX_SEM_RSRC)
         sem->count = sem->lim;
      else
         sem->count = 0;
   }
   return((bool)smx_SSRExit(pass, SMX_ID_SEM_CLEAR));
}

/*
*  smx_SemCreate()   SSR
*
*  Allocates a semaphore control block from the SCB pool and initializes it.
*  If parameters are invalid or allocation fails because no block is available,
*  returns NULL. Otherwise returns semaphore handle.
*/
SCB_PTR smx_SemCreate(SMX_SEM_MODE mode, u8 lim, const char* name, SCB_PTR* shp)
{
   SCB_PTR sp;
   u32 *p, count = 0;

   smx_SSR_ENTER4(SMX_ID_SEM_CREATE, mode, lim, name, shp);
   smx_EXIT_IF_IN_ISR(SMX_ID_SEM_CREATE, NULL);

   /* block multiple creates and verify current task has create permission */
   if ((sp = (SCB_PTR)smx_ObjectCreateTestH((u32*)shp)) && !smx_errno)
   {
      /* check parameters */
      if (mode > SMX_SEM_WRTR || ((mode == SMX_SEM_RSRC || mode == SMX_SEM_THRES)
            && lim < 1) || (mode == SMX_SEM_GATE && lim != 1))
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_SEM_CREATE);

      /* get a semaphore control block */
      if ((sp = (SCB_PTR)sb_BlockGet(&smx_scbs, 4)) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_SCBS, NULL, 0, SMX_ID_SEM_CREATE);

      /* Initialize SCB */
      p = (u32*)sp + 2;
      if (mode == SMX_SEM_RSRC)
         count = lim;

     #if __LITTLE_ENDIAN__
      *p = lim << 24 | count << 16 | mode << 8 | SMX_CB_SEM;
     #else
      *p = SMX_CB_SEM << 24 | mode << 16 | count << 8 | lim;
     #endif

      sp->shp = shp;
      if (name && *name)
         sp->name = name;

      /* load semaphore handle */
      if (shp)
         *shp = sp;
   }
   return((SCB_PTR)smx_SSRExit((u32)sp, SMX_ID_SEM_CREATE));
}

/*
*  smx_SemDelete()   SSR
*
*  Deletes a semaphore created by smx_SemCreate(). Resumes all waiting tasks
*  with false return values. Clears and releases its SCB, and sets its handle
*  to NULL.
*/
bool smx_SemDelete(SCB_PTR* shp)
{
   bool     pass;
   SCB_PTR  sem = (shp ? *shp : NULL);

   smx_SSR_ENTER1(SMX_ID_SEM_DELETE, sem);  /* record actual handle */
   smx_EXIT_IF_IN_ISR(SMX_ID_SEM_DELETE, false);

   /* resume all waiting tasks */
   if (pass = smx_SemClear_F(sem))
   {
      /* clear and release SCB and set sem handle to nullcb */
      sb_BlockRel(&smx_scbs, (u8*)sem, sizeof(SCB));
      *shp = NULL;
   }
   return((bool)smx_SSRExit(pass, SMX_ID_SEM_DELETE));
}

/*
*  smx_SemPeek()   SSR
*
*  Return the specified information about sem.
*/
u32 smx_SemPeek(SCB_PTR sem, SMX_PK_PAR par)
{
   u32 val = 0;

   smx_SSR_ENTER2(SMX_ID_SEM_PEEK, sem, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_SEM_PEEK, 0);

   /* verify that sem is valid and that current task has access permission */
   if (val = smx_SCBTest(sem, SMX_PRIV_LO))
   {
      switch (par)
      {
         case SMX_PK_FIRST:
            val = (u32)sem->fl;
            break;
         case SMX_PK_LAST:
            val = (u32)sem->bl;
            break;
         case SMX_PK_MODE:
            val = (u32)sem->mode;
            break;
         case SMX_PK_COUNT:
            val = (u32)sem->count;
            break;
         case SMX_PK_LIMIT:
            val = (u32)sem->lim;
            break;
         case SMX_PK_NAME:
            val = (u32)sem->name;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_SEM_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_SEM_PEEK);
}

/*
*  smx_SemSet()   SSR
*
*  Sets the specified semaphore parameter to the specified value.
*  Not permitted in umode.
*/
bool smx_SemSet(SCB_PTR sem, SMX_ST_PAR par, u32 v1, u32 v2)
{
   bool pass;
   smx_SSR_ENTER4(SMX_ID_SEM_SET, sem, par, v1, v2);
   smx_EXIT_IF_IN_ISR(SMX_ID_SEM_SET, false);

   /* verify that sem is valid and that current task has access permission */
   if (pass = smx_SCBTest(sem, SMX_PRIV_HI))
   {
      #if SMX_CFG_SSMX
      /* a utask cannot make semaphore changes */
      if (smx_ct->flags.umode == 1)
         smx_ERROR_EXIT(SMXE_PRIV_VIOL, false, 0, SMX_ID_SEM_SET);
      #endif

      /* perform set operation on semaphore */
      switch (par)
      {
         case SMX_ST_CBFUN:
            sem->cbfun = (CBF_PTR)v1;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_SEM_SET);
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_SEM_SET));
}

/*
*  smx_SemSignal()   SSR
*
*  If SCB is not valid, or if SMX_CFG_TOKENS and ct does not have access token,
*  returns false, else executes.
*
*  RSRC mode:
*     resumes first waiting task with true; else if count < lim, count++;
*  EVENT mode:
*     resumes first waiting task with true; else:
*        for lim == 1 and count < lim, count++; else:
*           for if count < 255, count++, else: SMXE_SEM_CTR_OVFL error.
*  THRES mode:
*     if task waiting and count >= (lim - 1): count - (lim - 1) and resume task
*     with true; else if count < 0xFF, count++; else SMXE_SEM_CTR_OVFL error.
*  GATE mode:
*     resume all waiting tasks with true
*/
bool smx_SemSignal(SCB_PTR sem)
{
   bool     pass;
   TCB_PTR  task;
   bool     dq = false;

   smx_SSR_ENTER1(SMX_ID_SEM_SIGNAL, sem);
   smx_EXIT_IF_IN_ISR(SMX_ID_SEM_SIGNAL, false);

   /* verify that sem is valid and that current task has access permission */
   if (pass = smx_SCBTest(sem, SMX_PRIV_LO))
   {
      if (sem->fl == 0) /* no task waiting */
      {
         switch (sem->mode)
         {
            case SMX_SEM_RSRC:
               if (sem->count < sem->lim)
                  sem->count++;
               break;

            case SMX_SEM_EVENT:
               if (sem->lim == 1) /* binary event semaphore */
               {
                  if (sem->count < sem->lim)
                     sem->count++;
                  break;
               }

            case SMX_SEM_THRES: /* non-binary event or threshold semaphore */
               if ((sem->count < 0xFF))
                  sem->count++;
               else
                  smx_ERROR_EXIT(SMXE_SEM_CTR_OVFL, false, 0, SMX_ID_SEM_SIGNAL);
               break;

            case SMX_SEM_GATE:
               break;

            default:
               smx_ERROR_EXIT(SMXE_INV_SCB, false, 0, SMX_ID_SEM_SIGNAL);
         }
      }
      else /* task waiting */
      {
         if (sem->mode == SMX_SEM_THRES)
         {
            if (sem->count >= sem->lim - 1)
            {
               sem->count -= (sem->lim - 1);
               dq = true;
            }
            else
               sem->count++;
         }
         if ((sem->mode != SMX_SEM_THRES) || dq)
         {
            do /* resume first task */
            {
               task = smx_DQFTask((CB_PTR)sem);
               smx_NQRQTask(task);  /* q is set by macro */
               smx_timeout[task->indx] = SMX_TMO_INF;
               task->rv = true;
               smx_PUT_RV_IN_EXR0(task)
            } while (sem->mode == SMX_SEM_GATE && sem->fl); /* resume all tasks */
            smx_DO_CTTEST();
         }
      }
      /* callback */
      if (sem->cbfun)
         sem->cbfun((u32)sem);
   }
   return((bool)smx_SSRExit(pass, SMX_ID_SEM_SIGNAL));
}

/*
*  smx_SemTest()   SSR
*
*  Suspend SSR version. Enters SSR, calls smx_SemTest_F(), then exits SSR.
*  Aborts if called from LSR and tmo != SMX_TMO_NOWAIT. Clears lockctr if
*  called from a task and tmo != SMX_TMO_NOWAIT.
*/
bool smx_SemTest(SCB_PTR sem, u32 timeout)
{
   bool pass = false;

   smx_SSR_ENTER2(SMX_ID_SEM_TEST, sem, timeout);
   smx_EXIT_IF_IN_ISR(SMX_ID_SEM_TEST, false);
   if (!smx_clsr || timeout == SMX_TMO_NOWAIT)
   {
      if (timeout > 0)
         smx_lockctr = 0;
      pass = smx_SemTest_F(sem, timeout);
   }
   else
      smx_ERROR(SMXE_WAIT_NOT_ALLOWED, 0);
   return((bool)smx_SSRExit(pass, SMX_ID_SEM_TEST));
}

/*
*  smx_SemTestStop()   SSR
*
*  Stop SSR version. If called from an LSR, reports OP_NOT_ALLOWED error and
*  returns to LSR. Else, enters SSR, sets sched = STOP, calls smx_SemTest_F(),
*  then exits SSR. Return value is passed via taskMain(par), when task restarts.
*/
void smx_SemTestStop(SCB_PTR sem, u32 timeout)
{
   bool pass;

   if (smx_clsr)
      smx_ERROR_RET_VOID(SMXE_OP_NOT_ALLOWED, 0);
   smx_RET_IF_IN_ISR_VOID();
   smx_SSR_ENTER2(SMX_ID_SEM_TEST_STOP, sem, timeout);
   pass = smx_SemTest_F(sem, timeout);
  #if SMX_CFG_TOKENS
   if (smx_ct->flags.tok_ok)
  #endif
      smx_sched = SMX_CT_STOP;
   smx_SSRExit(pass, SMX_ID_SEM_TEST_STOP);
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  smx_SemTest_F()
*
*  If SCB is not valid, or if SMX_CFG_TOKENS and ct does not have access token,
*  return false, else proceed.
*  If semaphore mode is:
*     RSRC, EVENT, or GATE mode: if count > 0, count-- else pass = false.
*     THRES mode: if count >= lim, count - lim else pass = false.
*     Invalid mode: report SMXE_INV_SCB and return false.
*  If pass == false and timeout > 0, move ct to sem wait queue.
*  Return pass.
*  Called from smx_SemTest() or smx_SemTestStop()
*/
static bool smx_SemTest_F(SCB_PTR sem, u32 timeout)
{
   TCB_PTR  ct = smx_ct; /* globals optimization */
   bool     pass;

   /* verify that sem is valid and that current task has access permission */
   if (pass = smx_SCBTest(sem, SMX_PRIV_LO))
   {
      /* test semaphore*/
      switch (sem->mode)
      {
         case SMX_SEM_RSRC:
         case SMX_SEM_EVENT:
         case SMX_SEM_GATE:
            if (sem->count > 0)
               sem->count--;
            else
               pass = false;
            break;
         case SMX_SEM_THRES:
            if (sem->count >= sem->lim)
               sem->count -= (u8)sem->lim;
            else
               pass = false;
            break;
         default:
            smx_ERROR_RET(SMXE_INV_SCB, false, 0);
      }
      /* sem test failed */
      if (!pass && timeout)
      {
         smx_sched = SMX_CT_SUSP;
         smx_DQRQTask(ct);
         if (sem->mode == SMX_SEM_GATE)
            smx_NQTask((CB_PTR)sem, ct); /* FIFO queue */
         else
            smx_PNQTask((CB_PTR)sem, ct, SMX_CB_SEM); /* priority queue */
         smx_TimeoutSet(ct, timeout);
      }
   }
   return pass;
}


/*
*  smx_SemClear_F()
*
*  Resumes all tasks waiting at sem, with NULL return values. Returns true, if
*  successful.
*/
static bool smx_SemClear_F(SCB_PTR sem)
{
   bool     pass;
   TCB_PTR  task = NULL;

   /* verify that sem is valid and that current task has access permission */
   if (pass = smx_SCBTest(sem, SMX_PRIV_HI))
   {
      while (sem->fl)  /* clear task queue */
      {
         task = smx_DQFTask((CB_PTR)sem);
         smx_NQRQTask(task);
         smx_DO_CTTEST();
         smx_timeout[task->indx] = SMX_TMO_INF;
      }
   }
   return pass;
}