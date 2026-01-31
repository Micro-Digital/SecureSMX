/*
* main.c                                                    Version 6.0.0
*
* Application main() and initialization code running non-partitioned.
*
* Copyright (c) 1989-2026 Micro Digital Inc.
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
* Authors: David Moore, Ralph Moore
*
*****************************************************************************/

/*============================================================================
         hmode = handler mode (no tasks, uses main stack)
         pmode = privileged mode (tasks, uses task stacks)
         umode = unprivileged mode (tasks, uses task stacks)

         SMX_CFG_SSMX == 0 non-partitioned system (see xcfg.h)
         SMX_CFG_SSMX == 1 partitioned system

         <n> = footnote n
============================================================================*/

#include "xsmx.h"
#include "main.h"
#include "cpmap.h"

#if SMX_CFG_SSMX && SB_CPU_ARMM7
#pragma diag_suppress=Ta168  /* ignore warning that next line overrides existing prefix */
#pragma section_prefix = ".pb1"  /* plug block sections */
#endif

void smx_StartupChecks(void);

vbool   tick_cben;
ISR_PTR tick_cbptr;

/*============================================================================
               MAIN FUNCTION and APPLICATION INITIALIZATION
============================================================================*/

/* main  (hmode)
*
* Entry point from assembly startup code. Runs in hmode with main stack.
*/
#if defined(SMX_STM32CUBEMX)
int smx_main(void)
#else
int main(void)
#endif
{
   sb_HWInitAtMain();         /* hardware init after compiler startup code <5> */
   sb_INT_DISABLE();
   smx_StartupChecks();       /* abort if checks fail */
   sb_TickInit();             /* initialize tick <2><3> */
   sb_IRQsMask();             /* mask all interrupts */

  #if SMX_CFG_SSMX
   mp_MPUInit();              /* initialize the MPU */
   sb_INT_ENABLE();           /* enable exceptions for SVC calls <4> */
  #else
   *ARMM_MPU_CTRL = 0;        /* else disable MPU */
  #endif

   smx_Go();                  /* start multitasking (does not return) */
   return(0);
}

#if SMX_CFG_PROFILE
#pragma default_variable_attributes = @ ".sys.data"
u32   smx_rtcb[SMX_RTCB_SIZE][SMX_NUM_TASKS + 5];
#pragma default_variable_attributes =
#endif

/* ainit (pmode)
*
*  Application initialization runs under smx_Idle at PRI_SYS so no other
*  task can preempt. When done, the smx_Idle main function is changed to
*  smx_IdleMain() and its priority is lowered to PRI_MIN, and normal operation
*  can begins. <6>
*/
void ainit(u32) 
{
   sb_MSFill();               /* fill whole main stack with a pattern */
   sb_IRQsUnmask();           /* unmask interrupts */
   sb_TickIntEnable();        /* enable tick interrupt */

  #if SB_CFG_CON
   opcon_init();              /* initialize operation control */
  #endif

  #if CP_PORTAL
   cp_init(CPS_SLOTS);        /* initialize console portal server */
  #endif

  #if SMX_CFG_PROFILE
   smx_rtcbi = &smx_rtcb[0][0];
   smx_ProfileInit();         /* initialize rtc buffer and pointers */
  #endif

   sb_PeripheralsInit();      /* initialize peripherals */
   sb_ConsoleOutInit();       /* initialize console output */

   if (smx_errno)             /* if error, report and exit */
      aexit(smx_errno);

   sb_INT_ENABLE();           /* <1> */
   sb_ConsoleInInit();        /* initialize console input */

  #if (defined(SMX_STM32CUBEMX) && defined(SMX_TXPORT))
   tx_application_define(0);  /* initialize middleware and application */
  #else
   if (!mw_modules_init())    /* initialize middleware */
      smx_ERROR(SMXE_INIT_MOD_FAIL, 2);
   appl_init();               /* initialize application */
  #endif

  #if SMX_CFG_EVB
   smx_evbn = smx_evbi;       /* start event monitoring */
  #endif

 #if SMX_CFG_SSMX
   /* change to idle template */
  #if SB_CPU_ARMM7
   mp_MPACreate(smx_Idle, &mpa_tmplt_idle, 0xF, 8);
  #else
   mp_MPACreate(smx_Idle, &mpa_tmplt_idle, 0xF, 9);
  #endif
 #endif

   /* restart smx_Idle as the idle task at PRI_MIN */
   smx_TaskStartNew(smx_ct, 0, PRI_MIN, smx_IdleMain); 
}

/*============================================================================
                                 TICK ISR
============================================================================*/

#if SMX_CFG_SSMX
#pragma default_function_attributes = @ ".sys.text"
#endif

/* TickISR  (pmode)
*
*  Invoked by the tick interrupt. Runs in hmode with mstack. Performs optional 
*  ISR profiling and logging, then invokes smx_KeepTimeLSR. Hooks 
*  smx_TickISRCBPtr() to perform additional functions. <7>
*/
void smx_TickISR(void)
{
   smx_ISR_ENTER();
   smx_EVB_LOG_ISR(smx_TickISRH);

  #if SMX_CFG_PROFILE
   if (smx_rtc_frame_ctr == 1) /* if end of profile frame */
   {
      smx_i_rtc = smx_isr_rtc; /* capture isr rtc */
      smx_isr_rtc = 0;
      smx_l_rtc = smx_lsr_rtc; /* capture lsr rtc */
      smx_lsr_rtc = 0;
   }
  #endif

   smx_LSR_INVOKE(smx_KeepTimeLSR, 0);

   if (tick_cben)
   {
      tick_cbptr();
      tick_cben = false;
   }

  #if (defined(SMX_STM32CUBEMX) && defined(SMX_TXPORT))
   HAL_IncTick();
  #endif

   smx_EVB_LOG_ISR_RET(smx_TickISRH);
   smx_ISR_EXIT();
}

