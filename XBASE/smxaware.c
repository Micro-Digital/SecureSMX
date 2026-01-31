/*
* smxaware.c                                                Version 6.0.0
*
* smxAware init, print ring, and utilities.
* smxAware Live init, server task, and subroutines.
*
* Copyright (c) 1999-2026 Micro Digital Inc.
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
* Author: Marty Cochran
*
*****************************************************************************/

#if defined(SMXAWARE)  /* condition out whole file if SMXAWARE is not enabled */

#include "bbase.h"
#include "smxaware.h"

#if SMX_CFG_PROFILE
extern u32        smx_rtcb[SMX_RTCB_SIZE][SMX_NUM_TASKS + 5];
#endif

#if SMX_CFG_SSMX
#pragma diag_suppress=Ta168 /* ignore warning that next line overrides existing prefix */
#pragma section_prefix = ".sys"
#endif

#pragma section = "EB"
#pragma section = "EVB"

/* Configuration */

#define MMO_REGIONS 0               /* Set to 1 to enable all memory regions display */

#if defined(SMXAWARE_LIVE)
#define LOCALIP    "10.1.1.100"     /* Set our IP here */
//#define LOCALIP    "192.168.0.10"   /* Set our IP here */
#define LOCALMASK  "255.255.255.0"  /* Set our subnet mask here */
#endif

/* configuration table */
struct
{
   u32   lq_size        = SMX_SIZE_LQ;          /* KEEP FIRST <1> */
   u32   sec            = SMX_TICKS_PER_SEC;
   u32   num_blocks     = SMX_NUM_BLOCKS;
   u32   num_egs        = SMX_NUM_EGS;
   u32   num_eqs        = SMX_NUM_EQS;
   u32   num_msgs       = SMX_NUM_MSGS;
   u32   num_mtxs       = SMX_NUM_MTXS;
   u32   num_pipes      = SMX_NUM_PIPES;
   u32   num_pools      = SMX_NUM_POOLS;
   u32   num_sems       = SMX_NUM_SEMS;
   u32   num_tasks      = SMX_NUM_TASKS;
   u32   num_tmrs       = SMX_NUM_TIMERS;
   u32   num_xchgs      = SMX_NUM_XCHGS;
   u32   heap_address   = SMX_HEAP_ADDRESS;
   u32   heap_space     = SMX_HEAP_SPACE;
   u32   heap_dc_sz     = SMX_HEAP_DC_SIZE;
   u32   stk_scan       = SMX_CFG_STACK_SCAN;
   u32   stk_pad_sz     = SMX_SIZE_STACK_PAD;
   u32   stkpl_blk_num  = SMX_NUM_STACKS;
   u32   stkpl_blk_sz   = SMX_SIZE_STACK_BLK;
   u32   stk_sz_idle    = SMX_SIZE_STACK_IDLE;
   u32   rtcb_size      = SMX_RTCB_SIZE;
   u32   rtc_frame      = SMX_RTC_FRAME;
   u32   eb_size        = (u32)__section_size("EB");
   u32   evb_size       = (u32)__section_size("EVB");
   u32   ht_size        = SMX_SIZE_HT;
} smx_cf;

#ifdef __cplusplus
extern "C" {
#endif

/* These variables must be public so smxAware can read them. */

u32      sa_ready = 0;              /* sa_ready is read only by smxAware DLL */
u32      sa_booleansz = 1;          /* sa_booleansz is read only by smxAware DLL */
u32      sa_SMX_NUM_ERRORS;         /* sa_SMX_NUM_ERRORS is read only by smxAware DLL */
SA_PRINT sa_print;
u32      sa_LiveIsInCapture = false;
u32      sa_endian_test  = 0x01234567;
u32      sa_SMX_HEAP_FENCES = EH_NUM_FENCES; /* dwords above and below data block */
u32      sa_EH_NUM_HEAPS    = EH_NUM_HEAPS;

u32      sa_SMX_RSA_SIZE    = SMX_RSA_SIZE;
u32      sb_stk_fill_val    = SB_STK_FILL_VAL;
TCB_PTR* sa_smx_ct_address  = 0;
u16      smx_evbnDiagCnt;               /* simple counter for diagnostics used in xevb.h */
u32      smx_ehv_size = sizeof(EHV);
u32      smx_tcb_size = sizeof(struct TCB);
u32      smx_lcb_size = sizeof(struct LCB);
u32      smx_BuildFlags = 0;
u32      sb_CFG_OMB_SIZE = SB_SIZE_OMB; /* Output Message Buffer size (bytes) */
u32      smx_pri_num  = SMX_PRI_NUM;    /* now it's the number of priority levels, to support priorities going up or down */
u32      smx_pri_up   = 1;
u32      smx_Processor = SB_PROCESSOR;  /* smx processor version encoding (consulted by smxAware) */

#if SMX_CFG_SSMX
#if SB_CPU_ARMM7
u32      mp_mpu_type  = MP_MPU_TYPE_ARMM7;
#elif SB_CPU_ARMM8
u32      mp_mpu_type  = MP_MPU_TYPE_ARMM8;
#endif
u32      mp_mpu_fas   = MP_MPU_FAS;
u32      mp_mpu_sz    = MP_MPU_SZ;
u32      smx_mpr_size = 0;
#endif
#if SMX_CFG_PROFILE
u32      smx_rtcb_size = 0;
#endif
#if SMX_CFG_DIAG
u32      sa_susploc = true;
#endif
#if defined(SMX_FRPORT)
u32      smx_frport = 1;            /* FreeRTOS port in use */
#endif

static void dummy_use(u8 * par);
static SCB_PTR in_sa_print;         /* protect the sa_Print() call */

u32      smx_mssize;

bool smxaware_live_init(void);
bool smxaware_live_exit(void);

#ifdef __cplusplus
}
#endif


