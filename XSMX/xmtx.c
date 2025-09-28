/*
* xmtx.c                                                    Version 5.4.0
*
* smx Mutex Functions
*
* Copyright (c) 2004-2025 Micro Digital Inc.
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
/* smx_MutexGetFast() and smx_MutexRelFast() in xsmx.h since shared */
static bool smx_MutexGet_F(MUCB_PTR mtx, u32 timeout);
static void smx_MutexRemFromMOL(MUCB_PTR mtx, TCB_PTR task);
static void smx_PriReduce(TCB_PTR task);

/*
*  smx_MutexClear()   SSR
*
*  Frees mutex, regardless of owner and nest count, and clears the task
*  queue by resuming all tasks in it. This would normally be used when
*  the mutex is being deleted or for recovery.
*/
bool smx_MutexClear(MUCB_PTR mtx)
{
   u8       np;
   MUCB_PTR nxm;
   bool     pass;
   TCB_PTR  task;

   smx_SSR_ENTER1(SMX_ID_MUTEX_CLEAR, mtx);
   smx_EXIT_IF_IN_ISR(SMX_ID_MUTEX_CLEAR, false);

   /* verify that mutex is valid and that current task has access permission */
   if (pass = smx_MUCBTest(mtx, SMX_PRIV_HI))
   {
      task = mtx->onr;

      if (task != NULL)
      {
         smx_MutexRemFromMOL(mtx, task);

         if (task->pri != task->prinorm)
         {
            if (task->molp == NULL)  /* Restore normal priority */
               np = task->prinorm;
            else /* Set ct->pri to highest mutex ceiling or waiting task priority */
            {
               np = task->prinorm;
               for (nxm = task->molp; nxm != NULL; nxm = nxm->molp)
               {
                  if (nxm->ceil > np)
                     np = nxm->ceil;
                  if (nxm->pi && (nxm->fl != NULL) && (((TCB_PTR)nxm->fl)->pri > np))
                     np = ((TCB_PTR)nxm->fl)->pri;
               }
            }
            /* If ct->pri changes, requeue task and enable preemption */
            if (task->pri != np)
            {
               task->pri = np;
               smx_ReQTask(task);
            }
         }
      }
      mtx->ncnt = 0;                        /* Clear nesting count. */
      mtx->onr  = NULL;                     /* Free mutex. */

      /* Empty task wait queue. */
      while (mtx->fl != NULL)
      {
         if (mtx->fl >= (TCB_PTR)smx_tcbs.pi && mtx->fl <= (TCB_PTR)smx_tcbs.px)
         {
            task = (TCB_PTR)mtx->fl;        /* Get next task. */

            /* Dequeue task from task wait queue. */
            if (task->fl == (CB_PTR)mtx)
               mtx->fl = NULL;
            else
               mtx->fl = (TCB_PTR)task->fl;

            task->fl = NULL;
            task->flags.in_prq = 0;
            task->flags.mtx_wait = 0;           /* Reset task's mutex wait flag. */
            smx_NQRQTask(task);                 /* Enqueue task into rq. */
            smx_DO_CTTEST();                    /* Possible preemption. */
            smx_timeout[task->indx] = SMX_TMO_INF; /* Reset task timer. */
         }
         else
         {
            smx_ERROR(SMXE_BROKEN_Q, 0)
            mtx->fl = NULL;                     /* <2> */
            continue;                           /* Abort emptying task wait queue. */                         
         }
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_MUTEX_CLEAR));
}

/*
*  smx_MutexCreate()   SSR
*
*  Gets an MUCB and initializes it. If parameters are not valid or cannot get
*  an MUCB, does an error exit and returns NULL. Otherwise returns mutex handle.
*/
MUCB_PTR smx_MutexCreate(u8 pi, u8 ceiling, const char* name, MUCB_PTR* muhp)
{
   MUCB_PTR  mp;

   smx_SSR_ENTER4(SMX_ID_MUTEX_CREATE, pi, ceiling, name, muhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_MUTEX_CREATE, NULL);

   /* block multiple creates and verify current task has create permission */
   if ((mp = (MUCB_PTR)smx_ObjectCreateTestH((u32*)muhp)) && !smx_errno)
   {
      /* get a mutex control block */
      if ((mp = (MUCB_PTR)sb_BlockGet(&smx_mucbs, 4)) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_MUCBS, NULL, 0, SMX_ID_MUTEX_CREATE);

      /* initialize MUCB */
      mp->cbtype = SMX_CB_MTX;
      mp->pi     = pi;
      mp->ceil   = (u32)ceiling >= SMX_PRI_NUM ? (u8)SMX_PRI_NUM-1 : ceiling;
      mp->muhp = muhp;
      if (name && *name)
         mp->name = name;

      /* load mutex handle */
      if (muhp)
         *muhp = mp;
   }
   return((MUCB_PTR)smx_SSRExit((u32)mp, SMX_ID_MUTEX_CREATE));
}

