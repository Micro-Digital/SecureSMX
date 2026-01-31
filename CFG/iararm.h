/*
* iararm.h                                                  Version 6.0.0
*
* Master Preinclude File for IAR ARM. Selects which target board header
* file to use and which SMX product libraries and demos to include.
* This file is included ahead of all other header files (in the IDE
* project, on the Preprocessor tab in C/C++ Compiler settings).
*
* IMPORTANT: Pragmas below OVERRIDE IDE settings!
*
* Copyright (c) 2002-2026 Micro Digital Inc.
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

/*
* Settings are done here rather than in the IDE project to ensure consistency
* across all libraries and the application. Remember that these settings
* OVERRIDE the IDE settings.
*/

/*
* Select SMX Modules/Libraries (by uncommenting)
*/
#define SMXAWARE         /* smxAware */
#define MW_FATFS         /* FatFs */

/*
* Select Tests
*/
//#define SMX_TSMX
//#define SMX_FRPORT_TEST  /* FreeRTOS port test */
//#define SMX_TXPORT_TEST  /* ThreadX port test */

/*
* Select RTOS Porting Layer to Migrate From
*/
//#define SMX_FRPORT       /* FreeRTOS Port */
//#define SMX_TXPORT       /* ThreadX Port */

/*
* Select Code Generator
*/
//#define SMX_STM32CUBEMX  /* STM32CubeMX-generated project vs. standard SMX project */

/*
* Select the target board.
*/
//#include "lpc55s69evk.h"
//#include "stm32746geval.h"
#include "stm32f746gdis.h"
