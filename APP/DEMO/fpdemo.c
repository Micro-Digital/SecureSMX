/*
* fpdemo.c                                                  Version 5.4.0
*
* File portal demo for FatFs.
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

extern   FATFS   SDFatFs;  /* file system struct for SD card logical drive */

/* defines */
#define  CP_CSLOT    3           /* cp pmsg region client slot */
#define  CP_PCH      &cpcli_fpd  /* client portal struct pointer for cp calls */
#define  FP_CSLOT    4           /* fp pmsg region client slot */
#define  FPD_PCH     &fpcli_fpd  /* client portal struct pointer for fp calls */
#define  FPDEMO_DLY  1000        /* msec between runs */
#define  FILENAME    "0:\\fpdemo.txt"  /* file name */
#define  FILESZ      (4*PM_BUFSZ)/* file size, must be multiple of PM_BUFSZ */
#define  MSG_COL     10          /* message column */
#define  TOP_LINE    2           /* top line of fpdemo display */

/* variables */
FPCS     cpcli_fpd;        /* console portal client structure for fpdemo */
char     dispbuf[80];      /* display buffer */
FIL*     fop;              /* file struct pointer */
TPCS     fpcli_fpd;        /* file portal client structure for fpdemo */
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
   /* open console portal for fpdemo */
   mp_FPortalOpen(&cpcli_fpd, CP_CSLOT, 80, 1, PRI_LO, 5, "fpd_rxchg");
   sb_ConWriteString(0,TOP_LINE, SB_CLR_LIGHTMAGENTA,SB_CLR_BLACK,!SB_CON_BLINK,"FPDEMO: ");

   /* get pmsg from mheap for file system portal */
   fpcli_fpd.pmsg = smx_PMsgGetHeap(PM_BLKSZ, (u8**)&fpcli_fpd.mhp, 
                                               FP_CSLOT, MP_DATARW, 0, NULL);
   u8 pri = (u8)smx_TaskPeek(fpdemo, SMX_PK_PRI);

   /* SD controller and card init */
  #if defined(SB_CPU_STM32)
   while (sbu_BSP_SD_Init() != MSD_OK)
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
      /* open file system portal for fpdemo */
      mp_TPortalOpen(&fpcli_fpd, PM_BLKSZ, PM_THDRSZ, pri, FP_CTMO, "ssem", "csem");

      /* run fpdemo */
      fpdemo_perf();

      /* close file system portal */
      mp_TPortalClose(&fpcli_fpd, 10); /* give server enough time to close */
   }

   /* end demo */
   smx_PMsgRel(&fpcli_fpd.pmsg, 0);
   fpcli_fpd.mhp = NULL;
   smx_SemSignal(fpddone);
}

void fpdemo_perf(void)
{
   char  bufw[12];
   char  bufr[12];
   u32   i;
   u32   p;    /* performance (kb/sec) */
   u32   bwr;  /* num bytes written/read */
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
      u32 pattern = 0x01010101;

      /* fill output buffer */
      for (i = 0; i < FILESZ/4; i++)
      {
         wtext[i] = pattern;
         pattern += 0x01010101;
      }

      /* create and open a new file with write access */
      if (f_open(&fop, FILENAME, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
      {
         fpdemo_error("FatFs Open for Write Error", TOP_LINE+2);
         return;
      }

      /* write data to the new file and measure time */
      sb_TMStart(&fpdtms);
      res = f_write(fop, wtext, sizeof(wtext), &bwr);
      sbu_TMEnd(fpdtms, &fpdwtm, fpdctm);
      f_close(fop);

      if (res != FR_OK)
      {
         fpdemo_error("FatFs Write or EOF Error", TOP_LINE+2);
         return;
      }

      /* open same file with read access */
      if (f_open(&fop, FILENAME, FA_READ) != FR_OK)
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
      sbu_TMEnd(fpdtms, &fpdrtm, fpdctm);
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

      /* compute read and write performances */
      p = (SB_CPU_HZ*(FILESZ/1024))/fpdrtm;
      ultoa(p, bufr, 10);
      p = (SB_CPU_HZ*(FILESZ/1024))/fpdwtm;
      ultoa(p, bufw, 10);

      /* display performances */
      strcpy(dispbuf, "R/W speed is ");
      strcat(dispbuf, bufr);
      strcat(dispbuf, "/");
      strcat(dispbuf, bufw);
      strcat(dispbuf, " KB/sec  ");
      fpdemo_msg(dispbuf, TOP_LINE+1);

      //f_unlink(FILENAME);  /* delete file */
      smx_DelayMsec(FPDEMO_DLY);
   }
   fpdemo_msg("FatFs Demo Done", TOP_LINE+2);
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
      mp_MPACreate(fpdemo, &mpa_tmplt_fpd, 0x07, 8);
      smx_TaskSet(fpdemo, SMX_ST_IRQ, (u32)sb_irq_perm_uart, 0);
      smx_TaskStart(fpdemo);
   }
   fpddone = smx_SemCreate(SMX_SEM_THRES, 1, "fpdemo done");
   fpdctm = sbu_TMCal;
}

void fpdemo_exit(void)
{
   fpdexit = true;
   if (fpddone != NULL)
      smx_SemTest(fpddone, FPDEMO_DLY|MSEC);
}

#endif /* MW_FATFS_DEMO */
