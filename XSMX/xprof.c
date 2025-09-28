/*
* xprof.c                                                   Version 5.4.0
*
* Profiling functions.
*
* Copyright (c) 2011-2025 Micro Digital Inc.
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
#include "bsp.h"

#if SMX_CFG_PROFILE || SMX_CFG_RTLIM
/* internal subroutines */
static void smx_ldval(u32 v, char* pb);
#endif

#define DIVR(x,y) ((2*((x)%(y)) < (y)) ? (x)/(y) : ((x)/(y))+1)
#define SF  2  /* smoothing factor */

#if CP_PORTAL
#include "cprtl.h"

#if defined(SMX_TSMX)
extern  FPCS    cpcli_tsmx;
#define CP_PCH &cpcli_tsmx
#else
extern  FPCS    cpcli_idle;
#define CP_PCH &cpcli_idle
#endif
#endif

/*============================================================================
                              PROFILING VARIABLES
============================================================================*/

#if SMX_CFG_PROFILE || SMX_CFG_RTLIM
typedef enum {SMX_OVH, SMX_TASK, SMX_LSR} SMX_PF;
u32            smx_lsr_rtc;   /* LSR runtime counter */
SMX_PF         smx_pf;        /* profile flag */
static s32     smx_ptime;     /* precise time */
#endif

#if SMX_CFG_PROFILE
CPS            smx_cpa;       /* coarse profile accumulator */
CPS            smx_cpd;       /* coarse profile display */
u32            smx_i_rtc;     /* captured ISR rtc at end of frame */
u32            smx_isr_rtc;   /* ISR runtime counter */
u32            smx_l_rtc;     /* captured LSR rtc at end of frame */
u32            smx_pftc;      /* profile frame total count */
u32            smx_pidle;     /* % of time idle */
u32            smx_povhd;     /* % of time in smx */
u32            smx_pwork;     /* % of time doing useful work */
static u32     smx_pisrnest;  /* profiled ISR nesting level */
u32*           smx_rtcbi;     /* start of smx_rtcb[][] */
static u32*    smx_rtcbn;     /* next cell to fill */
static u32*    smx_rtcbx;     /* end */
bool           smx_rtcs_clr;  /* clear all rtc's when start 1st frame */
#if (SB_CFG_CON)
char           smx_prof_buf[60];
#endif
#endif /* SMX_CFG_PROFILE */

/*============================================================================
                          PROFILING CAPTURE FUNCTIONS
============================================================================*/

#if SMX_CFG_PROFILE
/* The following functions are inserted into smx and ISRs by macros of similar
*  names in xsmx.h. */

void smx_RTC_ISRStart(void)
{
   s32 d, e;
   u32 istate;
   istate = sb_IntStateSaveDisable();  /* ISR may enter with ints enabled */
   if (++smx_pisrnest == 1)
   {
      d = smx_ptime;
      smx_ptime = sb_PtimeGet();
      e = smx_ptime - d;
      if (e < 0)
         e += SB_TICK_TMR_COUNTS_PER_TICK;
      if (smx_pf == SMX_TASK)
      {
         smx_ct->rtc += e;
        #if SMX_CFG_RTLIM
         if (smx_ct->parent)
            *(u32*)smx_ct->rtlimctr += e; /* update top parent rtlimctr */
         else
            smx_ct->rtlimctr += e;        /* update smx_ct->rtlimctr */
         #endif
      }
      if (smx_pf == SMX_LSR)
         smx_lsr_rtc += e;
   }
   sb_IntStateRestore(istate);
}

void smx_RTC_ISREnd(void)
{
   s32 d, e;
   if (--smx_pisrnest == 0)
   {
      d = smx_ptime;
      smx_ptime = sb_PtimeGet();
      e = smx_ptime - d;
      if (e < 0)
         e += SB_TICK_TMR_COUNTS_PER_TICK;
      smx_isr_rtc += e;
      smx_pf = SMX_OVH;
   }
   /* No sb_INT_ENABLE() */
}

void smx_RTC_LSRStart(void)
{
   s32 d, e;
   sb_INT_DISABLE();
   d = smx_ptime;
   smx_ptime = sb_PtimeGet();
   e = smx_ptime - d;
   if (e < 0)
      e += SB_TICK_TMR_COUNTS_PER_TICK;
   if (smx_pf == SMX_TASK)
      smx_ct->rtc += e;
   smx_pf = SMX_LSR;
   sb_INT_ENABLE();
}
#endif /* SMX_CFG_PROFILE */

