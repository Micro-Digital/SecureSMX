/*
* fpcli.c                                                   Version 5.4.0
*
* File portal client for FatFs
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

#include "smx.h"
#include "fp.h"

#if FP_PORTAL
#if SMX_CFG_SSMX
#pragma section_prefix = ".fpd"  /* file portal demo sectors */
#include "xapiu.h"
#endif

/*===========================================================================*
*                          FatFs Portal Shells                               *
*===========================================================================*/
#define PFAIL 100 /* portal failure must be > maximum FRESULT error number */ 

FRESULT fp_mount(FATFS* fs, const TCHAR* path, BYTE opt, TPCS* pch)
{
   mp_PTL_CALLER_SAV();
   FPSH* shp = (FPSH*)pch->shp;
   char* mdp = (char*)pch->mdp; 
   strcpy(mdp, (char const*)path);
   mp_SHL3(F_MOUNT, (u32)fs, (u32)mdp, (u32)opt, PFAIL);
   mp_TPortalCall(pch, FP_CTMO);
   return (FRESULT)shp->ret;
}

#pragma diag_suppress=pe550
FRESULT fp_open(FIL** fpp, const TCHAR* path, BYTE mode, TPCS* pch)
{
   mp_PTL_CALLER_SAV();
   FPSH* shp = (FPSH*)pch->shp;
   char* mdp = (char*)pch->mdp; 
   strcpy(mdp, (char const*)path);
   mp_SHL3(F_OPEN, 0, (u32)mdp, (u32)mode, PFAIL); /* server loads fp */
   mp_TPortalCall(pch, FP_CTMO);
   *fpp = (FIL*)shp->p1;
   return (FRESULT)shp->ret;
}

FRESULT fp_close(FIL* fp, TPCS* pch)
{
   mp_PTL_CALLER_SAV();
   FPSH* shp = (FPSH*)pch->shp;
   mp_SHL1(F_CLOSE, (u32)fp, PFAIL);
   mp_TPortalCall(pch, FP_CTMO);
   return (FRESULT)shp->ret;
}

FRESULT fp_read(FIL* fp, void* buff, UINT btr, UINT* br, TPCS* pch)
{
   mp_PTL_CALLER_SAV();
   FPSH* shp = (FPSH*)pch->shp;
   mp_SHL4(F_READ, (u32)fp, (u32)pch->mdp, 0, 0, PFAIL); /*<1>*/
   mp_TPortalReceive(pch, (u8*)buff, (u32)btr, FP_CTMO);
   *br = pch->mhp->cmpsz;
   return (FRESULT)shp->ret;
}

FRESULT fp_write(FIL* fp, const void* buff, UINT btw, UINT* bw, TPCS* pch)
{
   mp_PTL_CALLER_SAV();
   FPSH* shp = (FPSH*)pch->shp;
   mp_SHL4(F_WRITE, (u32)fp, (u32)pch->mdp, 0, 0, PFAIL); /*<1>*/
   mp_TPortalSend(pch, (u8*)buff, (u32)btw, FP_CTMO);
   *bw = pch->mhp->cmpsz;
   return (FRESULT)shp->ret;
}

FRESULT fp_unlink(const TCHAR* path, TPCS* pch)
{
   mp_PTL_CALLER_SAV();
   FPSH* shp = (FPSH*)pch->shp;
   char* mdp = (char*)pch->mdp; 
   strcpy(mdp, (char const*)path);
   mp_SHL1(F_UNLINK, (u32)mdp, PFAIL);
   mp_TPortalCall(pch, FP_CTMO);
   return (FRESULT)shp->ret;
}

/* Notes:
   1. Uses p4 in pmsg service header for bytes read/written counter.
*/
#endif /* FP_PORTAL */
