/*
* xsys.c                                                    Version 6.0.0
*
* smx System Service Functions
*
* Copyright (c) 1993-2026 Micro Digital Inc.
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
* Author: Ralph Moore
*
*****************************************************************************/

#include "xsmx.h"

/* internal subroutines */
static void smx_TickRecovery(void);

/* internal variables */
static u32  pd_te;
static u32  pd_ts;
static u32  ticks_lost;

/*
*  smx_SysPeek()   Function
*
*  Returns requested system value.
*/
u32 smx_SysPeek(SMX_PK_PAR par)
{
   u32 val = 0;

   smx_SSR_ENTER1(SMX_ID_SYS_PEEK, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_SYS_PEEK, 0);

   switch (par)
   {
      case SMX_PK_ETIME:
         val = smx_etime;
         break;
      case SMX_PK_ETIME_MS:
         val = smx_etime*(1000/SMX_TICKS_PER_SEC);;
         break;
      case SMX_PK_SEC:
         val = SMX_TICKS_PER_SEC;
         break;
      case SMX_PK_STIME:
         val = smx_stime;
         break;
      default:
         smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_SYS_PEEK);
   }
   return (u32)smx_SSRExit(val, SMX_ID_SYS_PEEK);
}

#define  SMX_PSEUDO_HANDLE_MIN 0xFFFFF000 /*<1>*/
#define  SMX_PSEUDO_HANDLE_MAX 0xFFFFFFFD /*<1>*/

/*
*  smx_SysPseudoHandleCreate()   Function
*
*  Creates a pseudo handle for use in the event buffer and handle table.
*/
void* smx_SysPseudoHandleCreate(void)
{
   static u32 next_ph = SMX_PSEUDO_HANDLE_MIN;
   return (void*)(next_ph > SMX_PSEUDO_HANDLE_MAX ? 0 : next_ph++);
}

/*
*  smx_SysPowerDown()   SSR
*
*  Called from the idle task to power down processor. Updates all tick-based
*  timing when power returns, by simulating the number of ticks lost. LSRs
*  and tasks are blocked from running until this SSR completes. However, ISRs
*  can run and the tick ISR can invoke smx_KeepTimeLSR, so no ticks will be
*  lost. The profile frame and timeslice count will continue from where they
*  were at power down. Define sleep_mode values (levels) for your application.
*/
bool smx_SysPowerDown(u32 sleep_mode)
{
   if (sleep_mode == 0) return false;
   sb_TM_START(&pd_ts);
   smx_SSR_ENTER1(SMX_ID_SYS_POWER_DOWN, sleep_mode);
   smx_EXIT_IF_IN_ISR(SMX_ID_SYS_POWER_DOWN, false);

   /* power down processor */
   ticks_lost = sb_PowerDown(sleep_mode);

   /* power up: recover ticks lost */
   smx_stime += (smx_tick_ctr + ticks_lost)/SMX_TICKS_PER_SEC;
   smx_tick_ctr = (smx_tick_ctr + ticks_lost)%SMX_TICKS_PER_SEC;
   smx_TickRecovery();

   sb_TM_END(pd_ts, &pd_te);
   return((bool)smx_SSRExit(true, SMX_ID_SYS_POWER_DOWN));
}

/*
* smx_SysTest()  Function
*/
u32 msp_sav;

#pragma optimize=none  /* disable for next function */

void recursive_fct(void)
{
   recursive_fct();
}

void smx_SysTest(u32 test)
{
   switch (test)
   {
      #if defined(SMX_DEBUG)
      case 1:
         /* save msp and call recursive_fct() */
         __asm
         (
            "THUMB \n\t"
            "ldr r0, =msp_sav \n\t"
            "mrs r1, msp \n\t"
            "str r1, [r0] \n\t" 
         );
         recursive_fct(); /* trigger UF */
         break;
      #endif
      default:
         smx_ERROR(SMXE_INV_OP, 0);
   }
}

/*
*  smx_SysWhatIs()   SSR
*
*  Returns type corresponding to handle provided. If h == 0 or type is out
*  of range, returns NULL. h is not checked for validity.
*/
SMX_CBTYPE smx_SysWhatIs(void* h)
{
   SMX_CBTYPE type;

   smx_SSR_ENTER1(SMX_ID_SYS_WHAT_IS, h);
   smx_EXIT_IF_IN_ISR(SMX_ID_SYS_WHAT_IS, SMX_CB_NULL);
   if (h == NULL)
      type = SMX_CB_NULL;
   else if (smx_bcbs.pi <= (u8*)h && (u8*)h <= smx_bcbs.px)
      type = SMX_CB_BCB;
   else
      type = ((CB_PTR)h)->cbtype;
   if (type > SMX_CB_LSR)
      smx_ERROR_EXIT(SMXE_INV_PAR, SMX_CB_NULL, 0, SMX_ID_SYS_WHAT_IS);
   return((SMX_CBTYPE)smx_SSRExit((u32)type, SMX_ID_SYS_WHAT_IS));
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  smx_TickRecovery()
*
*  Called from smx_SysPowerDown() to process timed events that expired during
*  power off. These are processed in the order they would have occured if
*  power had remained on.
*/
static void smx_TickRecovery(void)
{
   TMRCB_PTR tmr;
   u32 etmr = smx_etime; /* next timer expiration etime */
   u32 etmo = smx_etime; /* next task timeout etime */
   smx_etime += ticks_lost;

   /* set etmr for first expired timer during power off or -1 if none. */
   if ((tmr = (TMRCB_PTR)smx_tq->fl) != NULL)
      if (tmr->diffcnt <= ticks_lost)
         etmr += tmr->diffcnt; /* etmr = first expiration time */
      else
      {
         tmr->diffcnt -= ticks_lost;
         etmr = -1; /* timer search is over */
      }
   else
      etmr = -1;  /* timer search is over */

   /*set etmo for first task timeout during power off or -1 if none */
   if (smx_tmo_min <= smx_etime)
      etmo = smx_tmo_min; /* update to first task timeout */
   else
      etmo = -1; /* task timeout search is over */

   /* process expirations that occurred during power off in order */
   while (etmr <= smx_etime || etmo <= smx_etime)
   {
      if (etmr <= etmo)
      {
         /* process expired timer in smx_tq */
         tmr->diffcnt = 0;
         smx_TimerTimeout();

         /* test smx_tq for more waiting timers */
         if ((tmr = (TMRCB_PTR)smx_tq->fl) != NULL)
            if (tmr->diffcnt <= (smx_etime - etmr))
               etmr += tmr->diffcnt; /* etmr = next expiration time */
            else
            {
               tmr->diffcnt -= (smx_etime - etmr);
               etmr = -1; /* timer search is over */
            }
         else
            etmr = -1;  /* timer search is over */
      }
      else
      {
         /* process task timeout */
         smx_TaskTimeout(etmo);

         /* etmo = next timeout (done if etmo > smx_etime */
         etmo = smx_tmo_min;
      }
   }
}

/* Notes:
   1. Pseudo Handles: ISRs don't have handles, so to log in the event buffer or
      add to the handle table, we have to assign them pseudo handles with
      smx_SysPseudoHandleCreate(). This is the range of handles to use (assumes
      there are no real handles this high). The range is large since these
      are also used for user events that can be logged in the event buffer.
*/