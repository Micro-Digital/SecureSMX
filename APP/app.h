/*
* app.h                                                     Version 5.4.0
*
* Application header file.
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