/*
* tasks.c                                                   Version 6.0.0
*
* FRPort Task Functions
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
* For FreeRTOS Kernel V10.4.3
*
*******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "stack_macros.h"

#include "bsp.h"

void* pvTaskTags[SMX_NUM_TASKS];

/* Task mapping functions */
BaseType_t xTaskAbortDelay( TaskHandle_t xTask )
{
    return (BaseType_t)smx_TaskResume((TCB_PTR)xTask);
}

void vTaskAllocateMPURegions( TaskHandle_t xTaskToModify,
                              const MemoryRegion_t * const xRegions )
{ 
}

BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask,
                                         void * pvParameter )
{
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);
    TaskHookFunction_t fun = (TaskHookFunction_t)pvTaskTags[task->indx];
    (*fun)(pvParameter);
    return pdTRUE;
}

BaseType_t xTaskCatchUpTicks( TickType_t xTicksToCatchUp )
{
    return 0;
}

BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut,
                                 TickType_t * const pxTicksToWait )
{
    return pdFALSE;
}

eSleepModeStatus eTaskConfirmSleepModeStatus( void )
{
    eSleepModeStatus eReturn = eStandardSleep;
    return eReturn;
}

BaseType_t xTaskCreate( TaskFunction_t pxTaskCode,
                         const char * const pcName,
                         const configSTACK_DEPTH_TYPE usStackDepth,
                         void * const pvParameters,
                         UBaseType_t uxPriority,
                         TaskHandle_t * const pxCreatedTask )
{
    if (smx_TaskCreate((FUN_PTR)pxTaskCode, uxPriority, usStackDepth*4, 0, pcName, 
                                                     NULL, (TCB**)pxCreatedTask));
    {
        if (smx_TaskStart((TCB*)*pxCreatedTask, (u32)pvParameters))
            return pdPASS;
    }
    return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
}

TaskHandle_t xTaskCreateStatic( TaskFunction_t pxTaskCode,
                               const char * const pcName,
                               const uint32_t ulStackDepth,
                               void * const pvParameters,
                               UBaseType_t uxPriority,
                               StackType_t * const puxStackBuffer,
                               StaticTask_t * const pxTaskBuffer )
{
    TCB_PTR task = smx_TaskCreate((FUN_PTR)pxTaskCode, uxPriority, ulStackDepth*4, 0, pcName);
    if (task != NULL)
    {
        if (smx_TaskStart(task, (u32)pvParameters))
            return (TaskHandle_t)task;
    }
    return NULL;
}

BaseType_t xTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition,
                                 TaskHandle_t * pxCreatedTask )
{
   return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
}

BaseType_t xTaskCreateRestrictedStatic( const TaskParameters_t * const pxTaskDefinition,
                                       TaskHandle_t * pxCreatedTask )
{
   return errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
}

void vTaskDelay( const TickType_t xTicksToDelay )
{
    smx_TaskSuspend(smx_ct, (u32)xTicksToDelay);
}

BaseType_t xTaskDelayUntil( TickType_t * const pxPreviousWakeTime,
                            const TickType_t xTimeIncrement )
{
    BaseType_t pass = pdFALSE;
    smx_LSRsOff();
    u32 tmo = (u32)(*pxPreviousWakeTime + xTimeIncrement);
    if (tmo > smx_etime)
    {
        *pxPreviousWakeTime = (TickType_t)tmo;
        pass = (BaseType_t)smx_TaskSuspend(smx_ct, tmo - smx_etime);
    }
    smx_LSRsOn();
    return pass;
}

void vTaskDelete( TaskHandle_t xTaskToDelete )
{
    TCB_PTR task = (xTaskToDelete == NULL ? smx_ct : (TCB_PTR)xTaskToDelete);
    pvTaskTags[task->indx] = NULL;
    smx_TaskDelete(&task);
}

void vTaskEndScheduler( void )
{
}

void vTaskEnterCritical( void )
{
}

void vTaskExitCritical( void )
{
}

BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify,
                               UBaseType_t uxIndexToNotify,
                               uint32_t ulValue,
                               eNotifyAction eAction,
                               uint32_t * pulPreviousNotificationValue )
{
    return pdFAIL;
}

BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify,
                                      UBaseType_t uxIndexToNotify,
                                      uint32_t ulValue,
                                      eNotifyAction eAction,
                                      uint32_t * pulPreviousNotificationValue,
                                      BaseType_t * pxHigherPriorityTaskWoken )
{
    return pdFAIL;
}

void vTaskGenericNotifyGiveFromISR( TaskHandle_t xTaskToNotify,
                                    UBaseType_t uxIndexToNotify,
                                    BaseType_t * pxHigherPriorityTaskWoken )
{
}

