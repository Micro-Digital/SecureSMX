/*
* mpa7.c                                                    Version 6.2.0
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

#if defined(SMXFS) || (defined(SMXUSBD) && SFS_PORTAL_SD)
#include "fcfg.h"
#endif

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

/*===========================================================================*
                                 PMODE TEMPLATES
*===========================================================================*/

MPA mpa_dflt = /* default MPA must show all active regions <1> */
{
   RGN(0 | RA("sys_code")  | V, PCODE    | SRD("sys_code")  | RSIC(scsz)       | EN, "sys_code"),
   RGN(1 | RA("sys_data")  | V, PDATARWC | SRD("sys_data")  | RSIC(sdsz)       | EN, "sys_data"),
   RGN(2 | RA("rom_block") | V, PCODE    | SRD("rom_block") | RSI("rom_block") | EN, "rom_block"),
   RGN(3 | RA("sram_block")| V, PDATARWC | SRD("sram_block")| RSI("sram_block")| EN, "sram_block"),
   RGN(4 | RA("ram_block") | V, PDATARWC | SRD("ram_block") | RSI("ram_block") | EN, "ram_block"),
   RGN(5 | 0x40000000      | V, PIOR                        | RSIN(0x80000)    | EN, "IO Regs"),
   RGN(6 | V, 0, 0),
   RGN(7 | V, 0, 0),
};

/* MPA template for idle task during initialization or exit. Allows access to
   all memory that is in use. */
MPA mpa_tmplt_init = 
{
   RGN(0 | RA("rom_block_all") | V, PCODE    | SRD("rom_block_all") | RSI("rom_block_all") | EN, "rom_block_all"),
   RGN(1 | RA("sram_block_all")| V, PDATARWC | SRD("sram_block_all")| RSI("sram_block_all")| EN, "sram_block_all"),
   RGN(2 | RA("ram_block")     | V, PDATARWC | SRD("ram_block")     | RSI("ram_block")     | EN, "ram_block"),
   RGN(3 | 0x40000000          | V, PIOR                            | RSIN(0x80000)        | EN, "IO Regs"),
};

/* MPA template for idle task during normal operation */
MPA mpa_tmplt_idle = 
{
   RGN(0 | RA("sys_code")  | V, PCODE   | SRD("sys_code") | RSIC(scsz) | EN, "sys_code"),
   RGN(1 | RA("sys_data")  | V, PDATARWC| SRD("sys_data") | RSIC(sdsz) | EN, "sys_data"),
   RGN(2 | 0x40011000      | V, IOR                       | ( 9 << 1)  | EN, "USART1"),
   RGN(3 | RA("EVB")       | V, PDATARWC| SRD("EVB")      | RSI("EVB") | EN, "EVB"),
};

/* MPA template for opcon task */
MPA mpa_tmplt_opcon = 
{
   RGN(0 | RA("sys_code")  | V, PCODE   | SRD("sys_code") | RSIC(scsz) | EN, "sys_code"),
   RGN(1 | RA("sys_data")  | V, PDATARWC| SRD("sys_data") | RSIC(sdsz) | EN, "sys_data"),
   RGN(2 | RA("EVB")       | V, PDATARWC| SRD("EVB")      | RSI("EVB") | EN, "EVB"),
// RGN(3 | V, 0, "cp pmsg"),  /* console portal pmsg slot */
};

/*===========================================================================*
                                 UMODE TEMPLATES
*===========================================================================*/

