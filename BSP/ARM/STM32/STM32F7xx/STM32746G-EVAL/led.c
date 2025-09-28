/*
* led.c                                                     Version 5.4.0
*
* Routines to write to LEDs LD1 to LD4 on STMicro STM32746G-EVAL board.
* Copied code from STMicro example and modified.
*
* Copyright (c) 2001-2025 Micro Digital Inc.
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
    BSP_LED_Init(LED2);
    BSP_LED_Init(LED3);
    BSP_LED_Init(LED4);    
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
    /* LED 2 */
    on = (val & 0x02) ? true : false;
    on ? BSP_LED_On(LED2) : BSP_LED_Off(LED2);
    /* LED 3 */
    on = (val & 0x04) ? true : false;
    on ? BSP_LED_On(LED3) : BSP_LED_Off(LED3);
    /* LED 4 */
    on = (val & 0x08) ? true : false;
    on ? BSP_LED_On(LED4) : BSP_LED_Off(LED4);
}

