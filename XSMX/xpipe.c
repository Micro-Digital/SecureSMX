/*
* xpipe.c                                                   Version 6.0.0
*
* smx Pipe Functions
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
* Authors: Ralph Moore
*
*****************************************************************************/

#include "xsmx.h"

/* internal subroutines */
void  pktcpy(u8* sp, u8* dp, u32 sz);

static bool  smx_PipeClear_F(PICB_PTR pipe);
static void  smx_PipeGetPkt_F(PICB_PTR pipe, u8* pdst);
static bool  smx_PipeGetPktWait_F(PICB_PTR pipe, void* pdst, u32 timeout);
static void  smx_PipePutPkt_F(PICB_PTR pipe, u8* psrc, SMX_PIPE_MODE mode=SMX_PUT_TO_BACK);
static bool  smx_PipePutPktWait_F(PICB_PTR pipe, void* psrc, u32 timeout, SMX_PIPE_MODE mode=SMX_PUT_TO_BACK);
static bool  smx_PipeResume_F(PICB_PTR pipe);

/*
*  smx_PipeClear()   SSR
*
*  Clears a pipe by resuming all waiting tasks with NULL return values and
*  resetting pipe pointers and full flag. Returns true, if successful.
*/
bool smx_PipeClear(PICB_PTR pipe)
{
   bool pass;
   smx_SSR_ENTER1(SMX_ID_PIPE_CLEAR, pipe);
   smx_EXIT_IF_IN_ISR(SMX_ID_PIPE_CLEAR, false);

   /* verify that pipe is valid and that current task has access permission */
   if (pass = smx_PICBTest(pipe, SMX_PRIV_HI))
   {
      /* clear task queue */
      smx_PipeClear_F(pipe);

      /* reset pipe pointers and return */
      sb_INT_DISABLE();
      pipe->rp = pipe->wp = pipe->bi;
      pipe->flags.full = 0;
      sb_INT_ENABLE();
   }
   return (bool)smx_SSRExit(pass, SMX_ID_PIPE_CLEAR);
}

/*
*  smx_PipeCreate()   SSR
*
*  Gets a PICB and initializes it. Accepts block at ppb as pipe buffer.
*  Also loads name, if any, into PICB and handle table. If parameters
*  are not valid or cannot get a PICB, does an error exit and returns
*  NULL. Else returns pipe handle.
*/
PICB_PTR smx_PipeCreate(void* ppb, u8 width, u16 length, const char* name, PICB_PTR* php)
{
   PICB_PTR p;

   smx_SSR_ENTER5(SMX_ID_PIPE_CREATE, ppb, width, length, name, php);
   smx_EXIT_IF_IN_ISR(SMX_ID_PIPE_CREATE, NULL);

   /* block multiple creates and verify current task has create permission */
   if ((p = (PICB_PTR)smx_ObjectCreateTestH((u32*)php)) && !smx_errno)
   {
      if ((ppb == NULL) || (width == 0) || (length == 0))
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PIPE_CREATE)

      /* get a pipe control block */
      if ((p = (PICB_PTR)sb_BlockGet(&smx_picbs, 4)) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_PICBS, NULL, 0, SMX_ID_PIPE_CREATE);

      /* initialize PICB */
      p->cbtype = SMX_CB_PIPE;
      p->flags.full = 0;
      p->width = width;
      p->length = length;
      p->bi = p->rp = p->wp = (u8 *)ppb;
      p->bx = (u8 *)ppb + (length-1)*width;
      p->php = php;
      if (name && *name)
         p->name = name;

      /* load pipe handle */
      if (php)
         *php = p;
   }
   return((PICB_PTR)smx_SSRExit((u32)p, SMX_ID_PIPE_CREATE));
}