/* MPA template for console partition */
MPA mpa_tmplt_cp = 
{ 
   RGN(0 | RA("cp_code")   | V, CODE   | SRD("cp_code")  | RSIC(cpcsz)   | EN, "cp_code"),
   RGN(1 | RA("cp_data")   | V, DATARWC| SRD("cp_data")  | RSIC(cpdsz)   | EN, "cp_data"),
   RGN(2 | RA("ucom_code") | V, CODE   | SRD("ucom_code")| RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(3 | 0x40011000      | V, IOR                      | ( 9 << 1)     | EN, "USART1"),
// RGN(4 | V, 0, "cp pmsg"),  /* console portal pmsg slot */
};

/* MPA template for led task */
MPA mpa_tmplt_led = 
{
   RGN(0 | RA("ucom_code")  | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data")  | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
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
   RGN(1 | RA("ucom_data")  | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz)  | EN, "ucom_data"),
   RGN(2 | RA("lcd_code")   | V, CODE   | SRD("lcd_code")  | RSIC(lcdcsz)   | EN, "lcd_code"),
   RGN(3 | RA("lcd_data")   | V, DATARWC| SRD("lcd_data")  | RSIC(lcddsz)   | EN, "lcd_data"),
   RGN(4 | RA("LCD_BUF")    | V, DATARWC| SRD("LCD_BUF")   | RSIC(lcdbufsz) | EN, "LCD_BUF"),
   RGN(5 | 0x40005400       | V, IOR                       | ( 9 << 1)      | EN, "I2C1"),
};

/* MPA template for fpu demo task */
MPA mpa_tmplt_fpu = 
{
   RGN(0 | RA("ucom_code")  | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data")  | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
   RGN(2 | RA("fpu_code")   | V, CODE   | SRD("fpu_code")  | RSIC(fpucsz)  | EN, "fpu_code"),
   RGN(3 | RA("fpu_data")   | V, DATARWC| SRD("fpu_data")  | RSIC(fpudsz)  | EN, "fpu_data"),
  #if CP_PORTAL
   RGN(4 | V, 0, "cp pmsg"),  /* console portal pmsg slot */
  #else
   RGN(4 | 0x40011000       | V, IOR                       | ( 9 << 1)     | EN, "USART1"),
  #endif
};

#if defined(MW_FATFS)
/* MPA template for FatFs */
MPA mpa_tmplt_fs = 
{
   RGN(0 | RA("ucom_code")  | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data")  | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
   RGN(2 | RA("fs_code")    | V, CODE   | SRD("fs_code")   | RSIC(fscsz)   | EN, "fs_code"),
   RGN(3 | RA("fs_data")    | V, DATARWC| SRD("fs_data")   | RSIC(fsdsz)   | EN, "fs_data"),
   RGN(4 | 0x40012c00       | V, IOR                       | ( 9 << 1)     | EN, "SDMMC1"),
   RGN(5 | 0x40026400       | V, IOR                       | ( 9 << 1)     | EN, "DMA2"),
// RGN(6 | V, 0, "fp pmsg"),  /* file portal pmsg slot */
};

/* MPA template for fpdemo */
MPA mpa_tmplt_fpd =
{
   RGN(0 | RA("ucom_code") | V, CODE    | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("fpd_code")  | V, CODE    | SRD("fpd_code")  | RSIC(fpdcsz)  | EN, "fpd_code"),
   RGN(2 | RA("fpd_data")  | V, DATARWC | SRD("fpd_data")  | RSIC(fpddsz)  | EN, "fpd_data"),
// RGN(3 | V, 0, "cp pmsg"),  /* console portal pmsg slot */
// RGN(4 | V, 0, "fp pmsg"),  /* file portal pmsg slot */
};
#endif /* MW_FATFS */

#if defined(SMXFS) || SFS_DRV_MMCSD
 #if SFS_DRV_MMCSD
/* MPA template for fs portal server task and SD card. Also USB mass storage portal.
   Has I/O regions to access SDMMC1 and DMA2 registers needed for SD card and the
   dynamic region for a tunnel portal for USB mass storage.
*/
MPA mpa_tmplt_fs = 
{
   RGN(0 | RA("ucom_code") | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz)| EN, "ucom_code"),
   RGN(1 | RA("ucom_data") | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz)| EN, "ucom_data"),
   RGN(2 | RA("fs_code")   | V, CODE   | SRD("fs_code")   | RSIC(fscsz)  | EN, "fs_code"),
   RGN(3 | RA("fs_data")   | V, DATARWC| SRD("fs_data")   | RSIC(fsdsz)  | EN, "fs_data"),
   RGN(4 | 0x40012C00      | V, IOR                       | ( 9 << 1)    | EN, "SDMMC1"),
   RGN(5 | 0x40026400      | V, IOR                       | ( 9 << 1)    | EN, "DMA2"),
   RGN(6 | V, 0, "dynamic region"),
   RGN(7 | V, 0, "stack"),
};
 #elif SFS_DRV_USB
/* MPA template for fs portal server task and USB mass storage portal or direct.
   Has I/O region to access USBHS controller registers for direct access and the
   dynamic region for a tunnel portal for USB mass storage.
*/
MPA mpa_tmplt_fs = 
{
   RGN(0 | RA("ucom_code") | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz)| EN, "ucom_code"),
   RGN(1 | RA("ucom_data") | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz)| EN, "ucom_data"),
   RGN(2 | RA("fs_code")   | V, CODE   | SRD("fs_code")   | RSIC(fscsz)  | EN, "fs_code"),
   RGN(3 | RA("fs_data")   | V, DATARWC| SRD("fs_data")   | RSIC(fsdsz)  | EN, "fs_data"),
   RGN(4 | V, 0, 0),
   RGN(5 | V, 0, "dynamic region"),  /* client slot to USBH ms portal */
   RGN(6 | V, 0, "dynamic region"),  /* server slot for FS portal */
   RGN(7 | V, 0, "stack"),
};
 #else
 #error Enable MMCSD or USB driver in fcfg.h.
 #endif