#if MMO_REGIONS
/*
   USER: Modify the definitions to match your .icf file, or set them to zero.
   Providing this information is optional and is used to create a more
   accurate smxAware Memory Map Overview display.
*/
/* these are in .icf file */
extern char RAM_S;
extern char RAM_E;
extern char SRAM_S;
extern char SRAM_E;
char* sa_RAM_S   = &RAM_S;
char* sa_RAM_E   = &RAM_E;
char* sa_RAM0_S  = 0;
char* sa_RAM0_E  = 0;
char* sa_RAM1_S  = 0;
char* sa_RAM1_E  = 0;
char* sa_RAM2_S  = 0;
char* sa_RAM2_E  = 0;
char* sa_SRAM_S  = &SRAM_S;
char* sa_SRAM_E  = &SRAM_E;
char* sa_SRAM0_S = 0;
char* sa_SRAM0_E = 0;
char* sa_SRAM1_S = 0;
char* sa_SRAM1_E = 0;
char* sa_SRAM2_S = 0;
char* sa_SRAM2_E = 0;
char* sa_XRAM_S  = 0;
char* sa_XRAM_E  = 0;
#endif /* MMO_REGIONS */


/******* smxaware_init()
* Initialize variables and buffers smxAware needs.
*****************************************************************************/