/*
*  smx_PipeDelete()   SSR
*
*  Deletes a pipe created by smx_PipeCreate(). Resumes all waiting tasks with
*  false return values. Clears and releases its PICB and sets its handle to
*  NULL. Returns address of pipe buffer if successful or NULL, if not;
*  Note: user must release pipe buffer.
*/
void* smx_PipeDelete(PICB_PTR* pipep)
{
   PICB_PTR pipe = (pipep ? *pipep : NULL);
   u8* pb;

   smx_SSR_ENTER1(SMX_ID_PIPE_DELETE, pipe);  /* record actual handle */
   smx_EXIT_IF_IN_ISR(SMX_ID_PIPE_DELETE, NULL);

   /* verify that pipe is valid and that current task has access permission */
   if (pb = (u8*)smx_PICBTest(pipe, SMX_PRIV_HI))
   {
      /* resume all waiting tasks */
      smx_PipeClear_F(pipe);

      /* release and clear PICB and set pipe handle to nullcb */
      pb = pipe->bi;
      sb_BlockRel(&smx_picbs, (u8*)pipe, sizeof(PICB));
      *pipep = NULL;
   }
   return((void*)smx_SSRExit((u32)pb, SMX_ID_PIPE_DELETE));
}

/*
*  smx_PipeGet8()   Function
*
*  Transfers oldest byte from pipe to the byte at b, advances rp,
*  cyclically and returns true. If pipe is empty returns false.
*  Does minimal parameter testing, for speed. Can be used from an ISR.
*/
bool smx_PipeGet8(PICB_PTR pipe, u8* bp)
{
   u8 *rp;

   if (pipe == NULL || bp == NULL)
      return false;

   rp = pipe->rp;
   if (!smx_PIPE_EMPTY(pipe, rp))
   {
      *bp = *rp++;
      if (rp > pipe->bx)
      {
         rp = pipe->bi;
      }
      pipe->rp = rp;
      pipe->flags.full = 0;
      return true;
   }
   else
   {
      return false;
   }
}

/*
*  smx_PipeGet8M()   Function
*
*  Transfers up to lim oldest bytes from pipe to the buffer at bp, advances rp,
*  cyclically, and returns the number of bytes actually transferred.
*  Does minimal parameter testing, for speed. Can be used from an ISR.
*/
u32 smx_PipeGet8M(PICB_PTR pipe, u8* bp, u32 lim)
{
   u32 i, n;
   u8 *rp, *wp;

   if (pipe == NULL || bp == NULL)
      return 0;

   rp = pipe->rp;
   wp = pipe->wp;

   if (rp > wp)
   {
      n = (pipe->bx - rp) + (wp - pipe->bi) + 1;
   }
   else
   {
      n = (wp - rp);
   }

   n = (n <= lim ? n : lim);

   for (i = 0; i < n; i++)
   {
      *bp++ = *rp++;
      if (rp > pipe->bx)
         rp = pipe->bi;
   }
   pipe->rp = rp;
   pipe->flags.full = 0;
   return(n);
}

/*
*  smx_PipeGetPkt()   Function
*
*  Transfers oldest data pkt from pipe to the buffer at pdst, advances rp,
*  cyclically, and returns true. For invalid parameter or pipe empty, returns 
*  false. Does not wait. Updating pipe->rp last avoids conflict with pipe
*  put from ISR.
*/
bool smx_PipeGetPkt(PICB_PTR pipe, void* pdst)
{
   u8 *rp = pipe->rp;
   u32 w  = pipe->width; 

   if (pipe == NULL || pdst == NULL)
      return false;

   if (!smx_PIPE_EMPTY(pipe, rp))
   {
      pktcpy(rp, (u8*)pdst, w);
      rp += w;
      if (rp > pipe->bx)
         rp = pipe->bi;
      pipe->rp = rp;
      pipe->flags.full = 0;
      return true;
   }
   else
      return false;
}

/*
*  smx_PipeGetPktWait()   SSR
*
*  Suspend SSR version. Enters SSR, calls smx_PipeGetPktWait_F(), and exits SSR.
*  Aborts if called from LSR and tmo != SMX_TMO_NOWAIT. Clears lockctr if
*  called from a task and tmo != SMX_TMO_NOWAIT.
*/
bool smx_PipeGetPktWait(PICB_PTR pipe, void* pdst, u32 timeout)
{
   bool  gotpkt = false;

   smx_SSR_ENTER3(SMX_ID_PIPE_GET_PKT_WAIT, pipe, pdst, timeout);
   smx_EXIT_IF_IN_ISR(SMX_ID_PIPE_GET_PKT_WAIT, false);
   if (!(smx_clsr && timeout))
   {
      gotpkt = smx_PipeGetPktWait_F(pipe, pdst, timeout);
      if (gotpkt == NULL && timeout)
         smx_sched = SMX_CT_SUSP;
      if (timeout)
         smx_lockctr = 0;   
   }
   else
      smx_ERROR(SMXE_WAIT_NOT_ALLOWED, 0);
   return((bool)smx_SSRExit(gotpkt, SMX_ID_PIPE_GET_PKT_WAIT));
}

