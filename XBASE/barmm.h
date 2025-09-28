/*
* barmm.h                                                   Version 5.4.0
*
* ARMM (Cortex-M) definitions and macros.
*
* Copyright (c) 2008-2025 Micro Digital Inc.
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
* Author: David Moore
*
*****************************************************************************/

#ifndef SB_BARMM_H
#define SB_BARMM_H

/*===========================================================================*
*                                DEFINITIONS                                 *
*===========================================================================*/

#define SB_ARMM_DISABLE_WITH_BASEPRI    0  /* <1> also change barmm.inc */
#define SB_ARMM_BASEPRI_VALUE        0x20  /* <2> also change barmm.inc */
#define SB_ARMM_DISABLE_TRAP            0  /* trap in umode on sb_INT_DISABLE() 
                                              or sb_INT_ENABLE() <6> */
/* ARMM Register Definitions used by smx */
#define ARMM_FPU_FPCCR               ((vu32 *)0xE000EF34)  /* Floating-point Context Control Register */
#define ARMM_MPU_TYPE                ((vu32 *)0xE000ED90)  /* MPU Type */
#define ARMM_MPU_CTRL                ((vu32 *)0xE000ED94)  /* MPU Control */
#define ARMM_MPU_RNR                 ((vu32 *)0xE000ED98)  /* MPU Region Number */
#define ARMM_MPU_RBAR                ((vu32 *)0xE000ED9C)  /* MPU Region Base Address */

#if SB_CPU_ARMM7
#define ARMM_MPU_RASR                ((vu32 *)0xE000EDA0)  /* MPU Region Base Attribute and Size */
#elif SB_CPU_ARMM8
#define ARMM_MPU_RLAR                ((vu32 *)0xE000EDA0)  /* MPU Region Limit Address */
#define ARMM_MPU_MAIR0               ((vu32 *)0xE000EDC0)  /* MPU MAIR0 */
#define ARMM_MPU_MAIR1               ((vu32 *)0xE000EDC4)  /* MPU MAIR1 */
#endif

#define ARMM_NVIC_SYSTICK_CTRL       ((vu32 *)0xE000E010)  /* SysTick Control and Status */
#define ARMM_NVIC_SYSTICK_RELOAD     ((vu32 *)0xE000E014)  /* SysTick Reload */
#define ARMM_NVIC_SYSTICK_CURRENT    ((vu32 *)0xE000E018)  /* SysTick Current Value */
#define ARMM_NVIC_SYSTICK_CAL        ((vu32 *)0xE000E01C)  /* SysTick Calibration Value */
#define ARMM_NVIC_IRQ_SET_EN_BASE    ((vu32 *)0xE000E100)  /* IRQ Set Enable First Reg */
#define ARMM_NVIC_IRQ_CLR_EN_BASE    ((vu32 *)0xE000E180)  /* IRQ Clear Enable First Reg */
#define ARMM_NVIC_IRQ_SET_PEND_BASE  ((vu32 *)0xE000E200)  /* IRQ Set Enable First Reg */
#define ARMM_NVIC_IRQ_CLR_PEND_BASE  ((vu32 *)0xE000E280)  /* IRQ Clear Enable First Reg */
#define ARMM_NVIC_IRQ_PRI_BASE       ((vu32 *)0xE000E400)  /* IRQ Priority First Reg */
#define ARMM_NVIC_INT_CTRL           ((vu32 *)0xE000ED04)  /* Interrupt Control State Register */
#define ARMM_NVIC_VTOR               ((vu32 *)0xE000ED08)  /* Vector Table Offset Register */
#define ARMM_NVIC_APPINT_RST_CTRL    ((vu32 *)0xE000ED0C)  /* App Interrupt and Reset Control Register */
#define ARMM_NVIC_CONFIG_CTRL        ((vu32 *)0xE000ED14)  /* Configuration Control */
#define ARMM_NVIC_SYSEXC_PRI_0704    ((vu32 *)0xE000ED18)  /* Exceptions  4- 7 Priority */
#define ARMM_NVIC_SYSEXC_PRI_1108    ((vu32 *)0xE000ED1C)  /* Exceptions  8-11 Priority */
#define ARMM_NVIC_SYSEXC_PRI_1512    ((vu32 *)0xE000ED20)  /* Exceptions 12-15 Priority */
#define ARMM_NVIC_SYSHAN_CTRL_STATE  ((vu32 *)0xE000ED24)  /* System Handler Control and State */
#define ARMM_SCB_CPACR               ((vu32 *)0xE000ED88)  /* Coprocessor Access Control Register */
#define ARMM_CFSR                    ((vu32 *)0xE000ED28)  /* Configurable Fault Status Register */
#define ARMM_MMFSR                   ((vu8  *)0xE000ED28)  /* Memory Fault Status Register (first byte of CFSR) */
#define ARMM_BFSR                    ((vu16 *)0xE000ED29)  /* Bus Fault Status Register (second byte of CFSR) */
#define ARMM_UFSR                    ((vu16 *)0xE000ED2A)  /* Usage Fault Status Register (upper half of CFSR) */
#define ARMM_HFSR                    ((vu32 *)0xE000ED2C)  /* Hard Fault Status Register */
#define ARMM_STIR                    ((vu32 *)0xE000EF00)  /* Software trigger interrupt register */

