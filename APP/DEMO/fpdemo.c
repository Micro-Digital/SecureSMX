/*
* fpdemo.c                                                  Version 6.0.0
*
* File portal demo for FatFs.
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
* Authors: Ralph Moore, David Moore
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"
#include "app.h"

#if MW_FATFS_DEMO

#include "fp.h"
#include "cpmap.h"
#include "fpmap.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".fpd"  /* file portal demo sectors */
#include "xapiu.h"
#endif

/* internal function prototypes */
static void fpdemo_error(const char* str, int line);
static void fpdemo_main(u32 par);
static void fpdemo_msg(const char* str, int line);
static void fpdemo_perf(void);
static void fpdemo_results(void);

extern   FATFS   SDFatFs;  /* file system struct for SD card logical drive */

/* defines */
#define  CP_CSLOT    3           /* cp pmsg region client slot */
#define  CP_PCH      &cpcli_fpd  /* client portal struct pointer for cp calls */
#define  FP_CSLOT    4           /* fp pmsg region client slot */
#define  FPD_PCH     &fpcli_fpd  /* client portal struct pointer for fp calls */
#define  FPDEMO_DLY  1000        /* msec between runs */
#define  FILENAME    "0:\\fpdemo.txt"  /* file name */
#if SMX_CFG_SSMX
#define  FILESZ      (4*PM_BUFSZ)/* file size, must be multiple of PM_BUFSZ */
#else
#define  FILESZ      (4096)      /* file size */
#endif
#define  MSG_COL     10          /* message column */
#define  PSNUM       2           /* performance sample number to average */
#define  TOP_LINE    2           /* top line of fpdemo display */

/* variables */
#if SMX_CFG_SSMX
FPCS     cpcli_fpd;        /* console portal client structure for fpdemo */
TPCS     fpcli_fpd;        /* file portal client structure for fpdemo */
FIL*     fop;              /* file struct pointer */
#else
FIL      fs;               /* file struct */
FIL*     fop = &fs;        /* file struct pointer */
#endif
char     dispbuf[80];      /* display buffer */
TCB_PTR  fpdemo;           /* file portal demo task */
SCB_PTR  fpddone;          /* file portal demo done semaphore */
bool     fpdexit;          /* fpdemo time to exit */
u32      fpdctm;           /* fpdemo calibration time measurement */
u32      fpdrtm;           /* fpdemo read time measurement */
u32      fpdtms;           /* fpdemo time measurement start */
u32      fpdwtm;           /* fpdemo write time measurement */
u32      passcnt;          /* pass count */
u32      rtext[FILESZ/4];  /* input buffer */
u32      wtext[FILESZ/4];  /* output buffer */

void fpdemo_main(u32)
{
  #if SMX_CFG_SSMX
   /* open console portal for fpdemo */
   mp_FPortalOpen(&cpcli_fpd, CP_CSLOT, 80, 1, 5, "fpd_rxchg");
   sb_ConWriteString(0,TOP_LINE, SB_CLR_LIGHTMAGENTA,SB_CLR_BLACK,!SB_CON_BLINK,"FPDEMO: ");

   /* get pmsg from mheap for file system portal */
   fpcli_fpd.pmsg = smx_PMsgGetHeap(PM_BLKSZ, (u8**)&fpcli_fpd.mhp, 
                                               FP_CSLOT, MP_DATARW, 0, NULL);
  #endif

   /* SD controller and card init */
  #if defined(SB_CPU_STM32)
  #if SMX_CFG_SSMX
   while (sbu_BSP_SD_Init() != MSD_OK)
  #else
   while (BSP_SD_Init() != MSD_OK)
  #endif
   {
      fpdemo_msg("FatFs Insert SD Card", TOP_LINE);
      smx_DelaySec(1);
      fpdemo_msg("                    ", TOP_LINE);
      smx_DelayMsec(10);
   }
  #else
   #error Implement fpdemo_main() for your CPU HAL.
  #endif

   while (!fpdexit)
   {
     #if SMX_CFG_SSMX
      /* open file system portal for fpdemo */
      mp_TPortalOpen(&fpcli_fpd, PM_BLKSZ, PM_THDRSZ, FP_CTMO, "ssem", "csem");
     #endif

      /* run fpdemo */
      fpdemo_perf();

     #if SMX_CFG_SSMX
      /* close file system portal */
      mp_TPortalClose(&fpcli_fpd);
      smx_DelayMsec(FPDEMO_DLY);
     #endif
   }

  #if SMX_CFG_SSMX
   /* end demo */
   smx_PMsgRel(&fpcli_fpd.pmsg, 0);
   fpcli_fpd.mhp = NULL;
   smx_SemSignal(fpddone);
  #endif
}

