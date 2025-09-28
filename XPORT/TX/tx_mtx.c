/*
* tx_mtx.c                                                  Version 5.4.0
*
* TXPort Mutex Functions
*
* Copyright (c) 1994-2025 Micro Digital Inc.
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

UINT  tx_mutex_create(TX_MUTEX *mutex_ptr, CHAR *name_ptr, UINT inherit)
{
   u8 pi = (inherit == TX_INHERIT ? 1 : 0);
   if (*mutex_ptr = smx_MutexCreate(pi, 0, name_ptr))
      return TX_SUCCESS;
   else
      return TX_MUTEX_ERROR;
}

UINT  tx_mutex_delete(TX_MUTEX *mutex_ptr)
{
   if (smx_MutexDelete(mutex_ptr))
      return TX_SUCCESS;
   else
      return TX_MUTEX_ERROR;
}

UINT  tx_mutex_get(TX_MUTEX *mutex_ptr, ULONG wait_option)
{
   if (smx_MutexGet(*mutex_ptr, wait_option))
      return TX_SUCCESS;
   else
      return TX_NOT_AVAILABLE;
}

UINT  tx_mutex_put(TX_MUTEX *mutex_ptr)
{
   if (smx_MutexRel(*mutex_ptr))
      return TX_SUCCESS;
   else
   {
      if (smx_errno == SMXE_MTX_NON_ONR_REL)
         return TX_NOT_OWNED;
      else
         return TX_MUTEX_ERROR;
   }
}

UINT  tx_mutex_info_get(TX_MUTEX *mutex_ptr, CHAR **name, ULONG *count, TX_THREAD **owner,
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_MUTEX **next_mutex)
{
   u32 cnt;
   MUCB_PTR mtx = *mutex_ptr;
   TCB_PTR task;

   if (!smx_MUCBTest(mtx, SMX_PRIV_LO))
      return TX_MUTEX_ERROR;
   if (name != TX_NULL)
     *name = (CHAR*)mtx->name;
   if (count != TX_NULL)
     *count = mtx->ncnt;
   if (owner != TX_NULL)
     *owner = (TCB**)&*mtx->onr;
   if (first_suspended != TX_NULL)
   {
      if (mtx->fl != NULL)
         *first_suspended = (TCB**)&*mtx->fl;
      else
         *first_suspended = NULL;
   }
   if (suspended_count != TX_NULL)
   {
      if (mtx->fl != NULL)
      {
         for (task = mtx->fl, cnt = 1; task->fl != (CB_PTR)mtx; task = (TCB_PTR)task->fl, cnt++) {}
         *suspended_count = cnt;
      }
      else
         *suspended_count = 0;
   }
   if (next_mutex != TX_NULL)
     *next_mutex = NULL;
   return TX_SUCCESS;
}

UINT  tx_mutex_performance_info_get(TX_MUTEX *mutex_ptr, ULONG *puts, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts, ULONG *inversions, ULONG *inheritances)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_mutex_performance_system_info_get(ULONG *puts, ULONG *gets, ULONG *suspensions, 
                                ULONG *timeouts, ULONG *inversions, ULONG *inheritances)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_mutex_prioritize(TX_MUTEX *mutex_ptr)
{
   if (smx_MUCBTest(*mutex_ptr, SMX_PRIV_HI))
      return TX_SUCCESS;
   else
      return TX_MUTEX_ERROR;
}
