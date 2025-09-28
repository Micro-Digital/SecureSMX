/*
* mpudef.h                                                  Version 5.4.0
*
* MPU definitions.
*
* Copyright (c) 2016-2025 Micro Digital Inc.
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
* Author: Ralph Moore
*
*****************************************************************************/

#ifndef MP_MPUDEF_H
#define MP_MPUDEF_H

/* MPA template externs <1> */
extern MPA mpa_tmplt_cp;
extern MPA mpa_tmplt_idle;
extern MPA mpa_tmplt_init;
extern MPA mpa_tmplt_tinit;
extern MPA mpa_tmplt_fpu;
extern MPA mpa_tmplt_fs;
extern MPA mpa_tmplt_fpd;
extern MPA mpa_tmplt_lcd;
extern MPA mpa_tmplt_led;
extern MPA mpa_tmplt_opcon;

/* IRQ permission table externs */
extern const IRQ_PERM sb_irq_perm_uart[];

/* heap numbers */
extern u32 mheap_hn; /* main heap number */
extern u32 ucom_hn;  /* ucom heap number */
extern u32 cp_hn;    /* cp heap number */

/* Notes:
   1. These are defined by the application and are put here for convenience,
      to save defining and including an mpa.h file. There are no details in
      these, and unused ones can be here, so one can just add the superset
      of all externs used by all apps. This saves adding an include to various
      files and adding a path to the app dir to all middleware library projects.
*/

#endif /* MP_MPUDEF_H */