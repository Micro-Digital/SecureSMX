/*
* lcd.c                                                     Version 5.4.0
*
* Routines to write to LCD on STMicro STM32746G-EVAL board.
*
* Copyright (c) 2009-2025 Micro Digital Inc.
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
#include "lcd.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".lcd"
#endif

static void *GetLCDBuffer(void);

void sb_LCDInit(void)
{
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(LCD_FOREGROUND_LAYER, (uint32_t)GetLCDBuffer());
    BSP_LCD_SelectLayer(LCD_FOREGROUND_LAYER);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
}

void sb_LCDWriteString(const char *str, uint line, u32 color, u32 attr)
{
    (void)attr;
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DisplayStringAtLine(line, (uint8_t*)str);
}

static void *GetLCDBuffer(void)
{
#pragma section = "LCD_BUF" /* defined in linker command file (.icf) */
    return (void *)__section_begin("LCD_BUF");
}

