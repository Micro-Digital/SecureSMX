/*
* fp.h                                                      Version 6.0.0
*
* File portal header file for FatFs.
*
* Copyright (c) 2025-2026 Micro Digital Inc.
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

#ifndef FS_FP_H
#define FS_FP_H

#include "ff.h"

#if FP_PORTAL

enum fatfs_api {F_MOUNT, F_OPEN, F_CLOSE, F_READ, F_WRITE, F_UNLINK};

#define FP_SSLOT     6           /* file portal server pmsg region slot */
#define FP_CTMO      5000        /* file portal csem timeout in ticks <1> */
#define FP_STMO      120         /* file portal ssem timeout in ticks <2> */
#define PM_THDRSZ    (sizeof(TPMH) + sizeof(FPSH)) /* pmsg total header size */
#define PM_BUFSZ     1024                          /* pmsg buffer size */
#define PM_BLKSZ     (PM_THDRSZ + PM_BUFSZ)        /* pmsg block size */

typedef struct FPSH {   /* FILE PORTAL SERVICE HEADER */
   fatfs_api   fid;           /* function ID */
   u32         p1;            /* parameter 1 */
   u32         p2;            /* parameter 2 */
   u32         p3;            /* parameter 3 */
   u32         p4;            /* parameter 4 */
   u32         ret;           /* return value */
   void*       caller;        /* caller addr (debug) */
} FPSH;

#ifdef __cplusplus
extern "C" {
#endif

bool fp_init(u8 ssn);
bool fp_exit(void);

/* portal shell function prototypes */
FRESULT fp_mount(FATFS* fs, const TCHAR* path, BYTE opt, TPCS* pch);
FRESULT fp_open(FIL** fp, const TCHAR* path, BYTE mode, TPCS* pch);
FRESULT fp_close(FIL* fp, TPCS* pch);
FRESULT fp_read(FIL* fp, void* buff, UINT btr, UINT* br, TPCS* pch);
FRESULT fp_write(FIL* fp, const void* buff, UINT btw, UINT* bw, TPCS* pch);
FRESULT fp_unlink(const TCHAR* path, TPCS* pch);

#ifdef __cplusplus
}
#endif

#endif /* FP_PORTAL */

/* Notes:
   1. Set longer than FatFs and SD driver timeouts.
   2. Make as short as possible to avoid wasting server bandwidth if a client 
      hangs up.
*/

#endif /* FS_FP_H */