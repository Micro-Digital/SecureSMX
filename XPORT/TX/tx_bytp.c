/*
* tx_bytp.c                                                 Version 6.0.0
*
* TXPort Byte Pool Functions
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
  
extern u32*     hbin[];
extern SCB_PTR  hsem[];

#define BK_LNK(cp)   (CCB_PTR)((u32)cp->blf & (u32)~EH_FLAGS)

UINT  tx_byte_pool_create(TX_BYTE_POOL *pool_ptr, CHAR *name_ptr, VOID *pool_start, ULONG pool_size)
{
   u32 hn;
   for (hn = 0; pool_ptr != eh_hvp[hn]; hn++)
   {
      if (hn >= EH_NUM_HEAPS)
         return TX_POOL_ERROR;
   }

   if (hsem[hn] = smx_SemCreate(SMX_SEM_EVENT, 1, "heap_sem"))
   {
      eh_hvpn = hn;
      if (smx_HeapInit(pool_size, 0, (u8*)pool_start, pool_ptr, hbin[2*hn], 
                          (HBCB*)hbin[2*hn+1], (EH_EDA | EH_EM | EH_PRE) != -1))
      {
         eh_hvp[hn]->name = (const char*)name_ptr;
         eh_hvp[hn]->mode.fl.cmerge = ON;
         return TX_SUCCESS;
      }
      else
      {
         smx_SemDelete(&hsem[hn]);
         return TX_POOL_ERROR;
      }
   }
   else
      return TX_POOL_ERROR;
}

UINT  tx_byte_pool_delete(TX_BYTE_POOL *pool_ptr)
{
   u32 hn;
   for (hn = 0; pool_ptr != eh_hvp[hn]; hn++)
   {
      if (hn >= EH_NUM_HEAPS)
         return TX_POOL_ERROR;
   }

   if (smx_SemDelete(&hsem[hn]))
   {
      eh_hvp[hn]->mode.fl.init = 0;
      return TX_SUCCESS;
   }
   else
      return TX_POOL_ERROR;
}

UINT  tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size, ULONG wait_option)
{
   u32*  bp;
   u32   hn;
   for (hn = 0; pool_ptr != eh_hvp[hn]; hn++)
   {
      if (hn >= EH_NUM_HEAPS)
         return TX_POOL_ERROR;
   }
   SCB_PTR sem = hsem[hn];

   /* suppress heap error reporting for first malloc unless TX_NO_WAIT */
   if (wait_option != TX_NO_WAIT)
      eh_hvp[hn]->mode.fl.em_en = 0;

   /* get block if enough heap space is available */
   if (bp = (u32*)smx_HeapMalloc(memory_size+4, 3, hn))
   {
      *bp++ = hn;          /* store heap number in first word */
      *memory_ptr = bp;
      eh_hvp[hn]->mode.fl.em_en = 1;   /* restore heap error reporting */
      return TX_SUCCESS;
   }
   else /* wait for available heap space */
   {
      eh_hvp[hn]->mode.fl.em_en = 1;   /* restore heap error reporting */
      do 
      {
         if (smx_SemTest(sem, wait_option)) /* requires smx modification for finite timeout */
         {
            if (bp = (u32*)smx_HeapMalloc(memory_size+4, 3, hn)) /*<1>*/
            {
               *bp++ = hn;
               *memory_ptr = bp;
               return TX_SUCCESS;
            }
         }
         else
            break;
      } while (wait_option);
   }
   return TX_NO_MEMORY;
}

UINT  tx_byte_release(VOID *memory_ptr)
{
   u32*    bp = (u32*)memory_ptr - 1;
   u32     hn = *bp;
   SCB_PTR sem;

   if (hn >= eh_hvpn)
      return TX_PTR_ERROR; 
   else
      sem = hsem[hn];

   smx_LSRsOff();
   if (sem->fl) /* task waiting */
      smx_SemSignal(sem);
   if (!smx_HeapFree(bp, hn))
      return TX_PTR_ERROR;
   smx_LSRsOn();
   return TX_SUCCESS;
}

UINT  tx_byte_pool_info_get(TX_BYTE_POOL *pool_ptr, CHAR **name, ULONG *available_bytes, 
                    ULONG *fragments, TX_THREAD **first_suspended, 
                    ULONG *suspended_count, TX_BYTE_POOL **next_pool)
{
   CICB_PTR cp, cpp;
   u32      cnt  = 0;
   u32      free = 0;
   u32      hn;
   for (hn = 0; pool_ptr != eh_hvp[hn]; hn++)
   {
      if (hn >= EH_NUM_HEAPS)
         return TX_POOL_ERROR;
   }
   SCB_PTR  sem = hsem[hn];
   TCB_PTR  task;

   if (name != TX_NULL)
     *name = (CHAR*)pool_ptr->name;
   if ((available_bytes != TX_NULL) || (fragments != TX_NULL))
   {
      cp = pool_ptr->pi;
      do
      {
         cpp = cp;
         cp = (CICB_PTR)cp->fl;
         if (BK_LNK(cp != (u32)cpp))
            return TX_POOL_ERROR;   /* quit if broken link */
         cnt++;
         if (((u32)(cp->blf) & 3) == EH_FREE)
            free += ((u32)cp->fl - (u32)cp - sizeof(CCB));
      } while (cp != pool_ptr->px);
   }
   if (available_bytes != TX_NULL)
     *available_bytes = free;
   if (fragments != TX_NULL)
     *fragments = cnt - 2;    /* dc and end chunks do not count (and start chunk wasn't counted) */
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
         for (task = sem->fl, cnt = 1; task->fl != (CB_PTR)sem; task = (TCB_PTR)task->fl, cnt++) {}
         *suspended_count = cnt;
      }
      else
         *suspended_count = 0;
   }
   if (next_pool != TX_NULL)
     *next_pool = NULL;
   return TX_SUCCESS;
}

UINT  tx_byte_pool_performance_info_get(TX_BYTE_POOL *pool_ptr, ULONG *allocates, ULONG *releases,
                    ULONG *fragments_searched, ULONG *merges, ULONG *splits, ULONG *suspensions, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_byte_pool_performance_system_info_get(ULONG *allocates, ULONG *releases,
                    ULONG *fragments_searched, ULONG *merges, ULONG *splits, ULONG *suspensions, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_byte_pool_prioritize(TX_BYTE_POOL *pool_ptr)
{
   if (pool_ptr == NULL)
      return TX_POOL_ERROR;
   else
      return TX_SUCCESS;
}

/* Notes:
   1. eh_hvp[hn]->mode.fl.cmerge should be ON. 
*/