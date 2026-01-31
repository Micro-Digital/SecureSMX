/*
* xmsg.c                                                    Version 6.0.0
*
* smx Message and Exchange Functions
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
* Authors: Ralph Moore, Alan Moore
*
*****************************************************************************/

#include "xsmx.h"

/* internal subroutines */
static void smx_MsgPriorityPromotion(XCB_PTR xchg, MCB_PTR msg);
static bool smx_MsgXchgClear_F(XCB_PTR xchg);

/* shared subroutines -- see xsmx.h */
/* smx_MsgReceive_F(), smx_MsgRel_F(), smx_MsgRelAll_F() */

/*============================================================================
*                             MESSAGE SERVICES                               *
============================================================================*/

/*
*  smx_MsgBump()   SSR
*
*  Abort, report error, and return false if msg or pri are invalid. Change the
*  priority of a message unless pri == SMX_PRI_NOCHG. End and return true if
*  msg is not in a queue, or is the only msg in the queue, Otherwise, requeue 
*  msg even if priority did not change. If queue is a pass exchange queue and
*  priority inheritance is enabled, increase the xchg owner priority to the 
*  new msg priority, if greater.
*/
bool smx_MsgBump(MCB_PTR msg, u8 pri)
{
   bool  pass;
   CB_PTR   q;

   smx_SSR_ENTER2(SMX_ID_MSG_BUMP, msg, pri);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_BUMP, false);

   /* verify that msg is valid and that current task has access permission */
   if (pass = smx_MCBTest(msg, SMX_PRIV_HI))
   {
      if (!smx_TEST_PRIORITY(pri))
         smx_ERROR_EXIT(SMXE_INV_PRI, false, 0, SMX_ID_MSG_BUMP);

      /* change msg priority */
      if (pri != SMX_PRI_NOCHG)
         msg->pri = pri;

      /* requeue msg if in a queue */
      if (msg->fl != NULL)
      {
         /* find queue head */
         for (q = msg->fl; q != (CB_PTR)msg; q = q->fl)
            if (q->cbtype != SMX_CB_MCB)
               break;

         /* abort if queue head is not an XCB */
         if (q->cbtype != SMX_CB_XCHG)
            smx_ERROR_EXIT(SMXE_INV_XCB, false, 0, SMX_ID_MSG_BUMP);

         /* requeue message in priority order */
         smx_DQMsg(msg);
         smx_PNQMsg(q, msg, q->cbtype);

         /* check if q is a pass exchange */
         XCB_PTR xchg = (XCB_PTR)q;
         if (xchg->mode == SMX_XCHG_PASS)
         {
            /* possible msg priority promotion, if enabled */ 
            if (xchg->flags.pi == 1)
               smx_MsgPriorityPromotion(xchg, msg);
         }
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_MSG_BUMP));
}

/*
*  smx_MsgGet()   SSR
*
*  Gets block from block pool and loads msg block pointer into bpp. If bpp == 
*  NULL, operation proceeds normally, but no msg data ptr is returned.
*
*  Notes:
*     1. For proper operation there must be at least as many MCBs as there
*        are active smx messages in a system.
*     2. Interrupt safe wrt to sb_BlockGet() and sb_BlockRel() operating on
*        the same block pool.
*/
MCB_PTR smx_MsgGet(PCB_PTR pool, u8** bpp, u16 clrsz, MCB_PTR* mhp)
{
   MCB_PTR msg;
   u8     *bp;

   smx_SSR_ENTER4(SMX_ID_MSG_GET, pool, bpp, clrsz, mhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_GET, NULL);

   /* block multiple gets and verify current task has msg get permission */
   if ((msg = (MCB_PTR)smx_ObjectCreateTestH((u32*)mhp)) && !smx_errno)
   {
      /* verify that pool is valid and that current task has access permission */
      if (msg = (MCB_PTR)smx_PCBTest(pool, SMX_PRIV_LO))
      {
         /* get block if pool not empty */
         if (pool->pn != NULL)
         {
            /* get MCB */
            if ((msg = (MCB_PTR)smx_mcbs.pn) == NULL)
               smx_ERROR_EXIT(SMXE_OUT_OF_MCBS, NULL, 0, SMX_ID_MSG_GET);
            smx_mcbs.pn = *(u8**)msg;
            smx_mcbs.num_used++;

            /* get next block and update pcb.pn, with interrupts disabled */
            sb_INT_DISABLE();
            bp = pool->pn;
            pool->pn = *(u8**)bp;
            pool->num_used++;
            sb_INT_ENABLE();

            /* initialize MCB */
            msg->fl = NULL;
            msg->cbtype = SMX_CB_MCB;
            msg->bp = bp;
            msg->bs = (u32)pool;
            msg->onr = (smx_clsr ? (TCB_PTR)smx_clsr : smx_ct);
            msg->pri = 0;
            msg->rpx = 0xFF;
            msg->mhp = mhp;

            /* clear clrsz bytes, up to block size */
            if (clrsz)
            {
               if (clrsz < pool->size)
                  memset(bp, 0, clrsz);
               else
                  memset(bp, 0, pool->size);
            }
            /* load message block pointer and message handle */
            if (bpp)
               *bpp = bp;
            if (mhp)
               *mhp = msg;
         }
         else
            smx_ERROR_EXIT(SMXE_POOL_EMPTY, NULL, 0, SMX_ID_MSG_GET);
      }
   }
   return((MCB_PTR)smx_SSRExit((u32)msg, SMX_ID_MSG_GET));
}

