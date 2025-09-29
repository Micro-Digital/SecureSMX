/*
* tx_tmr.c                                                  Version 5.4.0
*
* TXPort Timer Functions
*
* Copyright (c) 2024-2025 Micro Digital Inc.
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
* For ThreadX/Azure Kernel V6.1
*
*******************************************************************************/

#include "xsmx.h"
#include "tx_api.h"

LCB_PTR TimerLSRs[SMX_NUM_TIMERS];

UINT  tx_timer_create(TX_TIMER *timer_ptr, CHAR *name_ptr, 
            VOID (*expiration_function)(ULONG id), ULONG expiration_input,
            ULONG initial_ticks, ULONG reschedule_ticks, UINT auto_activate)
{
   int i;

   if (auto_activate == TX_AUTO_ACTIVATE)
   {
      *timer_ptr = NULL;
      for (i = 0; i < SMX_NUM_TIMERS && TimerLSRs[i] != 0 && TimerLSRs[i] != NULL; i++) {}
      if (i < SMX_NUM_TIMERS)
      {
         TimerLSRs[i] = smx_LSRCreate((FUN_PTR)expiration_function, SMX_FL_TRUST, "TimerLSR");
         /* create timer and set to invoke LSR */
         if (smx_TimerStart(timer_ptr, initial_ticks, reschedule_ticks, 
            TimerLSRs[i], name_ptr))
         {
            (*timer_ptr)->par = expiration_input;
            return TX_SUCCESS;
         }
         else
            return TX_TIMER_ERROR;
      }
      else
         return TX_TIMER_ERROR;
   }
   else
      return TX_ACTIVATE_ERROR;
}

UINT  tx_timer_delete(TX_TIMER *timer_ptr)
{
   if (smx_TimerStop(*timer_ptr))
      return TX_SUCCESS;
   else
      return TX_TIMER_ERROR;
}

UINT  tx_timer_change(TX_TIMER *timer_ptr, ULONG initial_ticks, ULONG reschedule_ticks)
{
   if (smx_TimerStart(timer_ptr, initial_ticks, reschedule_ticks, 
      (*timer_ptr)->lsr, (*timer_ptr)->name))
      return TX_SUCCESS;
   else
      return TX_TIMER_ERROR;
}

UINT  tx_timer_deactivate(TX_TIMER *timer_ptr)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_timer_activate(TX_TIMER *timer_ptr)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_timer_info_get(TX_TIMER *timer_ptr, CHAR **name, UINT *active, ULONG *remaining_ticks, 
                ULONG *reschedule_ticks, TX_TIMER **next_timer)
{
   TMRCB_PTR tmr = *timer_ptr;

   if (!smx_TMRCBTest(tmr, SMX_PRIV_LO) || tmr == NULL)
      return TX_TIMER_ERROR;
   if (name != TX_NULL)
     *name = (CHAR*)tmr->name;
   if (active != TX_NULL)
     *active = TX_TRUE;
   if (remaining_ticks != TX_NULL)
     *remaining_ticks = smx_TimerPeek(tmr, SMX_PK_TIME_LEFT);
   if (reschedule_ticks != TX_NULL)
      *reschedule_ticks = smx_TimerPeek(tmr, SMX_PK_PERIOD);
   if (next_timer != TX_NULL)
     *next_timer = NULL;
   return TX_SUCCESS;
}

UINT  tx_timer_performance_info_get(TX_TIMER *timer_ptr, ULONG *puts, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts, ULONG *inversions, ULONG *inheritances)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_timer_performance_system_info_get(ULONG *puts, ULONG *gets, ULONG *suspensions, 
                                ULONG *timeouts, ULONG *inversions, ULONG *inheritances)
{
   return TX_FEATURE_NOT_ENABLED;
}

