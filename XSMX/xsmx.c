/*
* xsmx.c                                                    Version 5.4.0
*
* smx internal subroutines.
*
* Copyright (c) 2001-2025 Micro Digital Inc.
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

/*===========================================================================*
*                            QUEUEING FUNCTIONS                              *
*===========================================================================*/

/*
*  smx_DQFMsg(q)
*
*  Dequeues the first msg in queue q and returns its handle. Assumes q is valid.
*  If q is an exchange and becomes empty, sets mq = 0.
*/
MCB_PTR smx_DQFMsg(CB_PTR q)
{
   MCB_PTR  m;

   m = (MCB_PTR)q->fl;
   if (m->fl == q)
   {
      q->fl = NULL;
      if (m->fl->cbtype == SMX_CB_XCHG)
         ((XCB_PTR)(m->fl))->flags.mq = 0;
   }
   else
   {
      q->fl = m->fl;
      m->fl->bl = q;
   }
   m->fl = NULL;
   return m;
}

/*
*  smx_DQMsg(m)
*
*  Dequeues msg m from the queue it is in. Assumes m is valid and in a queue.
*  If queue is an exchange and becomes empty, sets mq = 0.
*/
void smx_DQMsg(MCB_PTR m)
{
   if (m->fl == m->bl)
   {
      m->fl->fl = NULL;
      if (m->fl->cbtype == SMX_CB_XCHG)
         ((XCB_PTR)(m->fl))->flags.mq = 0;
   }
   else
   {
      m->fl->bl = m->bl;
      m->bl->fl = m->fl;
   }
   m->fl = NULL;
}

/*
*  smx_NQMsg(q, m)
*
*  Enqueues msg m at the end of the queue headed by QCB q. If q is empty
*  sets fl and bl simultaneously for speed. Assumes q is valid.
*/
void smx_NQMsg(CB_PTR q, MCB_PTR m)
{
   if (q->fl)
   {
      m->fl = q;
      m->bl = q->bl;
      q->bl->fl = (CB_PTR)m;
      q->bl = (CB_PTR)m;
   }
   else
   {
      m->fl = (m->bl = q);
      q->fl = (q->bl = (CB_PTR)m);
   }
}

/*
*  smx_PNQMsg(q, m, cb)
*
*  Enqueues msg m in queue q, by priority -- highest first. Does a simple
*  linear search of q. Assumes valid q and m. If q is broken, reports
*  SMXE_BROKEN_Q and enqueues m before the break as the last msg.
*/
void smx_PNQMsg(CB_PTR q, MCB_PTR m, u32 cb)
{
   MCB_PTR n, p;

   if (q->fl == NULL)  /* Fast enqueue, if queue empty. */
   {
      m->bl  = q;
      m->fl  = q;
      q->bl = (CB_PTR)m;
      q->fl = (CB_PTR)m;
   }
   else
   {
      /* if m->pri is the minimum priority */
      if (m->pri == 0)
      {
         /* put m at end */
         q->bl->fl = (CB_PTR)m;
         m->bl = q->bl;
         q->bl = (CB_PTR)m;
         m->fl = q;
      }
      else   /* enqueue in decreasing priority order */
      {
         p = n = (MCB_PTR)q->fl;

         /* scan queue for position, end, or break */
         while ((n->cbtype == SMX_CB_MCB) && (m->pri <= n->pri))
         {
            p = n;
            n = (MCB_PTR)n->fl;
         }

         if (n->cbtype == SMX_CB_MCB || n->cbtype == cb)
         {
            /* Insert m ahead of n. */
            m->fl = (CB_PTR)n;
            m->bl = n->bl;
            n->bl->fl = (CB_PTR)m;
            n->bl = (CB_PTR)m;
         }
         else /* broken queue */
         {
            smx_ERROR(SMXE_BROKEN_Q, 0);

            /* insert m ahead of break */
            m->fl = q;
            q->bl = (CB_PTR)m;
            m->bl = (CB_PTR)p;
            p->fl = (CB_PTR)m;
         }
      }
   }
}

/*
*  smx_DQFTask(q)
*
*  Dequeues the first task in a queue and returns its handle. Assumes q is valid.
*/
TCB_PTR smx_DQFTask(CB_PTR q)
{
   TCB_PTR  t;

   t = (TCB_PTR)q->fl;
   if (t->fl == q)
      q->fl = NULL;
   else
   {
      q->fl = t->fl;
      t->fl->bl = q;
   }
   t->fl = NULL;
   t->flags.in_prq = 0;
   return t;
}

