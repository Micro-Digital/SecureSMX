/*
* xarmm.c                                                   Version 5.4.0
*
* ARMM (Cortex-M) functions.
*
* Copyright (c) 2020-2025 Micro Digital Inc.
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

#include "xsmx.h"

#if SMX_CFG_MPU_ENABLE

/* smx_BROff() turns off the MPU background region and enables MPU */
void smx_BROff(void)
{
   __DMB();
   *ARMM_MPU_CTRL = 0x1;
   __DSB();
   __ISB();
}

/* smx_smx_BROn() turns on the MPU background region */
void smx_BROn(void)
{
   *ARMM_MPU_CTRL = 0x5; /* enable MPU with background region */
   __DSB();
   __ISB(); /* allow background region on to take effect */
}

/* smx_BRRestoreOff() turns the MPU background region off if it was off */
void smx_BRRestoreOff(void)
{
   if ((*ARMM_NVIC_INT_CTRL & ARMM_FL_RETTOBASE) || (*ARMM_NVIC_SYSHAN_CTRL_STATE & 0x7FF)) /*<1>*/
   {
      if (smx_mpu_br_off)
         smx_MPU_BR_OFF();
   }
}

/* smx_BRSaveOn() saves MPU background region status, turns it on, and enables interrupts */
void smx_BRSaveOn(void)
{
   sb_INT_DISABLE();
   if ((*ARMM_NVIC_INT_CTRL & ARMM_FL_RETTOBASE) || (*ARMM_NVIC_SYSHAN_CTRL_STATE & 0x7FF)) /*<1>*/
   {
      if (*ARMM_MPU_CTRL & 0x4)
         smx_mpu_br_off = false;
      else
         smx_mpu_br_off = true;
   }
   smx_MPU_BR_ON();
   sb_INT_ENABLE();
}
#endif /* SMX_CFG_MPU_ENABLE */

/* smx_ISREnter() enters smx ISR <5> */
void smx_ISREnter(void)
{
   smx_MPU_BR_SAVE_ON();  /*<2>*/
  #if SMX_CFG_PROFILE
   if (*ARMM_NVIC_INT_CTRL & ARMM_FL_RETTOBASE)
   {
      smx_RTC_ISR_START();
   }
  #endif
}

/* smx_ISRExit() exits smx ISR <5> */
void smx_ISRExit(void)
{
   sb_INT_DISABLE(); /*<3>*/
   if (*ARMM_NVIC_INT_CTRL & ARMM_FL_RETTOBASE)
   {
      /* last ISR (not nested) */
      smx_RTC_ISR_END();
      if (smx_srnest == 0)
      {
         /* task or overhead was interrupted */
         if (smx_lqctr > 0)
         {
            /* run LSR scheduler */
            smx_srnest = 1;
            *ARMM_NVIC_INT_CTRL = ARMM_FL_PENDSVSET; /* trigger PendSV */
            sb_INT_ENABLE();
            return; /* tail-chain to smx_PendSV_Handler */
         }
         else
         {
            smx_RTC_TASK_START(); /* resume task count */
         }
      }
      else
      {
         /* LSR or SSR was interrupted */
         if (smx_clsr)
         {
            smx_RTC_LSR_START();  /* resume LSR count */
         }
         else
         {
            smx_RTC_TASK_START(); /* resume task count */
         }
      }
   }
   /* return to point of interrupt */
   sb_INT_DISABLEF();         /* FAULTMASK = 1 <4> */
   sb_INT_ENABLE();           /* BASEPRI = 0 or PRIMASK = 0 */
   smx_MPU_BR_RESTORE_OFF();  /* restore BR if it was on when interrupted */
   return;                    /* exception exit */
}



/* Notes:
1. In smx_MPU_BR_SAVE_ON/OFF(), (ARMM_NVIC_SYSHAN_CTRL_STATE & 0x7FF) != 0
   means an smx ISR interrupted a processor exception handler, such as SVC
   or PendSV. These are not smx ISRs, so we are currently in the outermost
   smx ISR. However, RETTOBASE is 0 because we are nested in one of these
   exceptions. SysTick is excluded because it is an smx ISR. Reserved bits
   are assumed to be future exceptions.

2. An ISR may need to reference a variable in the partition it is associated
   with, e.g. FS, USB, etc., and the task that happened to be interrupted is
   likely from a different partition and doesn't have the region in its MPA
   to access it. The variable cannot be put into sys_data because then the
   umode partition code would not be able to access it.

3. DISABLE must be done ahead of read of ARMM_NVIC_INT_CTRL to make it atomic.
   If an ISR nests right after R0 is loaded with the address of this register
   but before reading it, then when the nested ISR returns, it appears
   RETTOBASE is not (yet?) set even though the original ISR is the outermost.
   In this case, it should have gone inside the if() and checked srnest,
   lq_ctr, and possibly run LSRs and scheduler, but instead, it returns to
   the point of interrupt, leaving any LSRs waiting and not clearing BR.
   This was detected in v5 with checks in the Idle task to see if BR is set
   and then analyzing the ETM trace. It's not clear why there should be a
   problem, but it was very repeatable, and always shows the nested ISR at
   the same place, right after loading R0 with ARMM_NVIC_INT_CTRL address.
   The problem was seen on Cortex-M7 and may or may not apply to others.
   Further testing seemed to show the problem is related to the MPU or our
   use of it in SecureSMX, so this may not be a problem prior to v4.4.0, or
   if SMX_CFG_SSMX is 0, but the change was applied to all versions to be safe.

4. Need to reenable interrupts because exception exit does NOT. But we don't
   want interrupts until after we return, so we first set FAULTMASK, which IS
   cleared by the processor on exception exit. This is here not just for the 
   sb_INT_DISABLE() above, but also in case the ISR used sb_INT_DISABLE() 
   and left interrupts disabled assuming the exception exit would reenable 
   them, as on other processors. We don't want to allow interrupts at the tail
   of smx_ISR_EXIT() because they could invoke LSRs, and they would wait until
   the next interrupt or SSR, which could be a long time. We have already
   passed the check of smx_lqctr, and since an interrupt at the tail of
   smx_ISR_EXIT() would be nested, it would skip all the smx_ISR_EXIT code
   and return to the point of interrupt (due to the RETTOBASE check at the
   top). Fortunately, the processor DOES clear FAULTMASK on exception exit so
   we set it to 1, clear BASEPRI or PRIMASK, and then return. FAULTMASK
   inhibits all interrupts like PRIMASK and faults too. FAULTMASK only remains
   set for a few instructions, until the return, which clears it.

5. smx_srnest handling is different from other CPU ports. Because ISR nesting
   is tracked by the interrupt controller (RETTOBASE flag), we do not need to
   increment/decrement smx_srnest in ISR exit/enter. This is important because
   it means ISRs do not need to start with all interrupts disabled. Also,
   because of PendSV and because some registers are automatically pushed onto
   the task's stack, the code here differs substantially from other ports.
*/