void fpdemo_perf(void)
{
   u32   bwr;  /* num bytes written/read */
   u32   i;
   bool  res;

   fpdemo_msg("Testing FatFs Performance", TOP_LINE);

   /* Register the file system object to the FatFs module */
   if (f_mount(&SDFatFs, FILENAME, 0) != FR_OK)
   {
      fpdemo_error("FatFs Initialization Error", TOP_LINE+2);
      return;
   }

   while (!fpdexit)
   {
      u32 pattern = sb_PtimeGet();  /* random seed */

      /* fill output buffer */
      for (i = 0; i < FILESZ/4; i++)
      {
         wtext[i] = pattern;
         pattern += 0x01010101;
      }

      /* create and open a new file with write access */
     #if SMX_CFG_SSMX
      if (f_open(&fop, FILENAME, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
     #else
      if (f_open(fop, FILENAME, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
     #endif
      {
         fpdemo_error("FatFs Open for Write Error", TOP_LINE+2);
         return;
      }

      /* write data to the new file and measure time */
      sb_TMStart(&fpdtms);
      res = f_write(fop, wtext, sizeof(wtext), &bwr);
     #if SMX_CFG_SSMX
      sbu_TMEnd(fpdtms, &fpdwtm, fpdctm);
     #else
      sb_TMEnd(fpdtms, &fpdwtm);
     #endif
      f_close(fop);

      if (res != FR_OK)
      {
         fpdemo_error("FatFs Write or EOF Error", TOP_LINE+2);
         return;
      }

      /* open same file with read access */
     #if SMX_CFG_SSMX
      if (f_open(&fop, FILENAME, FA_READ) != FR_OK)
     #else
      if (f_open(fop, FILENAME, FA_READ) != FR_OK)
     #endif
      {
         fpdemo_error("FatFs Open for Read Error", TOP_LINE+2);
         return;
      }

      /* clear rtext */
      u32* bp = (u32*)&rtext;
      for (i = 0; i < FILESZ/4; i++)
      {
         *bp++ = 0;
      }

      /* Read data from the text file and measure time */
      sb_TMStart(&fpdtms);
      res = f_read(fop, rtext, sizeof(rtext), &bwr);
     #if SMX_CFG_SSMX
      sbu_TMEnd(fpdtms, &fpdrtm, fpdctm);
     #else
      sb_TMEnd(fpdtms, &fpdrtm);
     #endif
      f_close(fop);
      passcnt++;

      if (res != FR_OK)
      {
         fpdemo_error("FatFs Read or EOF Error", TOP_LINE+2);
         return;
      }

      /* compare rtext to wtext */
      for (i = 0; i < FILESZ/4; i++)
      {
         if (rtext[i] != wtext[i])
         {
            fpdemo_error("FatFs Data Comparison Error", TOP_LINE+2);
            return;
         }
      }

      fpdemo_results();

      //f_unlink(FILENAME);  /* delete file */
      smx_DelayMsec(FPDEMO_DLY);
   }
   fpdemo_msg("FatFs Demo Done", TOP_LINE+2);
}

bool arrays_full = false;
u32  arp;        /* average read performance (kb/sec) */
u32  awp;        /* average write performance (kb/sec) */
u32  n = 0;
u32  m = 0;
u32  rpa[PSNUM]; /* read performance array */
u32  wpa[PSNUM]; /* write performance array */

void fpdemo_results(void)
{
   u32   nrp;              /* new read performance */
   u32   nwp;              /* new write performance */   
   char  rpbuf[12];        /* read performance buffer */
   char  wpbuf[12];        /* write performanc buffer */

   u32   j;


   if (!arrays_full)
   {
      /* fill read and write performance arrays */
      rpa[n] = (SB_CPU_HZ/fpdrtm)*(FILESZ/1024);
      wpa[n++] = (SB_CPU_HZ/fpdwtm)*(FILESZ/1024);
      if (n > PSNUM-1)
      {
         n = 0;
         arrays_full = true;
      }
   }
   else /* arrays full */
   {
      /* compute average read performance */
      for (j = 0, arp = 0; j < PSNUM; j++)
      {
         arp += rpa[j];
      }
      arp = arp/PSNUM;
      ultoa(arp, rpbuf, 10);

      /* compute average write performance */
      for (j = 0, awp = 0; j < PSNUM; j++)
      {
         awp += wpa[j];
      }
      awp = awp/PSNUM;
      ultoa(awp, wpbuf, 10);

      /* display average performances */
      strcpy(dispbuf, "R/W speed is ");
      strcat(dispbuf, rpbuf);
      strcat(dispbuf, "/");
      strcat(dispbuf, wpbuf);
      strcat(dispbuf, " KB/sec  ");
      fpdemo_msg(dispbuf, TOP_LINE+1);

      /* calculate new read performance */
      nrp = (SB_CPU_HZ/fpdrtm)*(FILESZ/1024);

      /* filter out abnormal read performance */
      if (nrp > arp*9/10 && nrp < arp*11/10)
      {
         /* save new read performance */
         rpa[n++] = nrp;
         n = (n < PSNUM ? n : 0);
      }

      /* calculate new write performance */
      nwp = (SB_CPU_HZ/fpdwtm)*(FILESZ/1024);

      /* filter out abnormal write performance */
      if (nwp > awp*9/10 && nwp < awp*11/10)
      {
         /* save new write performance */
         wpa[m++] = nwp;
         m = (m < PSNUM ? m : 0);
      }
   }
}

void fpdemo_msg(const char* str, int line)
{
   sb_ConWriteString(MSG_COL,line,SB_CLR_LIGHTMAGENTA,SB_CLR_BLACK,!SB_CON_BLINK,str);
}

void fpdemo_error(const char* str, int line)
{
   sb_ConWriteString(MSG_COL,line,SB_CLR_LIGHTRED,SB_CLR_BLACK,!SB_CON_BLINK,"                                 ");
   sb_ConWriteString(MSG_COL,line,SB_CLR_LIGHTRED,SB_CLR_BLACK,!SB_CON_BLINK,str);
}  

#if SMX_CFG_SSMX
#pragma default_function_attributes = @ ".pb1.text"
#include "xapip.h"
#endif

#ifdef __cplusplus
extern "C" {
void fpdemo_init(void);
void fpdemo_exit(void);
}
#endif /* __cplusplus */

void fpdemo_init(void)
{
   if (fpdemo = smx_TaskCreate(fpdemo_main, PRI_NORM, 1000, SMX_FL_UMODE, "fpdemo", NULL, NULL))
   {
     #if SMX_CFG_SSMX
      mp_MPACreate(fpdemo, &mpa_tmplt_fpd, 0x07, 8);
      smx_TaskSet(fpdemo, SMX_ST_IRQ, (u32)sb_irq_perm_uart, 0);
     #endif
      smx_TaskStart(fpdemo);
   }
   fpddone = smx_SemCreate(SMX_SEM_THRES, 1, "fpdemo done");
   fpdctm = sb_TMCal;
}

void fpdemo_exit(void)
{
   fpdexit = true;
   if (fpddone != NULL)
      smx_SemTest(fpddone, FPDEMO_DLY|SMX_FL_MSEC);
}

#endif /* MW_FATFS_DEMO */
