/*
* lpc55sxx.h                                                Version 6.0.0
*
* Defines for NXP LPC55Sxx
* All are similar so just use conditionals here for differences.
*
* Copyright (c) 2020 Micro Digital Inc.
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
* Author: Xingsheng Wan
*
*****************************************************************************/

#ifndef LPC55SXX_H
#define LPC55SXX_H

#include "fsl_clock.h"
#include "fsl_gpio.h"
#include "fsl_usart.h"
#include "fsl_iocon.h"
#include "fsl_power.h"

/* PLL Setup Values and Peripheral Bus Speed Divider */
/* Please keep the same value of these macro in the xram.mac */

#if defined(SB_BRD_NXP_LPC55S69_EVK)
#define XTAL_CLK        16000000                        /* Crystal Freq */
#define PLL0_M          150                             /* Multiplier of the XTAL Clock */
#define PLL0_N          8
#define PLL0_P          1
#define PLL0_CLK        ((XTAL_CLK / (2 * PLL0_N * PLL0_P)) * PLL0_M)           /* The PLL1 clock */
#define CPU_FREQ        PLL0_CLK                        /* The clock for CPU */
#define PERIPH_FREQ     PLL0_CLK

#else
#error Define XTAL_CLK, PLL1_MUL, etc. in lpc43xx.h for your board.
#endif

#endif /* LPC55SXX_H */