/*
*  smx_MsgMake()   SSR
*
*  Makes an smx message from any block. The block source, if any, is stored 
*  in msg->bs, else msg->bs = 0.
*/
MCB_PTR smx_MsgMake(u8* bp, u32 bs, MCB_PTR* mhp)
{
   MCB_PTR msg;
   PCB_PTR pp = (PCB_PTR)bs; /* allows seeing PCB fields in debugger */

   smx_SSR_ENTER3(SMX_ID_MSG_MAKE, bp, bs, mhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_MAKE, NULL);

   /* block multiple makes and verify current task has msg make permission */
   if ((msg = (MCB_PTR)smx_ObjectCreateTestH((u32*)mhp)) && !smx_errno)
   {
      /* test bp parameter */
      if (bp == NULL)
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_MSG_MAKE);

      /* get MCB */
      if ((msg = (MCB_PTR)smx_mcbs.pn) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_MCBS, NULL, 0, SMX_ID_MSG_MAKE);

      /* get MCB from MCB pool and initialize it */
      smx_mcbs.pn = *(u8**)msg;
      smx_mcbs.num_used++;
      msg->fl = NULL; /* clears free list link */
      msg->cbtype = SMX_CB_MCB;
      msg->bp = bp;
      msg->bs = (u32)pp;
      msg->onr = (smx_clsr ? (TCB_PTR)smx_clsr : smx_ct);
      msg->rpx = 0xFF;
      msg->mhp = mhp;

      /* load message handle */
      if (mhp)
         *mhp = msg;
   }
   return((MCB_PTR)smx_SSRExit((u32)msg, SMX_ID_MSG_MAKE));
}

/*
*  smx_MsgPeek()   SSR
*
*  Return the specified information about msg.
*/
u32 smx_MsgPeek(MCB_PTR msg, SMX_PK_PAR par)
{
   CICB_PTR cp;           /* chunk pointer */
   u32      hn = msg->bs; /* heap number */
   u32      pp = msg->bs; /* pool pointer */
   CB_PTR   q;
   u32      val;

   smx_SSR_ENTER2(SMX_ID_MSG_PEEK, msg, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_PEEK, 0);

   /* verify that msg is valid and that current task has access permission */
   if (val = smx_MCBTest(msg, SMX_PRIV_LO))
   {
      val = 0;
      switch (par)
      {
         case SMX_PK_BP:
            val = (u32)msg->bp;
            break;
         case SMX_PK_HN:
            val = (u32)msg->bs;
            if (val >= eh_hvpn)
               val = -1;
            break;
         case SMX_PK_NEXT:
            if (msg->fl != NULL && msg->fl->cbtype == SMX_CB_MCB)
               val = (u32)msg->fl;
            break;
         case SMX_PK_ONR:
            val = (u32)msg->onr;
            break;
         case SMX_PK_POOL:
            val = (u32)msg->bs;
            if (val < eh_hvpn)
               val = -1;
            break;
         case SMX_PK_PRI:
            val = (u32)msg->pri;
            break;
         case SMX_PK_REPLY:
            if (msg->rpx != 0xFF)
               val = (u32)((XCB_PTR)(smx_xcbs.pi) + msg->rpx);
            break;
         case SMX_PK_SIZE:
            if (hn < eh_hvpn)
            {
               cp = (CICB_PTR)((u32)msg->bp - EH_BP_OFFS);
               if ((u32)eh_hvp[hn]->pi < (u32)cp     && (u32)cp < (u32)eh_hvp[hn]->px &&
                   (u32)eh_hvp[hn]->pi < (u32)cp->fl && (u32)cp->fl < (u32)eh_hvp[hn]->px)
               {
                  if ((u32)cp->blf & EH_SSP)
                  {
                     /* remove spare space and offset */
                     val = (u32)(*(u32*)((u32)(cp->fl) - 4)) - (u32)cp - EH_BP_OFFS;
                  }
                  else
                  {
                     /* remove offset */
                     val = (u32)cp->fl - (u32)cp - EH_BP_OFFS;
                  }
               }
               else
                  break;
            }
            else if (pp != -1 && pp >= eh_hvpn)
            {
               val = ((PCB_PTR)pp)->size;
            }
            else
            {
               val = 0;
            }
            break;
         case SMX_PK_XCHG:
            if (msg->fl)
            {
               for (q = msg->fl; q != (CB_PTR)msg && q->cbtype == SMX_CB_MCB; q = q->fl);
               if (q->cbtype == SMX_CB_XCHG)
                  val = (u32)q;
               else
                  smx_ERROR_EXIT(SMXE_BROKEN_Q, 0, 0, SMX_ID_MSG_PEEK)
            }
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_MSG_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_MSG_PEEK);
}

/*
*  smx_MsgRel()   SSR
*
*  Releases a message obtained by smx_MsgGet(). Dequeues message, if at a
*  broadcast exchange, then releases message block to the heap or block pool,
*  from which it came and its MCB back to its pool. Clears clrsz bytes up to
*  the end of the block. Returns true if successful. Fails and returns false
*  if msg handle is invalid, if its pool or bp fields are invalid, or if it
*  is not owned by the current LSR or task.
*/
bool smx_MsgRel(MCB_PTR msg, u16 clrsz)
{
   bool pass;

   smx_SSR_ENTER2(SMX_ID_MSG_REL, msg, clrsz);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_REL, false);

   /* verify that msg is valid and that current task is its owner and has 
      access permission */
   if (pass = smx_MCBOnrTest(msg, SMX_PRIV_HI))
   {
      /* dequeue msg from broadcast exchange */
      if (msg->fl != NULL)
          smx_DQMsg(msg);
      /* release msg and return result <1> */
      pass = smx_MsgRel_F(msg, clrsz);
   }
   return((bool)smx_SSRExit(pass, SMX_ID_MSG_REL));
}