/*
*  smx_MutexDelete()   SSR
*
*  Deletes a mutex created by smx_MutexCreate(). Clears the MUCB and its handle.
*/
bool smx_MutexDelete(MUCB_PTR* muhp)
{
   MUCB_PTR mtx = (muhp ? *muhp : NULL);
   bool     pass;

   smx_SSR_ENTER1(SMX_ID_MUTEX_DELETE, mtx);
   smx_EXIT_IF_IN_ISR(SMX_ID_MUTEX_DELETE, false);

   if (pass = smx_MutexClear(mtx))
   {
      /* release and clear MUCB & set mutex handle to nullcb */
      sb_BlockRel(&smx_mucbs, (u8*)mtx, sizeof(MUCB));
      *muhp = NULL;
   }
   return((bool)smx_SSRExit(pass, SMX_ID_MUTEX_DELETE));
}

/*
*  smx_MutexFree()   SSR
*
*  Frees mutex, regardless of owner and nesting count. This would normally
*  be used when the current owner is being stopped or deleted. Makes the
*  next waiting task the new owner or frees the mutex if none waiting.
*/
bool smx_MutexFree(MUCB_PTR mtx)
{
   u8       np;
   MUCB_PTR nxm;
   bool     pass;
   TCB_PTR  task;

   smx_SSR_ENTER1(SMX_ID_MUTEX_FREE, mtx);
   smx_EXIT_IF_IN_ISR(SMX_ID_MUTEX_FREE, false);

   /* verify that mutex is valid and that current task has access permission */
   if (pass = smx_MUCBTest(mtx, SMX_PRIV_HI))
   {
      task = mtx->onr;

      if (task != NULL)
      {
         smx_MutexRemFromMOL(mtx, task);

         if (task->pri != task->prinorm)
         {
            if (task->molp == NULL)  /* Restore normal priority */
               np = task->prinorm;
            else /* Set ct->pri to highest mutex ceiling or waiting task priority */
            {
               np = task->prinorm;
               for (nxm = task->molp; nxm != NULL; nxm = nxm->molp)
               {
                  if (nxm->ceil > np)
                     np = nxm->ceil;
                  if (nxm->pi && (nxm->fl != NULL) && (((TCB_PTR)nxm->fl)->pri > np))
                     np = ((TCB_PTR)nxm->fl)->pri;
               }
            }
            /* If ct->pri changes, requeue task and enable preemption */
            if (task->pri != np)
            {
               task->pri = np;
               smx_ReQTask(task);
            }
         }

         if (mtx->fl == NULL)
         {                                   /* Task queue is empty: */
            mtx->ncnt = 0;                   /* Clear nesting count. */
            mtx->onr  = NULL;                /* Free mutex. */
         }
         else
         {                                   /* Task queue is not empty: */
            task = (TCB_PTR)mtx->fl;         /* Get next task. */
            mtx->onr = task;                 /* Give the mutex to the task. */
            mtx->ncnt = 1;                   /* Set nesting count = 1. */

            if (task->fl == (CB_PTR)mtx)     /* Dequeue task from mutex wait queue. */
               mtx->fl = NULL;               /* <3> */
            else
            {
               task->fl->bl = task->bl;
               task->bl->fl = task->fl;
            }

            /* Link mtx into task's mutex owned list. */
            if (task->molp != NULL)
               mtx->molp = task->molp;
            task->molp = mtx;

            /* Promote task priority to mtx ceiling. */
            if (task->pri < mtx->ceil)
               task->pri = mtx->ceil;

            task->flags.in_prq = 0;
            task->flags.mtx_wait = 0;           /* Reset task's mutex wait flag. */
            smx_NQRQTask(task);                 /* Enqueue task into rq. */
            smx_DO_CTTEST();                    /* Possible preemption. */
            smx_timeout[task->indx] = SMX_TMO_INF; /* Reset task timer. */
            task->rv = true;                    /* Prior smx_MutexGet from task has succeeded. */
            smx_PUT_RV_IN_EXR0(task)
         }
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_MUTEX_FREE));
}

