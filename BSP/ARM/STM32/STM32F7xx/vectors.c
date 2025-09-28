/*
* vectors.c                                                 Version 5.4.0
*
* Interrupt Vector Table for STM32F7xxxx.
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

#if defined(MW_FATFS)
void BSP_SDMMC_IRQHandler(void);
void DMA2_Stream3_IRQHandler(void);
void DMA2_Stream6_IRQHandler(void);
#endif

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
#ifdef __cplusplus
__root extern const intvec_elem  __vector_table[] @ ".intvec" =
#else
__root const intvec_elem  __vector_table[] @ ".intvec" =
#endif
{
/* from IAR cstartup_M.s */
/*  0 */  { .__ptr = __sfe( "CSTACK" ) },  /* Initial Stack Pointer */
/*  1 */  __iar_program_start,             /* Reset Vector (Entry Point) */
/*  2 */  NMI_Handler,           /* NMI */
/*  3 */  HF_Handler,            /* Hard Fault */
/*  4 */  MMF_Handler,           /* Mem Manage Fault */
/*  5 */  BF_Handler,            /* Bus Fault */
/*  6 */  smx_UF_Handler,        /* Usage Fault */
/*  7 */  Default_Handler,       /* reserved */
/*  8 */  Default_Handler,       /* reserved */
/*  9 */  Default_Handler,       /* reserved */
/* 10 */  Default_Handler,       /* reserved */
/* 11 */  smx_SVC_Handler,       /* SVC */
/* 12 */  Default_Handler,       /* Debug Monitor */
/* 13 */  Default_Handler,       /* reserved */
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
#if defined(MW_FATFS)
/* 65 / 49 */  SDMMC1_IRQHandler, /* BSP_SDMMC_IRQHandler */
#else
/* 65 / 49 */  Default_Handler,
#endif
/* 66 / 50 */  Default_Handler,
/* 67 / 51 */  Default_Handler,
/* 68 / 52 */  Default_Handler,
/* 69 / 53 */  Default_Handler,
/* 70 / 54 */  Default_Handler,
/* 71 / 55 */  Default_Handler,
/* 72 / 56 */  Default_Handler,
/* 73 / 57 */  Default_Handler,
/* 74 / 58 */  Default_Handler,
#if defined(MW_FATFS)
/* 75 / 59 */  DMA2_Stream3_IRQHandler, /* BSP_SDMMC_DMA_Rx_IRQHandler */
#else
/* 75 / 59 */  Default_Handler,
#endif
/* 76 / 60 */  Default_Handler,
/* 77 / 61 */  Default_Handler,
/* 78 / 62 */  Default_Handler,
/* 79 / 63 */  Default_Handler,
/* 80 / 64 */  Default_Handler,
/* 81 / 65 */  Default_Handler,
/* 82 / 66 */  Default_Handler,
/* 83 / 67 */  Default_Handler,
/* 84 / 68 */  Default_Handler,
#if defined(MW_FATFS)
/* 85 / 69 */  DMA2_Stream6_IRQHandler, /* BSP_SDMMC_DMA_Tx_IRQHandler */
#else
/* 85 / 69 */  Default_Handler,
#endif
/* 86 / 70 */  Default_Handler,
/* 87 / 71 */  Default_Handler,
/* 88 / 72 */  Default_Handler,
/* 89 / 73 */  Default_Handler,
/* 90 / 74 */  Default_Handler,
/* 91 / 75 */  Default_Handler,
/* 92 / 76 */  Default_Handler,
/* 93 / 77 */  Default_Handler,
/* 94 / 78 */  Default_Handler,
/* 95 / 79 */  Default_Handler,
/* 96 / 80 */  Default_Handler,
/* 97 / 81 */  Default_Handler,
/* 98 / 82 */  Default_Handler,
/* 99 / 83 */  Default_Handler,
/*100 / 84 */  Default_Handler,
/*101 / 85 */  Default_Handler,
/*102 / 86 */  Default_Handler,
/*103 / 87 */  Default_Handler,
/*104 / 88 */  Default_Handler,
/*105 / 89 */  Default_Handler,
/*106 / 90 */  Default_Handler,
/*107 / 91 */  Default_Handler,
/*108 / 92 */  Default_Handler,
/*109 / 93 */  Default_Handler,
/*110 / 94 */  Default_Handler,
/*111 / 95 */  Default_Handler,
/*112 / 96 */  Default_Handler,
/*113 / 97 */  Default_Handler,
#if defined(SB_CPU_STM32F76X) || defined(SB_CPU_STM32F77X)
/*114 / 98 */  Default_Handler,
/*115 / 99 */  Default_Handler,
/*116 /100 */  Default_Handler,
/*117 /101 */  Default_Handler,
/*118 /102 */  Default_Handler,
/*119 /103 */  Default_Handler,
/*120 /104 */  Default_Handler,
/*121 /105 */  Default_Handler,
/*122 /106 */  Default_Handler,
/*123 /107 */  Default_Handler,
/*124 /108 */  Default_Handler,
/*125 /109 */  Default_Handler
#endif
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
      sb_HFM();                        /* hard fault manager */
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
      smx_EM(SBE_CPU_MMF_VIOL, 1);        /* error manager */
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
   /* Tick is needed for clock, SRAM, and SDRAM init during startup.
      Later rehooked by smx tick. */
   HAL_IncTick();
}

#if defined(MW_FATFS)
/* These are renamed by macro in stm32756g_eval_sd.h.
#define BSP_SDMMC_IRQHandler           SDMMC1_IRQHandler         (IRQ 49)
#define BSP_SDMMC_DMA_Rx_IRQHandler    DMA2_Stream3_IRQHandler   (IRQ 59)
#define BSP_SDMMC_DMA_Tx_IRQHandler    DMA2_Stream6_IRQHandler   (IRQ 69)
*/
extern SD_HandleTypeDef uSdHandle;
extern LCB_PTR HAL_SD_IRQHandlerLSRH;
extern LCB_PTR HAL_DMA_IRQHandlerLSRH;

/* Handles SD1 card interrupt request. */
void BSP_SDMMC_IRQHandler(void)
{
   smx_ISR_ENTER();
   HAL_SD_IRQHandler(&uSdHandle);
   smx_ISR_EXIT();
}

/* Handles SDMMC1 DMA Rx transfer interrupt request. */
void BSP_SDMMC_DMA_Rx_IRQHandler(void)
{
   smx_ISR_ENTER();
   HAL_DMA_IRQHandler(uSdHandle.hdmarx); 
   smx_ISR_EXIT();
}

/* Handles SDMMC1 DMA Tx transfer interrupt request. */
void BSP_SDMMC_DMA_Tx_IRQHandler(void)
{
   smx_ISR_ENTER();
   HAL_DMA_IRQHandler(uSdHandle.hdmatx); 
   smx_ISR_EXIT();
}
#endif /* MW_FATFS */

/* Notes:
   1. SysTick is used for the smx tick. It could be statically hooked
      in the table above, but we do it dynamically when we are ready.
      Until then, the default ISR is hooked to it which does nothing.
      It is hooked in sb_TickInit() in bsp.c.
*/
