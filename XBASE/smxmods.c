/*
* smxmods.c                                                 Version 6.2.0
*
* Initialization, exit, and other routines for SMX component modules
* (e.g. smxFS, smxNS, smxUSBH). This necessary code was pulled from
* the demos so that the demo files could be completely discarded.
*
* This file contains sections separated by ***'s for:
*
*    - smxFLog routines
*    - smxFFS routines
*    - smxFS routines
*    - smxNS routines
*    - smxUSBD routines
*    - smxUSBH routines
*    - smxWiFi routines
*
* Search for USER comments to find places that may need your attention.
*
* Copyright (c) 1995-2026 Micro Digital Inc.
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
* Authors: various
*
*****************************************************************************/

#include "bbase.h"
#include "main.h"
#if CP_PORTAL
#include "cprtl.h"
#endif
#ifdef SMXFLOG
#include "smxflog.h"
#endif
#ifdef SMXFFS2
#include "smxffs.h"
#if SFF_DRV_NANDFLASH
#include "fdnand.h"
#endif
#if SFF_DRV_NORFLASH
#include "fdnor.h"
#endif
#if SFF_DRV_MMCSD
#include "fdmsd.h"
#endif
#if SFF_DRV_USB
#include "fdusb.h"
#endif
#endif
#ifdef SMXFS
#include "smxfs.h"
#endif
#ifdef SMXNETX
#include "rtip.h"
#endif
#ifdef SMXNS
#include "smxns.h"
#endif
#ifdef SMXUSBD
#include "smxusbd.h"
#endif
#ifdef SMXUSBH
#include "smxusbh.h"
#endif
#ifdef SMXWIFI
#include "smxwifi.h"
#endif

#if SMX_CFG_SSMX && defined(__IAR_SYSTEMS_ICC__)
#pragma section_prefix = ".sys"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SMXAWARE_LIVE
bool smxaware_live_init(void);
bool smxaware_live_exit(void);
#endif

#ifdef SMXFLOG
bool smxflog_init(void);
bool smxflog_exit(void);
#endif

#ifdef SMXFFS2
bool smxffs2_init(void);
bool smxffs2_exit(void);
#endif

#ifdef SMXFS
bool smxfs_init(void);
bool smxfs_exit(void);
#endif

#ifdef SMXNS
bool smxns_init(void);
bool smxns_exit(void);
#endif

#ifdef SMXUSBD
bool smxusbd_init(void);
bool smxusbd_exit(void);
#endif

#ifdef SMXUSBH
bool smxusbh_init(void);
bool smxusbh_exit(void);
#endif

#ifdef SMXWIFI
bool smxwifi_init(void);
bool smxwifi_exit(void);
#endif

#ifdef __cplusplus
}
#endif


/******* smx_modules_init()
*
* This routine initializes smx add-on modules (products).
* It is called from ainit().
*
***********************************************************************/

bool smx_modules_init(void)
{
  #if defined(SMXFLOG)
   if (!smxflog_init()) return(false);
  #endif
  #if defined(SMXFFS2)
   if (!smxffs2_init()) return(false);
  #endif
  #if defined(SMXFS)
   if (!smxfs_init()) return(false);
  #endif
  #if defined(SMXWIFI)
   if (!smxwifi_init()) return(false);
  #endif
  #if defined(SMXUSBH)
   if (!smxusbh_init()) return(false);
  #endif
  #if defined(SMXUSBD)
   if (!smxusbd_init()) return(false);
  #endif
  #if defined(SMXNS)
   if (!smxns_init()) return(false);
  #endif

   return(true);
}


/******* smx_modules_exit()
*
* This routine exits (shuts down) smx add-on modules (products).
* It is called from aexit().
*
***********************************************************************/

bool smx_modules_exit(void)
{
  #if defined(SMXNS)
   if (!smxns_exit()) return(false);
  #endif
  #if defined(SMXUSBD)
   if (!smxusbd_exit()) return(false);
  #endif
  #if defined(SMXUSBH)
   if (!smxusbh_exit()) return(false);
  #endif
  #if defined(SMXWIFI)
   if (!smxwifi_exit()) return(false);
  #endif
  #if defined(SMXFS)
   if (!smxfs_exit()) return(false);
  #endif
  #if defined(SMXFFS2)
   if (!smxffs2_exit()) return(false);
  #endif
  #if defined(SMXFLOG)
   if (!smxflog_exit()) return(false);
  #endif

   return(true);
}


#ifdef SMXFLOG

/**************************** smxFLog Routines ******************************
* This code initializes smxFLog.
*****************************************************************************/

bool smxflog_init(void)
{
   /* Initialize smxFLog */
// if(sfl_Init(SFL_INIT_ERASE_ALL) == 0)  /* uncomment to erase flash the first time */
   if(sfl_Init(SFL_INIT_CHECK) == 0)
      return true;
   else
      return false;
}


bool smxflog_exit(void)
{
   sfl_Release();
   return(true);
}

#endif /* SMXFLOG */


#ifdef SMXFFS2

/**************************** smxFFS2 Routines ******************************
* This code initializes smxFFS2.
*****************************************************************************/

