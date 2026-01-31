/*
* tx_ef.c                                                   Version 6.0.0
*
* TXPort Event Flag Functions
*
* Copyright (c) 2024-2026 Micro Digital Inc.
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
* For ThreadX/Azure Kernel V6.1
*
*****************************************************************************/

#include "xsmx.h"
#include "tx_api.h"

UINT  tx_event_flags_create(TX_EVENT_FLAGS_GROUP *group_ptr, CHAR *name_ptr)
{
   if (*group_ptr = smx_EventGroupCreate(0, name_ptr))
      return TX_SUCCESS;
   else
      return TX_GROUP_ERROR;
}

UINT  tx_event_flags_delete(TX_EVENT_FLAGS_GROUP *group_ptr)
{
   if (smx_EventGroupDelete(group_ptr))
      return TX_SUCCESS;
   else
      return TX_GROUP_ERROR;
}

UINT  tx_event_flags_get(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG requested_flags,
                    UINT get_option, ULONG *actual_flags_ptr, ULONG wait_option)
{
   EGCB_PTR eg = *group_ptr;
   u32 mode;
   u32 post_clear_mask;
   u32 test_mask = requested_flags;

   if (get_option == TX_AND || wait_option == TX_AND_CLEAR)
      mode = SMX_EF_AND;
   else
      mode = SMX_EF_OR;

   if (get_option == TX_AND_CLEAR || get_option == TX_OR_CLEAR)
      post_clear_mask = test_mask;
   else
      post_clear_mask = 0;

   if (actual_flags_ptr != NULL)
      *actual_flags_ptr = eg->flags; /* save eg flags in case ct is not suspended */

   smx_ct->afp = (u32*)actual_flags_ptr;  /* save afp in case ct is suspended */

   if (smx_EventFlagsTest(eg, test_mask, mode, post_clear_mask, wait_option))
      return TX_SUCCESS;
   else
      return TX_NO_EVENTS;
}


UINT  tx_event_flags_set(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG flags_to_set, UINT set_option)
{
   u32 set_mask, pre_clear_mask;
   if (set_option == TX_AND)
   {
      set_mask = 0;
      pre_clear_mask = ~flags_to_set;
      smx_ct->flags.ef_and = 1;
   }
   else /* TX_OR */
   {
      set_mask = flags_to_set;
      pre_clear_mask = 0;
      smx_ct->flags.ef_and = 0;
   } 
   if (smx_EventFlagsSet(*group_ptr, set_mask, pre_clear_mask))
      return TX_SUCCESS;
   else
      return TX_GROUP_ERROR;
}

UINT  tx_event_flags_info_get(TX_EVENT_FLAGS_GROUP *group_ptr, CHAR **name, ULONG *current_flags, 
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_EVENT_FLAGS_GROUP **next_group)
{
   u32 cnt;
   EGCB_PTR eg = *group_ptr;
   TCB_PTR task;

   if (!smx_EGCBTest(eg, SMX_PRIV_LO))
      return TX_GROUP_ERROR;
   if (name != TX_NULL)
     *name = (CHAR*)eg->name;
   if (current_flags != TX_NULL)
     *current_flags = eg->flags;
   if (first_suspended != TX_NULL)
   {
      if (eg->fl != NULL)
         *first_suspended = (TCB**)&*eg->fl;
      else
         *first_suspended = NULL;
   }
   if (suspended_count != TX_NULL)
   {
      if (eg->fl != NULL)
      {
         for (task = eg->fl, cnt = 1; task->fl != (CB_PTR)eg; task = (TCB_PTR)task->fl, cnt++) {}
         *suspended_count = cnt;
      }
      else
         *suspended_count = 0;
   }
   if (next_group != TX_NULL)
     *next_group = NULL;
   return TX_SUCCESS;
}

UINT  tx_event_flags_performance_info_get(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG *sets, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_event_flags_performance_system_info_get(ULONG *sets, ULONG *gets, ULONG *suspensions, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_event_flags_set_notify(TX_EVENT_FLAGS_GROUP *group_ptr, VOID (*events_set_notify)(TX_EVENT_FLAGS_GROUP *notify_group_ptr))
{
   if (smx_EventGroupSet(*group_ptr, SMX_ST_CBFUN, (u32)events_set_notify))
      return TX_SUCCESS;
   else
      return TX_GROUP_ERROR;
}