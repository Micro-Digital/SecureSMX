/*
* hwmain.c                                                  Version 6.2.0
*
* Hardware Init for STM32F7 processors.
*
* Copyright (c) 2015-2026 Micro Digital Inc.
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
* Author: David Moore
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

/* Macro for checking if the code is located in ROM */
#define SB_CODE_IN_ROM(x)  ((0x00200000 <= (x)) && ((x) < 0x002FFFFF)) || \
                           ((0x08000000 <= (x)) && ((x) < 0x080FFFFF))

#ifdef __cplusplus
extern "C" {
#endif
extern int __low_level_init(void);
extern const intvec_elem __vector_table[];
extern void ClockConfig(void);  /* clock.c */
extern void PinConfig(void);    /* pin.c */
#ifdef __cplusplus
}
#endif

#if (defined(SMX_STM32CUBEMX) && defined(SMX_TXPORT))
void sb_HWInitAtMain(void) {} /* already done in startup code for TXPort */
#else
void sb_HWInitAtMain(void)
{
   u32 i;
   u32 *evt_ram = (u32*)__section_begin("EVT");
   u32 *evt_rom = (u32*)__vector_table;

   /* Set Vector Table Offset Register to EVT in ROM temporarily. */
   *ARMM_NVIC_VTOR = (u32)evt_rom;

  #if __ARMVFPV3__ || __ARMVFPV4__ || __ARMVFPV5__
   /* Enable FPU. Compiler startup should have already done next statement. */
   *ARMM_SCB_CPACR |= ((3 << (10 * 2)) | (3 << (11 * 2)));
   asm("dsb");
   asm("isb");
// *ARMM_FPU_FPCCR &= 0x3FFFFFFF;  /* Disable FPU auto save and lazy stacking */
   *ARMM_FPU_FPCCR &= 0xBFFFFFFF;  /* Disable FPU lazy stacking <1> */
  #endif

   /* STM32F7xx HAL library initialization:
      - Configure the Flash prefetch
      - Systick timer is configured by default as source of time base, but user 
        can eventually implement his proper time base source (a general purpose 
        timer for example or other time source), keeping in mind that Time base 
        duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
        handled in milliseconds basis.
      - Set NVIC Group Priority to 4
      - Low Level Initialization
    */
   HAL_Init();

   /* Invalidate and enable instruction and data caches */
   SCB_InvalidateICache();
   SCB_InvalidateDCache();
   SCB_EnableICache();
  #if defined(SMX_TSMX)
   SCB_EnableDCache();
  #else
   //SCB_EnableDCache();
  #endif
    
   /* Enable branch prediction */
   ((SCB_Type*)SCB_BASE)->CCR |= (1 << 18); 
    __DSB();

   if (SB_CODE_IN_ROM((u32)__low_level_init))
   {
      ClockConfig();

      BSP_SDRAM_Init();        
  #if defined(SB_BRD_STMICRO_STM32746GEVAL) || defined(SB_BRD_STMICRO_STM32F769IEVAL) || defined(SB_BRD_STMICRO_STM32F779IEVAL)
      BSP_SRAM_Init();        
  #endif        
   } 

   sb_IntCtrlInit();

   /* Copy EVT from Flash to SRAM because SMX hooks vectors dynamically. */
   if (evt_ram != evt_rom)
   {
      for (i = 0; i < (16+SB_IRQ_MAX+1); i++)
         evt_ram[i] = evt_rom[i];
   }
   /* Set Vector Table Offset Register to new EVT location. */
   *ARMM_NVIC_VTOR = (u32)evt_ram;

   PinConfig();
}
#endif

/* Notes:
   1. Not reliable for tasks.
*/