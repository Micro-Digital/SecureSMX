/*
* vectors.c                                                 Version 5.4.0
*
* Interrupt Vector Table for NXP LPC55Sxx.
*
* Copyright (c) 2009-2025 Micro Digital Inc.
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

#include "bbase.h"
#include "bsp.h"
#include "xsmx.h"

#pragma language=extended  /* allow IAR extended syntax in this file */
#pragma segment="CSTACK"

#ifdef __cplusplus
extern "C" {
#endif

extern void __iar_program_start(void);  /* cmain.s (startup code in IAR dlib) */

extern void smx_PendSV_Handler(void);  /* XSMX\xarmm*.s */
extern void smx_SVC_Handler(void);     /* XSMX\xarmm*.s */
extern void smx_UF_Handler(void);      /* XSMX\xarmm*.s */

void NMI_Handler(void);
void HF_Handler(void);
void MMF_Handler(void);
void BF_Handler(void);
void Default_Handler(void);
void TempSysTick_Handler(void);

#ifdef __cplusplus
}
#endif


/*
   Interrupt Vector Table.

   The first 16 entries are common to all ARM-M processors. Below that
   are the processor specific IRQs for the peripherals on the chip.
   SMX modules hook ISRs dynamically. You can statically assign your
   ISRs here if you prefer.

   The name "__vector_table" has special meaning for IAR C-SPY:
   it is where the SP start value is found, and the NVIC vector
   table register (VTOR) is initialized to this address if != 0.

   The syntax used for slot 0 and the union type are needed for a C
   compile. A simpler way I tried only worked for C++.

   This table is copied to internal SRAM by our __low_level_init
   in startup.c.
*/
#if defined(__ICCARM__)
 #ifdef __cplusplus
__root extern const intvec_elem __vector_table[] @ ".intvec" =
 #else
__root const intvec_elem __vector_table[] @ ".intvec" =
 #endif
#else
#error Add declaration of __vector_table for your compiler in vectors.c.
#endif
{
#if defined(__ICCARM__)
/* from IAR cstartup_M.s */
/*  0 */  { .__ptr = __sfe( "CSTACK" ) },  /* Initial Stack Pointer */
/*  1 */  __iar_program_start,             /* Reset Vector (Entry Point) */
#else
#error Add stack ptr and startup code entry point for your compiler in vectors.c.
#endif

/*  2 */  NMI_Handler,           /* NMI */
/*  3 */  HF_Handler,            /* Hard Fault */
/*  4 */  MMF_Handler,           /* Mem Manage Fault */
/*  5 */  BF_Handler,            /* Bus Fault */
/*  6 */  smx_UF_Handler,        /* Usage Fault */
/*  7 */  Default_Handler,       /* Secure Fault */
/*  8 */  0,                     /* Image Length, set 0 if the Image Type is 0 */
/*  9 */  0,                     /* Image Type */
/* 10 */  0,                     /* Offset To Specific Header */
/* 11 */  smx_SVC_Handler,       /* SVC */
/* 12 */  Default_Handler,       /* Debug Monitor */
/* 13 */  0,                     /* Image Execution Address, set 0 if the Image Type is 0 */
/* 14 */  smx_PendSV_Handler,    /* PendSV */
/* 15 */  TempSysTick_Handler,   /* SysTick */

/* The interrupts which follow and number vary for every processor. */
/* Vector # / IRQ #. See sb_irq_table[] in irqtable.c for names. */

/* 16 /  0 */  Default_Handler,
/* 17 /  1 */  Default_Handler,
/* 18 /  2 */  Default_Handler,
/* 19 /  3 */  Default_Handler,
/* 20 /  4 */  Default_Handler,
/* 21 /  5 */  Default_Handler,
/* 22 /  6 */  Default_Handler,
/* 23 /  7 */  Default_Handler,
/* 24 /  8 */  Default_Handler,
/* 25 /  9 */  Default_Handler,
/* 26 / 10 */  Default_Handler,
/* 27 / 11 */  Default_Handler,
/* 28 / 12 */  Default_Handler,
/* 29 / 13 */  Default_Handler,
/* 30 / 14 */  Default_Handler,
/* 31 / 15 */  Default_Handler,
/* 32 / 16 */  Default_Handler,
/* 33 / 17 */  Default_Handler,
/* 34 / 18 */  Default_Handler,
/* 35 / 19 */  Default_Handler,
/* 36 / 20 */  Default_Handler,
/* 37 / 21 */  Default_Handler,
/* 38 / 22 */  Default_Handler,
/* 39 / 23 */  Default_Handler,
/* 40 / 24 */  Default_Handler,
/* 41 / 25 */  Default_Handler,
/* 42 / 26 */  Default_Handler,
/* 43 / 27 */  Default_Handler,
/* 44 / 28 */  Default_Handler,
/* 45 / 29 */  Default_Handler,
/* 46 / 30 */  Default_Handler,
/* 47 / 31 */  Default_Handler,
/* 48 / 32 */  Default_Handler,
/* 49 / 33 */  Default_Handler,
/* 50 / 34 */  Default_Handler,
/* 51 / 35 */  Default_Handler,
/* 52 / 36 */  Default_Handler,
/* 53 / 37 */  Default_Handler,
/* 54 / 38 */  Default_Handler,
/* 55 / 39 */  Default_Handler,
/* 56 / 40 */  Default_Handler,
/* 57 / 41 */  Default_Handler,
/* 58 / 42 */  Default_Handler,
/* 59 / 43 */  Default_Handler,
/* 60 / 44 */  Default_Handler,
/* 61 / 45 */  Default_Handler,
/* 62 / 46 */  Default_Handler,
/* 63 / 47 */  Default_Handler,
/* 64 / 48 */  Default_Handler,
/* 65 / 49 */  Default_Handler,
/* 66 / 50 */  Default_Handler,
/* 67 / 51 */  Default_Handler,
/* 68 / 52 */  Default_Handler,
/* 69 / 53 */  Default_Handler,
/* 70 / 54 */  Default_Handler,
/* 71 / 55 */  Default_Handler,
/* 72 / 56 */  Default_Handler,
/* 73 / 57 */  Default_Handler,
/* 74 / 58 */  Default_Handler,
/* 75 / 59 */  Default_Handler,
};