/*
*  smx_MsgRelAll()   SSR
*
*  Releases all messages belonging to task and returns number released.
*/
u32 smx_MsgRelAll(TCB_PTR task)
{
   u32   n;

   smx_SSR_ENTER1(SMX_ID_MSG_REL_ALL, task);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_REL_ALL, 0);
   smx_TASK_OP_PERMIT_VAL(task, SMX_ID_MSG_REL_ALL);
   n = smx_MsgRelAll_F(task);
   return(smx_SSRExit(n, SMX_ID_MSG_REL_ALL));
}

/*
*  smx_MsgUnmake()   SSR
*
*  Reverses smx_MsgMake() by releasing the msg MCB. Aborts with error and 
*  returns NULL, if the MCB is invalid or msg is not owned by current LSR or 
*  task. Returns msg block pointer and loads bs into bsp.
*/
u8* smx_MsgUnmake(MCB_PTR msg, u32* bsp)
{
   u8*  bp;
   MCB_PTR* mhp;

   smx_SSR_ENTER2(SMX_ID_MSG_UNMAKE, msg, bsp);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_UNMAKE, NULL);

   /* verify that msg is valid and that current task is its owner and has 
      access permission */
   if (bp = (u8*)smx_MCBOnrTest(msg, SMX_PRIV_HI))
   {
      bp = msg->bp;
      if (bsp != NULL)
      {
         *bsp = msg->bs;
      }

      if (msg->fl) /* dequeue msg if it is in a queue */
         smx_DQMsg(msg);

      /* release MCB to its pool and clear it and its handle */
      mhp = msg->mhp;
      sb_BlockRel(&smx_mcbs, (u8*)msg, sizeof(MCB));
      if (mhp)
         *mhp = NULL;
   }
   return((u8*)smx_SSRExit((u32)bp, SMX_ID_MSG_UNMAKE));
}

/*
*  smx_MsgReceive()   SSR
*
*  Suspend SSR version. Enters SSR, calls smx_MsgReceive_F(), then exits SSR.
*  Aborts if called from LSR and tmo != SMX_TMO_NOWAIT. Clears lockctr if
*  called from a task and tmo != SMX_TMO_NOWAIT.
*/
MCB_PTR smx_MsgReceive(XCB_PTR xchg, u8** bpp, u32 timeout, MCB_PTR* mhp)
{
   MCB_PTR  msg = 0;

   smx_SSR_ENTER4(SMX_ID_MSG_RECEIVE, xchg, bpp, timeout, mhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_RECEIVE, NULL);
   if (!(smx_clsr && timeout))
   {
      msg = smx_MsgReceive_F(xchg, bpp, timeout, mhp);
      if (msg == NULL && timeout)
         smx_sched = SMX_CT_SUSP;
      if (timeout)
         smx_lockctr = 0;   
   }
   else
      smx_ERROR(SMXE_WAIT_NOT_ALLOWED, 0);
   return((MCB_PTR)smx_SSRExit((u32)msg, SMX_ID_MSG_RECEIVE));
}