/*
*  smx_MutexGet()    SSR
*
*  If called by an LSR, returns true if mtx owner is NULL. If called by a task 
*  calls smx_MutexGet_F() and returns result.
*/
bool smx_MutexGet(MUCB_PTR mtx, u32 timeout)
{
   bool pass;

   if (smx_clsr)
      return !(bool)mtx->onr; /* allow LSR access if mutex is free */

   smx_SSR_ENTER2(SMX_ID_MUTEX_GET, mtx, timeout);
   smx_EXIT_IF_IN_ISR(SMX_ID_MUTEX_GET, false);
   if (timeout > 0)
      smx_lockctr = 0;
   pass = smx_MutexGet_F(mtx, timeout);
   return((bool)smx_SSRExit(pass, SMX_ID_MUTEX_GET));
}

/*
*  smx_MutexGetStop()   SSR
*
*  Stop SSR version. If called from an LSR, reports OP_NOT_ALLOWED error and
*  returns to LSR. Else, enters SSR, sets sched = STOP, calls smx_MutexGet_F(),
*  then exits SSR. Return value is passed via taskMain(par), when task restarts.
*/
void smx_MutexGetStop(MUCB_PTR mtx, u32 timeout)
{
   bool pass;

   if (smx_clsr)
      smx_ERROR_RET_VOID(SMXE_OP_NOT_ALLOWED, 0);
   smx_RET_IF_IN_ISR_VOID();
   smx_SSR_ENTER2(SMX_ID_MUTEX_GET_STOP, mtx, timeout);
   pass = smx_MutexGet_F(mtx, timeout);
  #if SMX_CFG_TOKENS
   if (smx_ct->flags.tok_ok)
  #endif
      smx_sched = SMX_CT_STOP;
   smx_SSRExit(pass, SMX_ID_MUTEX_GET_STOP);
}