BaseType_t xTaskGenericNotifyStateClear( TaskHandle_t xTask,
                                         UBaseType_t uxIndexToClear )
{
    return pdFAIL;
}

uint32_t ulTaskGenericNotifyTake( UBaseType_t uxIndexToWait,
                                  BaseType_t xClearCountOnExit,
                                  TickType_t xTicksToWait )
{
    return pdFAIL;
}

uint32_t ulTaskGenericNotifyValueClear( TaskHandle_t xTask,
                                        UBaseType_t uxIndexToClear,
                                        uint32_t ulBitsToClear )
{
    return pdFAIL;;
}

BaseType_t xTaskGenericNotifyWait( UBaseType_t uxIndexToWait,
                                   uint32_t ulBitsToClearOnEntry,
                                   uint32_t ulBitsToClearOnExit,
                                   uint32_t * pulNotificationValue,
                                   TickType_t xTicksToWait )
{
    return pdFAIL;
}

TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask )
{
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);
    return (TaskHookFunction_t)pvTaskTags[task->indx];
}

TaskHookFunction_t xTaskGetApplicationTaskTagFromISR( TaskHandle_t xTask )
{
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);
    return (TaskHookFunction_t)pvTaskTags[task->indx];
}

TaskHandle_t xTaskGetCurrentTaskHandle( void )
{
    return (TaskHandle_t)smx_ct;
}

TaskHandle_t xTaskGetHandle( const char * pcNameToQuery )
{
    TCB_PTR task;
    for (task = (TCB_PTR)smx_tcbs.pi; task < (TCB_PTR)smx_tcbs.px; task++)
    {
        if (task->cbtype == SMX_CB_TASK)
            if (strcmp(task->name, pcNameToQuery) == 0)
                return (TaskHandle_t)task;
    }
    return NULL;
}

uint32_t ulTaskGetIdleRunTimeCounter( void )
{
    return (uint32_t)smx_cpd.idle;
}

TaskHandle_t xTaskGetIdleTaskHandle( void )
{
    return (TaskHandle_t)smx_Idle;
}

void vTaskGetInfo( TaskHandle_t xTask,
                   TaskStatus_t * pxTaskStatus,
                   BaseType_t xGetFreeStackSpace,
                   eTaskState eState )
{ 
}

char * pcTaskGetName( TaskHandle_t xTaskToQuery )
{
    TCB_PTR task = (xTaskToQuery == NULL ? smx_ct : (TCB_PTR)xTaskToQuery);
    return (char*)task->name;
}

UBaseType_t uxTaskGetNumberOfTasks( void )
{
    return (UBaseType_t)smx_tcbs.num_used;
}

void vTaskGetRunTimeStats( char * pcWriteBuffer )
{
}

BaseType_t xTaskGetSchedulerState( void )
{
    smx_LSRsOff();
    if (smx_init == false)
        return taskSCHEDULER_NOT_STARTED;
    if (smx_lockctr > 0)
        return taskSCHEDULER_SUSPENDED;
    smx_LSRsOn();
    return taskSCHEDULER_RUNNING;
}

UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask )
{
    smx_LSRsOff();
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);
    u32 space = (u32)task->ssz - (u32)task->shwm;
    smx_LSRsOn();
    return (UBaseType_t)space;
}

configSTACK_DEPTH_TYPE uxTaskGetStackHighWaterMark2( TaskHandle_t xTask )
{
    smx_LSRsOff();
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);
    u32 space = (u32)task->ssz - (u32)task->shwm;
    smx_LSRsOn();
    return (configSTACK_DEPTH_TYPE)space;
}

eTaskState eTaskGetState( TaskHandle_t xTask )
{
    eTaskState Estate;
    SMX_STATE  state;
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);

    smx_LSRsOff();
    state = (SMX_STATE)smx_TaskPeek(task, SMX_PK_STATE);
    switch (state)
    {
        case SMX_TASK_RUN:
            Estate = eRunning;
            break;
        case SMX_TASK_READY:
            Estate = eReady;
            break;
        case SMX_TASK_WAIT:
            if (smx_timeout[task->indx] != SMX_TMO_INF)
                Estate = eBlocked;
            else
                Estate = eSuspended;
            break;
        case SMX_TASK_DEL:
            Estate = eDeleted;
            break;
        default:
            Estate = eInvalid;
    }
    smx_LSRsOn();
    return Estate;
}

UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray,
                                  const UBaseType_t uxArraySize,
                                  uint32_t * const pulTotalRunTime )
{
    return 0;
}

UBaseType_t uxTaskGetTaskNumber( TaskHandle_t xTask )
{
    if (xTask == NULL)
        return 0;
    else
        return (UBaseType_t)(((TCB_PTR)xTask)->indx);
}

void * pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery,
                                           BaseType_t xIndex )
{
    TCB_PTR task = (xTaskToQuery == NULL ? smx_ct : (TCB_PTR)xTaskToQuery);
    return (void*)(task->sbp + SMX_RSA_SIZE);
}

TickType_t xTaskGetTickCount( void )
{
    return (TickType_t)smx_etime;
}

TickType_t xTaskGetTickCountFromISR( void )
{
    return (TickType_t)smx_etime;
}

TaskHandle_t pvTaskIncrementMutexHeldCount( void )
{
    return NULL;
}

BaseType_t xTaskIncrementTick( void )
{
    return pdFALSE;
}

void vTaskInternalSetTimeOutState( TimeOut_t * const pxTimeOut )
{
}

void vTaskList( char * pcWriteBuffer )
{
}

void vTaskMissedYield( void )
{
}

void vTaskPlaceOnEventList( List_t * const pxEventList,
                            const TickType_t xTicksToWait )
{
}

void vTaskPlaceOnEventListRestricted( List_t * const pxEventList,
                                      TickType_t xTicksToWait,
                                      const BaseType_t xWaitIndefinitely )
{
}

void vTaskPlaceOnUnorderedEventList( List_t * pxEventList,
                                     const TickType_t xItemValue,
                                     const TickType_t xTicksToWait )
{
}

BaseType_t xTaskPriorityDisinherit( TaskHandle_t const pxMutexHolder )
{
    return pdFALSE;
}

void vTaskPriorityDisinheritAfterTimeout( TaskHandle_t const pxMutexHolder,
                                          UBaseType_t uxHighestPriorityWaitingTask )
{
}

UBaseType_t uxTaskPriorityGet( const TaskHandle_t xTask )
{
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);
    return (UBaseType_t)smx_TaskPeek(task, SMX_PK_PRI);
}

UBaseType_t uxTaskPriorityGetFromISR( const TaskHandle_t xTask )
{
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);
    return (UBaseType_t)task->pri;
}

BaseType_t xTaskPriorityInherit( TaskHandle_t const pxMutexHolder )
{
    return pdFALSE;
}

void vTaskPrioritySet( TaskHandle_t xTask,
                       UBaseType_t uxNewPriority )
{
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);
    smx_TaskBump(task, (u8)uxNewPriority);
}

BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList )
{
    return pdFALSE;
}

void vTaskRemoveFromUnorderedEventList( ListItem_t * pxEventListItem,
                                        const TickType_t xItemValue )
{ 
}

TickType_t uxTaskResetEventItemValue( void )
{
    return 0;
}

void vTaskResume( TaskHandle_t xTaskToResume )
{
    TCB_PTR task = (xTaskToResume == NULL ? smx_ct : (TCB_PTR)xTaskToResume);
    smx_TaskResume(task);
}

BaseType_t xTaskResumeAll( void )
{
    BaseType_t pass = pdFALSE;
    bool       hptw;

    smx_LSRsOff();
    hptw = ((RQCB_PTR)smx_rqtop - (RQCB_PTR)smx_rq) > smx_ct->pri ? true : false;
    if (smx_TaskUnlock() && hptw)
        pass = pdTRUE;
    smx_LSRsOn();
    return pass;
}

BaseType_t xTaskResumeFromISR( TaskHandle_t xTaskToResume )
{
    TCB_PTR task = (xTaskToResume == NULL ? smx_ct : (TCB_PTR)xTaskToResume);
    return smx_TaskResume(task);
}

void vTaskSetApplicationTaskTag( TaskHandle_t xTask,
                                 TaskHookFunction_t pxHookFunction )
{
    TCB_PTR task = (xTask == NULL ? smx_ct : (TCB_PTR)xTask);
    pvTaskTags[task->indx] = (void*)pxHookFunction;
}

void vTaskSetTaskNumber( TaskHandle_t xTask,
                         const UBaseType_t uxHandle )
{
}

void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet,
                                        BaseType_t xIndex,
                                        void * pvValue )
{
}

void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut )
{
}

void vTaskStartScheduler( void )
{
}

void vTaskStepTick( const TickType_t xTicksToJump )
{
}

void vTaskSuspend( TaskHandle_t xTaskToSuspend )
{
    TCB_PTR task = (xTaskToSuspend == NULL ? smx_ct : (TCB_PTR)xTaskToSuspend);
    smx_TaskSuspend(task, SMX_TMO_INF);
}

void vTaskSuspendAll( void )
{
    smx_TaskLock();
}

void vTaskSwitchContext( void )
{
    smx_TaskSuspend(smx_ct, SMX_TMO_INF);
}