/*
* mpudef.h                                                  Version 6.0.0
*
* MPU definitions.
*
* Copyright (c) 2016-2026 Micro Digital Inc.
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