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

#if SMX_CFG_SSMX
/*............................. SECTION CHANGE .............................*/
#pragma default_function_attributes = @ ".sys.text"
#endif

void display_protosystem_msgs(void);
void txp_test(void);
void main_full(void);

vbool          isren;      /* ISRFuncPtr() enable */
FUNV_PTR       ISRFuncPtr; /* pointer to ISR test function */

void appl_init(void)
{
//   display_protosystem_msgs();
   sb_MsgDisplay();
   sb_DelayMsec(20);
   txp_test();
//   main_full();
}

/***** APPLICATION EXIT
* Put application cleanup code here.
*****************************************************************************/

void appl_exit(void)
{
}

/***** SUBROUTINES
*****************************************************************************/

void display_protosystem_msgs(void)
{
  #if (SB_CFG_CON)
   sb_ConWriteString(0,0,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,"FreeRTOS to smx Port  (Built " __DATE__ " " __TIME__ ")");
   sb_ConWriteString(ESC_COL,0,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,"Full Demo");
  #endif
}
