/*
* app.c                                                     Version 5.4.0
*
* Protosystem application module.
*
* USER: This file should be replaced for your application. See below.
* Add-on demo modules are in the DEMO directory. Demo modules contain
* configuration constants that should be set and directions that you
* should read prior to building demos.
*
* Copyright (c) 1994-2025 Micro Digital Inc.
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

#if MW_FATFS_DEMO && FP_PORTAL
#include "fp.h"
#endif

/* appl_init   (hmode)
*
*  Display application header and initialize demos. <1>
*/
void appl_init(void)
{
   display_protosystem_msgs();

   leddemo_init();

  #if SB_LCD_DEMO && SB_LCD
   lcddemo_init();
  #endif

  #if SB_FPU_DEMO
   fpudemo_init();
  #endif

  #if MW_FATFS_DEMO
   fpdemo_init();
  #endif
}

/* appl_exit (hmode)
*
*  Call exit routines for demos.
*/
void appl_exit(void)
{
  #if MW_FATFS_DEMO
   fpdemo_exit();
  #endif

  #if SB_FPU_DEMO
   fpudemo_exit();
  #endif

  #if SB_LCD_DEMO && SB_LCD
   lcddemo_exit();
  #endif
}

#pragma section_prefix = ".ucom"  /* display_protosystem_msgs() also called from opcon */

/* display_protosystem_msgs   (pmode)
*
*  Displays the header message.
*/
void display_protosystem_msgs(void)
{
  #if SMX_CFG_SSMX
   const char *msg1 = "SecureSMX RTOS  (Built " __DATE__ " " __TIME__ ")";
  #else
   const char *msg1 = "SMX RTOS  (Built " __DATE__ " " __TIME__ ")";
  #endif
   const char *msg2 = "Esc to Exit";
  
 #if SB_CFG_CON
  #if CP_PORTAL
   if (smx_Idle->fun == ainit)  /* if called during ainit, do direct calls */
  #endif
   {
      sb_ConWriteString(0,0,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,msg1);
      sb_ConWriteString(ESC_COL,0,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,msg2);
   }
  #if CP_PORTAL
   else if (smx_ct == opcon)  /* call via console portal */
   {
      sbp_ConWriteString(0,0,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,msg1,&cpcli_opcon);
      sbp_ConWriteString(ESC_COL,0,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,msg2,&cpcli_opcon);
   }
  #endif
 #endif
}

/* Notes:
   1. Do not call smx functions that wait (i.e. those with a timeout parameter). 
      Do not call middleware routines, since they call smx functions that wait.
      ainit() (which calls this routine) must finish before other tasks run.
*/