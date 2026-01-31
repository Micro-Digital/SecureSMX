/*
* app.c                                                     Version 6.0.0
*
* Protosystem application module.
*
* USER: This file should be replaced for your application. See below.
* Add-on demo modules are in the DEMO directory. Demo modules contain
* configuration constants that should be set and directions that you
* should read prior to building demos.
*
* Copyright (c) 1994-2026 Micro Digital Inc.
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