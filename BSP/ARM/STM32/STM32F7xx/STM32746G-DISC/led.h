/*
* led.h                                                     Version 6.0.0
*
* Defines for LEDs LD1 to LD4 on STMicro STM32746G-DISC board.
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

#ifndef SMX_LED_H
#define SMX_LED_H

#define SB_LED_NUM_7SEG    0
#define SB_LED_NUM_IN_ROW  4

/* Patterns */

#define LED_PATTERN_NONE     0x0
#define LED_PATTERN_ALL      0xF
#define LED_PATTERN_ODD      0x5
#define LED_PATTERN_EVEN     0xA

#endif /* SMX_LED_H */
