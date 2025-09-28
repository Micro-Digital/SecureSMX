/*
* lpc55sxx.h                                                Version 5.4.0
*
* Defines for NXP LPC55Sxx
* All are similar so just use conditionals here for differences.
*
* Copyright (c) 2020 Micro Digital Inc.
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
