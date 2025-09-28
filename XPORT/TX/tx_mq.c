/*
* tx_mq.c                                                   Version 5.4.0
*
* TXPort Message Queue Functions
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
*******************************************************************************/

#include "xsmx.h"
#include "tx_api.h"

UINT  tx_queue_create(TX_QUEUE *queue_ptr, CHAR *name_ptr, UINT message_size, 
                        VOID *queue_start, ULONG queue_size)
{
   u8    width  = message_size*4;
   u32   length = queue_size/width;

   if (*queue_ptr = smx_PipeCreate(queue_start, width, length, (CHAR*)name_ptr))
      return TX_SUCCESS;
   else
      return TX_QUEUE_ERROR;
}

UINT  tx_queue_delete(TX_QUEUE *queue_ptr)
{
   if (smx_PipeDelete(queue_ptr))
      return TX_SUCCESS;
   else
      return TX_QUEUE_ERROR;
}

UINT  tx_queue_flush(TX_QUEUE *queue_ptr)
{
   PICB_PTR pipe = *queue_ptr;
   TCB_PTR task;

   if (smx_PICBTest(pipe, SMX_PRIV_HI))
   {
      pipe->wp = pipe->rp = pipe->bi;
      pipe->flags.full = 0;
      for (task = (TCB_PTR)pipe->fl; task != NULL; task = (TCB_PTR)pipe->fl)
      {
         task->flags.pipe_put = 0;
         task->flags.pipe_front = 0;
         smx_TaskResume(task);
         task->rv = true;
      }
      return TX_SUCCESS;
   }
   else
      return TX_QUEUE_ERROR;
}

UINT  tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option)
{
   if (smx_PipeGetPktWait(*queue_ptr, destination_ptr, wait_option))
      return TX_SUCCESS;
   else
      return TX_QUEUE_EMPTY;
}

UINT  tx_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)
{
   if (smx_PipePutPktWait(*queue_ptr, source_ptr, wait_option))
      return TX_SUCCESS;
   else
      return TX_QUEUE_FULL;
}

UINT  tx_queue_front_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option)
{
   if (smx_PipePutPktWait(*queue_ptr, source_ptr, wait_option, SMX_PUT_TO_FRONT))
      return TX_SUCCESS;
   else
      return TX_QUEUE_FULL;
}

UINT  tx_queue_info_get(TX_QUEUE *queue_ptr, CHAR **name, ULONG *enqueued, ULONG *available_storage,
                    TX_THREAD **first_suspended, ULONG *suspended_count, TX_QUEUE **next_queue)
{
   u32      cnt;
   PICB_PTR pipe   = *queue_ptr;
   u32      width  = pipe->width;
   u32      length = (pipe->bx - pipe->bi + width);
   TCB_PTR  task;
   int      used;

   if (!smx_PICBTest(pipe, SMX_PRIV_LO))
      return TX_QUEUE_ERROR;
   if (name != TX_NULL)
     *name = (CHAR*)pipe->name;
   if (enqueued != TX_NULL)
   {
      used = pipe->wp - pipe->rp;
      if (used == 0 && pipe->flags.full)
         used = length;
      if (used < 0)
         used += length;
     *enqueued = used/width;
   }
   if (available_storage != TX_NULL)
     *available_storage = (length - used)/width;
   if (first_suspended != TX_NULL)
   {
      if (pipe->fl != NULL)
         *first_suspended = (TCB**)&*pipe->fl;
      else
         *first_suspended = NULL;
   }
   if (suspended_count != TX_NULL)
   {
      if (pipe->fl != NULL)
      {
         for (task = pipe->fl, cnt = 1; task->fl != (CB_PTR)pipe; task = (TCB_PTR)task->fl, cnt++);
         *suspended_count = cnt;
      }
      else
         *suspended_count = 0;
   }
   if (next_queue != TX_NULL)
     *next_queue = NULL;
   return TX_SUCCESS;
}

UINT  tx_queue_prioritize(TX_QUEUE *queue_ptr)
{
   if (smx_PICBTest(*queue_ptr, SMX_PRIV_HI))
      return TX_SUCCESS;
   else
      return TX_QUEUE_ERROR;
}

UINT  tx_queue_performance_info_get(TX_QUEUE *queue_ptr, ULONG *messages_sent, ULONG *messages_received,
                    ULONG *empty_suspensions, ULONG *full_suspensions, ULONG *full_errors, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_queue_performance_system_info_get(ULONG *messages_sent, ULONG *messages_received,
                    ULONG *empty_suspensions, ULONG *full_suspensions, ULONG *full_errors, ULONG *timeouts)
{
   return TX_FEATURE_NOT_ENABLED;
}

UINT  tx_queue_send_notify(TX_QUEUE *queue_ptr, VOID (*queue_send_notify)(TX_QUEUE *notify_queue_ptr))
{
   if (smx_PipeSet(*queue_ptr, SMX_ST_CBFUN, (u32)queue_send_notify))
      return TX_SUCCESS;
   else
      return TX_QUEUE_ERROR;
}