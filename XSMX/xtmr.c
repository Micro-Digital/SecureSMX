/*
* xtmr.c                                                    Version 5.4.0
*
* smx Timer Functions
*
* Copyright (c) 1993-2025 Micro Digital Inc.
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
static bool smx_TimerStart_F(TMRCB_PTR* tmhp, u32 delay, u32 period,
                                               LCB_PTR lsr, const char* name);

/*
*  smx_TimerDup()   SSR
*
*  Creates and starts a duplicate timer after the timer duplicated. Returns
*  true if successful; returns false, if invalid parameter or out of TMRCBs,
*  reports error, and aborts.
*/
bool smx_TimerDup(TMRCB_PTR* tmrbp, TMRCB_PTR tmra, const char* name)
{
   bool      pass;
   TMRCB_PTR tmrb;

   smx_SSR_ENTER3(SMX_ID_TIMER_DUP, tmrbp, tmra, name);
   smx_EXIT_IF_IN_ISR(SMX_ID_TIMER_DUP, false);

   /* verify that tmra is valid and that current task has access permission for tmra <3> */
   if (pass = smx_TMRCBTest(tmra, SMX_PRIV_LO))
   {
      /* check tmrbp, block multiple duplications and verify current task 
         has permission to create tmrb */
      if (pass = smx_ObjectDupTest((u32*)tmrbp))
      {
         /* get a timer control block */
         if ((tmrb = (TMRCB_PTR)sb_BlockGet(&smx_tmrcbs, 4)) == NULL)
            smx_ERROR_EXIT(SMXE_OUT_OF_TMRCBS, false, 0, SMX_ID_TIMER_DUP);

         /* load TMRCB for tmrb and enqueue tmrb in smx_tq after tmra */
         memcpy((void*)tmrb,(void*)tmra, (sizeof(TMRCB) - 8));
         tmrb->name = name;
         tmrb->diffcnt = 0;
         tmrb->tmhp = tmrbp;
         tmrb->onr  = smx_clsr ? (TCB_PTR)smx_clsr : smx_ct;
         tmra->fl = tmrb;
         *tmrbp = tmrb;
      }
   }
   return (bool)smx_SSRExit(pass, SMX_ID_TIMER_DUP);
}