/*
*  smx_DQRQTask(t)
*
*  Dequeues task t from the ready queue. If the top level of rq is now empty,
*  finds the top task and points smx_rqtop to that level.
*
*  Note: Do not use this to dequeue and requeue a task.
*        Instead call smx_ReQTask().
*/
void smx_DQRQTask(TCB_PTR t)
{
   if (t->fl == t->bl)
   {
      t->fl->fl = NULL;
      ((RQCB_PTR)(t->fl))->tq = 0;
      if ((RQCB_PTR)(t->fl) == smx_rqtop)
      {
         while ((smx_rqtop->tq == 0) && (smx_rqtop > smx_rq))
            smx_rqtop--;
      }
   }
   else if (t->fl != NULL)
   {
      t->fl->bl = t->bl;
      t->bl->fl = t->fl;
   }
   t->fl = NULL;
   t->state = SMX_TASK_WAIT;
}

/*
*  smx_DQTask(t)
*
*  Dequeue task t from whatever queue it may be in:
*  Event Queue: Add the diff count to the next task's diff count, if there is
*               one, and clear flags.in_eq.
*  RQ: Do like smx_DQRQTask.
*  Alter the tq field only if RQ, XCB, or EVCB.
*  Clears task->sv. This function is called by functions such as smx_TaskStop(),
*  smx_TaskStart(), smx_TaskResume(), which can operate on a task waiting
*  anywhere, such as an event queue or a pipe, so we need to clear sv.
*
*  Note: Do not use this to dequeue and requeue a task. Instead call
*        smx_TaskRequeue().
*/
void smx_DQTask(TCB_PTR t)
{
   if (t->flags.in_eq == 1)
   {
      if (((TCB_PTR)t->fl)->cbtype == SMX_CB_TASK)
          ((TCB_PTR)t->fl)->sv += t->sv;
      t->flags.in_eq = 0;
   }
   if (t->fl == t->bl) /* only task in queue */
   {
      if ((t->fl >= (CB_PTR)smx_xcbs.pi) && (t->fl <= (CB_PTR)smx_xcbs.px))
      {
         /* clear tq if in an exchange queue */
         ((XCB_PTR)(t->fl))->flags.tq = 0;
      }
      if ((t->fl >= (CB_PTR)smx_rq) && (t->fl <= (CB_PTR)smx_rqx))
      {
         /* clear tq if in ready queue and move rqtop to lower level */
         ((RQCB_PTR)(t->fl))->tq = 0;
         if ((RQCB_PTR)(t->fl) == smx_rqtop)
            while ((smx_rqtop->tq == 0) && (smx_rqtop > smx_rq))
               smx_rqtop--;
      }
      t->fl->fl = NULL;
   }
   else
   {
      t->fl->bl = t->bl;
      t->bl->fl = t->fl;
   }
   t->flags.in_prq = 0;
   t->flags.mtx_wait = 0;
   t->state = SMX_TASK_WAIT;
   t->sv = 0;
   t->fl = NULL;
}

/*
*  smx_DQTimer()
*
*  Dequeues timer from tq and returns time left. Note: Test that timer is in tq
*  before calling this function (i.e. tmr != NULL && tmr != NULL).
*/
u32 smx_DQTimer(TMRCB_PTR tmr)
{
   u32 time;
   TMRCB_PTR nxt;

   /* accumulate time until tmr or end of queue found */
   time = tmr->diffcnt;
   for (nxt = (TMRCB_PTR)smx_tq; (nxt->fl != tmr) && nxt->fl != NULL; nxt = nxt->fl)
      time += nxt->fl->diffcnt;

   /* unlink timer if in tq and add its diffcnt to next timer, if any */
   if (nxt->fl)
   {
      nxt->fl = tmr->fl;
      if (nxt->fl != NULL)
         nxt->fl->diffcnt += tmr->diffcnt;
   }
   return time;
}