#if SFS_PORTAL
/* MPA template for fsdemo fs_reader_writer task using smxFS portal */
MPA mpa_tmplt_fsdp = 
{
   RGN(0 | RA("ucom_code") | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data") | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
   RGN(2 | RA("fsdp_code") | V, CODE   | SRD("fsdp_code") | RSIC(fsdpcsz) | EN, "fsdp_code"),
   RGN(3 | RA("fsdp_data") | V, DATARWC| SRD("fsdp_data") | RSIC(fsdpdsz) | EN, "fsdp_data"),
   RGN(4 | V, 0, 0),
  #if CP_PORTAL
   RGN(5 | V, 0, "cp pmsg"),  /* console portal pmsg */
  #else
   RGN(5 | 0x40011000      | V, IOR                       | ( 9 << 1)     | EN, "USART1"),  /*<5>*/
  #endif
   RGN(6 | V, 0, "dynamic region"),
   RGN(7 | V, 0, "stack"),
};
#else
/* MPA template for fsdemo fs_reader_writer task making direct smxFS calls */
MPA mpa_tmplt_fsdd = 
{
   RGN(0 | RA("ucom_code") | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data") | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
   RGN(2 | RA("fsdd_code") | V, CODE   | SRD("fsdd_code") | RSIC(fsddcsz) | EN, "fsdd_code"),
   RGN(3 | RA("fsdd_data") | V, DATARWC| SRD("fsdd_data") | RSIC(fsdddsz) | EN, "fsdd_data"),
   RGN(4 | 0x40010000      | V, IOR    | N0|N1|N3|N4|N67  | (13 << 1)     | EN, "USART1, SDMMC1"),  /*<2><5>*/
   RGN(5 | 0x40026400      | V, IOR                       | ( 9 << 1)     | EN, "DMA2"),
   RGN(6 | 0x40040000      | V, IOR                       | (17 << 1)     | EN, "USBHS"),
   RGN(7 | V, 0, "stack"),
};
#endif
#endif /* SMXFS */

#if defined(SMXUSBD)
/* MPA template for usbd dcd task and function tasks */
MPA mpa_tmplt_usbd = 
{
   RGN(0 | RA("ucom_code") | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data") | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
   RGN(2 | RA("usbd_code") | V, CODE   | SRD("usbd_code") | RSIC(usbdcsz) | EN, "usbd_code"),
   RGN(3 | RA("usbd_data") | V, DATARWC| SRD("usbd_data") | RSIC(usbddsz) | EN, "usbd_data"),
   RGN(4 | 0x40040000      | V, IOR                       | (17 << 1)     | EN, "USBHS"),
   RGN(5 | V, 0, 0),
   RGN(6 | V, 0, "dynamic region"),
   RGN(7 | V, 0, "stack"),
   RGN(8 | V, 0, "phantom"),  /* callback portal client pmsg slot <4> */
   RGN(9 | V, 0, "phantom"),  /* callback portal client pmsg slot <4> */
};

/* MPA template for usbddemo tasks using smxUSBD portals */
MPA mpa_tmplt_usbddp = 
{
   RGN(0 | RA("ucom_code")  | V, CODE   | SRD("ucom_code")  | RSIC(ucomcsz)  | EN, "ucom_code"),
   RGN(1 | RA("ucom_data")  | V, DATARWC| SRD("ucom_data")  | RSIC(ucomdsz)  | EN, "ucom_data"),
   RGN(2 | RA("usbddp_code")| V, CODE   | SRD("usbddp_code")| RSIC(usbddpcsz)| EN, "usbddp_code"),
   RGN(3 | RA("usbddp_data")| V, DATARWC| SRD("usbddp_data")| RSIC(usbddpdsz)| EN, "usbddp_data"),
   RGN(4 | V, 0, 0),
  #if CP_PORTAL
   RGN(5 | V, 0, "cp pmsg"),  /* console portal pmsg */
  #else
   RGN(5 | 0x40011000       | V, IOR                        | ( 9 << 1)      | EN, "USART1"),  /*<5>*/
  #endif
   RGN(6 | V, 0, "dynamic region"),
   RGN(7 | V, 0, "stack"),
};
#endif /* SMXUSBD */

#if defined(SMXUSBH)
/* MPA template for usbh hcd task, hub task, and portal server task */
MPA mpa_tmplt_usbh = 
{
   RGN(0 | RA("ucom_code") | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz) | EN, "ucom_code"),
   RGN(1 | RA("ucom_data") | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz) | EN, "ucom_data"),
   RGN(2 | RA("usbh_code") | V, CODE   | SRD("usbh_code") | RSIC(usbhcsz) | EN, "usbh_code"),
   RGN(3 | RA("usbh_data") | V, DATARWC| SRD("usbh_data") | RSIC(usbhdsz) | EN, "usbh_data"),
   RGN(4 | 0x40040000      | V, IOR                       | (17 << 1)     | EN, "USBHS"),
   RGN(5 | V, 0, 0),
   RGN(6 | V, 0, "dynamic region"),  /* server slot for USBH ms portal */
   RGN(7 | V, 0, "stack"),
   RGN(8 | V, 0, "phantom"),  /* callback portal client pmsg slot <4> */
   RGN(9 | V, 0, "phantom"),  /* callback portal client pmsg slot <4> */
};

/* MPA template for usbhdemo tasks using smxUSBH portals */
MPA mpa_tmplt_usbhdp = 
{
   RGN(0 | RA("ucom_code")  | V, CODE   | SRD("ucom_code")  | RSIC(ucomcsz)  | EN, "ucom_code"),
   RGN(1 | RA("ucom_data")  | V, DATARWC| SRD("ucom_data")  | RSIC(ucomdsz)  | EN, "ucom_data"),
   RGN(2 | RA("usbhdp_code")| V, CODE   | SRD("usbhdp_code")| RSIC(usbhdpcsz)| EN, "usbhdp_code"),
   RGN(3 | RA("usbhdp_data")| V, DATARWC| SRD("usbhdp_data")| RSIC(usbhdpdsz)| EN, "usbhdp_data"),
   RGN(4 | V, 0, 0),
  #if CP_PORTAL
   RGN(5 | V, 0, "cp pmsg"),  /* console portal pmsg */
  #else
   RGN(5 | 0x40011000       | V, IOR                        | ( 9 << 1)      | EN, "USART1"),  /*<5>*/
  #endif
   RGN(6 | V, 0, "dynamic region"),
   RGN(7 | V, 0, "stack"),
};
#endif /* SMXUSBH */

#if defined(SMXNS)
/* MPA template for NetLo task */
MPA mpa_tmplt_nslo =
{
   RGN(0 | RA("ucom_code") | V, CODE   | SRD("ucom_code") | RSIC(ucomcsz)  | EN, "ucom_code"),
   RGN(1 | RA("ucom_data") | V, DATARWC| SRD("ucom_data") | RSIC(ucomdsz)  | EN, "ucom_data"),
   RGN(2 | RA("nslo_code") | V, CODE   | SRD("nslo_code") | RSIC(nslocsz)  | EN, "nslo_code"),
   RGN(3 | RA("nslo_data") | V, DATARWC| SRD("nslo_data") | RSIC(nslodsz)  | EN, "nslo_data"),
   RGN(4 | RA("EMAC_BUF")  | V, DATARWC                   | RSIC(emacbufsz)| EN, "EMAC_BUF"),
   RGN(5 | 0x40020000      | V, IOR    | N3|N57           | (15 << 1)      | EN, "ETH, RCC, GPIO"),  /*<3>*/
   RGN(6 | V, 0, "dynamic region"),  /* server slot for NS portal */
   RGN(7 | V, 0, "stack"),
};
#endif /* SMXNS */

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
