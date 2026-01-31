/*
* cm2port.c                                                 Version 6.0.0
*
* CMSIS-RTOS2 to SMX Porting Functions
*
* Copyright (c) 2025-2026 Micro Digital Inc.
* All rights reserved. www.smxrtos.com
*
* This software is confidential and proprietary to Micro Digital Inc.
* It has been furnished under a license and may be used, copied, or
* disclosed only in accordance with the terms of that license and with
* the inclusion of this header. No title to nor ownership of this
* software is hereby transferred.
*
* Author: Ralph Moore
*
*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"
#include "smx.h"
#include "xsmx.h"
#include "xapiu.h"

#if SMX_CFG_SSMX
#pragma diag_suppress=Ta168
#pragma section_prefix = ".fs"
#endif

osKernelState_t osKernelGetState(void)
{
   u32 state = smx_TaskPeek(SMX_CT, SMX_PK_STATE);
   if (smx_TaskPeek(SMX_CT, SMX_PK_ERROR))
      return osKernelError;
   else
      switch (state)
      {
         case SMX_TASK_NULL:
            return osKernelInactive;
         case SMX_TASK_READY:
            return osKernelReady;
         case SMX_TASK_RUN:
            if(smx_TaskPeek(SMX_CT, SMX_PK_LOCK))
               return osKernelLocked;
            else
               return osKernelRunning;
         case SMX_TASK_WAIT:
            return osKernelSuspended;
         default:
            return osKernelError;
      }
}

uint32_t osKernelGetTickCount(void )
{
   return((uint32_t)smx_SysPeek(SMX_PK_ETIME));
}

osStatus_t osMessageQueueGet(osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)
{
   u32         err;
   osStatus_t  ret;
   bool        status;

   status = smx_PipeGetPktWait((PICB_PTR)mq_id, msg_ptr, timeout);

   if (status == true)
      ret = osOK;
   else
   {
      err = smx_TaskPeek(SMX_CT, SMX_PK_ERROR);
      if (err == SMXE_TMO)
         ret = osErrorTimeout;
      else
      {
         if (mq_id == NULL || err == SMXE_WAIT_NOT_ALLOWED)
            ret = osErrorParameter;
         else
            ret = osErrorResource;
      }
   }
   return ret;
}

osMessageQueueId_t osMessageQueueNew(uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)
{
   PICB_PTR xpipe;
   void* ppb = smx_HeapMalloc(msg_size*msg_count, NULL, NULL); 
   xpipe = smx_PipeCreate(ppb, msg_size, msg_count, NULL, NULL);
   return((osMessageQueueId_t)xpipe);
}

osStatus_t osMessageQueuePut(osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)
{
   u32         err;
   osStatus_t  ret;
   bool        status;

   status = smx_PipePutPktWait((PICB_PTR)mq_id, (void*)msg_ptr, timeout, SMX_PUT_TO_FRONT);

   if (status == true)
      ret = osOK;
   else
   {
      err = smx_TaskPeek(SMX_CT, SMX_PK_ERROR);
      if (err == SMXE_TMO)
         ret = osErrorTimeout;
      else
      {
         if (mq_id == NULL || err == SMXE_WAIT_NOT_ALLOWED)
            ret = osErrorParameter;
         else
            ret = osError;
      }
   }
   return ret;
}

osStatus_t osMutexAcquire (osMutexId_t mutex_id, uint32_t timeout)
{
   u32         err;
   osStatus_t  ret;
   bool        status;

   status = smx_MutexGet((MUCB_PTR)mutex_id, timeout);

   if (status == true)
      ret = osOK;
   else
   {
      err = smx_TaskPeek(SMX_CT, SMX_PK_ERROR);
      if (err == SMXE_TMO)
         ret = osErrorTimeout;
      else
          ret = osError;
   }
   return ret;
}

osStatus_t osMutexDelete(osMutexId_t mutex_id)
{
   MUCB_PTR mtx = (MUCB_PTR)mutex_id;
   if (smx_MutexDelete(&mtx))
      return osOK;
   else
      return osError;
}

osMutexId_t osMutexNew(const osMutexAttr_t *attr)
{
   MUCB_PTR mtx;
   mtx = smx_MutexCreate(SMX_PI, 0, NULL, NULL);
   return((osMutexId_t)mtx);
}

osStatus_t osMutexRelease(osMutexId_t mutex_id)
{
   if(smx_MutexFree((MUCB_PTR)mutex_id))
      return osOK;
   else
      return osError;
}

osSemaphoreId_t osSemaphoreNew(uint32_t max_count, uint32_t initial_count, const osSemaphoreAttr_t *attr)
{
   SCB_PTR xsem;
   xsem = smx_SemCreate(SMX_SEM_RSRC, max_count, NULL, NULL);
   xsem->count = initial_count;
   return((osSemaphoreId_t)xsem);
}