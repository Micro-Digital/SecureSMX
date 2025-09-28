/*
* xcfg.h                                                    Version 5.4.0
*
* smx configuration constants
*
* See the SMX Quick Start for documentation of these settings.
*
* Copyright (c) 1989-2025 Micro Digital Inc.
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

#ifndef SMX_XCFG_H
#define SMX_XCFG_H

#define SMX_CFG_DIAG             1  /* enable special diagnostics (.inc)<1> */
#define SMX_CFG_EVB              1  /* enable event buffer <1> */
#define SMX_CFG_PROFILE          1  /* enable profiling (.inc)<1> */
#define SMX_CFG_STACK_SCAN       1

#define SMX_CFG_SSMX             1  /* enable SecureSMX (.inc)<1> */

#if SMX_CFG_SSMX
#define SMX_CFG_MPU_ENABLE       1  /* enable MPU and umode (.inc)<1><2> */
#define SMX_CFG_PORTAL           1  /* enable portals */
#define SMX_CFG_RTLIM            1  /* enable runtime limits (.inc)<1> */
#define SMX_CFG_TOKENS           1
#else
#define SMX_CFG_MPU_ENABLE       0  /* keep 0 */
#define SMX_CFG_PORTAL           0  /* keep 0 */
#define SMX_CFG_RTLIM            0  /* keep 0 */
#define SMX_CFG_TOKENS           0  /* keep 0 */
#endif

#if SMX_CFG_RTLIM
#define SMX_IDLE_RTLIM           2  /* idle task passes per runtime limit frame */
#endif

#define SMX_LOCK_NEST_LIMIT      5  /* maximum nesting of task locking (smx_lockctr) */

#if SMX_CFG_PROFILE
#define SMX_RTCB_SIZE            3  /* number of runtime counter samples in smx_rtcb[][] */
#define SMX_RTC_FRAME          100  /* rtc frame in ticks */
#else
#define SMX_RTCB_SIZE            0  /* keep 0 */
#define SMX_RTC_FRAME            0  /* keep 0 */
#endif

/* standard priority levels */
enum SMX_PRIORITIES {PRI_MIN, PRI_LO, PRI_NORM, PRI_HI, PRI_MAX, PRI_SYS}; /*<3>*/

#define SMX_PRI_NUM              (PRI_SYS + 1)

/* Notes:
   1. Also change in the .inc file for your processor and compiler (e.g. xarmm_iar.inc).
   2. SMX_CFG_MPU_ENABLE is necessary to enable the MPU and for service calls to
      use the SVC exception. Otherwise the MPU is off, service calls are direct, 
      and all code runs in pmode. This is intended as an aid to avoid MMFs and 
      to be able to see the full call stack when debugging. It serves as a half
      step to convert pmode code to umode.
   3. PRI_SYS is reserved for smx use -- do not add priority levels above it.
*/
#endif /* SMX_XCFG_H */