/*
*  smx_MutexPeek()   SSR
*
*  Return the specified information about mtx.
*/
u32 smx_MutexPeek(MUCB_PTR mtx, SMX_PK_PAR par)
{
   u32 val = 0;

   smx_SSR_ENTER2(SMX_ID_MUTEX_PEEK, mtx, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_MUTEX_PEEK, 0);

   /* verify that mtx is valid and that current task has access permission */
   if (val = smx_MUCBTest(mtx, SMX_PRIV_LO))
   {
      switch (par)
      {
         case SMX_PK_FIRST:
            val = (u32)mtx->fl;
            break;
         case SMX_PK_LAST:
            val = (u32)mtx->bl;
            break;
         case SMX_PK_PI:
            val = (u32)mtx->pi;
            break;
         case SMX_PK_CEIL:
            val = (u32)mtx->ceil;
            break;
         case SMX_PK_NAME:
            val = (u32)mtx->name;
            break;
         case SMX_PK_ONR:
            val = (u32)mtx->onr;
            break;
         case SMX_PK_MOLP:
            val = (u32)mtx->molp;
            break;
         case SMX_PK_NCNT:
            val = (u32)mtx->ncnt;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_MUTEX_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_MUTEX_PEEK);
}

/*  smx_MutexRel()   SSR
*
*  Releases mutex, if owned by ct and nest count == 1; makes the next
*  waiting task, if any, the new owner. Else, if owned by ct and nest
*  count > 1, decrements nest count. Error otherwise. Skips if smx_clsr.
*/
bool smx_MutexRel(MUCB_PTR mtx)
{
   TCB_PTR  nxt;  /* next task */
   bool     pass;
   TCB_PTR  task = smx_ct;

   if (smx_clsr)
      return true; /* no-op if called from an LSR */

   smx_SSR_ENTER1(SMX_ID_MUTEX_REL, mtx);
   smx_EXIT_IF_IN_ISR(SMX_ID_MUTEX_REL, false);

   /* verify that mutex is valid and that current task has access permission */
   if (pass = smx_MUCBTest(mtx, SMX_PRIV_LO))
   {
      if (mtx->onr != task)
      {
         if (mtx->onr == NULL)
         {
            smx_ERROR_EXIT(SMXE_MTX_ALRDY_FREE, false, 0, SMX_ID_MUTEX_REL);
         }
         else
         {
            smx_ERROR_EXIT(SMXE_MTX_NON_ONR_REL, false, 0, SMX_ID_MUTEX_REL);
         }
      }
      else  /* Mutex owned by ct */
      {
         if (--mtx->ncnt == 0)               /* Decrement nesting count and test. */
         {
            smx_MutexRemFromMOL(mtx, task);

            /* reduce task priority if it was promoted */
            if (task->pri != task->prinorm)
               smx_PriReduce(task);

            if (mtx->fl != NULL)             /* If mutex task queue is not empty: */
            {
               nxt = (TCB_PTR)mtx->fl;       /* Get next task, nxt. */
               mtx->onr = nxt;               /* Give the mutex to nxt. */
               mtx->ncnt = 1;                /* Set nesting count = 1. */

               if (nxt->fl == (CB_PTR)mtx)    /* Dequeue nxt from mutex wait list. <3> */
                  mtx->fl = NULL;
               else
               {
                  nxt->fl->bl = nxt->bl;
                  nxt->bl->fl = nxt->fl;
               }

               /* Link mtx into nxt's mutex owned list. */
               if (nxt->molp != NULL)
                  mtx->molp = nxt->molp;
               nxt->molp = mtx;

               /* Promote nxt priority to mtx ceiling. */
               if (nxt->pri < mtx->ceil)
                  nxt->pri = mtx->ceil;

               nxt->flags.in_prq = 0;
               nxt->flags.mtx_wait = 0;            /* Reset task's mutex wait flag. */
               smx_NQRQTask(nxt);                  /* Enqueue nxt into rq. */
               smx_DO_CTTEST();                    /* Possible preemption. */
               smx_timeout[nxt->indx] = SMX_TMO_INF; /* Reset nxt timeout timer. */
               nxt->rv = true;                     /* Prior smx_MutexGet from nxt has succeeded. */
               smx_PUT_RV_IN_EXR0(nxt)
            }
            else
               mtx->onr = NULL;              /* Release mutex. */
         }
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_MUTEX_REL));
}

/*
*  smx_MutexSet()   SSR
*
*  Sets the specified mutex parameter to the specified value.
*  Not permitted in umode.
*/
bool smx_MutexSet(MUCB_PTR mtx, SMX_ST_PAR par, u32 v1, u32 v2)
{
   bool pass;
   smx_SSR_ENTER4(SMX_ID_MUTEX_SET, mtx, par, v1, v2);
   smx_EXIT_IF_IN_ISR(SMX_ID_MUTEX_SET, false);

   /* verify that mtx is valid and that current task has access permission */
   if (pass = smx_MUCBTest(mtx, SMX_PRIV_HI))
   {
      #if SMX_CFG_SSMX
      /* a utask cannot make mutex changes */
      if (smx_ct->flags.umode == 1)
         smx_ERROR_EXIT(SMXE_PRIV_VIOL, false, 0, SMX_ID_MUTEX_SET);
      #endif

      /* perform set operation on mutex */
      switch (par)
      {
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_MUTEX_SET);
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_MUTEX_SET));
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  smx_MutexGet_F()
*
*  Gets mutex, if free, and boosts ct priority to mutex ceiling, if enabled.
*  Increments nesting count if ct already owns mutex. Returns true in both cases.
*  If mutex is not free, and timeout > 0, waits and promotes priority of mutex
*  owner, if priority inheritance is enabled. If no timeout, returns. Returns
*  false in both cases.
*/
bool smx_MutexGet_F(MUCB_PTR mtx, u32 timeout)
{
   bool      pass;  
   RQCB_PTR  q;
   TCB_PTR   ct = smx_ct; 

   /* verify that mutex is valid and that current task has access permission */
   if ((pass = smx_MUCBTest(mtx, SMX_PRIV_LO)))
   {
      if (mtx->onr == NULL) /* mutex is free */
      {
         mtx->onr = ct;    /* get mutex */
         mtx->ncnt = 1;    /* nesting count = 1 */

         /* link mtx into ct's mutex owned list. */
         if (ct->molp != NULL)
            mtx->molp = ct->molp;
         ct->molp = mtx;

         /* promote ct priority to mtx ceiling, if necessary */
         if (ct->pri < mtx->ceil)
         {
            ct->pri = mtx->ceil;

            /* remove ct from rq. ct is re-queued and smx_rqtop is adjusted below */
            if (ct->fl == ct->bl)
            {
               ct->fl->fl = NULL;
               ((RQCB_PTR)(ct->fl))->tq = 0;
            }
            else
            {
               ct->fl->bl = ct->bl;
               ct->bl->fl = ct->fl;
            }

            /* Move ct to higher priority in rq. */
            q = smx_rq + ct->pri;
            smx_NQTask((CB_PTR)q, ct);
            q->tq = 1;
            if (smx_rqtop < q)
               smx_rqtop = q;
         }
      }
      else  /* mutex is not free: */
      {
         if (mtx->onr == ct) /* test if already owned by ct. */
         {
            mtx->ncnt++;     /* incr nesting count. */
         }
         else
         {
            if (timeout) /* wait for mutex to be released. */
            {
               /* move ct from rq to the mutex wait queue and set mutex wait flag */
               smx_DQRQTask(ct);
               smx_PNQTask((CB_PTR)mtx, ct, SMX_CB_MTX);
               ct->flags.mtx_wait = 1;

               /* priority promotion, if necessary */
               if ((mtx->pi != 0) && (ct->pri > mtx->onr->pri))
               {
                  mtx->onr->pri = ct->pri;
                  smx_ReQTask(mtx->onr);
               }
               smx_TimeoutSet(ct, timeout);
               smx_sched = SMX_CT_SUSP;
            }
            pass = false;
         }
      }
   }
   return pass;
}

/*
*  smx_MutexGetFast()
*
*  Fast get mutex for heap and similar internal mutexes. Can be used inside SSR  
*  or outside SSR. Mutex must be released only with smx_MutexRelFast().  
*  Increments nest count if ct already owns mutex. Returns true in both cases.
*  If mutex is not free, and timeout > 0, waits and promotes priority of mutex
*  owner, if priority inheritance is enabled. If no timeout, returns. Returns
*  false in both cases.
*/
bool smx_MutexGetFast(MUCB_PTR mtx, u32 timeout)
{
   TCB_PTR  ct = smx_ct;
   bool     pass;

   if (smx_clsr)
      return !(bool)mtx->onr; /* allow LSR access, if free */

   smx_srnest++;

   if (mtx->onr == NULL)   /* mutex is free */
   {
      mtx->onr = ct;       /* get mutex */
      mtx->ncnt = 1;       /* nesting count = 1 */
      pass = true;
   }
   else /* mutex is not free: */
   {
      if (mtx->onr == ct) /* test if already owned by ct. */
      {
         mtx->ncnt++;     /* incr nesting count. */
      }
      else
      {   
         if (timeout) /* wait for mutex to be released. */ 
         {
            smx_DQRQTask(ct);
            smx_PNQTask((CB_PTR)mtx, ct, SMX_CB_MTX);
            ct->flags.mtx_wait = 1;
            smx_TimeoutSet(ct, timeout);
            smx_lockctr = 0;
            smx_sched = SMX_CT_SUSP;

            /* if enabled, promote priority of mtx->onr */
            if ((mtx->pi != 0) && (ct->pri > mtx->onr->pri))
            {
               mtx->onr->pri = ct->pri;
               smx_ReQTask(mtx->onr);
            }
         }
         pass = false;
      }
   }
   return((bool)smx_SSRExitIF(pass));
}

/*  
*  smx_MutexRelFast()
*
*  Fast release mutex gotten by smx_MutexGetFast(). Makes the next waiting task,
*  if any, the new owner, and enables it to preempt the current task. Returns
*  SMX_HEAP_RETRY for svc heap functions to indicate that function recall is
*  necessary.
*/
void smx_MutexRelFast(MUCB_PTR mtx)
{
   TCB_PTR  nxt;  /* next task */

   if (smx_clsr)
      return;     /* no-op if called from an LSR */

   smx_srnest++;  /* protect if not in an SSR */

   /* reduce smx_ct priority if it was promoted */
   if (smx_ct->pri != smx_ct->prinorm)
      smx_PriReduce(smx_ct);

   if (mtx->fl != NULL)
   {
      /* Dequeue next task from mutex wait list. <3> */
      nxt = (TCB_PTR)mtx->fl;
      if (nxt->fl == (CB_PTR)mtx)
         mtx->fl = NULL;
      else
      {
         nxt->fl->bl = nxt->bl;
         nxt->bl->fl = nxt->fl;
      }

      /* give mutex to nxt. */
      mtx->onr = nxt;
      mtx->ncnt = 1;

      /* get nxt ready to run */
      nxt->flags.in_prq = 0;
      nxt->flags.mtx_wait = 0;               /* reset task's mutex wait flag. */
      smx_NQRQTask(nxt);                     /* enqueue nxt into rq. */
      smx_DO_CTTEST();                       /* possible preemption. */
      smx_timeout[nxt->indx] = SMX_TMO_INF;  /* reset nxt timeout timer. */
      nxt->rv = SMX_HEAP_RETRY;              /* get mtx from nxt has succeeded. */
      smx_PUT_RV_IN_EXR0(nxt)                /* nxt->sp = rv */
   }
   else if (mtx->onr == smx_ct) /* test if already owned by ct. */
   {
      mtx->ncnt--;         /* decr nesting count. */
      if (mtx->ncnt == 0)
         mtx->onr = NULL;  /* release mutex */
   }
   smx_SSRExitIF(SMX_HEAP_RETRY);  /* enable preemption */
}

/*
*  smx_MutexRemFromMOL(mtx)
*
*  Find and remove mtx from task's mutex-owned list (MOL). Also check MOL and
*  report if broken. If break is found before mtx, mtx will not be removed
*  from the queue, but the queue head (task->molp) will be cleared <4>.
*/
void smx_MutexRemFromMOL(MUCB_PTR mtx, TCB_PTR task)
{
   static MUCB_PTR  nxm;
   MUCB_PTR        *mpp;

   if (task->molp != NULL)     /* <4> */
   {
      mpp = &task->molp;

      /* trace MOL to end or break and remove mtx, when found */
      for (nxm = task->molp; nxm != NULL && nxm->cbtype == SMX_CB_MTX;
                                          mpp = &(nxm->molp), nxm = nxm->molp)
         if (nxm == mtx)
            *mpp = mtx->molp;

      /* report if MOL is broken */
      if (nxm != NULL)
      {
         smx_ERROR(SMXE_BROKEN_Q, 0)
         task->molp = NULL;   /* <4> */
      }
   }
   mtx->molp = NULL; /* <5> */
}

/* If task owns other mutexes, set its priority to the highest mutex ceiling 
   or waiting task priority, else to its the normal priority and requeue task 
   in rq, adjust rqtop, and invoke scheduler. 
*/
void smx_PriReduce(TCB_PTR task)
{
   u8       np;   /* new priority */
   MUCB_PTR nxm;  /* next mutex */
   RQCB_PTR nrq;  /* new rq level */

   np = task->prinorm;

   if (task->molp != NULL)
   {
    /* Set np at highest mutex ceiling or waiting task priority */
      for (nxm = task->molp; nxm != NULL; nxm = nxm->molp)
      {
         if (nxm->ceil > np)
            np = nxm->ceil;
         if (nxm->pi && (nxm->fl != NULL) && (((TCB_PTR)nxm->fl)->pri > np))
            np = ((TCB_PTR)nxm->fl)->pri;
      }
   }
   /* set task priority to np */
   task->pri = np;

   /* dequeue task from rq */
   if (task->fl == task->bl)
   {
      task->fl->fl = NULL;
      ((RQCB_PTR)(task->fl))->tq = 0;
   }
   else
   {
      task->fl->bl = task->bl;
      task->bl->fl = task->fl;
   }

   /* requeue task in rq */
   nrq = smx_rq + task->pri;
   smx_NQTask((CB_PTR)nrq, task);
   nrq->tq = 1;

   /* if the rqtop rq level is empty, adjust rqtop to the next lower-priority 
      level that is occupied */
   while ((smx_rqtop->tq == 0) && (smx_rqtop > smx_rq))
      smx_rqtop--;

   smx_DO_CTTEST();  /* invoke task scheduler */
}

/* Notes:
   1. If mutex is not owned, it cannot have a wait queue, either.
   2. Setting mtx->fl to NULL ensures that the mtx ends up cleared and that
      the return value is true despite the broken queue. The consequence
      of the broken queue is that the waiting tasks beyond the break are
      orphaned. However, task timeouts should restore them to the ready
      queue with indication of failure to get mtx.
   3. Not necessary to clear task->fl since it is immediately put into rq.
   4. Done so the broken queue will be reported only once and molp will
      end up correct after all owned mutexes have been released. Meanwhile,
      functions using this task's MOL will be impacted. Note that this
      function will return normally and the rest of the function calling
      it will proceed normally.
   5. It is known that mtx is owned by this task, hence it is OK to clear its
      molp even if it is not found in the task's MOL.
*/