#if SMX_CFG_PROFILE || SMX_CFG_RTLIM

void smx_RTC_LSREnd(void)
{
   s32 d, e;
   sb_INT_DISABLE();
   d = smx_ptime;
   smx_ptime = sb_PtimeGet();
   e = smx_ptime - d;
   if (e < 0)
      e += SB_TICK_TMR_COUNTS_PER_TICK;
   smx_lsr_rtc += e;
   smx_pf = SMX_OVH;
   sb_INT_ENABLE();
}

void smx_RTC_TaskStart(void)   /* interrupts enabled */
{
   sb_INT_DISABLE();
   smx_ptime = sb_PtimeGet();
   smx_pf = SMX_TASK;
   sb_INT_ENABLE();
}

void smx_RTC_TaskStartID(void) /* interrupts disabled */
{
   smx_ptime = sb_PtimeGet();
   smx_pf = SMX_TASK;
}

void smx_RTC_TaskEnd(void)   /* interrupts enabled */
{
   s32 d, e;
   sb_INT_DISABLE();
   if (smx_pf == SMX_TASK)
   {
      d = smx_ptime;
      smx_ptime = sb_PtimeGet();
      e = smx_ptime - d;
      if (e < 0)
         e += (SB_TICK_TMR_COUNTS_PER_TICK);
      if (e >= SB_TICK_TMR_COUNTS_PER_TICK)
         sb_DEBUGTRAP();
      smx_ct->rtc += e;
      smx_pf = SMX_OVH;

     #if SMX_CFG_RTLIM
      if (smx_ct->parent)
         *(u32*)smx_ct->rtlimctr += e; /* update top parent rtlimctr <1> */
      else
         smx_ct->rtlimctr += e;        /* update smx_ct->rtlimctr */
     #endif
   }
   sb_INT_ENABLE();
}
#endif /* SMX_CFG_PROFILE || SMX_CFG_RTLIM */

/*============================================================================
                         GENERAL PROFILING FUNCTIONS
============================================================================*/

#if SMX_CFG_PROFILE
/*
*  smx_ProfileInit()
*
*  This function is called during initialization to initialize smx_rtcb pointers,
*  clear smx_rtcb[][], and set a flag to clear all rtc's at the start of the
*  first frame. Setting smx_rtcbn allows profiling to begin. smx_rtcb[][] is
*  defined at the application level. Also determines smx_pftc.
*/
void smx_ProfileInit(void)
{
   u32  *p;
   if (SMX_RTCB_SIZE > 0)
   {
      smx_rtcbn = smx_rtcbi;
      smx_rtcbx = smx_rtcbi + ((SMX_NUM_TASKS + 5) * SMX_RTCB_SIZE) - 1;
      for (p = smx_rtcbi; p <= smx_rtcbx ; p++) /* clear smx_rtcb */
         *p = 0;
      smx_rtcs_clr = true; /* set to clear all rtc's at start */
      smx_pftc = SMX_RTC_FRAME * SB_TICK_TMR_COUNTS_PER_TICK;
   }
}

/*
*  smx_ProfileLSRMain()
*
*  This LSR is invoked by smx_KeepTimeLSR every profile frame. It gathers run
*  time counts (rtc's) and stores them in smx_rtc[][]. At the same time, it
*  resets all rtc's to start a new frame. It calculates overhead rtc by subtracting
*  the sum of all other rtc's from the total frame count.
*/
void smx_ProfileLSRMain(u32 par)
{
   TCB_PTR t;
   u32 i_rtc, l_rtc, o_rtc, tt_rtc, tot_rtc, idl_rtc;
   (void)par;

   if (smx_rtcbn == 0)
      return;

   if (smx_rtcs_clr)  /* clear rtc's on first frame */
   {
      smx_isr_rtc = 0;
      smx_lsr_rtc = 0;
      smx_i_rtc = 0;
      smx_l_rtc = 0;
      for (t = (TCB_PTR)smx_tcbs.pi; t <= (TCB_PTR)smx_tcbs.px; t++)
         t->rtc = 0;
      smx_rtcs_clr = false;
      return;
   }

   if (smx_rtcbn > smx_rtcbx)  /* wrap around rtcbn */
      smx_rtcbn = smx_rtcbi;

   i_rtc = smx_i_rtc;
   l_rtc = smx_l_rtc;
   *smx_rtcbn++ = smx_etime;
   *smx_rtcbn++ = i_rtc;
   *smx_rtcbn++ = smx_l_rtc;
   idl_rtc = smx_Idle->rtc;

   for (t = (TCB_PTR)smx_tcbs.pi, tt_rtc = 0; t <= (TCB_PTR)smx_tcbs.px; t++)
   {
      *smx_rtcbn++ = t->rtc;
      tt_rtc += t->rtc;
      t->rtc = 0;
   }

   tot_rtc = i_rtc + l_rtc + tt_rtc;
   if (smx_pftc > tot_rtc)
      o_rtc = smx_pftc - tot_rtc;
   else
      o_rtc = 0;
   *smx_rtcbn++ = tt_rtc;
   *smx_rtcbn++ = o_rtc;

   /* update smx_cpa fields and set smx_cpa.rdy */
   smx_cpa.idle += idl_rtc;
   smx_cpa.work += (tot_rtc - idl_rtc);
   smx_cpa.ovh  += o_rtc;
   smx_cpa.rdy = true;
}