/* ARMM register flags used by smx */
#define ARMM_FL_DACCVIOL             0x01                  /* Data access violation in MMFSR */
#define ARMM_FL_FORCED               0x40000000            /* Escalated fault in HFSR */
#define ARMM_FL_IACCVIOL             0x02                  /* Instruction access violation in MMFSR */
#define ARMM_FL_PENDSVSET            0x10000000            /* Enable PENDSV exception */
#define ARMM_FL_RETTOBASE            0x00000800            /* Return to point of initial interrupt */
#define ARMM_FL_STKOF                0x0010                /* Stack Overflow in UFSR */

#define SB_CACHE_LINE      16 /* minimum with or without cache */
#define SB_STACK_ALIGN     8  /* minimum alignment of stack elements by AAPCS */

typedef u32 CPU_FL;           /* saves the state of the interrupt flag(s) in
                                 sb_IntStateSaveDisable() and sb_IntStateRestore() */

/* range of IRQs permitted to be masked when using MPU */
typedef struct {
   u8 irqmin;
   u8 irqmax;
} IRQ_PERM;

/*===========================================================================*
*                                  MACROS                                    *
*===========================================================================*/

#define sb_HALTEXEC()         while(1) {}

#define sb_BREAKPOINT()       { __asm volatile ("bkpt 0"); }

#if defined(SMX_DEBUG)        /* Note: Add SMX_DEBUG to Release/ROM target when debugging them */
#define sb_DEBUGTRAP()        { __asm volatile ("bkpt 0"); }
#else
#define sb_DEBUGTRAP()        { }
#endif

/* interrupt enable and disable macros */
#if !SB_ARMM_DISABLE_TRAP  /* normal operation */

 #if SB_ARMM_DISABLE_WITH_BASEPRI
 #define sb_INT_DISABLE()     __set_BASEPRI(SB_ARMM_BASEPRI_VALUE);
 #define sb_INT_ENABLE()      __set_BASEPRI(0);
 #else
 #define sb_INT_DISABLE()     __asm volatile ("cpsid i");  /* PRIMASK = 1 */
 #define sb_INT_ENABLE()      __asm volatile ("cpsie i");  /* PRIMASK = 0 */
 #endif

#else /* trap in umode <6> */

 #if SB_ARMM_DISABLE_WITH_BASEPRI
 #define sb_INT_DISABLE()     if (sb_IN_UMODE()) \
                                 sb_DEBUGTRAP() \
                              else \
                                 __set_BASEPRI(SB_ARMM_BASEPRI_VALUE);
 #define sb_INT_ENABLE()      if (sb_IN_UMODE()) \
                                 sb_DEBUGTRAP() \
                              else \
                                 __set_BASEPRI(0);
 #else
 #define sb_INT_DISABLE()     if (sb_IN_UMODE()) \
                                 sb_DEBUGTRAP() \
                              else \
                                 __asm volatile ("cpsid i");  /* PRIMASK = 1 */
 #define sb_INT_ENABLE()      if (sb_IN_UMODE()) \
                                 sb_DEBUGTRAP() \
                              else \
                                 __asm volatile ("cpsie i");  /* PRIMASK = 0 */
 #endif
