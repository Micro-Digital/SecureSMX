/*
* fp.h                                                      Version 5.4.0
*
* File portal header file for FatFs.
*
* Copyright (c) 2025 Micro Digital Inc.
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

#ifndef FS_FP_H
#define FS_FP_H

#include "ff.h"

#if FP_PORTAL

enum fatfs_api {F_MOUNT, F_OPEN, F_CLOSE, F_READ, F_WRITE, F_UNLINK};

#define FP_SSLOT     6           /* file portal server pmsg region slot */
#define FP_CTMO      5000        /* file portal csem timeout in ticks <1> */
#define FP_STMO      4000        /* file portal ssem timeout in ticks <1> */
#define MSEC         0x80000000  /* convert ticks to msec */
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
void fp_exit(void);

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
*/

#endif /* FS_FP_H */