/*
*  smx_NQRQTask(t)
*
*  Enqueues task t into the appropriate level of the ready queue. Limits t->pri
*  to SMX_PRI_NUM-1, if necessary. If task is now above smx_rqtop, updates it.
*/
void smx_NQRQTask(TCB_PTR t)
{
   RQCB_PTR q = smx_rq;
   q += ((u32)(t->pri) < SMX_PRI_NUM) ? (u32)(t->pri) : SMX_PRI_NUM-1;
   if (q->fl)
   {
      t->fl = (CB_PTR)q;
      t->bl = (CB_PTR)q->bl;
      q->bl->fl = (CB_PTR)t;
      q->bl = t;
   }
   else
   {
      t->fl = (t->bl = (CB_PTR)q);
      q->fl = (q->bl = t);
      q->tq = 1;
   }

   if (q > smx_rqtop)
   {
      smx_rqtop = q;
   }
   t->state = SMX_TASK_READY;
}

/*
*  smx_NQTask(q, t)
*
*  Enqueues task t at the end of the queue headed by queue control block, q.
*  If q is empty, sets fl and bl simultaneously for speed. Assumes q is valid.
*/
void smx_NQTask(CB_PTR q, TCB_PTR t)
{
   if (q->fl)
   {
      t->fl = q;
      t->bl = q->bl;
      q->bl->fl = (CB_PTR)t;
      q->bl = (CB_PTR)t;
   }
   else
   {
      t->fl = (t->bl = q);
      q->fl = (q->bl = (CB_PTR)t);
   }
}

/*
*  smx_NQTimer()
*
*  Enqueues timer in tq, based upon delay.
*/
void smx_NQTimer(TMRCB_PTR tmr, u32 delay)
{
   TMRCB_PTR nxt;  /* next timer in tq */
   TMRCB_PTR prev; /* tq or previous timer in tq */

   prev = (TMRCB_PTR)smx_tq;
   for (nxt = prev->fl; nxt != NULL; nxt = nxt->fl)
   {
      if (delay < nxt->diffcnt)
      {
         nxt->diffcnt -= delay;
         break;
      }
      delay -= nxt->diffcnt;
      prev = nxt;
   }
   tmr->diffcnt = delay;
   prev->fl = tmr;
   tmr->fl = nxt;
}

/*
*  smx_PNQTask(q, t, cb)
*
*  Enqueues task t in queue q, by priority -- highest first. Does a simple
*  linear search of q. Assumes valid q and t. If q is broken, reports
*  SMXE_BROKEN_Q and enqueues t before the break as the last task. Sets in_prq
*  flag in TCB for t.
*/
void smx_PNQTask(CB_PTR q, TCB_PTR t, u32 cb)
{
   TCB_PTR n, p;

   if (q->fl == NULL)  /* Fast enqueue, if queue empty. */
   {
      t->bl  = q;
      t->fl  = q;
      q->bl = (CB_PTR)t;
      q->fl = (CB_PTR)t;
   }
   else  /* Enqueue in decreasing priority order. */
   {
      n = (TCB_PTR)q->fl;
      p = (TCB_PTR)q; /*CS*/

      /* scan queue for position, end, or break */
      while ((n != NULL) && (n->cbtype == SMX_CB_TASK) && (t->pri <= n->pri))
      {
         p = n;
         n = (TCB_PTR)n->fl;
      }

      if ((n != NULL) && (n->cbtype == SMX_CB_TASK || n->cbtype == cb))
      {
         /* Insert t ahead of n. */
         t->fl = (CB_PTR)n;
         t->bl = n->bl;
         n->bl->fl = (CB_PTR)t;
         n->bl = (CB_PTR)t;
      }
      else /* broken queue */
      {
         smx_ERROR(SMXE_BROKEN_Q, 0);

         /* insert t ahead of break */
         t->fl = q;
         q->bl = (CB_PTR)t;
         t->bl = (CB_PTR)p;
         p->fl = (CB_PTR)t;
      }
   }
   t->flags.in_prq = 1;
}

