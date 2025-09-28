/*
* xtime.c                                                   Version 5.4.0
*
* smx_KeepTimeLSR, smx_TimeoutLSR, and time-related functions.
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
* smx_KeepTimeLSR operates all the timing functions required by the kernel:
*
*   - Increment smx_etime and smx_stime.
*   - Process smx timers.
*   - Invoke TimeoutLSR.
*   - Invoke ProfileLSR.
*
* smx_TimeoutLSR resumes any tasks that have timed out waiting for an SSR to
* complete.
*
*****************************************************************************/

#include "xsmx.h"

/* internal subroutines */
/* smx_TaskTimeout() in xsmx.h since shared */

u32      smx_tick_ctr = 0;      /* counter used to time seconds */

#if SMX_CFG_PROFILE
volatile u32 smx_rtc_frame_ctr = 0; /* determines rtc frame */
#endif

/*
*  smx_KeepTimeLSRMain
*
*  This LSR is invoked by smx_TickISR. It handles all timing in smx.
*/
void smx_KeepTimeLSRMain(u32 par)
{
   static u32 tick_ctr_hmng = 0; /* controls frequency of smx_HeapManager() runs */

   /* update etime */
   ++smx_etime;

   /* if smx_tq is not empty, decrement first timer diffcnt; if 0 call
      smx_TimerTimeout() */
   if (smx_tq->fl != NULL)
   {
      ((TMRCB_PTR)smx_tq->fl)->diffcnt--;
      if (((TMRCB_PTR)smx_tq->fl)->diffcnt == 0)
         smx_TimerTimeout();
   }

   /* invoke task timeout LSR */
   if (smx_etime >= smx_tmo_min) {
      smx_LSR_INVOKE(smx_TimeoutLSR, 0); }

   /* update stime, enable MS scan, and transfer coarse profile counts */
   if (++smx_tick_ctr >= SMX_TICKS_PER_SEC)
   {
      smx_tick_ctr -= SMX_TICKS_PER_SEC;
      ++smx_stime;
      smx_mshwmv = false;  /* enable main stack to be scanned */

      #if SMX_CFG_PROFILE
      /* if new coarse profile counts are ready and previous counts have been
         displayed, transfer new counts from cpa to cpd. */
      if (smx_cpa.rdy && !smx_cpd.rdy)
      {
         smx_cpd.idle = smx_cpa.idle;
         smx_cpa.idle = 0;
         smx_cpd.work = smx_cpa.work;
         smx_cpa.work = 0;
         smx_cpd.ovh = smx_cpa.ovh;
         smx_cpa.ovh = 0;
         smx_cpd.rdy = smx_cpa.rdy;
         smx_cpa.rdy = 0;
      }
      #endif
   }

   /* set flag to run HeapManager next time in idle */
   if (++tick_ctr_hmng >= SMX_TICKS_PER_SEC/10)
   {
      tick_ctr_hmng = 0;
      smx_hmng = true;
   }

   #if SMX_CFG_RTLIM
   /* check if runtime limit reached for smx_ct <1> */
   u32 rtlim = (smx_ct->parent == NULL ? smx_ct->rtlim : *(u32*)smx_ct->rtlim); /*<2>*/
   if (rtlim > 0) /*<3>*/
   {
      u32 rtlimctr = (smx_ct->parent == NULL ? smx_ct->rtlimctr : *(u32*)smx_ct->rtlimctr); /*<2>*/
      if (rtlimctr >= rtlim)
      {
         /* suspend smx_ct */
         smx_DQRQTask(smx_ct);
         smx_NQTask((CB_PTR)smx_rtlimsem, smx_ct);
         if (smx_sched != SMX_CT_STOP)
            smx_sched = SMX_CT_SUSP;
      }
   }
   #endif

   #if SMX_CFG_PROFILE
   /* first time init */
   if (smx_rtc_frame_ctr == 0)
      smx_rtc_frame_ctr = SMX_RTC_FRAME;

   /* invoke profile LSR at the end of a profile frame */
   if (--smx_rtc_frame_ctr == 0)
   {
      smx_LSR_INVOKE(smx_ProfileLSR, 0);
      smx_rtc_frame_ctr = SMX_RTC_FRAME;
   }
   #endif
}

/*
*  smx_TimeoutLSRMain
*
*  This LSR is invoked by smx_KeepTimeLSR, if smx_etime >= minimum timeout. It
*  calls smx_TaskTimeout to resume the timed-out task and re-invokes itself, if
*  another task has also timed out.
*/
void smx_TimeoutLSRMain(u32 par)
{
   u32 etime = smx_etime;
   if (etime >= smx_tmo_min)              /* check again in case multiple invokes before run */
      smx_TaskTimeout(etime);
   if (etime >= smx_tmo_min)
      smx_LSR_INVOKE(smx_TimeoutLSR, 0);  /* allow other LSRs to run */
}

