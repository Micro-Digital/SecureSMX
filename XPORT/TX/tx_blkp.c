/*
* tx_blkp.c                                                 Version 6.0.0
*
* TXPort Block Pool Functions
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

UINT  tx_block_pool_create(TX_BLOCK_POOL *pool_ptr, CHAR *name_ptr, ULONG block_size,
                    VOID *pool_start, ULONG pool_size)
{
   u32      blk_sz = block_size + 4;
   u32      blk_num = pool_size/blk_sz;
   PCB_PTR  pool = pool_ptr;
   SCB_PTR  sem;

   if (sem = smx_SemCreate(SMX_SEM_RSRC, blk_num, "pool_sem"))
      if (sb_BlockPoolCreate((u8*)pool_start, pool_ptr, blk_num, blk_sz, name_ptr))
      {
         pool->sem = sem;
         return TX_SUCCESS;
      }
      else
      {
         smx_SemDelete(&sem);
         return TX_POOL_ERROR;
      }
   else
      return TX_POOL_ERROR;
}

UINT  tx_block_pool_delete(TX_BLOCK_POOL *pool_ptr)
{
   PCB_PTR pool = pool_ptr;
   smx_SemDelete((SCB_PTR*)&pool->sem);
   if (sb_BlockPoolDelete(pool))
      return TX_SUCCESS;
   else
      return TX_POOL_ERROR;
}

UINT  tx_block_allocate(TX_BLOCK_POOL *pool_ptr, VOID **block_ptr, ULONG wait_option)
{
   u32*     bp;
   PCB_PTR  pool = pool_ptr;
   SCB_PTR  sem = (SCB_PTR)pool->sem;

   if (smx_SemTest(sem, wait_option) && (bp = (u32*)sb_BlockGet(pool, 0)))
   {
      *bp++ = (u32)pool;
      *block_ptr = bp;
      return TX_SUCCESS;
   }
   else
      return TX_NO_MEMORY;
}

UINT  tx_block_release(VOID *block_ptr)
{
   u32* bp = (u32*)block_ptr;
   PCB_PTR pool = (PCB_PTR)*(bp - 1);
   SCB_PTR sem  = (SCB_PTR)pool->sem;

   smx_LSRsOff();
   if (sem->fl) /* task waiting */
   {
      smx_SemSignal(sem);
   }
   sb_BlockRel(pool, (u8*)bp, 0);
   smx_LSRsOn();
   return TX_SUCCESS;
}

UINT  tx_block_pool_info_get(TX_BLOCK_POOL *pool_ptr, CHAR **name, ULONG *available_blocks, 
                    ULONG *total_blocks, TX_THREAD **first_suspended, 
                    ULONG *suspended_count, TX_BLOCK_POOL **next_pool)
{
   u32 cnt;
   PCB_PTR pool = pool_ptr;
   SCB_PTR sem = (SCB_PTR)pool->sem;
   TCB_PTR task;

   if (name != TX_NULL)
     *name = (CHAR*)pool->name;
   if (available_blocks != TX_NULL)
     *available_blocks = pool->num - pool->num_used;
   if (total_blocks != TX_NULL)
     *total_blocks = pool->num;
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

UINT  tx_block_pool_performance_info_get(TX_BLOCK_POOL *pool_ptr, ULONG *allocates, ULONG *releases,
                    ULONG *suspensions, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_block_pool_performance_system_info_get(ULONG *allocates, ULONG *releases, ULONG *suspensions, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_block_pool_prioritize(TX_BLOCK_POOL *pool_ptr)
{
   if (smx_PCBTest(pool_ptr, SMX_PRIV_HI))
      return TX_SUCCESS;
   else
      return TX_POOL_ERROR;
}