void BF_Handler(void)
{
   /* Peripheral not enabled? */
   if (!sb_handler_en)
   {
      __asm ("bkpt 0 \n\t");           /* halt on BF */
   }
   else
   {
      smx_EM(SBE_CPU_BF_VIOL, 2);
      *(u32*)ARMM_CFSR = *ARMM_CFSR;   /* clear logged faults */
   }
}

void HF_Handler(void)
{
   if (!sb_handler_en)
   {
      __asm ("bkpt 0 \n\t");           /* halt on HF */
   }
   else
   {
      sb_HFM();
     #if defined(SMX_TSMX)
      /* only TSMX returns here because smx_EM() normally exits */
      smx_SFModPC(2);                  /* advance to next instruction for exception return */
      *(u32*)ARMM_CFSR = *ARMM_CFSR;   /* clear logged faults */
     #endif
   }
}

void NMI_Handler(void)
{
   if (!sb_handler_en)
   {
      __asm ("bkpt 0 \n\t");           /* halt on NMI */
   }
   else
   {
      smx_EM(SMXE_NO_ISR, 2);
      *(u32*)ARMM_CFSR = *ARMM_CFSR;   /* clear logged faults */
   }
}

void MMF_Handler(void)
{
   if (!sb_handler_en)
   {
      __asm ("bkpt 0 \n\t");           /* halt on MMF */
   }
   else
   {
      smx_EM(SBE_CPU_MMF_VIOL, 1);
      *(u32*)ARMM_CFSR = *ARMM_CFSR;   /* clear logged faults */
   }
}

void Default_Handler(void)
{
   /* ISR not hooked to vector? */
   smx_EM(SMXE_NO_ISR, 2);
}

void TempSysTick_Handler(void)
{
   /* Implement if needed */
}

/*
   Notes:

   1. SysTick is used for the smx tick. It could be statically hooked
      in the table above, but we do it dynamically when we are ready.
      Until then, the default ISR is hooked to it which does nothing.
      It is hooked in sb_TickInit() in bsp.c.
*/