/*
*  smx_ReQTask()
*
*  Requeues task due to a priority change.
*/
bool smx_ReQTask(TCB_PTR t)
{
   MUCB_PTR  mtx; /* mutex */
   TCB_PTR   nxt; /* next task */
   CB_PTR      q; /* queue head found by search */

   /* abort if task is not in a queue */
   if (t->fl == 0)
      return false;

   /* find queue head. stop if come back to task */
   for (q = (CB_PTR)t->fl; (q != (CB_PTR)t) && (q->cbtype == SMX_CB_TASK);
        q = (CB_PTR)q->fl){}

   /* abort if queue is broken */
   if (!smx_TEST_BRKNQ(q))
      smx_ERROR_RET(SMXE_BROKEN_Q, false, 0);

   if (t->state == SMX_TASK_RUN || t->state == SMX_TASK_READY) /* task is in smx_rq */
   {
      /* remove task from smx_rq & adjust rqtop, if necessary <2> */
      smx_DQRQTask(t);

      /* enqueue task at new level and adjust rqtop, if necessary <2> */
      smx_NQRQTask(t);

      /* preempt if task is now top task */
      smx_DO_CTTEST();
   }
   else if (t->flags.in_prq) /* task is in a priority queue <3> */
   {
      /* try for higher postion */
      nxt = (TCB_PTR)t->bl;
      if ((nxt->cbtype == SMX_CB_TASK) && (t->pri > nxt->pri))
      {
         while ((nxt->cbtype == SMX_CB_TASK) && (t->pri > nxt->pri))
            nxt = (TCB_PTR)nxt->bl;
         if ((nxt->cbtype != SMX_CB_TASK) && (nxt->cbtype != q->cbtype))
            smx_ERROR_RET(SMXE_BROKEN_Q, false, 0);
      }
      else
      {
         /* try for lower position */
         nxt = (TCB_PTR)t->fl;
         while ((nxt->cbtype == SMX_CB_TASK) && (t->pri <= nxt->pri))
            nxt = (TCB_PTR)nxt->fl;

         /* back up one postion <5> */
         nxt = (TCB_PTR)nxt->bl;
      }

      /* move task, if necessary */
      if ((nxt->fl != (CB_PTR)t) && (nxt != t))
      {
         /* dequeue task */
         t->fl->bl = t->bl;
         t->bl->fl = t->fl;

         /* requeue task after nxt */
         t->bl    = (CB_PTR)nxt;
         t->fl    = nxt->fl;
         nxt->fl->bl = (CB_PTR)t;
         nxt->fl     = (CB_PTR)t;
      }

      /* promote mutex owner priority and requeue it, if inheritance is
         enabled and new_priority is higher */
      if (t->bl->cbtype == SMX_CB_MTX) /* task must be first in mutex wait queue */
      {
         mtx = (MUCB_PTR)t->bl;
         if (mtx->pi && (mtx->onr->pri < t->pri))
         {
            mtx->onr->pri = t->pri;
            smx_ReQTask(mtx->onr);  /* recursion <4> */
         }
      }
   }
   return true;
}

/*===========================================================================*
*                       CONTROL BLOCK TEST FUNCTIONS                         *
*===========================================================================*/

bool smx_BCBTest(BCB_PTR blk, bool priv)
{
   if (!(blk != 0 && blk >= (BCB_PTR)smx_bcbs.pi && blk <= (BCB_PTR)smx_bcbs.px))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_BCB, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)blk->bhp, priv));
}

bool smx_EGCBTest(EGCB_PTR eg, bool priv)
{
   if (!(eg != 0 && eg >= (EGCB_PTR)smx_egcbs.pi && eg <= (EGCB_PTR)smx_egcbs.px &&
         eg->cbtype == SMX_CB_EG)) 
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_EGCB, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)eg->eghp, priv));
}

bool smx_EQCBTest(EQCB_PTR eq, bool priv)
{
   if (!(eq != 0 && eq >= (EQCB_PTR)smx_eqcbs.pi && eq <= (EQCB_PTR)smx_eqcbs.px &&
         eq->cbtype == SMX_CB_EQ)) 
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_EQCB, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)eq->eqhp, priv));
}

bool smx_LCBTest(LCB_PTR lsr, bool priv)
{
   if (!(lsr != 0 && lsr >= (LCB_PTR)smx_lcbs.pi && lsr <= (LCB_PTR)smx_lcbs.px &&
        lsr->cbtype == SMX_CB_LSR))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_LCB, false, 0);
   }
   /* abort if an lsr is making the call */
   if (smx_clsr)
      smx_ERROR_RET(SMXE_PRIV_VIOL, false, 0);

//   return(smx_TOKEN_TEST(smx_ct, (u32)lsr->mhp, priv));
   return true; // temporary
}

bool smx_MCBTest(MCB_PTR msg, bool priv)
{
   if (!(msg != 0 && msg >= (MCB_PTR)smx_mcbs.pi && msg <= (MCB_PTR)smx_mcbs.px &&
        msg->cbtype == SMX_CB_MCB))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_MCB, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)msg->mhp, priv));
}