/*============================================================================
                              OTHER FUNCTIONS
============================================================================*/

#if SMX_CFG_SSMX && SB_CPU_ARMM7
#pragma default_function_attributes = @ ".pb1.text"
#endif

/* smx_StartupChecks()  (hmode)
*
* This routine does safety checks from the start of main().
*/
void smx_StartupChecks(void)
{
   bool pass = true;

   /* Ensure SS has sufficient headroom and trap if SS too small. */
   if (sb_MSScan() < 40)
   {
      sb_DEBUGTRAP();
      pass = false;
   }

   /* If fail, set compiler to allow 8-bit enums. If not available, control
      blocks will be larger */
   if (sizeof(SMX_CBTYPE) != 1)
   {
      sb_DEBUGTRAP();
      pass = false;
   }

   /* Ensure BSS was cleared by the startup code by testing a few separate
      locations. Using bitwise | instead of || for efficiency. */
   if ((u32)smx_init | (u32)smx_srnest | (u32)smx_sched | (u32)smx_lqctr | (u32)smx_clsr)
   {
      sb_DEBUGTRAP();
      pass = false;
   }
   if (pass == false)
      sb_Exit(SMXE_ABORT);
}

/* aexit  (pmode)
*
*  Application exit due to Esc, fault, or irrecoverable error. <6>
*/
void aexit(SMX_ERRNO errno)
{
   if (smx_init)
   {
      appl_exit();         /* exit application */
      mw_modules_exit();   /* exit middleware modules */

     #if (SB_CFG_CON)
      #define CP_PCH &cpcli_idle /* define client structure for cp portal calls <8> */

       /* display pending messages, then reason for exit */
      sb_MsgDisplay();
      sb_ConWriteString(0,EXIT_ROW,SB_CLR_LIGHTGRAY,SB_CLR_BLACK,!SB_CON_BLINK,"Exit Due To: ");

      if (errno == 0)
         sb_ConWriteString(13,EXIT_ROW,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,"Normal Exit");
      else
         sb_ConWriteString(13,EXIT_ROW,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,smx_errmsgs[errno]);

      if (smx_ebi->err != 0)
      {
         /* display first error in error buffer */
         sb_ConWriteString(0,EXIT_ROW+1,SB_CLR_LIGHTGRAY,SB_CLR_BLACK,!SB_CON_BLINK,"EB 1st Err:  ");
         if (smx_ebi->err < SMX_NUM_ERRORS)
            sb_ConWriteString(13,EXIT_ROW+1,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,smx_errmsgs[smx_ebi->err]);
         sb_DelayMsec(100);
      }

      #if CP_PORTAL
      cp_ConPortalClose(&cpcli_idle, 0);
      cp_exit(CPS_SLOTX);
      #endif
     #endif /* SB_CFG_CON */
   }
   sb_Reboot();
}

/* $Sub$$__call_ctors()
*
*  Called during startup after data init and before C++ global object 
*  constructors. Creates smx control block pools and initializes main heap. 
*/
#if defined(__cplusplus)
extern "C"
{
extern void $Super$$__call_ctors(void const *pibase, void const *ilimit);

void $Sub$$__call_ctors(void const *pibase, void const *ilimit)
{
   smx_CBPoolsCreate();
   eh_hvpn = 0;
   mheap_init();           /* initialize main heap */
   smx_htmo = 5;           /* mutex timeout for all heaps */
   smx_ct->name = "dtcb";  /* dummy TCB for initialization */
   $Super$$__call_ctors(pibase, ilimit);
}
}
#endif /* __cplusplus */

/*
   Notes:
   1. It is not necessary to explicitly call sb_INT_ENABLE() in ainit(),
      since interrupts will be enabled as soon as the first smx SSR is
      called. However, it is done so if the system hangs because interrupts
      are enabled, this will be clear. Otherwise, it might appear that the
      SSR caused the hang, which would result in wasted time debugging.
   2. The tick timer's counter is also used by sb_PtimeGet() for
      sb_DelayUsec(), smx_EVB (event buffer), smx_RTC (profiling), and
      sb_TM (time measurement) routines, so it is initialized early at the
      start of main(). If needed earlier, the call could be moved to the
      startup code, but mask the interrupt or don't enable until
      sb_TickIntEnable().
   3. sb_TickInit() unmasks the tick interrupt. It is purposely called
      before sb_MaskIRQs() which saves the current mask then masks all.
      The interrupt is unmasked (restored) by sb_UnmaskIRQs().
   4. Needed for SVC in sb_TM_INIT() else get hard fault. Does not enable
      IRQs because they are masked.
   5. Chip vendor BSP library init routines may need to run at main(),
      after the compiler startup code has done data init from ROM for static
      initializers. Call it from sb_HWInitAtMain().
   6. init/exit code can be put into .text since it only runs once at start/end,
      so it is not needed during normal operation. This prevents it from
      being called again, and allows it to be used to fill gaps.
   7. Caution: An SVC call (smxu_) in an ISR will cause a hard fault, because
      SVC priority is lower.
   8. See cpmap.h and cpcli.c.
*/

