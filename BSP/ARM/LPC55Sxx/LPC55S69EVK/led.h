/*
* led.h                                                     Version 5.4.0
*
* Defines for the RGB LED on the NXP LPC55S69-EVK Board.
*
* Copyright (c) 2006-2020 Micro Digital Inc.
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

#ifndef SMX_LED_H
#define SMX_LED_H

#define SB_LED_NUM_7SEG    0
#define SB_LED_NUM_IN_ROW  7

/* Patterns */

#define LED_PATTERN_NONE     0x0
#define LED_PATTERN_ALL      0x7
#define LED_PATTERN_ODD      0x5
#define LED_PATTERN_EVEN     0x2

#endif /* SMX_LED_H */