bool smxffs2_init(void)
{
   /* Initialize smxFFS */
   int iID = 0;
   if (sff_init() != SB_PASS)
   {
      sb_ConWriteString(0,1,SB_CLR_LIGHTRED,SB_CLR_BLACK,!SB_CON_BLINK,"smxFFS Init Failed");
      return(false);
   }
   /* Register built-in device drivers. */
  #if SFF_DRV_NANDFLASH
   sff_devreg(sfs_GetNAND0Interface(), iID++);
   //sff_devreg(sfs_GetNAND1Interface(), iID++);
  #endif
  #if SFF_DRV_NORFLASH
   sff_devreg(sfs_GetNOR0Interface(), iID++);
   //sff_devreg(sfs_GetNOR1Interface(), iID++);
  #endif
  #if SFF_DRV_MMCSD
   sff_devreg(sfs_GetMMCSD0Interface(), iID++);
   //sff_devreg(sfs_GetMMCSD1Interface(), iID++);
  #endif
  #if SFF_DRV_USB
   sff_devreg(sfs_GetUSB0Interface(), iID++);
   //sff_devreg(sfs_GetUSB1Interface(), iID++);
  #endif

   return(true);
}


bool smxffs2_exit(void)
{
   sff_exit();
   return(true);
}

#endif /* SMXFFS2 */


#ifdef SMXFS

#if SFS_DRV_USB && defined(SMXUSBH) && SU_PORTAL && SFS_PORTAL
extern int g_FirstUSBDisk;
extern int g_LastUSBDisk;
#endif

/**************************** smxFS Routines ********************************
* This code initializes smxFS.
*****************************************************************************/

bool smxfs_init(void)
{
   /* Initialize smxFS */
   int iID = 0;
   if(sfs_init() == SB_FAIL)
      return(false);

   /* Register built-in device drivers. */
  #if SFS_DRV_WINDISK
   sfs_devreg(sfs_GetWinDisk0Interface(), iID++);
   //sfs_devreg(sfs_GetWinDisk1Interface(), iID++);
  #endif
  #if SFS_DRV_RAMDISK
   sfs_devreg(sfs_GetRAM0Interface(), iID++);
   //sfs_devreg(sfs_GetRAM1Interface(), iID++);
  #endif
  #if SFS_DRV_USB
   #if defined(SMXUSBH) && SU_PORTAL && SFS_PORTAL
   g_FirstUSBDisk = iID;
   #endif
   sfs_devreg(sfs_GetUSB0Interface(), iID++);
   //sfs_devreg(sfs_GetUSB1Interface(), iID++);
   #if defined(SMXUSBH) && SU_PORTAL && SFS_PORTAL
   g_LastUSBDisk = iID - 1;
   #endif
  #endif
  #if SFS_DRV_MMCSD
   sfs_devreg(sfs_GetMMCSD0Interface(), iID++);
   //sfs_devreg(sfs_GetMMCSD1Interface(), iID++);
  #endif
  #if SFS_DRV_NANDFLASH
   sfs_devreg(sfs_GetNAND0Interface(), iID++);
   //sfs_devreg(sfs_GetNAND1Interface(), iID++);
  #endif
  #if SFS_DRV_NORFLASH
   sfs_devreg(sfs_GetNOR0Interface(), iID++);
   //sfs_devreg(sfs_GetNOR1Interface(), iID++);
  #endif
  #if SFS_DRV_ATA
   sfs_devreg(sfs_GetATA0Interface(), iID++);
   //sfs_devreg(sfs_GetATA1Interface(), iID++);
  #endif
  #if SFS_DRV_CF
   sfs_devreg(sfs_GetCFInterface(), iID++);
  #endif
  #if SFS_DRV_DOC
   sfs_devreg(sfs_GetDOC0Interface(), iID++);
   //sfs_devreg(sfs_GetDOC1Interface(), iID++);
  #endif
  #ifdef SMXAWARE
   smxaware_smxfs_init();
  #endif

   return(true);
}


bool smxfs_exit(void)
{
   sfs_exit();
   return(true);
}

#endif /* SMXFS */


#ifdef SMXNS

/**************************** smxNS Routines ********************************
* This code initializes smxNS.
*****************************************************************************/

bool smxns_init(void)
{
   /* Initialize the smxNS TCP/IP stack */
   Nprintf("smxNS Init\n");
   if (Ninit() < 0)
   {
      Nprintf("smxNS Init Failed\n");
      return(false);
   }
  #ifdef SMXAWARE
   smxaware_smxns_init();
  #endif

   return(true);
}


bool smxns_exit(void)
{
   Nterm();
   return(true);
}

#endif /* SMXNS */


#ifdef SMXUSBD

/**************************** smxUSBD Routines ******************************
* This code initializes smxUSBD device stack.
*****************************************************************************/

bool smxusbd_init(void)
{
   /* Initialize smxUSBD */
   if (sud_Initialize(SUD_ALL_MASK) == 0)
      return(false);

  #ifdef SMXAWARE
   smxaware_smxusbd_init();
  #endif

   return(true);
}


bool smxusbd_exit(void)
{
  #if SFS_PORTAL_SD && SFS_DRV_MMCSD
   /* Uninit SD disk portals for smxUSBD mass storage */
   for (int i = 0; i < MMCSD_NUM; i++)
      sfsp_sd_exit(i);
  #endif

   sud_Release();
   return(true);
}

#endif /* SMXUSBD */


#ifdef SMXUSBH

/**************************** smxUSBH Routines ******************************
* This code initializes smxUSBH host stack.
*****************************************************************************/

bool smxusbh_init(void)
{
   /* Initialize smxUSBH */
   if (su_Initialize() == 0)
      return(false);

  #ifdef SMXAWARE
   smxaware_smxusbh_init();
  #endif

   return(true);
}


bool smxusbh_exit(void)
{
   su_Release();
   return(true);
}

#endif /* SMXUSBH */


#ifdef SMXWIFI

/**************************** smxWiFi Routines ******************************
* This code initializes smxWiFi stack.
*****************************************************************************/

bool smxwifi_init(void)
{
   /* Initialize smxWiFi */
   return(swf_Init() == 0);
}


bool smxwifi_exit(void)
{
   swf_Release();
   return(true);
}

#endif /* SMXWIFI */

