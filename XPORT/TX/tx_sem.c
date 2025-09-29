/*
* tx_sem.c                                                  Version 5.4.0
*
* TXPort Semaphore Functions
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
*****************************************************************************/

#include "xsmx.h"
#include "tx_api.h"

UINT  tx_semaphore_create(TX_SEMAPHORE *semaphore_ptr, CHAR *name_ptr, ULONG initial_count)
{
   if (*semaphore_ptr = smx_SemCreate(initial_count == 0 ? SMX_SEM_EVENT : SMX_SEM_RSRC, initial_count, name_ptr))
      return TX_SUCCESS;
   else
      return TX_SEMAPHORE_ERROR;
}

UINT  tx_semaphore_delete(TX_SEMAPHORE *semaphore_ptr)
{
   if (smx_SemDelete(semaphore_ptr))
      return TX_SUCCESS;
   else
      return TX_SEMAPHORE_ERROR;
}

UINT  tx_semaphore_ceiling_put(TX_SEMAPHORE *semaphore_ptr, ULONG ceiling)
{
   SCB_PTR sem = *semaphore_ptr;
   if (sem->count < ceiling)
   {
      if (smx_SemSignal(sem))
         return TX_SUCCESS;
      else
         return TX_SEMAPHORE_ERROR;
   }
   else
      return TX_CEILING_EXCEEDED;
}

UINT  tx_semaphore_get(TX_SEMAPHORE *semaphore_ptr, ULONG wait_option)
{
   if (smx_SemTest(*semaphore_ptr, wait_option))
      return TX_SUCCESS;
   else
      return TX_SEMAPHORE_ERROR;
}

UINT  tx_semaphore_put(TX_SEMAPHORE *semaphore_ptr)
{
   if (smx_SemSignal(*semaphore_ptr))
      return TX_SUCCESS;
   else
      return TX_SEMAPHORE_ERROR;
}

UINT  tx_semaphore_info_get(TX_SEMAPHORE *semaphore_ptr, CHAR **name, ULONG *current_value, 
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_SEMAPHORE **next_semaphore)
{
   u32 count;
   SCB_PTR sem = *semaphore_ptr;
   TCB_PTR task;

   if (!smx_SCBTest(sem, SMX_PRIV_LO))
      return TX_SEMAPHORE_ERROR;
   if (name != TX_NULL)
     *name = (CHAR*)sem->name;
   if (current_value != TX_NULL)
     *current_value = sem->count;
   if (first_suspended != TX_NULL)
   {
      if (sem->fl != NULL)
         *first_suspended = (TCB**)&*sem->fl;
      else
         *first_suspended = NULL;
   }
   if (suspended_count != TX_NULL)
   {
      if (sem->fl != NULL)
      {
         for (task = sem->fl, count = 1; task->fl != (CB_PTR)sem; task = (TCB_PTR)task->fl, count++) {}
         *suspended_count = count;
      }
      else
         *suspended_count = 0;
   }
   if (next_semaphore != TX_NULL)
     *next_semaphore = NULL;
   return TX_SUCCESS;
}

UINT  tx_semaphore_prioritize(TX_SEMAPHORE *semaphore_ptr)
{
   if (smx_SCBTest(*semaphore_ptr, SMX_PRIV_HI))
      return TX_SUCCESS;
   else
      return TX_SEMAPHORE_ERROR;
}

UINT  tx_semaphore_performance_info_get(TX_SEMAPHORE *semaphore_ptr, ULONG *puts, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_semaphore_performance_system_info_get(ULONG *puts, ULONG *gets, ULONG *suspensions, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_semaphore_put_notify(TX_SEMAPHORE *semaphore_ptr, VOID (*semaphore_put_notify)(TX_SEMAPHORE *notify_semaphore_ptr))
{
   if (smx_SemSet(*semaphore_ptr, SMX_ST_CBFUN, (u32)semaphore_put_notify))
      return TX_SUCCESS;
   else
      return TX_SEMAPHORE_ERROR;
}