/*
*  smx_MsgReceiveStop()   SSR
*
*  Stop SSR version. If called from an LSR, reports OP_NOT_ALLOWED error and
*  returns to LSR. Else, enters SSR and verifies that bpp points to a static
*  location. If not, aborts with INV_PAR error. If ok, sets sched = STOP, calls
*  smx_MsgReceive_F(), then exits SSR. Return value is passed via taskMain(par)
*  when task restarts.
*/
void smx_MsgReceiveStop(XCB_PTR xchg, u8** bpp, u32 timeout, MCB_PTR* mhp)
{
   MCB_PTR  msg = NULL;

   if (smx_clsr)
      smx_ERROR_RET_VOID(SMXE_OP_NOT_ALLOWED, 0);
   smx_RET_IF_IN_ISR_VOID();
   smx_SSR_ENTER4(SMX_ID_MSG_RECEIVE_STOP, xchg, bpp, timeout, mhp);

   /* verify that bpp points to a static location, not to task stack */
   if ((smx_ct->sbp > (u8*)bpp) && ((u8*)bpp >= smx_ct->spp))
   {
      smx_ERROR(SMXE_INV_PAR, 0);
   }
   else
   {
      msg = smx_MsgReceive_F(xchg, bpp, timeout, mhp);
  #if SMX_CFG_TOKENS
   if (smx_ct->flags.tok_ok)
  #endif
      smx_sched = SMX_CT_STOP;
   }
   smx_SSRExit((u32)msg, SMX_ID_MSG_RECEIVE_STOP);
}

/*
*  smx_MsgSend()   SSR
*
*  Abort, report error, and exit with false if msg or xchg are invalid or if msg
*  is not owned by current LSR or task. Dequeue msg if already in a queue.
*  Load pri and the reply index into the corresponding msg fields.
*  If tq == 1, dequeue the first task and pass the message to it. If xchg is a 
*  pass exchange, assign the message's priority to the task, enqueue the task 
*  on rq and clear the task's timeout unless there is a mutex conflict. If
*  priority promotion is enabled, the task becomes the new xchg owner.
*  If tq == 0, enqueue the message at xchg, in priority order. If xchg is a
*  pass exchange and priority inheritance is enabled, increase the xchg owner 
*  priority to the new msg priority, if it is greater.
*
*  If xchg is a broadcast exchange:
*  If tq == 1, give msg handle to all waiting tasks and resume them.
*  Else if mq = 1, release old msg and enqueue new msg, unless handles are the
*  same.
*/
bool smx_MsgSend(MCB_PTR msg, XCB_PTR xchg, u8 pri, void* reply)
{
   MCB_PTR* mhp;
   MCB_PTR  mpo;
   bool     pass;
   TCB_PTR  task;

   smx_SSR_ENTER4(SMX_ID_MSG_SEND, msg, xchg, pri, reply);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_SEND, false);

   /* verify that msg is valid and that current task is its owner and has 
      access permission */
   if (pass = smx_MCBOnrTest(msg, SMX_PRIV_LO))
   {
      /* verify that xchg is valid and that current task has access permission */
      if (pass = smx_XCBTest(xchg, SMX_PRIV_LO))
      {
         /* test for valid parameters */
         if (reply == NULL)
            reply = NULL;
         if ((reply != NULL && (reply < (void*)smx_xcbs.pi || reply > (void*)smx_xcbs.px)) 
              || (pri != SMX_PRI_NOCHG && pri >= SMX_PRI_NUM))
            smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_MSG_SEND);

         if (msg->fl) /* dequeue msg if it is in a queue already */
            smx_DQMsg(msg);

         if (pri != SMX_PRI_NOCHG)
            msg->pri = pri;

         msg->rpx = ((u32)reply > 0 ? (XCB_PTR)reply - (XCB_PTR)(smx_xcbs.pi) : msg->rpx);

        #if SMX_CFG_SSMX
         /* pass taskp priv to msg */
         TCB_PTR  ct = smx_ct;
         TCB_PTR  taskp;
         if (ct->parent)
           taskp = ct->parent;
         else
           taskp = ct;
         msg->priv = taskp->priv;
        #endif

         mhp = msg->mhp;
        #if SMX_CFG_SSMX
         /* clear msg handle if not bound to current task */
         if (mhp && msg->con.bnd == 0)
            *mhp = NULL;
        #else
         /* clear msg handle */
         if (mhp)
            *mhp = NULL;
        #endif

         /* normal or pass exchange */
         if (xchg->mode != SMX_XCHG_BCST)
         {
            if (xchg->flags.tq == 1) /* a task is waiting */
            {
               /* give msg to first waiting task and resume task */
               task = smx_DQFTask((CB_PTR)xchg);

              #if SMX_CFG_PORTAL
               if (msg->con.bnd)
               {
                  xchg->bct = smx_ct; /* client */
                  /* msg->onr  = smx_ct */
               }
               else
              #endif
               {
                  xchg->bct = NULL;
                  msg->onr  = task; /* (server) */
               }

               /* if task queue is empty reset tq flag */
               if (xchg->fl == 0)
                  xchg->flags.tq = 0;

               /* if priority promotion is enabled, xchg owner = task */
               if (xchg->flags.pi)
                  xchg->onr = task;

               task->rv = (u32)msg;
               smx_PUT_RV_IN_EXR0(task)
               mhp = (MCB_PTR*)task->sv2;
               msg->mhp = mhp;

               /* load message block pointer at address saved in sv if !NULL */
               if (task->sv != NULL)
                  *(u8**)task->sv = msg->bp;

               /* if pass exchange, change task priority */
               if (xchg->mode == SMX_XCHG_PASS)
               {
                  task->prinorm = msg->pri;
                  /* raise task priority, but lower it only if task does not own a mutex. */
                  if ((msg->pri > task->pri) || ((msg->pri < task->pri) && (task->molp == NULL)))
                     task->pri = msg->pri;
                  /* if priority promotion enabled, task becomes xchg owner */
                  if (xchg->flags.pi)
                     xchg->onr = task;
               }

              #if SMX_CFG_SSMX
               /* if not fixed, set taskp priv = msg priv */
               if (task->parent)
                  taskp = task->parent;
               else
                  taskp = task;
               if (!taskp->flags.priv_fixed)
                  taskp->priv = msg->priv;
              #endif

               smx_NQRQTask(task);
               smx_DO_CTTEST();
               smx_timeout[task->indx] = SMX_TMO_INF;
            }
            else  /* no task waiting */
            {
               /* enqueue message by priority, set mq */
               smx_PNQMsg((CB_PTR)xchg, msg, xchg->cbtype);
               xchg->flags.mq = 1;

               /* possible priority promotion, if enabled */ 
               if (xchg->flags.pi == 1)
                  smx_MsgPriorityPromotion(xchg, msg);

              #if SMX_CFG_PORTAL
               if (msg->con.bnd)
               {
                  xchg->bct = smx_ct; /* client */
               /* msg->onr  = smx_ct */
               }
               else
              #endif
               {
                  xchg->bct = NULL;
                  msg->onr = (TCB_PTR)xchg; /* exchange */
               }
            }
         }
         else /* broadcast exchange */
         {
            /* give msg handle to all waiting tasks and resume them */
            while (xchg->flags.tq == 1)
            {
               task = smx_DQFTask((CB_PTR)xchg);
               if (xchg->fl == 0)
                  xchg->flags.tq = 0;
               task->rv = (u32)msg;
               smx_PUT_RV_IN_EXR0(task)

               /* load message block pointer at address saved in sv if !NULL */
               if (task->sv != NULL)
                  *(u8**)task->sv = msg->bp;

               smx_NQRQTask(task);
               smx_DO_CTTEST();
               smx_timeout[task->indx] = SMX_TMO_INF;
            }
            /* remove old msg, if present */
            mpo = (MCB_PTR)xchg->fl;
            if (mpo != NULL)
            {
               smx_DQMsg(mpo);
               smx_MsgRel_F(mpo, 0);
            }
            /* enqueue new msg */
            msg->fl = (msg->bl = (CB_PTR)xchg);
            xchg->fl = (xchg->bl = (CB_PTR)msg);
            xchg->flags.mq = 1;
         }
         /* callback */
         if (xchg->cbfun)
            xchg->cbfun((u32)xchg);
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_MSG_SEND));
}

