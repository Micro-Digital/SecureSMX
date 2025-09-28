/*
* queue.c                                                   Version 5.4.0
*
* FRPort Queue Functions
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
* For FreeRTOS Kernel V10.4.3
*
*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Mutex Functions */

QueueHandle_t xQueueCreateMutex( const uint8_t ucQueueType )
{
    return (QueueHandle_t)smx_MutexCreate(1, 0);
}

QueueHandle_t xQueueCreateMutexStatic( const uint8_t ucQueueType,
                                       StaticQueue_t * pxStaticQueue )
{
    return (QueueHandle_t)smx_MutexCreate(1, 0);
}

TaskHandle_t xQueueGetMutexHolder( QueueHandle_t xSemaphore )
{
    return (TaskHandle_t)(((MUCB_PTR)(xSemaphore))->onr);
}

TaskHandle_t xQueueGetMutexHolderFromISR( QueueHandle_t xSemaphore )
{
    return (TaskHandle_t)(((MUCB_PTR)(xSemaphore))->onr);
}

BaseType_t xQueueGiveMutexRecursive( QueueHandle_t xMutex )
{
    return (BaseType_t)smx_MutexRel((MUCB_PTR)xMutex);
}

BaseType_t xQueueTakeMutexRecursive( QueueHandle_t xMutex,
                                     TickType_t xTicksToWait )
{
    return (BaseType_t)smx_MutexGet((MUCB_PTR)xMutex, (u32)xTicksToWait);
}


/* Queue Functions */

void vQueueAddToRegistry( QueueHandle_t xQueue,
                          const char * pcQueueName )
{
    smx_HT_ADD((void*)xQueue, pcQueueName);
}


BaseType_t xQueueAddToSet( QueueSetMemberHandle_t xQueueOrSemaphore,
                           QueueSetHandle_t xQueueSet )
{
    return pdFAIL;
}

QueueSetHandle_t xQueueCreateSet( const UBaseType_t uxEventQueueLength )
{
    return NULL;
}

BaseType_t xQueueCRReceive( QueueHandle_t xQueue,
                            void * pvBuffer,
                            TickType_t xTicksToWait )
{
    return pdFAIL;
}

BaseType_t xQueueCRReceiveFromISR( QueueHandle_t xQueue,
                                   void * pvBuffer,
                                   BaseType_t * pxCoRoutineWoken )
{
    return pdFAIL;
}

BaseType_t xQueueCRSend( QueueHandle_t xQueue,
                         const void * pvItemToQueue,
                         TickType_t xTicksToWait )
{
    return pdFAIL;
}

BaseType_t xQueueCRSendFromISR( QueueHandle_t xQueue,
                                const void * pvItemToQueue,
                                BaseType_t xCoRoutinePreviouslyWoken )
{
    return pdFAIL;
}

void vQueueDelete( QueueHandle_t xQueue )
{
    smx_LSRsOff();
    switch (((CB_PTR)xQueue)->cbtype)
    {
        case SMX_CB_SEM:
            smx_SemDelete((SCB_PTR*)&xQueue);
            break;
        case SMX_CB_MTX:
            smx_MutexDelete((MUCB_PTR*)&xQueue);
            break;
        case SMX_CB_PIPE:
            smx_PipeDelete((PICB_PTR*)&xQueue);
    }
    smx_LSRsOn();
}

QueueHandle_t xQueueGenericCreate( const UBaseType_t uxQueueLength,
                                   const UBaseType_t uxItemSize,
                                   const uint8_t ucQueueType )
{
    if (ucQueueType == queueQUEUE_TYPE_BINARY_SEMAPHORE)
         return (QueueHandle_t)smx_SemCreate(SMX_SEM_RSRC, 1); 
    else
    {
        PICB_PTR pipe;
        void* ppb;
        smx_LSRsOff();
        u16 len = (u16)uxQueueLength;
        ppb = smx_HeapMalloc((u32)(uxItemSize*len));
        pipe = smx_PipeCreate(ppb, (u8)uxItemSize, len);
        smx_LSRsOn();
        return (QueueHandle_t)pipe;
    }
}

