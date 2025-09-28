/*
* hwmain.c                                                  Version 5.4.0
*
* Hardware Init for LPC55Sxx processors.
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
* Author: David Moore
*
*****************************************************************************/

#include "bbase.h"
#include "bsp.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

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

   //ClockConfig() called in sb_TickInit(). See comment there.

   sb_IntCtrlInit();

   /* Copy EVT from Flash to SRAM because SMX hooks vectors dynamically. */
   if (evt_ram != evt_rom)
   {
      for (i = 0; i < (16+SB_IRQ_MAX+1); i++)
         evt_ram[i] = evt_rom[i];
   }
   /* Set Vector Table Offset Register to new EVT location. */
   *ARMM_NVIC_VTOR = (u32)evt_ram;

   //PinConfig();
}

/* Notes:
   1. Not reliable for tasks.
*/