bool smx_MCBOnrTest(MCB_PTR msg, bool priv)
{
   if (!(msg != 0 && msg >= (MCB_PTR)smx_mcbs.pi && msg <= (MCB_PTR)smx_mcbs.px &&
        msg->cbtype == SMX_CB_MCB))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_MCB, false, 0);
   }
   if (!(msg->onr == smx_ct || smx_clsr != NULL))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_NOT_MSG_ONR, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)msg->mhp, priv));
}

bool smx_MUCBTest(MUCB_PTR mtx, bool priv)
{
   if (!(mtx != 0 && mtx >= (MUCB_PTR)smx_mucbs.pi && mtx <= (MUCB_PTR)smx_mucbs.px &&
        mtx->cbtype == SMX_CB_MTX))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_MUCB, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)mtx->muhp, priv));
}

bool smx_PCBTest(PCB_PTR pool, bool priv)
{
   if (!(pool != 0 && pool >= (PCB_PTR)smx_pcbs.pi && pool <= (PCB_PTR)smx_pcbs.px &&
         pool->cbtype == SB_CB_PCB))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_PCB, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)pool->php, priv));
}

bool smx_PICBTest(PICB_PTR pipe, bool priv)
{
   if (!(pipe != 0 && pipe >= (PICB_PTR)smx_picbs.pi && pipe <= (PICB_PTR)smx_picbs.px &&
         pipe->cbtype == SMX_CB_PIPE))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_PICB, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)pipe->php, priv));
}

bool smx_SCBTest(SCB_PTR sem, bool priv)
{
   if (!(sem != 0 && sem >= (SCB_PTR)smx_scbs.pi && sem <= (SCB_PTR)smx_scbs.px &&
         sem->cbtype == SMX_CB_SEM))
   {      
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_SCB, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)sem->shp, priv));
}    

bool smx_TCBTest(TCB_PTR task, bool priv)
{
   if (!(task != 0 && task >= (TCB_PTR)smx_tcbs.pi && task <= (TCB_PTR)smx_tcbs.px &&
         task->cbtype == SMX_CB_TASK))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_TCB, false, 0);
   }
   if (task != smx_ct)
      return(smx_TOKEN_TEST(smx_ct, (u32)task->thp, priv));
   else
      return true;
}

bool smx_TMRCBTest(TMRCB_PTR tmr, bool priv)
{
  #if !defined(SMX_FRPORT)
   if (!(tmr != 0 && tmr >= (TMRCB_PTR)smx_tmrcbs.pi && tmr <= (TMRCB_PTR)smx_tmrcbs.px && 
         tmr->cbtype == SMX_CB_TMR && tmr == *(tmr->tmhp))) /*<6>*/
  #else
   if (!(tmr != 0 && tmr >= (TMRCB_PTR)smx_tmrcbs.pi && tmr <= (TMRCB_PTR)smx_tmrcbs.px && 
         tmr->cbtype == SMX_CB_TMR)) /*<6>*/
  #endif
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_TMRCB, false, 0);
   }
   if (tmr && tmr != NULL)
      return(smx_TOKEN_TEST(smx_ct, (u32)tmr->tmhp, priv));
   else
      return true;
}

bool smx_XCBTest(XCB_PTR xchg, bool priv)
{
   if (!(xchg != 0 && xchg >= (XCB_PTR)smx_xcbs.pi && xchg <= (XCB_PTR)smx_xcbs.px &&
         xchg->cbtype == SMX_CB_XCHG))
   {
      smx_ct->flags.tok_ok = 1;  /*<1>*/
      smx_ERROR_RET(SMXE_INV_XCB, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)xchg->xhp, priv));
}

bool smx_ObjectCreateTest(u32* hp)
{
   /* block multiple creates of same object */
   if (hp != NULL && *hp != NULL && *hp != NULL)
   {
      smx_ERROR_RET(SMXE_INV_OP, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)hp, SMX_PRIV_HI));
}

u32 smx_ObjectCreateTestH(u32* hp)
{
   /* block multiple creates of same object */
   smx_errno = SMXE_OK;
   if (hp != NULL && *hp != NULL)
   {
      smx_ERROR_RET(SMXE_INV_OP, *hp, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)hp, SMX_PRIV_HI));
}

