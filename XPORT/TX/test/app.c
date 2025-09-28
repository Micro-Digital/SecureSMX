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