/*
*  smx_TimerPeek()   SSR
*
*  Permits looking at timer parameters, time left for timer, number of timers
*  waiting at tq, and the delay of the last timer (max delay).
*/
u32 smx_TimerPeek(TMRCB_PTR tmr, SMX_PK_PAR par)
{
   TMRCB_PTR nxt;
   u32 val;

   smx_SSR_ENTER2(SMX_ID_TIMER_PEEK, tmr, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_TIMER_PEEK, 0);

   /* verify that tmr is valid and that current task has access permission for tmr <3> */
   if (val = smx_TMRCBTest(tmr, SMX_PRIV_LO))
   {
      val = 0;
      switch (par)
      {
         case SMX_PK_COUNT:
            val = (u32)tmr->count;
            break;
         case SMX_PK_DELAY:
            val = (u32)tmr->nxtdly;
            break;
         case SMX_PK_DIFF_CNT:
            val = (u32)tmr->diffcnt;
            break;
         case SMX_PK_LPAR:
            val = (u32)tmr->par;
            break;
         case SMX_PK_LSR:
            val = (u32)tmr->lsr;
            break;
         case SMX_PK_MAX_DELAY:
            for (nxt = (TMRCB_PTR)smx_tq; nxt->fl != NULL; nxt = nxt->fl)
               val += nxt->fl->diffcnt;
            break;
         case SMX_PK_NAME:
            val = (u32)tmr->name;
            break;
         case SMX_PK_NEXT:
            val = (u32)tmr->fl;
            break;
         case SMX_PK_NUM:
            for (nxt = (TMRCB_PTR)smx_tq; nxt->fl != NULL; nxt = nxt->fl)
               val++;
            break;
         case SMX_PK_ONR:
            val = (u32)tmr->onr;
            break;
         case SMX_PK_OPT:
            val = (u32)tmr->flags.opt;
            break;
         case SMX_PK_PERIOD:
            val = (u32)tmr->period;
            break;
         case SMX_PK_PULSE:
            val = (u32)tmr->flags.state;
            break;
         case SMX_PK_TIME_LEFT:
            val = tmr->diffcnt;
            for (nxt = (TMRCB_PTR)smx_tq; (nxt->fl != tmr) && nxt->fl != NULL; nxt = nxt->fl)
               val += nxt->fl->diffcnt;
            break;
         case SMX_PK_WIDTH:
            val = (u32)tmr->width;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_TIMER_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_TIMER_PEEK);
}
/*
*  smx_TimerReset()   SSR
*
*  If timer is already stopped returns false and time left == 0. If it is still
*  running, dequeues it from tq and returns time left @ tlp. Then requeues timer
*  in tq. If tlp is NULL, time left is not returned. Returns true if
*  timer is successfully requeued in tq. If tmr is not a valid timer handle,
*  aborts, returns false, and reports SMXE_INV_TMRCB error.
*/
bool smx_TimerReset(TMRCB_PTR tmr, u32* tlp)
{
   u32  delay; 
   bool pass;;
   u32  time = 0;

   smx_SSR_ENTER2(SMX_ID_TIMER_RESET, tmr, tlp);
   smx_EXIT_IF_IN_ISR(SMX_ID_TIMER_RESET, false);

   /* verify that tmr is valid and that current task has access permission for tmr <3> */
   if (pass = smx_TMRCBTest(tmr, SMX_PRIV_LO))
   {
      pass = false;
      /* if tmr has not timed out <1> */
      if (tmr && tmr == *(tmr->tmhp))
      {
         time = smx_DQTimer(tmr);
         if (tmr->period == 0)   /* one-shot timer */
            delay = tmr->nxtdly;
         else
            delay = tmr->period - tmr->width;
         smx_NQTimer(tmr, delay);
         tmr->flags.state = SMX_TMR_LO;
         pass = true;
      }
      if (tlp)
         *tlp = time;
   }
   return (bool)smx_SSRExit(pass, SMX_ID_TIMER_RESET);
}

/*
*  smx_TimerSetLSR()   SSR
*
*  Changes timer's LSR, par, and parameter option.
*/
bool smx_TimerSetLSR(TMRCB_PTR tmr, LCB_PTR lsr, SMX_TMR_OPT opt, u32 par)
{
   bool pass;

   smx_SSR_ENTER4(SMX_ID_TIMER_SET_LSR, tmr, lsr, opt, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_TIMER_SET_LSR, false);

   /* verify that tmr is valid and that current task has access permission for tmr <3> */
   if (pass = smx_TMRCBTest(tmr, SMX_PRIV_HI))
   {
      /* verify parameters are valid */
      if (lsr == NULL || opt > 3)
         smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_TIMER_SET_LSR);

      tmr->lsr = lsr;
      tmr->par = par;
      tmr->flags.opt = opt;
   }
   return (bool)smx_SSRExit(pass, SMX_ID_TIMER_SET_LSR);
}

/*
*  smx_TimerSetPulse()   SSR
*
*  Sets timer's pulse width and period. If pulse LO, sets timer's next delay
*  = width. If pulse HI, the current period is allowed to finish normally and
*  width and period changes take effect on the next period.
*/
bool smx_TimerSetPulse(TMRCB_PTR tmr, u32 period, u32 width)
{
   bool pass;

   smx_SSR_ENTER3(SMX_ID_TIMER_SET_PULSE, tmr, period, width);
   smx_EXIT_IF_IN_ISR(SMX_ID_TIMER_SET_PULSE, false);

   /* verify that tmr is valid and that current task has access permission for tmr <3> */
   if (pass = smx_TMRCBTest(tmr, SMX_PRIV_HI))
   {
      /* verify parameters are valid */
      if (width >= period)
         smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_TIMER_SET_PULSE);

      tmr->period = period;
      tmr->width = width;
      if (tmr->flags.state == SMX_TMR_LO)
         tmr->nxtdly = width;
   }
   return (bool)smx_SSRExit(pass, SMX_ID_TIMER_SET_PULSE);
}