/*
*  smx_PipeGetPktWaitStop()   SSR
*
*  Stop SSR version. If called from an LSR, reports OP_NOT_ALLOWED error and
*  returns to LSR. Else, enters SSR, sets sched = STOP, and calls
*  smx_PipeGetPktWait_F(), and exits SSR. Return value is passed via taskMain(par),
*  when task restarts.
*/
void smx_PipeGetPktWaitStop(PICB_PTR pipe, void* pdst, u32 timeout)
{
   bool  gotpkt;

   if (smx_clsr)
      smx_ERROR_RET_VOID(SMXE_OP_NOT_ALLOWED, 0);
   smx_RET_IF_IN_ISR_VOID();
   smx_SSR_ENTER3(SMX_ID_PIPE_GET_PKT_WAIT_STOP, pipe, pdst, timeout);
   gotpkt = smx_PipeGetPktWait_F(pipe, pdst, timeout);
   smx_sched = SMX_CT_STOP;
   smx_SSRExit(gotpkt, SMX_ID_PIPE_GET_PKT_WAIT_STOP);
}



/*
*  smx_PipePut8()   Function
*
*  Puts byte into pipe, then advances wp, cyclically, and returns true.
*  If pipe is full, returns false. Does minimal parameter testing, for speed.
*  Can be used from an ISR.
*/
bool smx_PipePut8(PICB_PTR pipe, u8 b)
{
   u8 *wp;

   if (pipe == NULL)
      return false;

   wp = pipe->wp;
   if (!pipe->flags.full)
   {
      *wp++ = b;
      if (wp > pipe->bx)
      {
         wp = pipe->bi;
      }
      pipe->wp = wp;
      if (wp == pipe->rp)
         pipe->flags.full = 1;
      return true;
   }
   else
   {
      return false;
   }
}

/*
*  smx_PipePut8M()   Function
*
*  Puts up to lim bytes into pipe, advances wp, cyclically, and returns the
*  number of bytes actually put into pipe. Does minimal parameter testing,
*  for speed. Can be used from an ISR.
*/
u32 smx_PipePut8M(PICB_PTR pipe, u8* bp, u32 lim)
{
   u32 i, n;
   u8 *rp, *wp;

   if (pipe == NULL || bp == NULL)
      return 0;

   rp = pipe->rp;
   wp = pipe->wp;

   if (rp > wp)
   {
      n = (rp - wp -1);
   }
   else
   {
      n = (pipe->bx - wp) + (rp - pipe->bi);
   }

   n = (n <= lim ? n : lim);

   for (i = 0; i < n; i++)
   {
      *wp++ = *bp++;
      if (wp > pipe->bx)
      {
         wp = pipe->bi;
      }
   }
   pipe->wp = wp;
   if (wp == pipe->rp)
      pipe->flags.full = 1;
   return(n);
}

/*
*  smx_PipePutPkt()   Function
*
*  Puts a packet into pipe from the buffer at psrc, advances wp cyclically
*  and returns true. For invalid parameter or pipe full, returns false. Does 
*  not wait. Updating pipe->wp last avoids conflict with pipe get from ISR.
*/
bool smx_PipePutPkt(PICB_PTR pipe, void* psrc)
{
   u8 *wp = pipe->wp;
   u32 w  = pipe->width;

   if (pipe == NULL || psrc == NULL)
      return false;

   if (!pipe->flags.full)
   {
      pktcpy((u8*)psrc, wp, w);
      wp += w;
      if (wp > pipe->bx)
         wp = pipe->bi;
      pipe->wp = wp;
      if (wp == pipe->rp)
         pipe->flags.full = 1;
      return true;
   }
   else
      return false;
}