bool smxaware_init(void)
{
   /* Do a dummy use of each of these variables to force them to be linked, since
      a clever optimizer realizes they aren't needed and strips them out otherwise.
      They are referenced by the smxAware DLL, which is not linked to the app.
   */
   dummy_use((u8 *) &smx_Processor);
   dummy_use((u8 *) &smx_Version);
   dummy_use((u8 *) &smx_htpres);
   dummy_use((u8 *) &smx_hti);
   dummy_use((u8 *) &smx_htn);
   dummy_use((u8 *) &smx_htx);
   dummy_use((u8 *) &sa_endian_test);
   dummy_use((u8 *) &sa_SMX_HEAP_FENCES);
   dummy_use((u8 *) &sa_EH_NUM_HEAPS);
   dummy_use((u8 *) &sa_SMX_RSA_SIZE);
   dummy_use((u8 *) &sb_stk_fill_val);
   dummy_use((u8 *) &sa_smx_ct_address);
   dummy_use((u8 *) &sb_ticktmr_clkhz);
   dummy_use((u8 *) &smx_bcbs);
   dummy_use((u8 *) &smx_egcbs);
   dummy_use((u8 *) &smx_eqcbs);
   dummy_use((u8 *) &smx_mcbs);
   dummy_use((u8 *) &smx_lcbs);
   dummy_use((u8 *) &smx_mucbs);
   dummy_use((u8 *) &smx_pcbs);
   dummy_use((u8 *) &smx_picbs);
   dummy_use((u8 *) &smx_scbs);
   dummy_use((u8 *) &smx_tcbs);
   dummy_use((u8 *) &smx_tmrcbs);
   dummy_use((u8 *) &smx_xcbs);
   dummy_use((u8 *) &smx_pri_num);
   dummy_use((u8 *) &smx_ehv_size);
   dummy_use((u8 *) &smx_tcb_size);
   dummy_use((u8 *) &smx_lcb_size);
   dummy_use((u8 *) &smx_pri_up);
   dummy_use((u8 *) &smx_BuildFlags);
   dummy_use((u8 *) &sb_CFG_OMB_SIZE);

  #if MMO_REGIONS
   dummy_use((u8 *) &sa_RAM_S);
   dummy_use((u8 *) &sa_RAM_E);
   dummy_use((u8 *) &sa_RAM0_S);
   dummy_use((u8 *) &sa_RAM0_E);
   dummy_use((u8 *) &sa_RAM1_S);
   dummy_use((u8 *) &sa_RAM1_E);
   dummy_use((u8 *) &sa_RAM2_S);
   dummy_use((u8 *) &sa_RAM2_E);
   dummy_use((u8 *) &sa_SRAM_S);
   dummy_use((u8 *) &sa_SRAM_E);
   dummy_use((u8 *) &sa_SRAM0_S);
   dummy_use((u8 *) &sa_SRAM0_E);
   dummy_use((u8 *) &sa_SRAM1_S);
   dummy_use((u8 *) &sa_SRAM1_E);
   dummy_use((u8 *) &sa_SRAM2_S);
   dummy_use((u8 *) &sa_SRAM2_E);
   dummy_use((u8 *) &sa_XRAM_S);
   dummy_use((u8 *) &sa_XRAM_E);
  #endif /* MMO_REGIONS */

  #if SMX_CFG_SSMX
   dummy_use((u8 *) &mp_mpu_type);
   dummy_use((u8 *) &mp_mpu_fas);
   dummy_use((u8 *) &mp_mpu_sz);
   dummy_use((u8 *) &smx_mpr_size);
   smx_mpr_size = sizeof(MPR);
  #endif
  #if SMX_CFG_PROFILE
   dummy_use((u8 *) &smx_rtcb_size);
   smx_rtcb_size = sizeof(smx_rtcb);
  #endif
  #if SMX_CFG_DIAG
   dummy_use((u8 *) &sa_susploc);
  #endif
  #if defined(SMX_FRPORT)
   dummy_use((u8 *) &smx_frport);
  #endif
   smx_mstop = (u32)sb_MS_GET_TOP();
   smx_mssize = sb_MS_GET_SIZE();

   /* Initialize the tracing/logging feature */
   sa_PrintInit();

   sa_booleansz = sizeof(smx_htpres);
   sa_SMX_NUM_ERRORS = SMX_NUM_ERRORS;
   sa_smx_ct_address = &smx_ct;
  #if SMX_CFG_TOKENS
   smx_BuildFlags |= SMX_CFG_TOKENS_DEFINED;    /* smx_BuildFlags contains up to 32 build settings */
  #endif

   /* smxAware is ready. smxAware changes sa_ready to 0x77.
      The special value should work fine in all cases, but if there is a
      problem, try the first setting. */
   //sa_ready = 1;         /* normal value */
   sa_ready = 0x11223344;  /* special value for attach to running app */

   return true;
}


/******* sa_PrintInit()
* Initialize buffer and semaphore for user print statements.
*****************************************************************************/

void sa_PrintInit(void)
{
   int i;

   sa_print.rng_buf[SMX_SIZE_SA_PRT_RING] = 0x77; /* fence */

   /* write a dummy first entry since algorithm in smxAware skips first */
   *(u32 *)sa_print.rng_buf = 0xFFFFFFFFL;    /* dummy timestamp */
   sa_print.rng_buf[4] = 0;                   /* dummy string */

   for (i = 5; i < SMX_SIZE_SA_PRT_RING; i++)
      sa_print.rng_buf[i] = 0;

   sa_print.next    = 5;
   sa_print.length  = SMX_SIZE_SA_PRT_RING;
   sa_print.version = 1;
   in_sa_print = smx_SemCreate(SMX_SEM_RSRC, 1, "in_sa_print");
}

#if SMX_CFG_SSMX
/*+++++++++++++++++++++++ LIMITED SWI/SVC API (UMODE) ++++++++++++++++++++++*/
//#include "xapiu.h" -- causes SemTest() problem for sa_Print()
#endif

/******* sa_Print()
* sa_Print() puts the null terminated string into a ring buffer
* to be read by the smxAware DLL kernel aware extension.
* The string will be displayed as a line in the print window.
* It also adds a record to the Event Buffer. Before the string, it puts
* smx_etime as the timestamp. This is correlated with the Event Buffer.
*
* The smxAware code assumes strings are at least 4 non-zero characters,
* so here we pad them with spaces at the end, if necessary. If a null
* string is passed or a 0 string pointer, we put "(no string)" into
* the buffer to alert the user of a possible problem in his call.
*
* Note: You may want to use sprintf() to prepare a string for sa_Print(),
*       but sprintf() requires as much as 2KB of stack for local buffers
*       and can cause undetected stack overflow because it moves the stack
*       pointer with sub sp, nn which can skip over the pad. Also, if your
*       version of sprintf() is non-reentrant, protect it with a semaphore.
*
* Note: This calls SSRs and must not be used from an ISR.
*
*****************************************************************************/