/*
*  smx_TimerStart()   SSR
*
*  Creates and starts or restarts an smx timer. Accepts delay from now. Returns
*  true if successful; returns false, if invalid parameter or out of TMRCBs,
*  reports error, and aborts.
*/
bool smx_TimerStart(TMRCB_PTR* tmhp, u32 delay, u32 period, LCB_PTR lsr, 
                                                          const char* name)
{
   bool pass;

   smx_SSR_ENTER5(SMX_ID_TIMER_START, tmhp, delay, period, lsr, name);
   smx_EXIT_IF_IN_ISR(SMX_ID_TIMER_START, false);
   pass = smx_TimerStart_F(tmhp, delay, period, lsr, name);
   return (bool)smx_SSRExit(pass, SMX_ID_TIMER_START);
}

/*
*  smx_TimerStartAbs()   SSR
*
*  Same as smx_TimerStart() except it accepts absolute time from startup.
*/
bool smx_TimerStartAbs(TMRCB_PTR* tmhp, u32 time, u32 period, LCB_PTR lsr, 
                                                            const char* name)
{
   bool pass;
   u32  delay = time > smx_SysPeek(SMX_PK_ETIME) ? time - smx_SysPeek(SMX_PK_ETIME) : 0;

   smx_SSR_ENTER5(SMX_ID_TIMER_START_ABS, tmhp, time, period, lsr, name);
   smx_EXIT_IF_IN_ISR(SMX_ID_TIMER_START_ABS, false);
   pass = smx_TimerStart_F(tmhp, delay, period, lsr, name);
   return (bool)smx_SSRExit(pass, SMX_ID_TIMER_START_ABS);
}