/*============================================================================
*                         MESSAGE EXCHANGE SERVICES                          *
============================================================================*/

/*
*  smx_MsgXchgClear()   SSR
*
*  Clears a message exchange by resuming all waiting tasks with NULL return
*  values, or releasing all waiting messages. Returns true, if successful.
*/
bool smx_MsgXchgClear(XCB_PTR xchg)
{
   bool pass;

   smx_SSR_ENTER1(SMX_ID_MSG_XCHG_CLEAR, xchg);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_XCHG_CLEAR, false);
   pass = smx_MsgXchgClear_F(xchg);
   return (bool)smx_SSRExit(pass, SMX_ID_MSG_XCHG_CLEAR);
}

/*
*  smx_MsgXchgCreate()   SSR
*
*  Allocates an exchange control block from the XCB pool and initializes it.
*  If allocation fails because of invalid parameter or no block is available,
*  returns NULL. Otherwise returns exchange handle.
*/
XCB_PTR smx_MsgXchgCreate(SMX_XMODE mode, const char* name, XCB_PTR* xhp, u8 pi)
{
   XCB_PTR  x;

   smx_SSR_ENTER4(SMX_ID_MSG_XCHG_CREATE, mode, name, xhp, pi);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_XCHG_CREATE, NULL);

   /* block multiple creates and verify current task has create permission */
   if ((x = (XCB_PTR)smx_ObjectCreateTestH((u32*)xhp)) && !smx_errno)
   {
      /* check type parameter */
      if (!(mode == SMX_XCHG_NORM || mode == SMX_XCHG_PASS || mode == SMX_XCHG_BCST))
         smx_ERROR_EXIT(SMXE_WRONG_MODE, NULL, 0, SMX_ID_MSG_XCHG_CREATE);

      /* get an exchange control block */
      if ((x = (XCB_PTR)sb_BlockGet(&smx_xcbs, 4)) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_XCBS, NULL, 0, SMX_ID_MSG_XCHG_CREATE);

      /* initialize XCB */
      x->cbtype = SMX_CB_XCHG;
      x->mode = mode;
      x->xhp = xhp;
      x->onr = NULL;
      if ((mode == SMX_XCHG_PASS) && pi)
         x->flags.pi = 1;
      if (name && *name)
         x->name = name;
      /* load exchange handle */
      if (xhp)
         *xhp = x;
   }
   return((XCB_PTR)smx_SSRExit((u32)x, SMX_ID_MSG_XCHG_CREATE));
}

