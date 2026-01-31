/*
* timers.c                                                  Version 6.0.0
*
* FRPort Timer Functions
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
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

LCB_PTR TimerLSRs[SMX_NUM_TIMERS];

void vAssertCalled( uint32_t ulLine, const char *pcFile )
{
}

TimerHandle_t xTimerCreate( const char * const pcTimerName,
                            const TickType_t xTimerPeriodInTicks,
                            const UBaseType_t uxAutoReload,
                            void * const pvTimerID,
                            TimerCallbackFunction_t pxCallbackFunction )
{
    TMRCB_PTR tp = NULL;
    u32 i;

    smx_LSRsOff();
    for (i = 0; (i < SMX_NUM_TIMERS) && (TimerLSRs[i] != 0); i++) {}
    if (i < SMX_NUM_TIMERS)
    {
        TimerLSRs[i] = smx_LSRCreate((FUN_PTR)pxCallbackFunction, SMX_FL_TRUST, 
                                                                  "Timer LSR");
        if (smx_TimerStart(&tp, SMX_TMO_INF, (u32)xTimerPeriodInTicks, 
                                             TimerLSRs[i], pcTimerName)) /*<1>*/
        {
            tp->par = (u32)pvTimerID;
            tp->flags.os = (uxAutoReload == pdFALSE) ? true : false;
            tp->flags.act = false;
        }
    }
    smx_LSRsOn();
    return (TimerHandle_t)tp;
}

TimerHandle_t xTimerCreateStatic( const char * const pcTimerName,
                                  const TickType_t xTimerPeriodInTicks,
                                  const UBaseType_t uxAutoReload,
                                  void * const pvTimerID,
                                  TimerCallbackFunction_t pxCallbackFunction,
                                  StaticTimer_t * pxTimerBuffer )
{
    TMRCB_PTR tp = NULL;
    u32 i;

    smx_LSRsOff();
    for (i = 0; (i < SMX_NUM_TIMERS) && (TimerLSRs[i] != 0); i++) {}
    if (i < SMX_NUM_TIMERS)
    {
        TimerLSRs[i] = smx_LSRCreate((FUN_PTR)pxCallbackFunction, SMX_FL_TRUST, 
                                                                  "Timer LSR");
        if (smx_TimerStart(&tp, SMX_TMO_INF, (u32)xTimerPeriodInTicks, 
                                             TimerLSRs[i], pcTimerName)) /*<1>*/
        {
            tp->par = (u32)pvTimerID;
            tp->flags.os = (uxAutoReload == pdFALSE) ? true : false;
            tp->flags.act = false;
        }
    }
    smx_LSRsOn();
    return (TimerHandle_t)tp;
}

BaseType_t xTimerGenericCommand( TimerHandle_t xTimer,
                                 const BaseType_t xCommandID,
                                 const TickType_t xOptionalValue,
                                 BaseType_t * const pxHigherPriorityTaskWoken,
                                 const TickType_t xTicksToWait )
{
    bool       pass = false;
    u32        period;
    TMRCB_PTR  tp = (TMRCB_PTR)xTimer;
    LCB_PTR    lsr;
    int        i;

    smx_LSRsOff();
    switch (xCommandID)
    {
        case tmrCOMMAND_CHANGE_PERIOD:
        case tmrCOMMAND_CHANGE_PERIOD_FROM_ISR:
            pass = smx_TimerStart(&tp, (u32)xOptionalValue, (u32)xOptionalValue, 
                                                             tp->lsr, tp->name);
            break;
        case tmrCOMMAND_DELETE:
            lsr = tp->lsr;
            pass = smx_TimerStop(tp);
            for (i = 0; (i < SMX_NUM_TIMERS) && (TimerLSRs[i] != lsr); i++) {} /*<2>*/
            if (TimerLSRs[i] == lsr)
                pass |= smx_LSRDelete(&TimerLSRs[i]);
            else
                pass |= smx_LSRDelete(&lsr);
            break;
        case tmrCOMMAND_RESET:
        case tmrCOMMAND_RESET_FROM_ISR:
            period = tp->flags.os ? 0 : tp->period;
            pass = smx_TimerStart(&tp, tp->nxtdly, period, tp->lsr, tp->name);
            tp->flags.act = true;
            break;
        case tmrCOMMAND_START:
        case tmrCOMMAND_START_FROM_ISR:
            period = tp->flags.os ? 0 : tp->period;
            pass = smx_TimerStart(&tp, tp->period, period, tp->lsr, tp->name);
            tp->flags.act = true;
            break;
        case tmrCOMMAND_STOP:
        case tmrCOMMAND_STOP_FROM_ISR:
            pass = smx_TimerStart(&tp, SMX_TMO_INF, tp->period, tp->lsr, tp->name); /*<1>*/
            tp->flags.act = false;
            break;
    }
    smx_LSRsOn();
    return (BaseType_t)pass;
}