/*
*  smx_TimerStop()   SSR
*
*  smx_TimerStop stops and deletes a timer created by smx_TimerStart() or
*  smx_TimerStartAbs(). If already stopped returns true and time_left == 0.
*  If still running control block is cleared and returned to the TMRCB pool
*  and its handle is cleared. If the optional parameter tlp is != NULL,
*  then *tlp = the unexpired time of the timer. If the timer is stopped,
*  or already stopped, retuns true.
*/
bool smx_TimerStop(TMRCB_PTR tmr, u32* tlp)
{
   bool pass;
   u32  time = 0;

   smx_SSR_ENTER2(SMX_ID_TIMER_STOP, tmr, tlp);
   smx_EXIT_IF_IN_ISR(SMX_ID_TIMER_STOP, false);

   /* verify that tmr is valid and that current task has access permission for tmr <3> */
   if (pass = smx_TMRCBTest(tmr, SMX_PRIV_LO))
   {
      #if !defined (SMX_FRPORT)
      /* if tmr not already stopped and not timed out <1> */
      if (tmr && (tmr == *(tmr->tmhp)))
      #else
      /* if tmr not already stopped <2> */
      if (tmr && (tmr != NULL))
      #endif
      {
         time = smx_DQTimer(tmr);

         #if !defined(SMX_FRPORT)
         /* clear timer handle <2> */
         *(tmr->tmhp) = NULL;
         #endif
         /* clear and release TMRCB */
         sb_BlockRel(&smx_tmrcbs, (u8*)tmr, sizeof(TMRCB));
      }
      if (tlp)
         *tlp = time;
   }
   return (bool)smx_SSRExit(pass, SMX_ID_TIMER_STOP);
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  smx_TimerStart_F()
*
*  Called by smx_TimerStart() and smx_TimerStartAbs(). If starting timer the
*  first time, allocates timer control block from the TMRCB pool and loads
*  permanent TMRCB fields. If restarting timer, dequeues it from the timer
*  queue, smx_tq. In both cases, loads TMRCB fields which can change on restart,
*  then enqueues the timer in smx_tq.
*/
static bool smx_TimerStart_F(TMRCB_PTR* tmhp, u32 delay, u32 period, 
                                                 LCB_PTR lsr, const char* name)
{
   bool      pass;
   TMRCB_PTR tmr;

   /* verify current task has timer start permission */
   pass = smx_TOKEN_TEST(smx_ct, (u32)tmhp, SMX_PRIV_LO);
   if (pass)
   {
      /* verify parameters are valid */
      if (tmhp == NULL || delay == 0 || lsr == NULL)
         smx_ERROR_RET(SMXE_INV_PAR, false, 0);
      tmr = *tmhp;

      /* new timer start */
      if (tmr == NULL)
      {
         /* get a timer control block */
         if ((tmr = (TMRCB_PTR)sb_BlockGet(&smx_tmrcbs, 4)) == NULL)
            smx_ERROR_RET(SMXE_OUT_OF_TMRCBS, false, 0);

         /* load static TMRCB fields */
         tmr->cbtype = SMX_CB_TMR;
         tmr->flags.state = SMX_TMR_LO;
         tmr->flags.opt = SMX_TMR_PAR;
         tmr->width = 0;
         tmr->par = 0;
         tmr->tmhp = tmhp;
      }
      else  /* restart existing timer */
      {
         /* verify that tmr is valid and that current task has access permission 
            for tmr */
         if (pass = smx_TMRCBTest(tmr, SMX_PRIV_LO))
            smx_DQTimer(tmr); /* dequeue timer from tq */
      }

      /* load TMRCB fields that can change on restart */
      if (pass)
      {
         tmr->count   = 0;
         tmr->name    = name;
         tmr->nxtdly  = delay;
         tmr->period  = period;
         tmr->lsr     = lsr;
         tmr->onr     = smx_clsr ? (TCB_PTR)smx_clsr : smx_ct;

         /* enqueue timer in tq based upon its delay, and its load handle */
         smx_NQTimer(tmr, delay);
         *tmhp = tmr;
      }
   }
   return pass;
}

/*
*  smx_TimerTimeout()   Internal Subroutine
*
*  Called from smx_KeepTimeLSRMain(), if the first timer in smx_tq has expired.
*  Dequeues expired timer from smx_tq. If cyclic timer, requeues it. If not
*  in pulse mode, use its period, else use the nxtdly in the TMRCB, toggle the
*  HI/LOW mode flag, and load next nxtdly. Then invoke timer's LSR with the
*  selected parameter. If a one-shot timer, clear its handle and delete it.
*  Repeat for all other expired timers. <1>
*/
void smx_TimerTimeout(void)
{
  #if defined(SMX_FRPORT)
   u32      count;
  #endif
   u32      nxt_delay, par;
   TMRCB_PTR tmr = (TMRCB_PTR)smx_tq->fl;

   /* loop for all expired timers */
   for (; tmr && (tmr->diffcnt == 0); tmr = (TMRCB_PTR)smx_tq->fl)
   {
      /* de-queue timer & increment its timeout count */
      smx_tq->fl = (CB_PTR)tmr->fl;
      tmr->count++;

      /* requeue cyclic timer */
      if (tmr->period != 0)
      {
         if (tmr->width == 0) /* normal mode */
            nxt_delay = tmr->period;
         else  /* pulse timer */
         {
            nxt_delay = tmr->nxtdly;
            tmr->flags.state = (tmr->flags.state == SMX_TMR_HI) ?
                                                SMX_TMR_LO : SMX_TMR_HI;
            if (tmr->flags.state == SMX_TMR_HI)
               tmr->nxtdly = tmr->period - tmr->width;
            else
               tmr->nxtdly = tmr->width;
         }
         smx_NQTimer(tmr, nxt_delay);
      }

      /* invoke LSR */
      if (tmr->lsr != NULL)
      {
         switch (tmr->flags.opt)
         {
            case SMX_TMR_PAR:
               par = tmr->par;
               break;
            case SMX_TMR_STATE:
               par = tmr->flags.state;
               break;
            case SMX_TMR_TIME:
               par = smx_etime;
               break;
            case SMX_TMR_COUNT:
               par = tmr->count;
         }
         smx_LSR_INVOKE(tmr->lsr, par);
      }

      /* one-shot timer */
      if (tmr->period == 0)
      {
         #if !defined(SMX_FRPORT)
         /* delete timer and clear its handle */
         *(tmr->tmhp) = NULL;
         sb_BlockRel(&smx_tmrcbs, (u8*)tmr, sizeof(TMRCB));
         #else
         /* preserve timeout count and put timer into inactive state */
         count = tmr->count;
         smx_TimerStart(&tmr, SMX_TMO_INF, 0, tmr->lsr, tmr->name);
         tmr->flags.act = false;
         tmr->count = count;
         #endif
      }
   }
}

/* Note:
   1. Used in other smx files.
*/