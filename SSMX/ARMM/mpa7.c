/*
* mpa7.c                                                    Version 6.0.0
*
* MPA ARMM7 templates for STM32F7xx processors.
*
* Copyright (c) 2017-2026 Micro Digital Inc.
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
* Author: Ralph Moore, David Moore
*
*****************************************************************************/

#include "smx.h"
#include "mpatmplt.h"

#if SMX_CFG_SSMX

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

/*===========================================================================*
                                 PMODE TEMPLATES
*===========================================================================*/

/* Default MPA template -- assigned when a task is created. Allows access to all 
   MPU regions. <1> */
MPA mpa_dflt = 
{
   RGN(0 | RA("sys_code")  | V, PCODE   | SRD("sys_code") | RSIC(scsz)       | EN, "sys_code"),
   RGN(1 | RA("sys_data")  | V, PDATARW | SRD("sys_data") | RSIC(sdsz)       | EN, "sys_data"),
   RGN(2 | RA("ram_block") | V, PDATARW | SRD("ram_block")| RSI("ram_block") | EN, "ram_block"),
   RGN(3 | 0x40000000      | V, PIOR                      | RSIN(0x80000)    | EN, "IO Regs"),
   RGN(4 | V, 0, 0),
   RGN(5 | V, 0, 0),
   RGN(6 | V, 0, 0),
   RGN(7 | V, 0, "stack"),
};

/* MPA template for idle task during initialization or exit. Allows access to
   all memory that is in use. */
MPA mpa_tmplt_init = 
{
   RGN(0 | RA("rom_block_all") | V, PCODE   | SRD("rom_block_all") | RSI("rom_block_all") | EN, "rom_block_all"),
   RGN(1 | RA("sram_block_all")| V, PDATARW | SRD("sram_block_all")| RSI("sram_block_all")| EN, "sram_block_all"),
   RGN(2 | RA("ram_block")     | V, PDATARW | SRD("ram_block")     | RSI("ram_block")     | EN, "ram_block"),
   RGN(3 | 0x40000000          | V, PIOR                           | RSIN(0x80000)        | EN, "IO Regs"),
};

/* MPA template for idle task during normal operation */
MPA mpa_tmplt_idle = 
{
   RGN(0 | RA("sys_code")  | V, PCODE   | SRD("sys_code") | RSIC(scsz) | EN, "sys_code"),
   RGN(1 | RA("sys_data")  | V, PDATARW | SRD("sys_data") | RSIC(sdsz) | EN, "sys_data"),
   RGN(2 | 0x40011000      | V, IOR                       | ( 9 << 1)  | EN, "USART1"),
   RGN(3 | RA("EVB")       | V, PDATARW | SRD("EVB")      | RSI("EVB") | EN, "EVB"),
};

/* MPA template for opcon task */
MPA mpa_tmplt_opcon = 
{
   RGN(0 | RA("sys_code")  | V, PCODE   | SRD("sys_code") | RSIC(scsz) | EN, "sys_code"),
   RGN(1 | RA("sys_data")  | V, PDATARW | SRD("sys_data") | RSIC(sdsz) | EN, "sys_data"),
   RGN(2 | RA("EVB")       | V, PDATARW | SRD("EVB")      | RSI("EVB") | EN, "EVB"),
};

/*===========================================================================*
                                 UMODE TEMPLATES
*===========================================================================*/

