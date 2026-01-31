/*
* fpmap.h                                                   Version 6.0.0
*
* File portal shell function map file for FatFs. Include before FatFs calls.
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

#ifndef FS_FPMAP_H
#define FS_FPMAP_H

#if FP_PORTAL

#define f_mount(fs, path, opt)         fp_mount(fs, path, opt, FPD_PCH)
#define f_close(fp)                    fp_close(fp, FPD_PCH)
#define f_open(fp, path, mode)         fp_open(fp, path, mode, FPD_PCH)
#define f_read(fp, buff, btr, br)      fp_read(fp, buff, btr, br, FPD_PCH)
#define f_write(fp, buff, btw, bw)     fp_write(fp, buff, btw, bw, FPD_PCH)
#define f_unlink(path)                 fp_unlink(path, FPD_PCH)

#endif /* FP_PORTAL */

/* Notes:
   1. The FPD_PCH macro permits adding a needed parameter to the portal shell
      functions that is not in the FatFs functions. It must be defined ahead of
      FatFs service calls along with including this header file. For example: 
                  #define  FPD_PCH &fdcli_fpd 
      where fdcli_fpd is the tunnel portal client structure for fpdemo.
*/

#endif /* FS_FPMAP_H */