/*
*  smx_TimeoutSet()   Internal Subroutine (Disables Interrupts)
*
*  Sets timeout for task and adjusts smx_tmo_min and smx_tmo_indx if timeout is 
*  less. If timeout & SMX_FL_MSEC, converts from msec to ticks, rounding up.
*/

void smx_TimeoutSet(TCB_PTR task, u32 timeout)
{
   u32 ti = task->indx;

   sb_INT_DISABLE();
   if (timeout == SMX_TMO_INF)
      smx_timeout[ti] = SMX_TMO_INF;
   else if (timeout != SMX_TMO_NOCHG)
   {
      if (timeout & SMX_FL_MSEC)
      {
         /* round up to next tick */
         timeout = ((timeout & ~SMX_FL_MSEC) * SMX_TICKS_PER_SEC + 999) / 1000;
      }
      timeout = smx_SysPeek(SMX_PK_ETIME) + timeout;
      smx_timeout[ti] = timeout;
      if (timeout < smx_tmo_min)
      {
         smx_tmo_min = timeout;
         smx_tmo_indx = ti;
      }
   }
   sb_INT_ENABLE();
}

/*
*  smx_EtimeRollover()   Function (Disables LSRs)
*
*  Checks if smx_etime and all active timeouts have reached 0x80000000.
*  If so, clears msb of etime and timeouts. Called by smx_IdleMain().
*/
void smx_EtimeRollover(void)
{
   u32 tn;
   /* globals optimization */
   u32 tn_lim = SMX_NUM_TASKS;

   if (smx_etime >= 0x80000000)
   {
      smx_LSRsOff(); /* block smx_KeepTimeLSR & smx_TimeoutLSR */
      for (tn = 0; tn < tn_lim; tn++)
         if (smx_timeout[tn] < 0x80000000)
            break;
      if (tn == tn_lim)
      {
         for (tn = 0; tn < tn_lim; tn++)
         {
            if (smx_timeout[tn] != 0xFFFFFFFF)
               smx_timeout[tn] &= 0x7FFFFFFF;
         }
         smx_tmo_min &= 0x7FFFFFFF;
         smx_etime &= 0x7FFFFFFF;
      }
      smx_LSRsOn();
   }
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  smx_TaskTimeout
*
*  Resumes the first timed-out task, unless the task has already been resumed.
*  Also sets resumed task->err = SMXE_TMO. It then finds the task with next
*  timeout and updates smx_tmo_min and smx_tmo_indx for it. Shared between
*  smx_KeepTimeLSRMain() and smx_TickRecovery(). Timers for tasks not timing
*  out == SMX_TMO_INF.
*/
void smx_TaskTimeout(u32 etime)
{
   u32 i;

   /* globals optimizations */
   TCB_PTR task;
   u32 num_tasks = SMX_NUM_TASKS;
   u32 tmo_min   = smx_tmo_min;
   u32 tmo_indx  = smx_tmo_indx;

   /* resume task if it has not already been resumed */
   if (smx_timeout[tmo_indx] == tmo_min)
   {
      task = (TCB_PTR)smx_tcbs.pi + tmo_indx;
      if (task->pritmo > task->pri)
      {
         task->pri     = task->pritmo;
         task->prinorm = task->pritmo;
      }
      else
         task->err = SMXE_TMO;
      smx_TaskResume(task); /* resume sets timeout[tmo_indx] = SMX_TMO_INF */
   }

   /* find the next soonest timeout and update tmo_min and tmo_indx */
   tmo_min = SMX_TMO_INF;
   tmo_indx = 0;
   for (i = 0; i < num_tasks; i++)
   {
      if (smx_timeout[i] < tmo_min)
      {
         tmo_min = smx_timeout[i];
         tmo_indx = i;
      }
   }
   smx_tmo_min = tmo_min;
   smx_tmo_indx = tmo_indx;
}

/* Notes:
   1. smx_Idle has no runtime limit. Instead, it runs the number of passes
      specified by SMX_IDLE_RTLIM, and the completion of those passes
      ends the frame. See the handling in smx_IdleMain() which resumes all
      tasks at rtlimsem and clears all non-child RTL counters.
   2. A child task uses its top parent task's rtlim and rtlimctr.
   3. If rtlim == 0, smx_ct has no runtime limit.
*/