/* MPA template for console partition */
MPA mpa_tmplt_cp = 
{ 
   RGN(0 | RA("cp_code")   | V, CODE   | SRD("cp_code")  | RSIC(cpcsz)   | EN, "cp_code"),
   RGN(1 | RA("cp_data")   | V, DATARW | SRD("cp_data")  | RSIC(cpdsz)   | EN, "cp_data"),
   RGN(2 | RA("ucom_code") | V, CODE   | SRD("ucom_code")| RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(3 | 0x40011000      | V, IOR                      | ( 9 << 1)     | EN, "USART1"),
// RGN(4 | V, 0, "pmsg"),  /* console portal pmsg slot */
};

/* MPA template for led task */
MPA mpa_tmplt_led = 
{
   RGN(0 | RA("ucom_code")  | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data")  | V, DATARW | SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
   RGN(2 | RA("led_code")   | V, CODE   | SRD("led_code")  | RSIC(ledcsz)  | EN, "led_code"),
  #if defined(SB_BRD_STMICRO_STM32F746G_DISCOVERY)
   RGN(3 | 0x40022000       | V, IOR                       | ( 9 << 1)     | EN, "GPIOI"),
  #elif defined(SB_BRD_STMICRO_STM32746GEVAL)
   RGN(3 | 0x40020000       | V, IOR    | N0|N2|N3|N4|N67  | (12 << 1)     | EN, "GPIOBF"),
  #endif
   RGN(4 | 0x40005400       | V, IOR                       | ( 9 << 1)     | EN, "I2C1"),
   RGN(5 | 0x40023800       | V, IOR                       | ( 9 << 1)     | EN, "RCC"),
};

/* MPA template for lcd demo task */
MPA mpa_tmplt_lcd = 
{
   RGN(0 | RA("ucom_code")  | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz)  | EN, "ucom_code"),
   RGN(1 | RA("ucom_data")  | V, DATARW | SRD("ucom_data") | RSIC(ucomdsz)  | EN, "ucom_data"),
   RGN(2 | RA("lcd_code")   | V, CODE   | SRD("lcd_code")  | RSIC(lcdcsz)   | EN, "lcd_code"),
   RGN(3 | RA("lcd_data")   | V, DATARW | SRD("lcd_data")  | RSIC(lcddsz)   | EN, "lcd_data"),
   RGN(4 | RA("LCD_BUF")    | V, DATARW | SRD("LCD_BUF")   | RSIC(lcdbufsz) | EN, "LCD_BUF"),
   RGN(5 | 0x40005400       | V, IOR                       | ( 9 << 1)      | EN, "I2C1"),
};

/* MPA template for fpu demo task */
MPA mpa_tmplt_fpu = 
{
   RGN(0 | RA("ucom_code")  | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data")  | V, DATARW | SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
   RGN(2 | RA("fpu_code")   | V, CODE   | SRD("fpu_code")  | RSIC(fpucsz)  | EN, "fpu_code"),
   RGN(3 | RA("fpu_data")   | V, DATARW | SRD("fpu_data")  | RSIC(fpudsz)  | EN, "fpu_data"),
  #if CP_PORTAL
   RGN(4 | V, 0, "cp pmsg"),  /* console portal pmsg slot */
  #else
   RGN(4 | 0x40011000       | V, IOR                       | ( 9 << 1)     | EN, "USART1"),
  #endif
};

/* MPA template for FatFs */
MPA mpa_tmplt_fs = 
{
   RGN(0 | RA("ucom_code")  | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data")  | V, DATARW | SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
   RGN(2 | RA("fs_code")    | V, CODE   | SRD("fs_code")   | RSIC(fscsz)   | EN, "fs_code"),
   RGN(3 | RA("fs_data")    | V, DATARW | SRD("fs_data")   | RSIC(fsdsz)   | EN, "fs_data"),
   RGN(4 | 0x40012c00       | V, IOR                       | ( 9 << 1)     | EN, "SDMMC1"),
   RGN(5 | 0x40026400       | V, IOR                       | ( 9 << 1)     | EN, "DMA2"),
// RGN(6 | V, 0, "fp pmsg"),  /* file portal pmsg slot */
};

/* MPA template for fpdemo */
MPA mpa_tmplt_fpd =
{
   RGN(0 | RA("ucom_code") | V, CODE    | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("fpd_code")  | V, CODE    | SRD("fpd_code")  | RSIC(fpdcsz)  | EN, "fpd_code"),
   RGN(2 | RA("fpd_data")  | V, DATARW  | SRD("fpd_data")  | RSIC(fpddsz)  | EN, "fpd_data"),
// RGN(3 | V, 0, "pmsg"),  /* console portal pmsg slot */
// RGN(4 | V, 0, "pmsg"),  /* file portal pmsg slot */
};

/* Notes:
   1. The default MPA must include all MPU regions. However templates need not 
      include unused upper regions, because mp_MPULoad() automatically clears
      them, and the stack slot, MPU[7] is loaded by other means.
   2. Example of bridging IO register spaces to save MPU slots. Minimum size to 
      enclose USART1 and SDMMC1 is 0x2000 but USART1 starts at 0x40011000 which 
      is not 0x2000-aligned, so we must increase the size to 0x4000. Then,
      subregion size is 0x800. As shown in the subregion field all but 2 and 5 
      are excluded from region 4. This works, but additional memory is included
      that should not be accessible.
*/
#endif /* SMX_CFG_SSMX */