TickType_t xTimerGetExpiryTime( TimerHandle_t xTimer )
{
    return (TickType_t)(smx_etime + smx_TimerPeek((TMRCB_PTR)xTimer, SMX_PK_TIME_LEFT));
}

const char * pcTimerGetName( TimerHandle_t xTimer )
{
    return ((TMRCB_PTR)xTimer)->name;
}

TickType_t xTimerGetPeriod( TimerHandle_t xTimer )
{
    return (TickType_t)(((TMRCB_PTR)xTimer)->period);
}

UBaseType_t uxTimerGetReloadMode( TimerHandle_t xTimer )
{
    if (((TMRCB_PTR)xTimer)->flags.os)
        return pdFALSE;
    else
        return pdTRUE;
}

TaskHandle_t xTimerGetTimerDaemonTaskHandle( void )
{
    return NULL;
}

void * pvTimerGetTimerID( const TimerHandle_t xTimer )
{
    return (void*)(((TMRCB_PTR)xTimer)->par);
}

UBaseType_t uxTimerGetTimerNumber( TimerHandle_t xTimer )
{
    return 0;
}

BaseType_t xTimerIsTimerActive( TimerHandle_t xTimer )
{
    if (((TMRCB_PTR)xTimer)->flags.act)
        return pdTRUE;
    else
        return pdFALSE;
}

BaseType_t xTimerPendFunctionCall( PendedFunction_t xFunctionToPend,
                                   void * pvParameter1,
                                   uint32_t ulParameter2,
                                   TickType_t xTicksToWait )
{
    return pdFALSE;
}

BaseType_t xTimerPendFunctionCallFromISR( PendedFunction_t xFunctionToPend,
                                          void * pvParameter1,
                                          uint32_t ulParameter2,
                                          BaseType_t * pxHigherPriorityTaskWoken )
{
    return pdFALSE;
}

void vTimerSetTimerID( TimerHandle_t xTimer, void * pvNewID )
{
    ((TMRCB_PTR)xTimer)->par = (u32)pvNewID;
}

void vTimerSetTimerNumber( TimerHandle_t xTimer, UBaseType_t uxTimerNumber )
{
}

void vTimerSetReloadMode( TimerHandle_t xTimer, const UBaseType_t uxAutoReload )
{
    if (uxAutoReload == pdFALSE)
        ((TMRCB_PTR)xTimer)->flags.os = true;
    else
        ((TMRCB_PTR)xTimer)->flags.os = false;
}

/*
   Notes:
   1. SMX_TMO_INF passed for delay parks the timer at the end of tq using an
      infinite timeout, until the timer is started. Done only for FRPort.
      Also, it is ok to set timer fields outside an SSR here since the timer
      is not running until xTimerStart() is called.
   2. Find free LSR handle in array so it is cleared by delete. if not found, 
      something went wrong, but delete the LSR anyway.
*/
