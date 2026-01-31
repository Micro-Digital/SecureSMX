/*
* fpudemo.c                                                 Version 6.0.0
*
* Floating Point Demo. Tests that FPU state is saved and restored
* properly on task switches using hardware auto save on Cortex-M4/M7
* or using smx hooked exit/entry routines otherwise.
*
* The values output on the terminal for Demo2 should be twice the values
* of Demo1 when it is working properly.
*
* SSMX: This is an example of a umode partition that accesses another partition 
* (console partition) via its portal in order to display messages.
*
* Copyright (c) 2010-2026 Micro Digital Inc.
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
* Authors: Xingsheng Wan and David Moore
*
* Portable to any ANSI compliant C compiler.
*
*****************************************************************************/

#include "smx.h"
#include "main.h"
#include "app.h"
#include <stdio.h>

#if SB_FPU_DEMO

#if SMX_CFG_SSMX
#pragma section_prefix = ".fpu"
#endif

#define FPU_DEMO_DISPLAY  1  /* Set to 1 to display results on console, 0 not to display. */
#define STATUS_COL  12
#define TOP_LINE     6

/* FPU auto save for Cortex-M4/M7. Ensure ASPEN is enabled in FPCCR.
   LSPEN is not yet supported.
   FPU_SAVE_HDW = 0 sw saves s0-s31; 1 hw saves s0-s15; 2 hw saves s0-s15 and sw s16-s31
*/
#if (defined(SB_CPU_STM32F7XX) || defined(SB_CPU_STM32H7XX))
#define FPU_SAVE_HDW 2  /* If 1, also set FPU_NUM_REGS to 16 in fpudemofun.s */
#else
#define FPU_SAVE_HDW 0
#endif

#if __ARMVFPV3__ || __ARMVFPV4__ || __ARM_NEON__
#if (FPU_SAVE_HDW == 1)  /* See note <1> */
#define FPU_NUM_REGS 16
#else
#define FPU_NUM_REGS 33
#endif
#endif

#if CP_PORTAL
FPCS    cpcli_fpu;       /* cp portal client struct for fpudemo */
#define CP_SLOT_FPU 4    /* MPA pmsg slot */
#endif

