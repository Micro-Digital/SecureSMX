/*
* lcddemo.c                                                 Version 5.4.0
*
* LCD demo for STMicro STM32746G-EVAL board.
*
* SSMX: This is an example of a umode partition that accesses physical devices 
* via a device driver (lcd.c). <1>
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
* Author: David Moore
*
*****************************************************************************/

#include "bbase.h"
#include "main.h"
#include "lcd.h"

#if SB_LCD && SB_LCD_DEMO

#define SMX_LINE      LCD_LINE_0
#define SECONDS_LINE  LCD_LINE_2

TCB_PTR lcddemo_task;

#ifdef __cplusplus
extern "C" {
#endif
void lcddemo_init(void);
void lcddemo_exit(void);
void lcddemo_task_main(u32);
#ifdef __cplusplus
}
#endif

/* lcddemo_init, lcddemo_exit  (hmode)
*
*  LCD demo initialization and exit
*/
void lcddemo_init(void)
{
#if SMX_CFG_SSMX
   lcddemo_task = smx_TaskCreate(lcddemo_task_main, PRI_NORM, 400, SMX_FL_UMODE, "lcddemo_task");
   mp_MPACreate(lcddemo_task, &mpa_tmplt_lcd, 0x3F, 8);
#else
   lcddemo_task = smx_TaskCreate(lcddemo_task_main, PRI_NORM, 400, SMX_FL_NONE, "lcddemo_task");
#endif
   smx_TaskStart(lcddemo_task);
}

void lcddemo_exit(void)
{
}

#if SMX_CFG_SSMX
#pragma section_prefix = ".lcd"  /*<1>*/
#include "xapiu.h"
#endif

void lcddemo_task_main(u32)
{
   static u32 stime0;
   char buf[21];

   /* write title */
  #if SMX_CFG_SSMX
   sb_LCDWriteString("SecureSMX RTOS on STM32F7", SMX_LINE, TEXT_COLOR, 0);
  #else
   sb_LCDWriteString("SMX RTOS on STM32F7", SMX_LINE, TEXT_COLOR, 0);
  #endif
   strcpy(buf, "seconds = ");
   stime0 = smx_SysPeek(SMX_PK_STIME);

   while (1)
   {
      itoa(smx_SysPeek(SMX_PK_STIME) - stime0, buf+10, 10);
      sb_LCDWriteString(buf, SECONDS_LINE, TEXT_COLOR, 0);
      smx_DelaySec(1);
   }
}
#endif /* SB_LCD && SB_LCD_DEMO */

/* Notes:
   1. See mpa7.c for MPA template and <processor>_app_mpu.icf for 
      linker command file.
*/

