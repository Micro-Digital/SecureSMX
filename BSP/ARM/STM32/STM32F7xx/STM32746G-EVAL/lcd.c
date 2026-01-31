/*
* lcd.c                                                     Version 6.0.0
*
* Routines to write to LCD on STMicro STM32746G-EVAL board.
*
* Copyright (c) 2009-2026 Micro Digital Inc.
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