void sa_Print(const char *string)
{
#define MIN_STRLEN 4
   u32    etime_save;
   u32    len;
   char   buf[MIN_STRLEN+1];

   if (in_sa_print == 0)
      return;   /* sa_PrintInit() has not run or failed */

   if (sa_LiveIsInCapture)
      return;   /* disable the filling of the print ring because smxAware Live is reading it */

   /* wait if another task is already in sa_Print() */
   if (!smx_SemTest(in_sa_print, SMX_TMO_NOWAIT))
   {
      if (smx_clsr)
         return;  /* skip printing message since we're in an LSR and can't wait */
      smx_SemTest(in_sa_print, SMX_TMO_INF);
   }

   /* handle short strings (see comment above this function) */
   len = strlen(string);
   if (len == 0)
      string = "(no string)";
   else if (len < MIN_STRLEN)
   {
      strcpy(buf, string);
      memset(buf+len, ' ', MIN_STRLEN-len);  /* pad with spaces */
      buf[MIN_STRLEN] = 0;
      string = buf;
   }

   /* Write smx_etime to print ring and in EVB record. Address can be on any
      boundary, so on processors that don't allow writes on byte boundaries,
      (e.g. ARM) write it one byte at a time.
   */
   etime_save = smx_SysPeek(SMX_PK_ETIME);  /* save smx_etime to ensure print ring and EVB use exact same value for smx_etime */
   sb_write32_unaligned(&sa_print.rng_buf[(u32)(sa_print.next)], etime_save);
   smx_EVB_LOG_USER_PRINT(etime_save, sa_print.next);

   sa_print.next += 4;
   if (sa_print.next >= SMX_SIZE_SA_PRT_RING) sa_print.next = 0;

   /* write string immediately after smx_etime */
   do
   {
      sa_print.rng_buf[(u32)(sa_print.next)] = *string;
      sa_print.next++;
      if (sa_print.next >= SMX_SIZE_SA_PRT_RING)
         sa_print.next = 0;
   } while (*string++);  /* make sure the null is written, then exit loop */
   smx_SemSignal(in_sa_print);
}

/******* sa_PrintVals("var top = %d   bottom = %d", (long) top, (long) bottom)
* sa_PrintVals() will merge one or two values in the string at the
* position of the %d.
* If only one value is to be merged then use only one %d
*
* Note: This calls SSRs and must not be used from an ISR.
*
*****************************************************************************/

void sa_PrintVals(const char *s, long val1, long val2)
{
   #define BUF_MAX  80
   char buf[BUF_MAX+40];
   char* bufp;
   int which_val = 1;

   bufp = buf;
   while (*s != 0x00 && bufp < buf+BUF_MAX)
   {
      if (*s == '%')
      {  /* assume ltoa() is reentrant */
         ltoa((which_val == 1) ? val1 : val2, bufp, 10);
         for (; *bufp != '\0'; ++bufp) {}  /* find the new end of string */

         which_val++;
         s+=2;   /* skip the %d */
      }
      else
         *bufp++ = *s++;
   }
   *bufp = 0x00;  /* needed if 1 or more chars follow last %d */
   sa_Print(buf);
}

#if SMX_CFG_SSMX
/*+++++++++++++++++++++++++ FULL DIRECT API (PMODE) ++++++++++++++++++++++++*/
#include "xapip.h"
#endif


/* This function is used to force reference to some variables so optimizer doesn't
   strip them out, thinking they're unused.
*/
volatile u32 dummy_count = 0;
static void dummy_use(u8 *par)
{
   dummy_count += *par;  /* do nothing */
}


#if defined(SMXAWARE_LIVE)  /* condition out whole section if SMXAWARE_LIVE is not enabled */
#error Copy this section from v5.3.1 and update to match SMX and change to use other TCP/IP stack.
#endif /* SMXAWARE_LIVE */

#endif /* SMXAWARE */

/*
Notes:
   1. lq_size is the only field of SMX_CONF needed in smx assembly code.
      Keep it first since assembly code accesses it at _cf. Accessing it
      via _cf avoids needing to define a SMX_CONF structure in the
      assembly header for each processor and keeping it consistent.
*/
