/*
* event_groups.c                                            Version 5.4.0
*
* FRPort Event Group Functions
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
* For FreeRTOS Kernel V10.4.3
*
*******************************************************************************/

#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "event_groups.h"

EventBits_t xEventGroupClearBits( EventGroupHandle_t xEventGroup,
                                  const EventBits_t uxBitsToClear )
{
    smx_LSRsOff();
    u32 flags = ((EGCB_PTR)xEventGroup)->flags;
    smx_EventFlagsSet((EGCB_PTR)xEventGroup, 0, (u32)uxBitsToClear);
    smx_LSRsOn();
    return (BaseType_t)flags;
}

BaseType_t xEventGroupClearBitsFromISR( EventGroupHandle_t xEventGroup,
                                        const EventBits_t uxBitsToClear )
{
    u32 flags = ((EGCB_PTR)xEventGroup)->flags;
    smx_EventFlagsSet((EGCB_PTR)xEventGroup, 0, (u32)uxBitsToClear);
    return (BaseType_t)flags;
}

EventGroupHandle_t xEventGroupCreate( void )
{
    return (EventGroupHandle_t)smx_EventGroupCreate(0);
}

EventGroupHandle_t xEventGroupCreateStatic( StaticEventGroup_t * pxEventGroupBuffer )
{
    return (EventGroupHandle_t)smx_EventGroupCreate(0);
}

void vEventGroupDelete( EventGroupHandle_t xEventGroup )
{
    smx_EventGroupDelete((EGCB_PTR*)&xEventGroup);
}

EventBits_t xEventGroupGetBitsFromISR( EventGroupHandle_t xEventGroup )
{
    return (EventBits_t)((EGCB_PTR)xEventGroup)->flags;
}

UBaseType_t uxEventGroupGetNumber( void * xEventGroup )
{
    return 0;
}

EventBits_t xEventGroupSetBits( EventGroupHandle_t xEventGroup,
                                const EventBits_t uxBitsToSet )
{
    smx_LSRsOff();
    smx_EventFlagsSet((EGCB_PTR)xEventGroup, uxBitsToSet, 0);
    u32 flags = ((EGCB_PTR)xEventGroup)->flags;
    smx_LSRsOn();
    return (EventBits_t)flags;
}

BaseType_t xEventGroupSetBitsFromISR( EventGroupHandle_t xEventGroup,
                                      const EventBits_t uxBitsToSet,
                                      BaseType_t * pxHigherPriorityTaskWoken )
{
    smx_EventFlagsSet((EGCB_PTR)xEventGroup, uxBitsToSet, 0);
    return (EventBits_t)((EGCB_PTR)xEventGroup)->flags;
}


void vEventGroupSetNumber( void * xEventGroup,
                           UBaseType_t uxEventGroupNumber )
{
}

EventBits_t xEventGroupSync( EventGroupHandle_t xEventGroup,
                             const EventBits_t uxBitsToSet,
                             const EventBits_t uxBitsToWaitFor,
                             TickType_t xTicksToWait )
{
    smx_EventFlagsSet((EGCB_PTR)xEventGroup, uxBitsToSet, 0);
    u32 flags = smx_EventFlagsTest((EGCB_PTR)xEventGroup, uxBitsToWaitFor, 
                                                   SMX_EF_AND, 0, xTicksToWait);
    flags |= ((EGCB_PTR)xEventGroup)->flags;
    return flags;
}

EventBits_t xEventGroupWaitBits( EventGroupHandle_t xEventGroup,
                                 const EventBits_t uxBitsToWaitFor,
                                 const BaseType_t xClearOnExit,
                                 const BaseType_t xWaitForAllBits,
                                 TickType_t xTicksToWait )
{
    EGCB_PTR eg = (EGCB_PTR)xEventGroup;
    u32 mode = xWaitForAllBits ? SMX_EF_AND : SMX_EF_OR;
    u32 post_clear_mask = xClearOnExit ? uxBitsToWaitFor : 0;
    u32 flags = smx_EventFlagsTest(eg, uxBitsToWaitFor, mode, post_clear_mask, xTicksToWait);
    smx_LSRsOff();
    if (flags && xClearOnExit)
        eg->flags &= ~uxBitsToWaitFor;
    smx_LSRsOn();
    return (EventBits_t)flags;
}