/*
*  smx_MsgXchgDelete()   SSR
*
*  Deletes an exchange created by smx_MsgXchgCreate(). Resumes all waiting
*  tasks with false return values or releases all waiting messages. Clears
*  and releases its XCB, removes its name from HT, and clears its handle.
*/
bool smx_MsgXchgDelete(XCB_PTR* xhp)
{
   XCB_PTR xchg = (xhp ? *xhp : NULL);

   smx_SSR_ENTER1(SMX_ID_MSG_XCHG_DELETE, xchg);  /* record actual handle */
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_XCHG_DELETE, false);

   /* if xchg is valid and smx_ct has token, resume all waiting tasks or 
      release all waiting messages */
   if (smx_MsgXchgClear_F(xchg) == false)
   {
      smx_ERROR_EXIT(SMXE_INV_XCB, false, 0, SMX_ID_MSG_XCHG_DELETE);
   }
   else
   {
      /* clear and release XCB and set exchange handle to nullcb */
      sb_BlockRel(&smx_xcbs, (u8*)xchg, sizeof(XCB));
      *xhp = NULL;
      return((bool)smx_SSRExit(true, SMX_ID_MSG_XCHG_DELETE));
   }
}

/*
*  smx_MsgXchgPeek()   SSR
*
*  Return the specified information about xchg. Only parameters specified in the
*  user's guide are allowed.
*/
u32 smx_MsgXchgPeek(XCB_PTR xchg, SMX_PK_PAR par)
{
   u32 val;

   smx_SSR_ENTER2(SMX_ID_MSG_XCHG_PEEK, xchg, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_XCHG_PEEK, 0);

   /* verify that xchg is valid and that current task has access permission */
   if (val = smx_XCBTest(xchg, SMX_PRIV_LO))
   {
      switch (par)
      {
         case SMX_PK_TASK:
            val = ((xchg->flags.tq) ? (u32)xchg->fl : 0);
            break;
         case SMX_PK_MSG:
            val = ((xchg->flags.mq) ? (u32)xchg->fl : 0);
            break;
         case SMX_PK_MODE:
            val = (u32)xchg->mode;
            break;
         case SMX_PK_NAME:
            val = (u32)xchg->name;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_MSG_XCHG_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_MSG_XCHG_PEEK);
}

/*
*  smx_MsgXchgSet()   SSR
*
*  Sets the specified message exchange parameter to the specified value.
*  Not permitted in umode.
*/
bool smx_MsgXchgSet(XCB_PTR xchg, SMX_ST_PAR par, u32 v1, u32 v2)
{
   bool pass;

   smx_SSR_ENTER4(SMX_ID_MSG_XCHG_SET, xchg, par, v1, v2);
   smx_EXIT_IF_IN_ISR(SMX_ID_MSG_XCHG_SET, false);

   /* verify that xchg is valid and that current task has access permission */
   if (pass = smx_XCBTest(xchg, SMX_PRIV_HI))
   {
      #if SMX_CFG_SSMX
      /* a utask cannot make exchange changes */
      if (smx_ct->flags.umode == 1)
         smx_ERROR_EXIT(SMXE_PRIV_VIOL, false, 0, SMX_ID_MSG_XCHG_SET);
      #endif

      /* perform set operation on exchange */
      switch (par)
      {
         case SMX_ST_CBFUN:
            xchg->cbfun = (CBF_PTR)v1;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_MSG_XCHG_SET);
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_MSG_XCHG_SET));
}

/*===========================================================================*
*                                 SUBROUTINES                                *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  smx_MsgPriorityPromotion()
*
*  Raises the priority of the server task to that of a higher priority message 
*  received at its exchange. For a tunnel portal, it also raises the priority 
*  of the bound client to that of the received message.
*/
void smx_MsgPriorityPromotion(XCB_PTR xchg, MCB_PTR msg)
{
   /* promote server priority, if msg priority is higher */
   TCB_PTR svr = xchg->onr;
   if (svr != NULL)
   {
      if (msg->pri > svr->pri)
      {
         svr->pri     = msg->pri;
         svr->prinorm = msg->pri; /*<3>*/
         if (svr->fl != NULL)
            smx_ReQTask(svr);
      }
      else if (msg->pri > svr->prinorm)
      {
         svr->prinorm = msg->pri; /*<3>*/
      }
   }
   /* promote bound client priority, if server priority promoted <4> */
   TCB_PTR bct = xchg->bct;
   if (bct != NULL)
   {
      if (bct->pri != svr->pri)
      {
         bct->pri = svr->pri;
         if (bct->fl != NULL)
            smx_ReQTask(bct);
      }
   }
}

