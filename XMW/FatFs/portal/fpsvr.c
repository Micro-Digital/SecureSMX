/*
* fpsvr.c                                                   Version 5.4.0
*
* File portal server for FatFs.
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
#include "bsp.h"
#include "fp.h"

#if FP_PORTAL
#if SMX_CFG_SSMX
#pragma section_prefix = ".fs"   /* file system regions */
#include "xapiu.h"
#endif

extern TPCS  fpcli_fpd;          /* file portal demo client struct */

TPCS*    fpcli_lst[] = {&fpcli_fpd};         /* permitted client list for FatFs */
u32      fpcli_lstsz = sizeof(fpcli_lst)/4;  /* size of fpcli_lst */
TPSS     fpsvr;                              /* file portal server struct */
TCB_PTR  fp_task;                            /* file portal task */
FIL      fs;                                 /* file structure */

/*
*  fp_main()
*
*  file portal task main function
*/
void fp_main(u32)
{
   mp_TPortalServer(&fpsvr, FP_STMO); /* see tportls.c */
}

/*
*  fp_server()
*
*  FatFs portal server -- called by mp_TPortalCallServerFunc() -- see tportls.c
*/
void fp_server(TPSS* ph)
{
   TPMH* mhp = (TPMH*)ph->mhp;   /* msg header pointer */
   FPSH* shp = (FPSH*)ph->shp;   /* service header pointer */

   switch (shp->fid) /*<1>*/
   {
      case F_MOUNT:
         shp->ret = f_mount((FATFS*)shp->p1, (const TCHAR*)shp->p2, (BYTE)shp->p3);
         break;
      case F_OPEN:
         shp->ret = (u32)f_open(&fs, (const char*)shp->p2, (BYTE)shp->p3);
         shp->p1 = (u32)&fs;
         break;
      case F_CLOSE:
         shp->ret = f_close((FIL*)shp->p1);
         break;
      case F_READ:
         shp->ret = f_read((FIL*)shp->p1, (void*)shp->p2, (UINT)PM_BUFSZ, (UINT*)((u32)&(shp->p4)));
         if (shp->ret != FR_OK)
             mhp->errno = SPE_TRANS_INC;  /* alert mp_TPortalReceive() transfer incomplete */
         break;
      case F_WRITE:
         shp->ret = f_write((FIL*)shp->p1, (const void*)shp->p2, (UINT)PM_BUFSZ, (UINT*)((u32)&(shp->p4)));
         if (shp->ret != FR_OK)
             mhp->errno = SPE_TRANS_INC;  /* alert mp_TPortalSend() transfer incomplete */
         break;
      case F_UNLINK:
         shp->ret = f_unlink((const TCHAR*)shp->p1);
         break;
      default:
         mp_PortalEM((PS*)ph, SPE_INV_FCT, &mhp->errno);
   }
}

/* Replaces stub in FatFs */
DWORD get_fattime(void)
{
   DATETIME t;
   u32 dttm = 0;  /* date/time (hi16/lo16) each according to FAT spec */

   sb_GetLocalTime(&t);
   dttm |= (t.wYear << 25);
   dttm |= (t.wMonth << 21);
   dttm |= (t.wDay << 16);
   dttm |= (t.wHour << 11);
   dttm |= (t.wMinute << 5);
   dttm |= (t.wSecond / 2);

   return dttm;
}

#if SMX_CFG_SSMX
#pragma default_function_attributes = @ ".pb1.text"
#include "xapip.h"
#endif

/*
*  fp_init()
*
*  Initialize file portal server.
*/
bool fp_init(u8 ssn)
{
   TPSS*  psh  = &fpsvr;
   TPCS** pclp = (TPCS**)&fpcli_lst;

   /* create file portal */
   if (!mp_TPortalCreate(&psh, pclp, fpcli_lstsz, ssn, "fportal", "fp sxchg"))
      return false;

   /* create file portal task */
   fp_task = smx_TaskCreate((FUN_PTR)fp_main, PRI_NORM, 0, SMX_FL_UMODE, "fptask", 0, NULL);
   if (fp_task == NULL)
   {
      mp_TPortalDelete(psh, pclp, fpcli_lstsz);
      return false;
   }
   else
   {
      mp_MPACreate(fp_task, &mpa_tmplt_fs, 0x3F, 8);
      smx_TaskStart(fp_task);
      psh->stask = fp_task;
      psh->sid  = FP;
      psh->ssid = 0;
      return true;
   }
}

/*
*  fp_exit()
*
*  Shut down portal server.
*/
void fp_exit(void)
{
   if (fpsvr.stask != NULL)
   {
      mp_TPortalDelete(&fpsvr, (TPCS**)fpcli_lst, fpcli_lstsz);
      smx_TaskDelete(&fpsvr.stask);
   }
}

#endif /* FP_PORTAL */