/*
*  smx_ProfileDisplay()
*
*  Calculates and displays percentages for idle, overhead, and work. Results are
*  smoothed before displaying.
*/
void smx_ProfileDisplay(void)
{
   u32    i, o, t, w;
  #if (SB_CFG_CON)
   u32    s;
  #endif

   if (smx_cpd.rdy && !sb_ConDbgMsgMode())
   {
      /* smx_cpd contains the accumulated coarse profile counts for the
         previous second; they are loaded by smx_KeepTimeLSR. */
      i = smx_cpd.idle;
      w = smx_cpd.work;
      o = smx_cpd.ovh;
      smx_cpd.rdy = false;

      /* calculate smoothed values for % overhead, idle, and work. scale t down
         by 10000 for .01% resolution. */
      t = (i + w + o)/10000;
      if (smx_pwork == 0) /* first time */
      {
         smx_pidle = DIVR(i,t);
         smx_pwork = DIVR(w,t);
         smx_povhd = DIVR(o,t);
      }
      else /* subsequent times */
      {
         smx_pidle = DIVR((DIVR(i,t) + SF*smx_pidle), SF + 1);
         smx_pwork = DIVR((DIVR(w,t) + SF*smx_pwork), SF + 1);
         smx_povhd = DIVR((DIVR(o,t) + SF*smx_povhd), SF + 1);
      }

     #if (SB_CFG_CON)
      /* Display profile counters with resolution of 0.1% */
      strcpy(smx_prof_buf, "Idle =       Work =       Ovh =       Sec =         ");
      smx_ldval(smx_pidle, smx_prof_buf + 7);
      smx_ldval(smx_pwork, smx_prof_buf + 20);
      smx_ldval(smx_povhd, smx_prof_buf + 32);
      s = smx_SysPeek(SMX_PK_ETIME)/SMX_TICKS_PER_SEC;
      ultoa(s, smx_prof_buf + 44, 10);
      sb_ConWriteString(0, 23, SB_CLR_LIGHTCYAN, SB_CLR_BLACK, !SB_CON_BLINK, smx_prof_buf);
     #endif /* SB_CFG_CON */
   }
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            * 
*===========================================================================*/

#if (SB_CFG_CON)
/*
*  Converts a value v < 100 to ASCII, loads at pb, locates the last digit, and
*  inserts decimal point ahead of it. If v < 1, adds a leading 0.
*/
void smx_ldval(u32 v, char* pb)
{
   char *pbn = pb + 1;
   itoa(v, pb, 10);

   if (*pbn == 0)             /* 0.0v */
   {
      *(pb+3) = *pb;
      *(pb+2) = '0';
      *(pb+1) = '.';
      *pb     = '0';
   }
   else if (*(pbn+1) == 0)    /* 0.vv */
   {
      *(pb+3) = *(pb+1);
      *(pb+2) = *pb;
      *(pb+1) = '.';
      *pb     = '0';
   }
   else if (*(pbn+2) == 0)    /* v.vv */
   {
      *(pb+3) = *(pb+2);
      *(pb+2) = *(pb+1);
      *(pb+1) = '.';
   }
   else                       /* vv.vv */
   {
      *(pb+4) = *(pb+3);
      *(pb+3) = *(pb+2);
      *(pb+2) = '.';
   }
}
#endif /* SB_CFG_CON */
#endif /* SMX_CFG_PROFILE */

/* Notes:
   1. Pointed to by smx_ct->rtlimctr.
*/