/*
* heap_4.c                                                  Version 5.4.0
*
* FRPort Heap Functions
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

#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE
#include "FreeRTOS.h"
#include "task.h"
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
    #error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE    ( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE         ( ( size_t ) 8 )

/* Define the linked list structure.  This is used to link free blocks in order
 * of their memory address. */
typedef struct A_BLOCK_LINK
{
    struct A_BLOCK_LINK * pxNextFreeBlock; /*<< The next free block in the list. */
    size_t xBlockSize;                     /*<< The size of the free block. */
} BlockLink_t;

void * pvPortMalloc( size_t xWantedSize )
{
    return (smx_HeapMalloc((u32)xWantedSize));
}

void vPortFree( void * pv )
{
   smx_HeapFree(pv);
}


size_t xPortGetFreeHeapSize( void )
{
    return 0;
}

size_t xPortGetMinimumEverFreeHeapSize( void )
{
    return 0;
}
#if 0
void vPortInitialiseBlocks( void )
{
    /* This just exists to keep the linker quiet. */
}
#endif

void vPortGetHeapStats( HeapStats_t * pxHeapStats )
{
}
