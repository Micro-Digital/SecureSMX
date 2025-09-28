/*
* armdefs.h                                                 Version 5.4.0
*
* ARM main definitions file. Contains:
*
* 1. Includes of the appropriate processor .h files for I/O register
*    definitions and function prototypes.
* 2. Misc processor architecture settings.
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
