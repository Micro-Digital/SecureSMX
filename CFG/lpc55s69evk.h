/*
* lpc55s69evk.h                                             Version 5.4.0
*
* Preinclude File for NXP LPC55S69-EVK board.
*
* Specifies global settings, such as the CPU type to ensure that all
* SMX libraries and the application are built consistently.
* The IDE project points to a prefix/preinclude file that includes it.
*
* Copyright (c) 2009-2025 Micro Digital Inc.
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
***********************************************************************/

/*
* Define the target board and processor so the code can check them.
*/
#define SB_BRD_NXP_LPC55S69_EVK
#define SB_CPU_LPC55S69
#define SB_CPU_LPC55S6X
#define SB_CPU_LPC55SXX

#define SB_CPU_ARMM
#define SB_CPU_ARMM7 0
#define SB_CPU_ARMM8 1

/* Special defines needed by NXP BSP. */
#define CPU_LPC55S69JBD100_cm33_core0  /* SMX running on core0 */

/*
* Set target processor type consistently in all projects. No pragma for it.
*/

