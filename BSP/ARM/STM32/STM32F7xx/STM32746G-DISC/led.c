/*
* led.c                                                     Version 6.0.0
*
* Routines to write LD1 on STMicro STM32746G-DISC board.
* Copied code from STMicro example and modified.
*
* Copyright (c) 2001-2026 Micro Digital Inc.
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
* Author: XiaoYi Chen
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"
#include "led.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".ucom"
#endif


/* Initializes all LEDs. */
void sb_LEDInit(void)
{
    BSP_LED_Init(LED1);
}


/* Writes hex number (2 digits). */
void sb_LEDWrite7SegNum(int num)
{
   (void)num;
   /* no 7-segment LED */
}


/* Writes any pattern passed in, to both LEDs (see defines in LED.H). */
void sb_LEDWrite7Seg(u32 val)
{
   (void)val;
   /* no 7-segment LED */
}


/* Light LEDs in row of LEDs. Bits in val indicate which to light. */
void sb_LEDWriteRow(u32 val)
{
    bool on;
    
    /* LED 1 */
    on = (val & 0x01) ? true : false;
    on ? BSP_LED_On(LED1) : BSP_LED_Off(LED1);
}