#endif /* !TRAP */

#define sb_INT_DISABLEF()     __asm volatile ("cpsid f");  /* FAULTMASK = 1 */
#define sb_INT_ENABLEF()      __asm volatile ("cpsie f");  /* FAULTMASK = 0 */
#define sb_IN_SVC()           ((*ARMM_NVIC_INT_CTRL & 0xFF) == 0x0B) /* in SVC handler? */
#define sb_IN_UMODE()         (__get_MSP() == 0)           /* <4> */
#define sb_SVC(id)            __asm("svc %0" : : "i" (id));

#pragma section = "CSTACK"
#define sb_MS_GET_TOP()       __section_begin("CSTACK")
#define sb_MS_GET_SIZE()      __section_size("CSTACK")

#define SB_CPU_BIG_ENDIAN     !(__LITTLE_ENDIAN__)

#if defined(SMX_FRPORT) && (SB_ARMM_DISABLE_WITH_BASEPRI != 1) /*<5>*/
#error Set SB_ARMM_DISABLE_WITH_BASEPRI to 1 and SB_ARMM_BASEPRI_VALUE in barmm.h and barmm_iar.inc
#endif

/* Notes:
   1. SB_ARMM_DISABLE_WITH_BASEPRI vs. PRIMASK: Disabling interrupts with BASEPRI
      leaves the top one or more priority levels unmasked vs. PRIMASK which
      disables all interrupts (except faults). PRIMASK is simpler and safer,
      but using BASEPRI gives one or more non-maskable priority levels, which
      may be necessary for motor control ISRs and similar. If you use BASEPRI,
      DO NOT define smx ISRs (those that use smx_ISR_ENTER/EXIT and invoke LSRs)
      at the unmaskable priority level(s).

   2. SB_ARMM_BASEPRI_VALUE: When using BASEPRI to disable interrupts, those
      with priority equal or lower (higher number) than this setting are
      inhibited. Those higher (lower number) are non-maskable. Remember that
      the number of priority bits can differ on different ARM-M processors.
      For example, if the processor uses a 3-bit priority, priorities are
      0x00, 0x20, 0x40, etc. Setting this threshold to 0x20 means that 0x00
      is non-maskable. If the processor uses a 4-bit priority, priorities are
      0x00, 0x10, 0x20, etc. Setting this threshold to 0x20 means that 0x00
      and 0x10 are non-maskable. You may change this setting as desired. But
      heed the warning in Note 1 and change any ISR priorities if necessary.

   4. sb_IN_UMODE() uses a trick, since CONTROL.nPriv cannot be checked in umode.
      Accessing CONTROL, MSP, PSP, etc. returns 0 in umode. Since MSP is always
      non-zero, we must be in umode if it reads as 0.

   5. FreeRTOS port (FRPort): Set SB_ARMM_DISABLE_WITH_BASEPRI to 1 and
      SB_ARMM_BASEPRI_VALUE the same as configMAX_SYSCALL_INTERRUPT_PRIORITY
      here and in barmm_iar.inc. It is simplest to set ours to the same numeric
      value rather than to use their constant, to avoid include file problems
      and an undefined symbol for assembly. Also it is recommended to change
      their value to match our default, or adjust priorities in our irqtable.c.
      irqtable.c was set assuming BASEPRI is 0x20, but in the FreeRTOS release
      we worked with, it was set to 0x40.

   6. Interrupt enable and disable are nop's in umode mode. Use the trap
      versions to find them to change them to interrupt mask and unmask.
*/
#endif /* SB_BARMM_H */