QueueHandle_t xQueueGenericCreateStatic( const UBaseType_t uxQueueLength,
                                         const UBaseType_t uxItemSize,
                                         uint8_t * pucQueueStorage,
                                         StaticQueue_t * pxStaticQueue,
                                         const uint8_t ucQueueType )
{
    if (ucQueueType == queueQUEUE_TYPE_BINARY_SEMAPHORE)
        return(xQueueCreateCountingSemaphoreStatic(1, 1, pxStaticQueue));
    else
    {
        PICB_PTR pipe;
        u16 len = (u16)uxQueueLength;
        pipe = smx_PipeCreate((void*)pucQueueStorage, (u8)uxItemSize, len);
        return (QueueHandle_t)pipe;
    }
}

BaseType_t xQueueGenericReset( QueueHandle_t xQueue,
                               BaseType_t xNewQueue )
{
    smx_PipeClear((PICB_PTR)xQueue);
    return pdPASS;
}

BaseType_t xQueueGenericSend( QueueHandle_t xQueue,
                              const void * const pvItemToQueue,
                              TickType_t xTicksToWait,
                              const BaseType_t xCopyPosition)
{
    switch (((CB_PTR)xQueue)->cbtype)
    {
        case SMX_CB_SEM:
            return (BaseType_t)smx_SemSignal((SCB_PTR)xQueue);
            break;
        case SMX_CB_MTX:
            return (BaseType_t)smx_MutexRel((MUCB_PTR)xQueue);
            break;
        case SMX_CB_PIPE:
            if (xCopyPosition == queueSEND_TO_FRONT || xCopyPosition == queueOVERWRITE)
                return (BaseType_t)smx_PipePutPktWait((PICB_PTR)xQueue, 
                        (void*)pvItemToQueue, (u32)xTicksToWait, SMX_PUT_TO_FRONT);
            else
                return (BaseType_t)smx_PipePutPktWait((PICB_PTR)xQueue, 
                        (void*)pvItemToQueue, (u32)xTicksToWait, SMX_PUT_TO_BACK);
            break;
        default:
            return pdFALSE;
    }
}

BaseType_t xQueueGenericSendFromISR( QueueHandle_t xQueue,
                                     const void * const pvItemToQueue,
                                     BaseType_t * const pxHigherPriorityTaskWoken,
                                     const BaseType_t xCopyPosition )
{
    if (xCopyPosition == queueSEND_TO_FRONT || xCopyPosition == queueOVERWRITE)
        return (BaseType_t)smx_PipePutPktWait((PICB_PTR)xQueue, 
                           (void*)pvItemToQueue, SMX_TMO_NOWAIT, SMX_PUT_TO_FRONT);
    else
        return (BaseType_t)smx_PipePutPktWait((PICB_PTR)xQueue, 
                           (void*)pvItemToQueue, SMX_TMO_NOWAIT, SMX_PUT_TO_BACK);
}

const char * pcQueueGetName( QueueHandle_t xQueue )
{
   #if defined(SMX_DEBUG)
    return smx_HTGetName((void*)xQueue);
   #else
    return NULL;
   #endif
}

UBaseType_t uxQueueGetQueueNumber( QueueHandle_t xQueue )
{
    return 0;
}

uint8_t ucQueueGetQueueType( QueueHandle_t xQueue )
{
    return 0;
}

BaseType_t xQueueGiveFromISR( QueueHandle_t xQueue,
                              BaseType_t * const pxHigherPriorityTaskWoken )
{
    return((BaseType_t)smx_SemSignal((SCB_PTR)xQueue));
}

BaseType_t xQueueIsQueueEmptyFromISR( const QueueHandle_t xQueue )
{
    return (BaseType_t)(smx_PIPE_EMPTY((PICB_PTR)xQueue, ((PICB_PTR)xQueue)->rp));
}

BaseType_t xQueueIsQueueFullFromISR( const QueueHandle_t xQueue )
{
    return (BaseType_t)(smx_PIPE_FULL((PICB_PTR)xQueue, ((PICB_PTR)xQueue)->wp));
}

UBaseType_t uxQueueMessagesWaiting( const QueueHandle_t xQueue )
{
    switch (((CB_PTR)xQueue)->cbtype)
    {
        case SMX_CB_SEM:
            return (UBaseType_t)((SCB_PTR)xQueue)->count;
            break;
        case SMX_CB_PIPE:
            return (UBaseType_t)smx_PipePeek((PICB_PTR)xQueue, SMX_PK_NUMPKTS);
            break;
        default:
            return 0;
    }
}

UBaseType_t uxQueueMessagesWaitingFromISR( const QueueHandle_t xQueue )
{
    return (UBaseType_t)smx_PipePeek((PICB_PTR)xQueue, SMX_PK_NUMPKTS);
}

