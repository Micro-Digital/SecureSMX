/*
* leddemo.c                                                 Version 5.4.0
*
* LED demo.
*
* SSMX: This is an example of a umode partition that accesses physical devices 
* via a device driver (led.c). <4>
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
* Author: David Moore
*
*****************************************************************************/

#include "smx.h"
#include "main.h"
#include "app.h"
#include "led.h"

/* Display count or pattern on LEDs <1> */
#if SB_LED_NUM_7SEG
#define LED_7SEG_COUNT      1
#define LED_7SEG_SNAKE      0
#define LED_7SEG_CIRCLE     0
#endif

#if SB_LED_NUM_IN_ROW
#define LED_ROW_COUNT       1
#define LED_ROW_ALTERNATE   0
#define LED_ROW_FLASH       0
#define LED_ROW_SCAN        0
#endif

#if SB_LED_NUM_7SEG
#if LED_7SEG_SNAKE
const u32 LED_SnakeSegs[8] =
   {LED_SEG_A, LED_SEG_B, LED_SEG_G, LED_SEG_E,
    LED_SEG_D, LED_SEG_C, LED_SEG_G, LED_SEG_F};
#elif LED_7SEG_CIRCLE
const u32 LED_CircleSegs[6] =
   {LED_SEG_A, LED_SEG_B, LED_SEG_C, LED_SEG_D,
    LED_SEG_E, LED_SEG_F};
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
void leddemo_init(void);
void leddemo_exit(void);
void leddemo_task_main(u32);
#ifdef __cplusplus
}
#endif

/* leddemo_init, leddemo_exit  (hmode)
*
*  LED demo initialization and exit
*/
void leddemo_init(void)
{
   TCB_PTR leddemo_task = smx_TaskCreate(leddemo_task_main, PRI_HI, 0, SMX_FL_UMODE, "leddemo_task");

 #if SMX_CFG_SSMX
  #if SB_CPU_ARMM7
   mp_MPACreate(leddemo_task, &mpa_tmplt_led, 0x3F, 8);
  #else
   mp_MPACreate(leddemo_task, &mpa_tmplt_led, 0x0F, 8);
  #endif
 #endif

   smx_TaskStart(leddemo_task); /*<2>*/
}

void leddemo_exit(void)
{
}

#if SMX_CFG_SSMX
#pragma default_function_attributes = @ ".led.text"
#include "xapiu.h"   /*<3>*/
#endif

/* leddemo_task_main (umode/pmode)
*
*  LED demo.
*/
void leddemo_task_main(u32)
{
   u32 ctr = 0;

   while (1)
   {
    #if SB_LED_NUM_IN_ROW
     #if LED_ROW_COUNT
      sb_LEDWriteRow(ctr);

     #elif LED_ROW_ALTERNATE
      sb_LEDWriteRow((ctr & 1) ? LED_PATTERN_ODD : LED_PATTERN_EVEN);

     #elif LED_ROW_FLASH
      sb_LEDWriteRow((ctr & 1) ? LED_PATTERN_ALL : LED_PATTERN_NONE);

     #elif LED_ROW_SCAN
      sb_LEDWriteRow(1 << (ctr % SB_LED_NUM_IN_ROW));
     #endif
    #endif /* SB_LED_NUM_IN_ROW */

    #if SB_LED_NUM_7SEG
     #if LED_7SEG_COUNT
      #if (SB_LED_NUM_7SEG == 2)
      sb_LEDWrite7SegNum(ctr % 100);  /* 0 to 99 */
      #else
      sb_LEDWrite7SegNum(ctr % 10);   /* 0 to 9 */
      #endif

     #elif LED_7SEG_SNAKE
      sb_LEDWrite7Seg(LED_SnakeSegs[ctr % 8]);

     #elif LED_7SEG_CIRCLE
      sb_LEDWrite7Seg(LED_CircleSegs[ctr % 6]);
     #endif
    #endif /* SB_LED_NUM_7SEG */

      ctr++;
      smx_TaskSuspend(SMX_CT, SMX_TICKS_PER_SEC);
   }
}

/* Notes:
   1. A board could have one or more of different types of LEDs or none.
   2. Tasks do not start running until ainit() finishes.
   3. See svc.c.
   4. See mpa7.c or mpa8.c for MPA template and <processor>app_mpu.icf for 
      linker command file.
*/