/*
*  smx_PipePutPktWait()   SSR
*
*  Suspend SSR version. Enters SSR, calls smx_PipePutPktWait_F(), and exits SSR.
*  Aborts if called from LSR and tmo != SMX_TMO_NOWAIT. Clears lockctr if
*  called from a task and tmo != SMX_TMO_NOWAIT.
*/
bool smx_PipePutPktWait(PICB_PTR pipe, void* psrc, u32 timeout, SMX_PIPE_MODE mode)
{
   bool  putpkt = false;

   smx_SSR_ENTER4(SMX_ID_PIPE_PUT_PKT_WAIT, pipe, psrc, timeout, mode);
   smx_EXIT_IF_IN_ISR(SMX_ID_PIPE_PUT_PKT_WAIT, false);
   if (!(smx_clsr && timeout))
   {
      putpkt = smx_PipePutPktWait_F(pipe, psrc, timeout, mode);
      if (putpkt == NULL && timeout)
         smx_sched = SMX_CT_SUSP;
      if (timeout)
         smx_lockctr = 0;   
   }
   else
      smx_ERROR(SMXE_WAIT_NOT_ALLOWED, 0);
   return((bool)smx_SSRExit(putpkt, SMX_ID_PIPE_PUT_PKT_WAIT));
}

/*
*  smx_PipePutPktWaitStop()   SSR
*
*  Stop SSR version. If called from an LSR, reports OP_NOT_ALLOWED error and
*  returns to LSR. Else, enters SSR, sets sched = STOP, and calls
*  smx_PipePutPktWait_F(), and exits SSR. Return value is passed via taskMain(par),
*  when task restarts.
*/
void smx_PipePutPktWaitStop(PICB_PTR pipe, void* psrc, u32 timeout, SMX_PIPE_MODE mode)
{
   bool  putpkt;

   if (smx_clsr)
      smx_ERROR_RET_VOID(SMXE_OP_NOT_ALLOWED, 0);
   smx_RET_IF_IN_ISR_VOID();
   smx_SSR_ENTER4(SMX_ID_PIPE_PUT_PKT_WAIT_STOP, pipe, psrc, timeout, mode);
   putpkt = smx_PipePutPktWait_F(pipe, psrc, timeout, mode);
  #if SMX_CFG_TOKENS
   if (smx_ct->flags.tok_ok)
  #endif
      smx_sched = SMX_CT_STOP;
   smx_SSRExit(putpkt, SMX_ID_PIPE_PUT_PKT_WAIT_STOP);
}

/*
*  smx_PipeResume()   SSR
*
*  Resumes first task waiting in pipe's task queue if its put or get can be
*  performed and returns true. Otherwise leaves task in queue and returns
*  false. Allows task to be resumed when an IO pipe operation completes.
*/
bool smx_PipeResume(PICB_PTR pipe)
{
   bool pass;
   smx_SSR_ENTER1(SMX_ID_PIPE_RESUME, pipe);
   smx_EXIT_IF_IN_ISR(SMX_ID_PIPE_RESUME, false);

   /* verify that pipe is valid and that current task has access permission */
   if (pass = (u32)smx_PICBTest(pipe, SMX_PRIV_HI))
   {
      pass = smx_PipeResume_F(pipe);
   }
   return((bool)smx_SSRExit(pass, SMX_ID_PIPE_RESUME));
}

