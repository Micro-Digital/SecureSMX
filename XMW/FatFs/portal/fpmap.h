/*
* fpmap.h                                                   Version 5.4.0
*
* File portal shell function map file for FatFs. Include before FatFs calls.
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
