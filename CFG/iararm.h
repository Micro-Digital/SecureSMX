/*
* iararm.h                                                  Version 5.4.0
*
* Master Preinclude File for IAR ARM. Selects which target board header
* file to use and which SMX product libraries and demos to include.
* This file is included ahead of all other header files (in the IDE
* project, on the Preprocessor tab in C/C++ Compiler settings).
*
* IMPORTANT: Pragmas below OVERRIDE IDE settings!
*
* Copyright (c) 2002-2025 Micro Digital Inc.
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
