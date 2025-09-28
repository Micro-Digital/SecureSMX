/*
* startup.c                                                 Version 5.4.0
*
* Startup code extension. Called early in the compiler's startup code.
* You can add any other early init code here, as desired.
*
* IAR:        Called by __iar_program_start() in cmain.s
* CrossWorks: Called by _start in thumb_crt0.s
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
* Author: David Moore, Xingsheng Wan, XiaoYi Chen
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

#ifdef __cplusplus
extern "C" {
#endif

static volatile u32 uwTick;

int __low_level_init(void)
{
   sb_MSFillResv(32);

   /* STMicro HAL low-level init */
   SystemInit();

   /* USER: You can add any other early init code here. sb_PeripheralsInit()
            is another place that runs later. Decide which is appropriate.
   */

   return 1;  /* 1 tells IAR startup code to run seg_init; 0 skips */
}

#if SMX_CFG_SSMX
/*+++++++++++++++++++++++ LIMITED SWI/SVC API (UMODE) ++++++++++++++++++++++*/
#include "xapiu.h"  /* need sbu_ and smxu_ prototypes */
#include "xapip.h"  /* but don't want mapping macros */
#endif

/*
   The following 3 functions are declared and have weak definitions in
   STMicro Library v1.5. They use the tick exception to generate delay
   for peripheral initialization. We have to override them here because
   SMX takes over the tick exception.
*/
void HAL_IncTick(void)
{
   uwTick++;
}

#if SMX_CFG_SSMX
/*............................. SECTION CHANGE .............................*/
#pragma default_function_attributes = @ ".ucom.text"
#endif

u32 HAL_GetTick(void)
{  
  #if SMX_CFG_SSMX
   if (sb_IN_UMODE())
   {
      return smxu_SysPeek(SMX_PK_ETIME_MS);  /*<3>*/
   }
   else
  #endif
   {
      if (sb_tick_init_done)  /*<1>*/
         return smx_SysPeek(SMX_PK_ETIME_MS);
      else
         return uwTick;
   }
}

void HAL_Delay(u32 Delay)
{
  #if SMX_CFG_SSMX
   if (sb_IN_UMODE() || sb_tick_init_done)  /*<1><2>*/
  #else
   if (sb_tick_init_done)  /*<2>*/
  #endif
   {
      sb_DelayMsec(Delay);
   }
   else
   {
      u32 tickstart;
      tickstart = HAL_GetTick();
      while ((HAL_GetTick() - tickstart) < Delay){}
   }
}

#ifdef __cplusplus
}
#endif

/* Notes:
   1. Necessary to check sb_tick_init_done because HAL_GetTick() and
      HAL_Delay() are called during low-level startup code, before smx
      tick is initialized in main(), so we use HAL uwTick until then.
      Also, it can be read directly because this case runs in pmode.
   2. Unnecessary to check sb_tick_init_done in umode case because
      tick init was done in pmode before we ever switch to umode.
*/