/*
* app.h                                                     Version 6.0.0
*
* Application header file.
*
* Copyright (c) 1989-2026 Micro Digital Inc.
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

#ifndef APP_H
#define APP_H

/* function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

void leddemo_init(void);
void leddemo_exit(void);

#if SB_LCD_DEMO
void lcddemo_init(void);
void lcddemo_exit(void);
#endif

#if SB_FPU_DEMO
void fpudemo_init(void);
void fpudemo_exit(void);
#endif

#if MW_FATFS_DEMO
void fpdemo_init(void);
void fpdemo_exit(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* APP_MAIN_H */