#ifdef __cplusplus
extern "C" {
#endif

void fpudemo_init(void);
void fpudemo_exit(void);

static void fpudemo_msg(int line, u32* dump);
static void fpu_demo1_main(u32);
static void fpu_demo2_main(u32);

void clear_fpu_registers(void);
void write_fpu_registers(int interval);
void read_fpu_registers(u32* dump);

#if (FPU_SAVE_HDW != 1)
/* These need not be in fpu_data since callback funcs run in pmode. */
static u32 fpubuf1[FPU_NUM_REGS];
static u32 fpubuf2[FPU_NUM_REGS];
#endif

static bool fpu_demo_exit = false;
#if FPU_DEMO_DISPLAY
static char bstr[61] = "                                                            ";
#endif

#if SMX_CFG_SSMX
#if CP_PORTAL && SB_CPU_ARMM8
TCB_PTR fpu_cpinit;
#endif
#endif
static TCB_PTR fpu_demo1;
static TCB_PTR fpu_demo2;
static SCB_PTR fpu_demo1_done;
static SCB_PTR fpu_demo2_done;

#if SMX_CFG_SSMX
#if SB_CFG_UARTI && !CP_PORTAL
#define DEMO_IRQ_PERM  sb_irq_perm_uart
#else
#define DEMO_IRQ_PERM  NULL
#endif
#if CP_PORTAL && SB_CPU_ARMM8
void fpu_cpinit_main(u32);
#endif
#endif /* SMX_CFG_SSMX */

#if (FPU_SAVE_HDW != 1)
/***** FPU SAVE MACROS AND ROUTINES
* Define as macros when possible. Depends on compiler's capability for
* macros and inline assembly.
*****************************************************************************/

#if __ARMVFPV3__ || __ARMVFPV4__ || __ARM_NEON__

static void FPU_SAVE(void *pbuf)
{
    /* r0 == pbuf */

   #if (FPU_SAVE_HDW == 2)  /* See note <1> */
    /* save s16-s31 */
    asm("vstmia.32 r0, {s16-s31}");

   #else
    /* save FPSCR */
    asm("vmrs r1, fpscr");
    asm("str r1, [r0, #0x80]");
 
    /* save s0-s31 */
    asm("vstmia.32 r0, {s0-s31}");
   #endif
}

static void FPU_RESTORE(void *pbuf)
{
    /* r0 == pbuf */

   #if (FPU_SAVE_HDW == 2)  /* See note <1> */
    /* restore s16-s31 */
    asm("vldmia.32 r0, {s16-s31}");

   #else
    /* restore FPSCR */
    asm("ldr r1, [r0, #0x80]");
    asm("vmsr fpscr, r1");

    /* restore s0-s31 */
    asm("vldmia.32 r0, {s0-s31}");
   #endif
}

#else
#error Define FPU macros and functions for your FPU.
#endif

static u32 fpu_demo1_cb(u32 mode);
static u32 fpu_demo2_cb(u32 mode);

#endif /* FPU_SAVE_HDW */


/***** INITIALIZATION
*****************************************************************************/

void fpudemo_init(void)
{
    int demos = 0;

   #if FPU_DEMO_DISPLAY
    sb_ConWriteString(0,TOP_LINE, SB_CLR_LIGHTMAGENTA,SB_CLR_BLACK,!SB_CON_BLINK,"FPU Demo1: ");
    sb_ConWriteString(0,TOP_LINE+5, SB_CLR_LIGHTMAGENTA,SB_CLR_BLACK,!SB_CON_BLINK,"FPU Demo2: ");
   #else
    sb_ConWriteString(0,TOP_LINE, SB_CLR_LIGHTMAGENTA,SB_CLR_BLACK,!SB_CON_BLINK,"FPU Demo Display Disabled");
   #endif

    fpu_demo1 = smx_TaskCreate(fpu_demo1_main, PRI_NORM, 0, SMX_FL_UMODE, "fpu_demo1");
   #if SMX_CFG_SSMX
    mp_MPACreate(fpu_demo1, &mpa_tmplt_fpu, 0x1F, 8);
    smx_TaskSet(fpu_demo1, SMX_ST_IRQ, (u32)DEMO_IRQ_PERM);
   #endif
   #if (FPU_SAVE_HDW != 1)
    smx_TaskSet(fpu_demo1, SMX_ST_CBFUN, (u32)fpu_demo1_cb, 1);
   #endif
    smx_TaskStart(fpu_demo1);
    demos++;

    fpu_demo1_done = smx_SemCreate(SMX_SEM_THRES, demos, "fpu_demo1_done");

    fpu_demo2 = smx_TaskCreate(fpu_demo2_main, PRI_NORM, 0, SMX_FL_UMODE, "fpu_demo2");
   #if SMX_CFG_SSMX
    mp_MPACreate(fpu_demo2, &mpa_tmplt_fpu, 0x1F, 8);
    smx_TaskSet(fpu_demo2, SMX_ST_IRQ, (u32)DEMO_IRQ_PERM);
   #endif
   #if (FPU_SAVE_HDW != 1)
    smx_TaskSet(fpu_demo2, SMX_ST_CBFUN, (u32)fpu_demo2_cb, 1);
   #endif
    smx_TaskStart(fpu_demo2);
    demos++;

    fpu_demo2_done = smx_SemCreate(SMX_SEM_THRES, demos, "fpu_demo2_done");

  #if CP_PORTAL
   #if SB_CPU_ARMM7
    /* open console portal for fpudemo */
    mp_FPortalOpen(&cpcli_fpu, CP_SLOT_FPU, 80, 1, 5, "fpu_rxchg");
   #elif SB_CPU_ARMM8
    /* use helper task to open console portal <2> */
    fpu_cpinit = smx_TaskCreate(fpu_cpinit_main, PRI_MAX, 500, SMX_FL_UMODE, "fpu_cpinit", NULL, NULL);
    mp_MPACreate(fpu_cpinit, &mpa_tmplt_fpu, 0x1F, 8);
    smx_TaskStart(fpu_cpinit, 0);
   #endif
  #endif
}

/***** EXIT
*****************************************************************************/

void fpudemo_exit(void)
{
    fpu_demo_exit = true;
    smx_SemTest(fpu_demo1_done, 2000|SMX_FL_MSEC);
    smx_SemTest(fpu_demo2_done, 2000|SMX_FL_MSEC);
}


#if SMX_CFG_SSMX
/*+++++++++++++++++++++++ LIMITED SWI/SVC API (UMODE) ++++++++++++++++++++++*/
#include "xapiu.h"

#if CP_PORTAL && SB_CPU_ARMM8
void fpu_cpinit_main(u32)
{
   /* open console portal for fpudemo <2> */
   mp_FPortalOpen(&cpcli_fpu, CP_SLOT_FPU, 80, 1, 5, "fpu_rxchg");

   smx_TaskDelete(&fpu_cpinit);
}
#endif
#endif /* SMX_CFG_SSMX */


/***** DEMO TASK1 ROUTINES
*****************************************************************************/

#if (FPU_SAVE_HDW != 1)
static u32 fpu_demo1_cb(u32 mode)
{
    switch (mode)
    {
        case SMX_CBF_EXIT:
            FPU_SAVE(fpubuf1);
            break;
        case SMX_CBF_ENTER:
            FPU_RESTORE(fpubuf1);
            break;
    }
    return 0;  /* unused */
}
#endif

void fpu_demo1_main(u32)
{
    static u32 dump[FPU_NUM_REGS+1];
    memset(dump, 0, FPU_NUM_REGS * 4);
    smx_DelaySec(3);

    clear_fpu_registers();

    while (!fpu_demo_exit)
    {
        read_fpu_registers(dump);
        fpudemo_msg(TOP_LINE, dump);
        write_fpu_registers(1);
        smx_DelayMsec(500);
    }
    smx_SemSignal(fpu_demo1_done);
}


/***** DEMO TASK2 ROUTINES
*****************************************************************************/

#if (FPU_SAVE_HDW != 1)
static u32 fpu_demo2_cb(u32 mode)
{
    switch (mode)
    {
        case SMX_CBF_EXIT:
            FPU_SAVE(fpubuf2);
            break;
        case SMX_CBF_ENTER:
            FPU_RESTORE(fpubuf2);
            break;
    }
    return 0;  /* unused */
}
#endif

void fpu_demo2_main(u32)
{
    static u32 dump[FPU_NUM_REGS+1];
    memset(dump, 0, FPU_NUM_REGS * 4);
    smx_DelaySec(3);

    clear_fpu_registers();

    while (!fpu_demo_exit)
    {
        read_fpu_registers(dump);
        fpudemo_msg(TOP_LINE+5, dump);
        write_fpu_registers(2);
        smx_DelayMsec(500);
    }
    smx_SemSignal(fpu_demo2_done);
}

/***** SUBROUTINES
*****************************************************************************/

#if CP_PORTAL
#include "cpmap.h"
#define CP_PCH &cpcli_fpu /* define client structure for cp portal calls <3> */
#endif

static void fpudemo_msg(int line, u32* dump)
{
   #if FPU_DEMO_DISPLAY
    static char str[128];
    char temp[16];
    int  i, j;

    for (i = 0; i < FPU_NUM_REGS/8; i++)
    {
        str[0] = 0;
        for (j = 0; j < 8; j++)
        {
            ultoa(dump[(i * 8) + j], temp, 10);
            strcat(str, temp);
            strcat(str, ",");
        }
        sb_ConWriteString(STATUS_COL,line+i,SB_CLR_LIGHTMAGENTA,SB_CLR_BLACK,!SB_CON_BLINK,bstr);
        sb_ConWriteString(STATUS_COL,line+i,SB_CLR_LIGHTMAGENTA,SB_CLR_BLACK,!SB_CON_BLINK,str);
    }
   #else
    (void)line;
    (void)dump;
   #endif /* FPU_DEMO_DISPLAY */
}

#ifdef __cplusplus
}
#endif

