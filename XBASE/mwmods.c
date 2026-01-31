/*
* mwmods.c                                                  Version 6.0.0
*
* Initialization, exit, and other routines for middleware modules
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
#include "xsmx.h"
#include "main.h"

#if CP_PORTAL
#include "cprtl.h"
#endif

#if defined(MW_FATFS)
#include "fp.h"
#include "ff_gen_drv.h"
#include "sd_diskio_dma_rtos.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MW_FATFS)
bool fatfs_init(void);
bool fatfs_exit(void);
#endif

/* External Functions */

/* Global Variables */

#ifdef __cplusplus
}
#endif


/******* mw_modules_init()
* This routine initializes middleware modules. It is called from ainit().
***********************************************************************/

bool mw_modules_init(void)
{
  #if defined(MW_FATFS)
   if (!fatfs_init()) return(false);
   #if FP_PORTAL
   if (!fp_init(FP_SSLOT)) return(false);
   #endif
  #endif
  #if defined(MW_LWIP)
   if (!lwip_init()) return(false);
  #endif

   return(true);
}


/******* mw_modules_exit()
* This routine exits middleware modules. It is called from aexit().
***********************************************************************/

bool mw_modules_exit(void)
{
  #if defined(MW_LWIP) && SMX_CFG_SSMX
   if (!lwip_exit()) return(false);
  #endif
  #if defined(MW_FATFS) && FP_PORTAL
   if (!fp_exit()) return(false);
  #endif

   return(true);
}


#if defined(MW_FATFS)

#include "cmsis_os2.h"
#define READ_CPLT_MSG      (uint32_t) 1
#define WRITE_CPLT_MSG     (uint32_t) 2
extern osMessageQueueId_t SDQueueID;

/*
*  BSP_SD_WriteCpltCallbackLSRMain()
*
*  LSR for ISR callback.
*/
void BSP_SD_WriteCpltCallbackLSRMain(u32)
{
  const u16 msg = WRITE_CPLT_MSG;
  smx_PipePutPktWait((PICB*)SDQueueID, (void*)&msg, SMX_TMO_NOWAIT);
}

/*
*  BSP_SD_ReadCpltCallbackLSRMain()
*
*  LSR for ISR callback.
*/
void BSP_SD_ReadCpltCallbackLSRMain(u32)
{
  const u16 msg = READ_CPLT_MSG;
  smx_PipePutPktWait((PICB*)SDQueueID, (void*)&msg, SMX_TMO_NOWAIT);
}

#pragma default_variable_attributes = @ ".fs.bss"
LCB_PTR  BSP_SD_WriteCpltCallbackLSR;
LCB_PTR  BSP_SD_ReadCpltCallbackLSR;
FATFS    SDFatFs;       /* File system object for SD card logical drive */
char     SDPath[4];     /* SD card logical drive path */

#if SMX_CFG_SSMX
/* fs partition heap definitions */
#pragma default_variable_attributes = @ ".fs.rodata"
u32 const   fs_binsz[] = /* single bin heap */
/* bin  0       end */
      {24, -1};
#pragma section = "fs_heap"
#pragma default_variable_attributes = @ ".fs.bss"
HBCB        fs_bin[(sizeof(fs_binsz)/4)-1];  /* fs_heap bins */
#if EH_STATS
u32         fs_bnum[(sizeof(fs_binsz)/4)-1]; /* fs_heap number of chunks per bin */
u32         fs_bsum[(sizeof(fs_binsz)/4)-1]; /* fs_heap sum of chunk sizes per bin */
#endif
u32         fs_hn;                           /* fs_heap number */
EHV         fs_hv;                           /* fs_heap variable struct */
#pragma default_variable_attributes = 
#endif /* SMX_CFG_SSMX */

/***************************** FatFs Routines *******************************
* This code initializes FatFs.
*****************************************************************************/

bool fatfs_init(void)
{
   /* SD controller and card init done in demo instead in case card not inserted. */
   //if (BSP_SD_Init() != MSD_OK)
   //   return false;

   /* link the micro SD disk I/O driver */
   if (FATFS_LinkDriver(&SD_Driver, SDPath) != 0)
      return false;

   /* create trusted LSRs for FatFs */
   if ((BSP_SD_WriteCpltCallbackLSR = smx_LSRCreate(BSP_SD_WriteCpltCallbackLSRMain,
     SMX_FL_TRUST, "SD_WriteCpltLSR", NULL, 0, &BSP_SD_WriteCpltCallbackLSR)) == NULL)
      return false;

   if ((BSP_SD_ReadCpltCallbackLSR = smx_LSRCreate(BSP_SD_ReadCpltCallbackLSRMain,
     SMX_FL_TRUST, "SD_ReadCpltLSR", NULL, 0, &BSP_SD_ReadCpltCallbackLSR)) == NULL)
      return false;

  #if SMX_CFG_SSMX
   /* create fs heap and initialize it */
   #pragma section = "fs_heap"
   memset((void*)&fs_hv, 0, sizeof(EHV));
   fs_hn = smx_HeapInit(__section_size("fs_heap"), 0, (u8*)__section_begin("fs_heap"),
                        &fs_hv, (u32*)fs_binsz, (HBCB*)fs_bin, EH_NORM, "fs_hp");
   if (fs_hn == -1)
   {
      smx_EM(SMXE_HEAP_INIT_FAIL, 2);
      sb_DEBUGTRAP();
      return false;
   }
   fs_hv.mgr = NULL;
   #if defined(SMX_DEBUG)
   fs_hv.mode.fl.fill = OFF;
   #endif
   #if EH_STATS
   fs_hv.bnump = (u32*)&fs_bnum;
   fs_hv.bsump = (u32*)&fs_bsum;
   #endif
  #endif /* SMX_CFG_SSMX */

   return true;
}


bool fatfs_exit(void)
{
   /* unlink the micro SD disk I/O driver */
   if (FATFS_UnLinkDriver(SDPath) != 0)
      return false;

   return true;
}

#endif /* MW_FATFS */