/*
*  smx_MsgRel_F()
*
*  If msg->bs = -1 msg block is standalone and is not freed. Otherwise, frees 
*  block to heap hn, if block came from it or frees block to pool at pp, if
*  block came from it. Then releases MCB back to its pool and clears it. 
*  Returns true if successful. Called by smx_MsgRel(), smx_MsgRelAll_F(), 
*  smx_MsgSend(), smx_MsgXchgClear_F(), and smx_PMsgRel().
*
*  Notes:
*  1. Standalone blocks, including those in heap blocks (e.g. task stacks), 
*     are not released.
*  2  Interrupt safe wrt to sb_BlockGet() and sb_BlockRel() operating on
*     the same block pool.
*  3. SMX_CB_PCB == SB_CB_PCB so they are treated as equivalent.
*/
bool smx_MsgRel_F(MCB_PTR msg, u16 clrsz)
{

   u8*     bp = msg->bp;
   CCB_PTR cp = 0;
   u32     hn = msg->bs;
   MCB_PTR* mhp;
   u32     pp = msg->bs;
   u8*     pi;
   u8*     px;

   if (msg->bs != -1)
   {
      /* get chunk pointer heap limits if hn is valid */
      if (hn < eh_hvpn)
      {
         cp = (CCB_PTR)((u32)bp - EH_BP_OFFS);
         pi = (u8*)eh_hvp[hn]->pi;
         px = (u8*)eh_hvp[hn]->px;

         /* check that block is from heap hn and not from another heap */
         if ((pi < (u8*)cp            && (u8*)cp < px) &&
             (pi < (u8*)cp->fl        && (u8*)cp->fl < px) && 
             (pi < (u8*)(cp->fl->blf) && (u8*)(cp->fl->blf) < px) &&
             (((u32)(cp->fl->blf) & 0xFFFFFFF8) == (u32)cp))
         {
            /* free block to heap hn <1> */
            if (!smx_HeapFree((void*)bp, hn))
               return false;
         }
      }
      else if (pp != 0)
      {
         /* release block to pool pp and clear it */
         if (!sb_BlockRel((PCB_PTR)pp, bp, clrsz))
            return false;
      }
   }

   /* release MCB to its pool, clear it, and clear its handle */
   mhp = msg->mhp;
   sb_BlockRel(&smx_mcbs, (u8*)msg, sizeof(MCB));
   if (mhp)
      *mhp = NULL;
   return true;
}

/*
*  smx_MsgRelAll_F()
*
*  Searches entire MCB pool and releases all messages  belonging to this task. 
*  Returns the number of messages released.
*/
u32 smx_MsgRelAll_F(TCB_PTR task)
{
   MCB_PTR  msg;
   int      n;

   /* verify that task is valid and that current task has access permission */
   if (n = smx_TCBTest(task, SMX_PRIV_HI))
   {
      if (smx_mcbs.pi)
         for (n = 0, msg = (MCB_PTR)smx_mcbs.px; msg >= (MCB_PTR)smx_mcbs.pi; msg--)
            if (msg->onr == task)
            {
               n++;
               smx_MsgRel_F(msg, sizeof(MCB));
            }
   }
   return n;
}

