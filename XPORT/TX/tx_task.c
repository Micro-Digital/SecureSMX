/*
* tx_task.c                                                 Version 5.4.0
*
* TXPort Task Functions
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

UINT  tx_thread_create(TX_THREAD *thread_ptr, CHAR *name_ptr, VOID (*entry_function)(ULONG id), ULONG entry_input,
                            VOID *stack_start, ULONG stack_size, UINT priority, UINT preempt_threshold,
                            ULONG time_slice, UINT auto_start)
{
   TCB_PTR task = smx_TaskCreate((FUN_PTR)entry_function, priority, stack_size, 0, name_ptr, (u8*)stack_start);
   if (task)
   {
      *thread_ptr = task;
      if (auto_start)
         smx_TaskStart(task, entry_input);
      else
         task->rv = entry_input;
      return TX_SUCCESS;
   }
   else
      return TX_THREAD_ERROR;
}

UINT  tx_thread_delete(TX_THREAD *thread_ptr)
{
   bool     pass = false;
   TCB_PTR  task = *thread_ptr;

   if (task->state == SMX_TASK_WAIT && task->sp == NULL)   /*<1>*/
      pass = smx_TaskDelete(&task);

   if (pass)
   {
      *thread_ptr = NULL;
      return TX_SUCCESS;
   }
   else
      return TX_DELETE_ERROR;
}

TX_THREAD  *tx_thread_identify(VOID)
{
   return &smx_ct;
}

VOID  tx_thread_relinquish(VOID)
{
   smx_TaskBump(smx_ct, SMX_PRI_NOCHG);
}

UINT  tx_thread_reset(TX_THREAD *thread_ptr)
{
   TCB_PTR task = *thread_ptr;
   if (task->state == SMX_TASK_WAIT && task->sp == 0)
   {
      smx_TaskLock();
      smx_TaskStart(task, 0);
      smx_TaskSuspend(task, SMX_TMO_INF);
      smx_TaskUnlock();
      return TX_SUCCESS;
   }
   else
      return TX_NOT_DONE;
}

UINT  tx_thread_suspend(TX_THREAD *thread_ptr)
{
   if (smx_TaskSuspend(*thread_ptr, SMX_TMO_INF))
      return TX_SUCCESS;
   else
      return TX_SUSPEND_ERROR;
}

UINT  tx_thread_terminate(TX_THREAD *thread_ptr)
{
   if (smx_TaskStop(*thread_ptr, SMX_TMO_INF))
      return TX_SUCCESS;
   else
      return TX_THREAD_ERROR;
}

UINT  tx_thread_sleep(ULONG timer_ticks)
{
   if (smx_TaskSuspend(smx_ct, timer_ticks))
      return TX_SUCCESS;
   else
      return TX_RESUME_ERROR;
}

UINT  tx_thread_resume(TX_THREAD *thread_ptr)
{
   if (smx_TaskResume(*thread_ptr))
      return TX_SUCCESS;
   else
      return TX_RESUME_ERROR;
}

UINT  tx_thread_wait_abort(TX_THREAD  *thread_ptr)
{
   if (smx_TaskResume(*thread_ptr))
      return TX_SUCCESS;
   else
      return TX_RESUME_ERROR;
}

UINT  tx_thread_priority_change(TX_THREAD *thread_ptr, UINT new_priority, UINT *old_priority)
{
   TCB_PTR task = *thread_ptr;

   *old_priority = (UINT)task->pri;
   if (smx_TaskBump(task, new_priority))
      return TX_SUCCESS;
   else
      return TX_THREAD_ERROR;
}

UINT  tx_thread_preemption_change(TX_THREAD *thread_ptr, UINT new_threshold, UINT *old_threshold)
{
   return tx_thread_priority_change(thread_ptr, new_threshold, old_threshold);
}

UINT  tx_thread_info_get(TX_THREAD *thread_ptr, CHAR **name, UINT *state, ULONG *run_count, 
                UINT *priority, UINT *preemption_threshold, ULONG *time_slice, 
                TX_THREAD **next_thread, TX_THREAD **next_suspended_thread)
{
   TCB_PTR task = *thread_ptr;

   if (!smx_TCBTest(task, SMX_PRIV_LO))
      return TX_THREAD_ERROR;
   if (name != TX_NULL)
     *name = (CHAR*)task->name;
   if (state != TX_NULL)
   {
      switch(task->state)
      {
         case SMX_TASK_READY:
         case SMX_TASK_RUN:
            *state = TX_READY;
            break;
         case SMX_TASK_WAIT:
            if (task->sp == NULL)
               *state = TX_TERMINATED;
            else
               *state = TX_SUSPENDED;
            break;
         default:
            *state = 0;
      }
   }
   if (run_count != TX_NULL)
     *run_count = 0;
   if (priority != TX_NULL)
     *priority = task->pri;
   if (preemption_threshold != TX_NULL)
     *preemption_threshold = task->pri;
   if (time_slice != TX_NULL)
     *time_slice = 0;
   if (next_thread != TX_NULL)
     *next_thread = 0;
   if (next_suspended_thread != TX_NULL)
     *next_suspended_thread = 0;
   return TX_SUCCESS;
}

UINT  tx_thread_entry_exit_notify(TX_THREAD *thread_ptr, VOID (*thread_entry_exit_notify)(TX_THREAD *notify_thread_ptr, UINT id))
{
   if (smx_TaskSet(*thread_ptr, SMX_ST_CBFUN, (u32)thread_entry_exit_notify, 1))
      return TX_SUCCESS;
   else
      return TX_THREAD_ERROR;
}

UINT  tx_thread_performance_info_get(TX_THREAD *thread_ptr, ULONG *resumptions, ULONG *suspensions, 
                ULONG *solicited_preemptions, ULONG *interrupt_preemptions, ULONG *priority_inversions,
                ULONG *time_slices, ULONG *relinquishes, ULONG *timeouts, ULONG *wait_aborts, TX_THREAD **last_preempted_by)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_thread_performance_system_info_get(ULONG *resumptions, ULONG *suspensions,
                ULONG *solicited_preemptions, ULONG *interrupt_preemptions, ULONG *priority_inversions,
                ULONG *time_slices, ULONG *relinquishes, ULONG *timeouts, ULONG *wait_aborts,
                ULONG *non_idle_returns, ULONG *idle_returns)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_thread_stack_error_notify(VOID (*stack_error_handler)(TX_THREAD *thread_ptr))
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_thread_time_slice_change(TX_THREAD *thread_ptr, ULONG new_time_slice, ULONG *old_time_slice)
{
   return TX_FEATURE_NOT_ENABLED;
}

/* Notes:
   1. Thus a task cannot delete itself.
*/