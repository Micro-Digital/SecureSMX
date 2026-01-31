/*
* armdefs.h                                                 Version 6.0.0
*
* ARM main definitions file. Contains:
*
* 1. Includes of the appropriate processor .h files for I/O register
*    definitions and function prototypes.
* 2. Misc processor architecture settings.
*
* Copyright (c) 2001-2026 Micro Digital Inc.
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

#ifndef SMX_ARMDEFS_H
#define SMX_ARMDEFS_H


/*--- Master Includes ------------------------------------------------------*/
/*
   Don't change the order of includes.

   IRQ Dispatch in Hardware or Software (ARM only not ARM-M)
   Some ARMs support hardware vectoring of interrupts. For these, our
   smx_ISR_ENTER and smx_ISR_EXIT macros are used. Otherwise, smx_irq_handler
   is hooked to the IRQ slot and it calls the IRQ dispatcher routine.
*/

#if defined(SB_CPU_LPC55SXX)
#include "lpc55sxx.h"
#endif

#if defined(SB_CPU_STM32F7XX)
#if defined(SB_BRD_STMICRO_STM32F746G_DISCOVERY)
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sd.h"
#include "stm32746g_discovery_sdram.h"
#elif defined(SB_BRD_STMICRO_STM32746GEVAL)
#include "stm32f7xx_hal.h"
#include "stm32756g_eval.h"
#include "stm32756g_eval_io.h"
#include "stm32756g_eval_sd.h"
#if defined(SB_BRD_STMICRO_STM32746GEVAL) 
#include "stm32f7xx_ll_fmc.h"
#include "stm32f7xx_hal_sdram.h"
#include "stm32f7xx_hal_sram.h"
#include "stm32756g_eval_sdram.h"
#include "stm32756g_eval_sram.h"
#else
#include "fmc.h"
#endif
#endif
#endif

#if defined(SB_BRD_STMICRO_STM32U575IEV)
#include "stm32u5xx_hal.h"
#include "stm32u575i_eval.h"
#include "stm32u575i_eval_sram.h"
#include "stm32u575i_eval_io.h"
#include "stm32u575i_eval_sd.h"
#endif

#endif /* SMX_ARMDEFS_H */