bool smx_ObjectDupTest(u32* hp)
{
   /* test for valid hp */
   if (hp == NULL)
   {
      smx_ERROR_RET(SMXE_INV_PAR, false, 0);
   }
   /* block multiple duplication of same object */
   if (*hp != NULL)
   {
      smx_ERROR_RET(SMXE_INV_OP, false, 0);
   }
   return(smx_TOKEN_TEST(smx_ct, (u32)hp, SMX_PRIV_HI));
}

#if SMX_CFG_TOKENS
/* Verify task or its top parent does not need token or has required token. */
bool smx_TokenTest(TCB_PTR task, u32 hp, bool priv)
{
   u32*  p;
   bool  pass = false;

   if (task->parent == NULL)
   {
      p = task->tap;
   }
   else
   {
      p = (u32*)*(task->tap);   /* use top parent's tap */
   }

   if (p == 0 || smx_clsr)
      pass = true;
   else
   {
      if (hp == NULL)
         smx_ERROR_RET(SMXE_INV_PAR, false, 0);
      for (; *p != 0; p++)
         if ((hp == (*p & 0xFFFFFFFC)) && ((*p & 1) >= priv) && (hp != NULL))
         {
            pass = true;
            break;
         }
   }
   if (pass)
   {
      smx_ct->flags.tok_ok = 1;
      return true;
   }
   else
   {
      smx_ct->flags.tok_ok = 0;
      smx_ERROR_RET(SMXE_TOKEN_VIOL, false, 1);
   }
}
#endif /* SMX_CFG_TOKENS */

/*===========================================================================*
*                          MISCELLANEOUS FUNCTIONS                           *
*===========================================================================*/

/*
*  smx_CBPoolsCreate()
*
*  Creates all enabled smx control block pools. Notes: 
*     1. Pool PCBs are defined statically in xglob.c and do not come from the 
*        smx_pcbs pool.
*     2. Pools, e.g. bcb_pool, are not globally accessible. Use smx_xxbs.pi,
*        instead.
*/
bool smx_CBPoolsCreate(void)
{
   bool pass = true;

   #if SMX_NUM_BLOCKS
   static BCB bcb_pool[SMX_NUM_BLOCKS];
   pass &= sb_BlockPoolCreate((u8*)&bcb_pool, &smx_bcbs, SMX_NUM_BLOCKS, 
                                                      sizeof(BCB), "smx_bcbs");
   #endif

   #if SMX_NUM_EGS
   static EGCB egcb_pool[SMX_NUM_EGS];
   pass &= sb_BlockPoolCreate((u8*)&egcb_pool, &smx_egcbs, SMX_NUM_EGS, 
                                                   sizeof(EGCB), "egcb_pool");
   #endif

   #if SMX_NUM_EQS
   static EQCB eqcb_pool[SMX_NUM_EQS];
   pass &= sb_BlockPoolCreate((u8*)&eqcb_pool, &smx_eqcbs, SMX_NUM_EQS, 
                                                   sizeof(EQCB), "eqcb_pool");
   #endif

   #if SMX_NUM_LSRS
   static LCB lcb_pool[SMX_NUM_LSRS];
   pass &= sb_BlockPoolCreate((u8*)&lcb_pool, &smx_lcbs, SMX_NUM_LSRS, 
                                                      sizeof(LCB), "smx_lcbs");
   #endif

   #if SMX_NUM_MSGS
   static MCB mcb_pool[SMX_NUM_MSGS];
   pass &= sb_BlockPoolCreate((u8*)&mcb_pool, &smx_mcbs, SMX_NUM_MSGS, 
                                                      sizeof(MCB), "smx_mcbs");
   #endif

   #if SMX_NUM_MTXS
   static MUCB mucb_pool[SMX_NUM_MTXS];
   pass &= sb_BlockPoolCreate((u8*)&mucb_pool, &smx_mucbs, SMX_NUM_MTXS, 
                                                   sizeof(MUCB), "mucb_pool");
   #endif

   #if SMX_NUM_PIPES
   static PICB picb_pool[SMX_NUM_PIPES];
   pass &= sb_BlockPoolCreate((u8*)&picb_pool, &smx_picbs, SMX_NUM_PIPES, 
                                                   sizeof(PICB), "picb_pool");
   #endif

   #if SMX_NUM_POOLS
   static PCB pcb_pool[SMX_NUM_POOLS];
   pass &= sb_BlockPoolCreate((u8*)&pcb_pool, &smx_pcbs, SMX_NUM_POOLS, 
                                                     sizeof(PCB), "smx_pcbs");
   #endif

   #if SMX_NUM_SEMS
   static SCB scb_pool[SMX_NUM_SEMS];
   pass &= sb_BlockPoolCreate((u8*)&scb_pool, &smx_scbs, SMX_NUM_SEMS, 
                                                     sizeof(SCB), "scb_pool");
   #endif

   #if SMX_NUM_TASKS
   static TCB tcb_pool[SMX_NUM_TASKS];
   pass &= sb_BlockPoolCreate((u8*)&tcb_pool, &smx_tcbs, SMX_NUM_TASKS, 
                                                     sizeof(TCB), "tcb_pool");
   #endif

   #if SMX_NUM_TIMERS
   static TMRCB tmcb_pool[SMX_NUM_TIMERS];
   pass &= sb_BlockPoolCreate((u8*)&tmcb_pool, &smx_tmrcbs, SMX_NUM_TIMERS, 
                                                   sizeof(TMRCB), "tmcb_pool");
   #endif

   #if SMX_NUM_XCHGS
   static XCB xcb_pool[SMX_NUM_XCHGS];
   pass &= sb_BlockPoolCreate((u8*)&xcb_pool, &smx_xcbs, SMX_NUM_XCHGS, 
                                                     sizeof(XCB), "xcb_pool");
   #endif

   if (pass == false)
      smx_EM(SMXE_SMX_INIT_FAIL, 2);

   return pass;
}