BaseType_t xQueuePeek( QueueHandle_t xQueue,
                       void * const pvBuffer,
                       TickType_t xTicksToWait )
{
    return errQUEUE_EMPTY; 
}

BaseType_t xQueuePeekFromISR( QueueHandle_t xQueue,
                              void * const pvBuffer )
{
    return errQUEUE_EMPTY;
}

BaseType_t xQueueReceive( QueueHandle_t xQueue,
                          void * const pvBuffer,
                          TickType_t xTicksToWait )
{
    if (smx_PipeGetPktWait((PICB_PTR)xQueue, (void*)pvBuffer, (u32)xTicksToWait))
        return pdPASS;
    else
        return errQUEUE_EMPTY;
}

BaseType_t xQueueReceiveFromISR( QueueHandle_t xQueue,
                                 void * const pvBuffer,
                                 BaseType_t * const pxHigherPriorityTaskWoken )
{
    switch (((CB_PTR)xQueue)->cbtype)
    {
        case SMX_CB_SEM:
            return (BaseType_t)smx_SemTest((SCB_PTR)xQueue, SMX_TMO_NOWAIT);
            break;
        case SMX_CB_PIPE:
            return (BaseType_t)smx_PipeGetPktWait((PICB_PTR)xQueue, (void*)pvBuffer, 
                                                                     SMX_TMO_NOWAIT);
            break;
        default:
            return pdFALSE;
    }
}

BaseType_t xQueueRemoveFromSet( QueueSetMemberHandle_t xQueueOrSemaphore,
                                QueueSetHandle_t xQueueSet )
{
    return pdFAIL;
}

QueueSetMemberHandle_t xQueueSelectFromSet( QueueSetHandle_t xQueueSet,
                                            TickType_t const xTicksToWait )
{
    return NULL;
}

QueueSetMemberHandle_t xQueueSelectFromSetFromISR( QueueSetHandle_t xQueueSet )
{
    return NULL;
}

void vQueueSetQueueNumber( QueueHandle_t xQueue,
                           UBaseType_t uxQueueNumber )
{
}

UBaseType_t uxQueueSpacesAvailable( const QueueHandle_t xQueue )
{
    PICB_PTR pipe = (PICB_PTR)xQueue;
    return (pipe->bx - pipe->bi + pipe->width)/pipe->width - smx_PipePeek(pipe, SMX_PK_NUMPKTS);
}

void vQueueUnregisterQueue( QueueHandle_t xQueue )
{
    smx_HT_DELETE((void*)xQueue);
}

void vQueueWaitForMessageRestricted( QueueHandle_t xQueue,
                                     TickType_t xTicksToWait,
                                     const BaseType_t xWaitIndefinitely )
{
}


/* Semaphore Functions */

QueueHandle_t xQueueCreateCountingSemaphore( const UBaseType_t uxMaxCount,
                                             const UBaseType_t uxInitialCount )
{
    SCB_PTR sem;
    if (uxInitialCount == 0)
    {
        u8 lim = (uxMaxCount == 1 ? 1 : 0);
        sem = smx_SemCreate(SMX_SEM_EVENT, lim);
    }
    else
        sem = smx_SemCreate(SMX_SEM_RSRC, (u8)uxMaxCount);
    return (QueueHandle_t)sem;
}

QueueHandle_t xQueueCreateCountingSemaphoreStatic( const UBaseType_t uxMaxCount,
                                                   const UBaseType_t uxInitialCount,
                                                   StaticQueue_t * pxStaticQueue )
{
    SCB_PTR sem;
    if (uxInitialCount == 0)
    {
        u8 lim = (uxMaxCount == 1 ? 1 : 0);
        sem = smx_SemCreate(SMX_SEM_EVENT, lim);
    }
    else
        sem = smx_SemCreate(SMX_SEM_RSRC, (u8)uxMaxCount);
    return (QueueHandle_t)sem;
}

BaseType_t xQueueSemaphoreTake( QueueHandle_t xQueue,
                                TickType_t xTicksToWait )
{
    if (((CB_PTR)xQueue)->cbtype == SMX_CB_MTX)
        return (BaseType_t)smx_MutexGet((MUCB_PTR)xQueue, xTicksToWait);
    else
        return (BaseType_t)smx_SemTest((SCB_PTR)xQueue, xTicksToWait);
}