/*
*  smx_MsgReceive_F()
*
*  If mq == 1 and xchg is a normal or pass exchange, dequeues the first
*  message and gives it to ct. If xchg is a pass exchange, assigns msg 
*  priority to ct->prinorm and changes ct->pri up or down if ct does not own a 
*  mutex. If priority promotion is enabled, ct becomes the xchg owner.
*  Requeues ct if its priority was changed and tests if it is still top task.
*
*  If mq == 0, timeout > 0, and call is not from an LSR, enqueues ct on xchg
*  and sets its timeout.
*
*  If xchg is a broadcast exchange, ct receives the msg handle and the message
*  body pointer is loaded into bpp. However, msg remains enqueued at xchg and
*  xchg remains its owner. 
*/
MCB_PTR smx_MsgReceive_F(XCB_PTR xchg, u8** bpp, u32 timeout, MCB_PTR* mhp)
{
   MCB_PTR  msg;
   TCB_PTR  ct = smx_ct;

   /* block multiple receives and verify current task has msg receive permission */
   if ((msg = (MCB_PTR)smx_ObjectCreateTestH((u32*)mhp)) && !smx_errno)
   {
      /* verify that xchg is valid and that current task has access permission */
      if (msg = (MCB_PTR)smx_XCBTest(xchg, SMX_PRIV_LO))
      {
         msg = NULL;
         if (xchg->flags.mq)  /* message waiting */
         {
            /* if not broadcast exchange, dequeue msg and assign clsr or ct as its
               owner if not bound msg */
            if (xchg->mode != SMX_XCHG_BCST)
            {
               msg = smx_DQFMsg((CB_PTR)xchg);

              #if SMX_CFG_PORTAL
               if (msg->con.bnd)
               {
                  xchg->bct = smx_ct; /* client */
                  /* msg->onr  = smx_ct */
               }
               else
              #endif
               {
                  xchg->bct = NULL;
                  msg->onr = (smx_clsr ? (TCB_PTR)smx_clsr : ct);
               }
            }
            else
               msg = (MCB_PTR)xchg->fl;

            if (xchg->fl == 0)
               xchg->flags.mq = 0;

            /* if pass exchange and not LSR, change task priority and xchg onr */
            if ((xchg->mode == SMX_XCHG_PASS) && !smx_clsr)
            {
               /* verify msg->pri is valid <2> */
               if (msg->pri >= SMX_PRI_NUM)
                  smx_ERROR_RET(SMXE_INV_PRI, NULL, 0);

               /* raise ct priority, but lower it only if ct does not own a mutex. */
               if ((msg->pri > ct->pri) || ((ct->molp == NULL) && (msg->pri < ct->pri)))
               {
                  ct->pri = msg->pri;
                  ct->prinorm = msg->pri;
                  /* move ct in rq and test if it is still top task */
                  smx_DQRQTask(ct);
                  smx_NQRQTask(ct);
                  smx_DO_CTTEST(); /* (only for resume case) */
               }
               /* if priority promotion is enabled, ct becomes xchg owner */
               if (xchg->flags.pi)
               {
                  xchg->onr = ct;
               }
            }
           #if SMX_CFG_SSMX
            TCB_PTR  taskp;
            /* if not fixed, set taskp priv = msg priv */
            if (ct->parent)
               taskp = ct->parent;
            else
               taskp = ct;
            if (!taskp->flags.priv_fixed)
               taskp->priv = msg->priv;
           #endif

            /* load message block pointer */
            if (bpp != NULL && msg != NULL)
               *bpp = msg->bp;

            msg->mhp = mhp;
            if (mhp)
               *mhp = msg;
         }
         else  /* no message waiting */
         {
            if (timeout)  /* wait for a message */
            {
               smx_DQRQTask(ct);

               /* enqueue task */
               if (xchg->mode != SMX_XCHG_BCST)
                  smx_PNQTask((CB_PTR)xchg, ct, xchg->cbtype);
               else
                  smx_NQTask((CB_PTR)xchg, ct);
               xchg->flags.tq = 1;
               ct->sv  = (u32)bpp;
               ct->sv2 = (u32)mhp;

               smx_TimeoutSet(smx_ct, timeout);
            }
         }
      }
   }
   return msg;
}

/*
*  smx_MsgXchgClear_F()
*
*  Clears a message exchange by resuming all waiting tasks with NULL return
*  values, if tq, or releasing all waiting messages, if mq. tq, mq, and
*  xchg->fl are cleared. Returns true, if successful or false if invalid xchg.
*  Aborts and reports error if invalid xchg or queue is broken.
*/
bool smx_MsgXchgClear_F(XCB_PTR xchg)
{
   MCB_PTR  msg;
   bool     pass;
   TCB_PTR  task = NULL;

   /* verify that xchg is valid and that current task has access permission */
   if (pass = smx_XCBTest(xchg, SMX_PRIV_HI))
   {
      if (xchg->fl != NULL)
      {
         if (xchg->flags.tq) /* resume all waiting tasks */
         {
            while (xchg->fl != NULL)
            {
               task = smx_DQFTask((CB_PTR)xchg);
               smx_NQRQTask(task);
               task->rv = 0;
               smx_PUT_RV_IN_EXR0(task)
               task->sv = 0;
               smx_timeout[task->indx] = SMX_TMO_INF;
            }
            smx_DO_CTTEST();
         }
         else /* release all waiting messages */
         {
            while (xchg->fl != NULL)
            {
               msg = (MCB_PTR)xchg->fl;
               smx_DQMsg(msg);
               smx_MsgRel_F(msg, sizeof(MCB));
            }
         }
      }
      xchg->flags.tq = 0;
      xchg->flags.mq = 0;
   }
   return pass;
}

/* Notes:
   1. Current task will be suspended for up to smx_htmo ticks if heap hn is
      busy. Operation aborts with smx HEAP TIMEOUT error if timeout occurs.
   2. Detects both < 0 and >= SMX_PRI_NUM because pri is defined as unsigned.
   3. If onr->pri has been or is promoted by a mutex, these ensure that 
      when it is released, onr->pri = fmsg->pri, where fmsg = first msg 
      waiting. If onr->pri has been increased by a timeout these ensure that
      if onr->pri is returned to onr->prinorm, onr->pri == fmsg->pri.
   4. A client task bound to a server must have the same priority as the server. 
      This occurs with tunnel portals. The client task is returned to its normal
      priority when its portal connection is closed.
*/