/* smx_ErrorLQOvf() is used by assembly ISRs to report SMXE_LQ_OVFL so it
   is not necessary to define the error number in the assembly header
   which requires keeping it consistent with xdef.h.
*/
void smx_ErrorLQOvf(void)
{
   smx_ERROR(SMXE_LQ_OVFL, 0);
}

/* This can be used anywhere a nop function is required. */
void smx_NullF(void)
{
}

/* Used by idleup. */
bool smx_NullSSR(void)
{
   smx_SSR_ENTER0(0);
   smx_EXIT_IF_IN_ISR(0, false);
   if (smx_sched != SMX_CT_STOP)
      smx_sched = SMX_CT_SUSP; /* needed for idleup */
   return((bool)smx_SSRExit(true, 0));
}

/* Test that queue is not broken */
bool smx_QTest(CB_PTR q, SMX_CBTYPE qh_type, SMX_CBTYPE qm_type)
{
   CB_PTR qb;

   for (qb = q->fl; qb != q; qb = qb->fl)
      if (qb->cbtype != qh_type && qb->cbtype != qm_type)
         break;

   if (qb->cbtype == qh_type || qb->cbtype == qm_type)
      return true;
   else
      return false;
}

void  smx_RelPoolStack(TCB_PTR task) 
{
   #if SMX_CFG_STACK_SCAN
   /* release stack to the scan pool to be scanned by smx_StackScan() and save 
      owner in the second word. */
   if (task->stp != NULL)
   {
      if (SMX_CFG_STACK_SCAN)
      {
         *(void**)smx_scanstack_end = (void*)(task->spp);
         smx_scanstack_end = (void*)(task->spp);
         *(void**)smx_scanstack_end = NULL;
         *((u32*)smx_scanstack_end + 1) = (u32)task;
      }
      else
      {
         *(void**)(task->spp) = smx_freestack;
         smx_freestack = (void*)(task->spp);
      }
      task->stp = NULL;
   }
   #else
   /* release strack to the free pool */
   if (task->stp != NULL)
   {
      *(void**)(task->spp) = smx_freestack;
      smx_freestack = (void*)(task->spp);
      task->stp = NULL;
   }
   #endif
}

/* Notes:
   1. To ensure that smx_ct is stopped for Stop SSRs if the CB test fails and
      the SSR is aborted.
   2. "Down" means lower priority (--> higher address). "Up" means higher
      priority (--> lower address).
   3. Do not requeue task if it is in a FIFO queue.
   4. This handles priority propagation for mutexes. However, it is recursive. 
      It may be desirable to limit the recursion.
   5. Broken queue in the forward direction has already been tested.
   6. It is possible that a timer may be stopped before srnest is set > 0, and
      that a preempting task will take TMRCB for its own use. Hence, it is
      necessary to do the additional test that tmhp points at tmr. For FreeRTOS
      it is not possible to set tmr->tmhp to point to tmr.
   7. If not, there is no reason to change the xchg owner's priority because it
      is >= the first msg priority.
*/