/*
*  smx_PipePeek()   SSR
*
*  Permits looking at pipe parameters.
*/
u32 smx_PipePeek(PICB_PTR pipe, SMX_PK_PAR par)
{
   CB_PTR   p;
   u8      *rp, *wp;
   u32      val;
   u32      x;

   smx_SSR_ENTER2(SMX_ID_PIPE_PEEK, pipe, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_PIPE_PEEK, 0);

   /* verify that pipe is valid and that current task has access permission */
   if (val = (u32)smx_PICBTest(pipe, SMX_PRIV_LO))
   {
      val = 0;
      switch (par)
      {
         case SMX_PK_FULL:
            val = pipe->flags.full;
            break;
         case SMX_PK_WIDTH:
            val = pipe->width;
            break;
         case SMX_PK_LENGTH:
            val = pipe->length;
            break;
         case SMX_PK_NUMPKTS:
            sb_INT_DISABLE();
            rp = pipe->rp;
            wp = pipe->wp;
            sb_INT_ENABLE();
            if (rp == wp && pipe->flags.full)
               val = pipe->length;
            else if (rp <= wp)
               val = (wp - rp)/pipe->width;
            else
               val = 1 + (wp - pipe->bi + pipe->bx - rp)/pipe->width;
            break;
         case SMX_PK_NUMTASKS:
            if (pipe->fl)
            {
               for (p = (CB_PTR)pipe->fl, x = 0; p != (CB_PTR)pipe; p = p->fl, x++);
                  val = x;
            }
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_PIPE_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_PIPE_PEEK);
}

/*
*  smx_PipeSet()   SSR
*
*  Sets the specified pipe parameter to the specified value.
*  Not permitted in umode.
*/
bool smx_PipeSet(PICB_PTR pipe, SMX_ST_PAR par, u32 v1, u32 v2)
{
   bool pass;
   smx_SSR_ENTER4(SMX_ID_PIPE_SET, pipe, par, v1, v2);
   smx_EXIT_IF_IN_ISR(SMX_ID_PIPE_SET, false);

   /* verify that pipe is valid and that current task has access permission */
   if (pass = (u32)smx_PICBTest(pipe, SMX_PRIV_HI))
   {
      #if SMX_CFG_SSMX
      /* a utask cannot make pipe changes */
      if (smx_ct->flags.umode == 1)
         smx_ERROR_EXIT(SMXE_PRIV_VIOL, false, 0, SMX_ID_PIPE_SET);
      #endif

      /* perform set operation on pipe */
      switch (par)
      {
         case SMX_ST_CBFUN:
            pipe->cbfun = (CBF_PTR)v1;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_PIPE_SET);
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_PIPE_SET));
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            * 
*===========================================================================*/

/*
*  smx_PipeClear_F()
*
*  Resumes all tasks waiting at pipe, with false return values. Returns true, if
*  successful.
*/
static bool smx_PipeClear_F(PICB_PTR pipe)
{
   TCB_PTR t = NULL;

   while (pipe->fl)  /* clear task queue */
   {
      t = smx_DQFTask((CB_PTR)pipe);
      t->sv = 0; /* clear buffer pointer */
      t->rv = false;
      t->flags.pipe_put   = 0;
      t->flags.pipe_front = 0;
      smx_NQRQTask(t);
      smx_DO_CTTEST();
      smx_timeout[t->indx] = SMX_TMO_INF;
   }
   return true;
}

/*
*  smx_PipeGetPkt_F()
*
*  Gets a packet from pipe and updates pipe read ptr, cyclically.
*  Updating pipe->rp last avoids conflict with pipe put from ISR.
*  Assumes pipe is not empty.
*/
static void smx_PipeGetPkt_F(PICB_PTR pipe, u8* pdst)
{
   u8 *rp = pipe->rp;
   u32 w  = pipe->width;

   pktcpy(rp, pdst, w);
   rp += w;
   if (rp > pipe->bx)
      rp = pipe->bi;
   pipe->rp = rp;
   pipe->flags.full = 0;
}

/*
*  smx_PipeGetPktWait_F()
*
*  If another task is waiting to put a packet, dequeues task. If a front put, 
*  gives its pkt to waiting task. If back put, gets front pkt in pipe, and puts
*  its pkt into back of pipe. Then resumes waiting task. Otherwise, if pipe is
*  not empty gets front pkt. If pipe is empty and if timeout, suspends ctask on 
*  pipe, saving pdst in smx_ct->sv.
*  Called by smx_PipeGetPktWait() or smx_PipeGetPktWaitStop().
*/

static bool smx_PipeGetPktWait_F(PICB_PTR pipe, void* pdst, u32 timeout)
{
   TCB_PTR  ct     = smx_ct;  /* current task */
   TCB_PTR  wtask;            /* waiting task */
   bool  gotpkt;           /* got pkt */

   /* verify that pipe is valid and that current task has access permission */
   if (gotpkt = smx_PICBTest(pipe, SMX_PRIV_LO))
   {
      gotpkt = false;
      if (!pdst)
         smx_ERROR_RET(SMXE_INV_PAR, false, 0);

      /* test if wtask is waiting to put pkt in pipe */
      if (pipe->fl && pipe->fl->flags.pipe_put)
      {
         wtask = smx_DQFTask((CB_PTR)pipe);
         if (wtask->flags.pipe_front)
         {
            /* copy pkt directly from wtask psrc to pdst */
            memcpy(pdst, (u8*)wtask->sv, pipe->width);
            wtask->flags.pipe_front = 0;
         }
         else
         {
            /* get first pkt from pipe */
            smx_PipeGetPkt_F(pipe, (u8*)pdst);
            /* put pkt from wtask psrc into pipe */
            smx_PipePutPkt_F(pipe, (u8*)wtask->sv);
         }
         gotpkt = true;

         /* resume wtask */
         smx_NQRQTask(wtask);
         smx_DO_CTTEST();
         smx_timeout[wtask->indx] = SMX_TMO_INF;
         wtask->sv = 0;
         wtask->flags.pipe_put = 0;
         wtask->rv = true;
         smx_PUT_RV_IN_EXR0(wtask)
      }
      else if (!smx_PIPE_EMPTY(pipe, pipe->rp))
      {
         /* get first pkt from pipe */
         smx_PipeGetPkt_F(pipe, (u8*)pdst);
         gotpkt = true;
      }
      else if (timeout)
      {
         /* suspend ct on pipe */
         smx_DQRQTask(ct);
         smx_PNQTask((CB_PTR)pipe, ct, SMX_CB_PIPE);
         ct->sv = (u32)pdst;     /* save buffer pointer */
         ct->flags.pipe_put = 0; /* get */
         smx_TimeoutSet(ct, timeout);
      }
   }
   return(gotpkt);
}

/*
*  smx_PipePutPkt_F()
*
*  If mode = BACK: Puts a packet into pipe and updates pipe->wp, cyclically.
*  Doing this last avoids conflict with pipe get from ISR for an empty pipe. 
*  If mode = FRONT: Moves pipe->rp back one cell, cyclically, then uses rp to  
*  put packet into pipe.
*  Assumes pipe is not full to start. Sets full if pipe becomes full.
*/
static void smx_PipePutPkt_F(PICB_PTR pipe, u8* psrc, SMX_PIPE_MODE mode)
{
   u32 is;
   u8 *rp = pipe->rp;
   u8 *wp = pipe->wp;
   u32 w  = pipe->width; 

   if (mode == 0) /* fill pipe back */
   {
      pktcpy(psrc, wp, w);
      wp += w;
      if (wp > pipe->bx)
         wp = pipe->bi;
      pipe->wp = wp;
   }
   else /* fill pipe front */
   {
      is = sb_IntStateSaveDisable();
      if (rp > pipe->bi)
         rp -= w;
      else
         rp = pipe->bx;
      pipe->rp = rp;
      pktcpy(psrc, rp, w);
      sb_IntStateRestore(is);
   }
   if (pipe->wp == pipe->rp)
      pipe->flags.full = 1;
}

/*
*  smx_PipePutPktWait_F()
*
*  If another task is waiting to get a packet, gives it pkt in the
*  buffer at psrc and resumes the waiting task with true, else if the pipe
*  is not full, copies the packet into pipe and advances wp, cyclically.
*  Returns true, in both cases. If neither case and timeout is nonzero, and
*  not called from LSR, suspends ctask on pipe, saving psrc in smx_ct->sv.
*  Returns false. Called by smx_PipePutPktWait() and smx_PipePutPktWaitStop().
*/
static bool smx_PipePutPktWait_F(PICB_PTR pipe, void *psrc, u32 timeout, SMX_PIPE_MODE mode)
{
   TCB_PTR  ct     = smx_ct; /* current task */
   TCB_PTR  wtask  = NULL;   /* waiting task */
   bool  putpkt;          /* put pkt */

   /* verify that pipe is valid and that current task has access permission */
   if (putpkt = smx_PICBTest(pipe, SMX_PRIV_LO))
   {
      putpkt = false;
      if (!psrc)
         smx_ERROR_RET(SMXE_INV_PAR, false, 0);

      /* test if wtask is waiting on pipe to get pkt */
      if (pipe->fl && !pipe->fl->flags.pipe_put)
      {
         /* dequeue wtask and give pkt to it */
         wtask = smx_DQFTask((CB_PTR)pipe);
         memcpy((void*)wtask->sv, psrc, (size_t)pipe->width);
         putpkt = true;
         wtask->sv = 0;

         /* resume wtask */
         smx_NQRQTask(wtask);
         smx_DO_CTTEST();
         smx_timeout[wtask->indx] = SMX_TMO_INF;
         wtask->rv = true; /* so PipeGet() in wtask will return true. */
         smx_PUT_RV_IN_EXR0(wtask)
      }
      else if (!pipe->flags.full) /* put new pkt into pipe */
      {
         smx_PipePutPkt_F(pipe, (u8 *)psrc, mode);
         putpkt = true;
      }
      else if (timeout)
      {
         /* suspend ct on pipe */
         smx_DQRQTask(ct);
         smx_PNQTask((CB_PTR)pipe, ct, SMX_CB_PIPE);
         ct->sv = (u32)psrc;
         ct->flags.pipe_front = mode;
         ct->flags.pipe_put = 1;
         smx_TimeoutSet(ct, timeout);
      }
      /* callback */
      if (pipe->cbfun)
         pipe->cbfun((u32)pipe);
   }
   return(putpkt);
}

/*
*  smx_PipeResume_F()
*
*  Resumes first task waiting in pipe's task queue. If possible to complete 
*  its put or get operation, does so and resumes wtask with true. If put or get 
*  operation cannot be completed leaves wtask in queue and returns false.
*  NOTE: Does not implement put-to-front because it not is used with IO pipe
*  functions.
*/
bool smx_PipeResume_F(PICB_PTR pipe)
{
   bool     pass = false;
   TCB_PTR  wtask; /* waiting task */

   if (pipe->fl)
   {
      /* dequeue first waiting task from pipe queue */
      wtask = smx_DQFTask((CB_PTR)pipe);

      if (wtask->flags.pipe_put)
      {
         if (!pipe->flags.full)
         {
            /* put pkt from wtask into pipe and resume/restart wtask */
            smx_PipePutPkt_F(pipe, (u8*)wtask->sv);
            wtask->flags.pipe_put = 0;
            wtask->flags.pipe_front = 0;
            pass = true;
         }
      }
      else if (!smx_PIPE_EMPTY(pipe, pipe->rp))
      {
         smx_PipeGetPkt_F(pipe, (u8*)wtask->sv);
         pass = true;
      }
   }
   if (pass)
   {
      /* resume or restart wtask */
      smx_NQRQTask(wtask);
      smx_DO_CTTEST();
      smx_timeout[wtask->indx] = SMX_TMO_INF;
      wtask->rv = true;
      smx_PUT_RV_IN_EXR0(wtask)
   }
   return pass;
}

/*
*  pktcpy() 
*
*  Copies a packet of sz bytes from sp to dp. Best performance if both
*  pointers are u32-aligned. Next best performance if both pointers are 
*  u16-aligned. Note: Not static since used by TSMX.
*/
void pktcpy(u8* sp, u8* dp, u32 sz)
{
   u32 i = 0;

   if (((u32)sp | (u32)dp)%4 == 0)
      for (; i+3 < sz; i += 4)
      {
         *(u32*)dp = *(u32*)sp;
         dp += 4;
         sp += 4;
      }
  if (((u32)sp | (u32)dp)%2 == 0)
      for (; i+1 < sz; i += 2)
      {
         *(u16*)dp = *(u16*)sp;
         dp += 2;
         sp += 2;
      }
   for (; i < sz; i++)
   {
      *dp++ = *sp++;
   }
}

/* Notes:
   1. If wtask = NULL, all wtask flags == 0.
   2. All tasks in a pipe queue must be either gets or puts.
*/