#endif /* SB_FPU_DEMO */

/*
   Notes:

   1. For Cortex-M4/M7, only s0-s15 are automatically saved/restored,
      so if the compiler uses more, you should use auto save to save
      s0-s15 and the hooked routines to save s16-s31 by setting
      FPU_SAVE_HDW = 2, or disable auto save in startup.c and use the
      hooked routines to save all registers by setting FPU_SAVE_HDW = 0.
   2. The console portal should be opened for each task that wants to use it.
      However, we take a shortcut here and open it once for use by all tasks
      in this demo. They all use the same MPA template, so they all have the
      same free slot to hold the pmsg region, so this is ok. However, for the
      ARMMv8 MPU, regions must not overlap, so there is a check in
      mp_RegionGetHeapT() to ensure that for a ptask, an auxiliary region
      (above the active regions) is used for the pmsg slot in the MPU because
      ptasks have the sys_data region which contains the main heap, from which
      the pmsg would be allocated. This check fails if the portal is opened
      from usbddemo_init() because it runs from Idle/ainit() which is a ptask.
      To get around this, we create a one-shot utask here just to do the
      portal open.
   3. See cpmap.h and cpcli.c.
   4. See mpa7.c or mpa8.c for MPA template and <processor>app_mpu.icf for 
      linker command file.
*/
