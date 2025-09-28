/*
* mpa8.c                                                     Version 5.4.0
*
* MPA ARMM8 templates for NXP LPC55Sxx processors.
*
* Copyright (c) 2017-2025 Micro Digital Inc.
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

/* Default MPA template assigned when a task is created. Allows access to all 
   MPU regions. <1> */
MPA mpa_dflt = 
{
   RGN(0, RA("rom_block") | PCODE,   RLA("rom_block") | AI(0) | EN, "rom_block"),
   RGN(1, RA("sram_block")| PDATARW, RLA("sram_block")| AI(0) | EN, "sram_block"),
   RGN(2, 0x40086000      | PIOR,    0x40086FE0       | AI(1) | EN, "USART0"),
   RGN(3, 0, 0, 0),
   RGN(4, 0, 0, 0),
   RGN(5, 0, 0, 0),
   RGN(6, 0, 0, 0),
   RGN(7, 0, 0, 0),
};

/* Init/Exit MPA template. Used by ainit() and aexit(). Needs IO regions for
   middleware module init/exit. */
MPA mpa_tmplt_init = 
{
   RGN(0, RA("rom_block") | PCODE,   RLA("rom_block") | AI(0) | EN, "rom_block"),
   RGN(1, RA("sram_block")| PDATARW, RLA("sram_block")| AI(0) | EN, "sram_block"),
   RGN(2, 0x40000000      | PIOR,    0x4010FFE0       | AI(1) | EN, "IO Regs"),
};

/* MPA template for idle task for normal operation */
MPA mpa_tmplt_idle = 
{
   RGN(0, RA("sys_code")  | PCODE,   RLA("sys_code")  | AI(0) | EN, "sys_code"),
   RGN(1, RA("sys_data")  | PDATARW, RLA("sys_data")  | AI(0) | EN, "sys_data"),
   RGN(2, 0x40086000      | PIOR,    0x40086FE0       | AI(1) | EN, "USART0"),
   RGN(3, RA("EVB")       | PDATARW, RLA("EVB")       | AI(0) | EN, "EVB"),
};

/* MPA template for opcon task */
MPA mpa_tmplt_opcon = 
{
   RGN(0, RA("sys_code")   | PCODE,   RLA("sys_code")   | AI(0) | EN, "sys_code"),
   RGN(1, RA("sys_data")   | PDATARW, RLA("sys_data")   | AI(0) | EN, "sys_data"),
   RGN(2, 0x40086000       | IOR,     0x40086FE0        | AI(1) | EN, "USART0"),
   RGN(3, RA("EVB")        | DATARW,  RLA("EVB")        | AI(0) | EN, "EVB"),
};

/*===========================================================================*
                                 UMODE TEMPLATES
*===========================================================================*/

/* MPA template for console partition */
MPA mpa_tmplt_cp = 
{
   RGN(0, RA("cp_code")    | CODE,    RLA("cp_code")    | AI(0) | EN, "cp_code"),
   RGN(1, RA("cp_data")    | DATARW,  RLA("cp_data")    | AI(0) | EN, "cp_data"),
   RGN(2, RA("ucom_code")  | CODE,    RLA("ucom_code")  | AI(0) | EN, "ucom_code"),
   RGN(3, 0x40086000       | IOR,     0x40086FE0        | AI(1) | EN, "USART0"),
// RGN(4 | V, 0, "pmsg"),  /* console portal pmsg slot */
};

/* MPA template for led task */
MPA mpa_tmplt_led = 
{
   RGN(0, RA("ucom_code")  | CODE,   RLA("ucom_code")   | AI(0) | EN, "ucom_code"),
   RGN(1, RA("ucom_data")  | DATARW, RLA("ucom_data")   | AI(0) | EN, "ucom_data"),
   RGN(2, RA("led_code")   | CODE,   RLA("led_code")    | AI(0) | EN, "led_code"),
   RGN(3, 0x4008C000       | IOR,    0x4008E4A0         | AI(1) | EN, "GPIO Ports"),
};

/* MPA template for fpu demo task */
MPA mpa_tmplt_fpu = 
{
   RGN(0, RA("ucom_code")  | CODE,   RLA("ucom_code")   | AI(0) | EN, "ucom_code"),
   RGN(1, RA("ucom_data")  | DATARW, RLA("ucom_data")   | AI(0) | EN, "ucom_data"),
   RGN(2, RA("fpu_code")   | CODE,   RLA("fpu_code")    | AI(0) | EN, "fpu_code"),
   RGN(3, RA("fpu_data")   | DATARW, RLA("fpu_data")    | AI(0) | EN, "fpu_data"),
  #if CP_PORTAL
   RGN(4, 0, 0, "cp pmsg"),  /* console portal pmsg slot */
  #else
   RGN(4, 0x40086000       | IOR,    0x40086FE0         | AI(1) | EN, "USART0"),
  #endif
};

/* Notes:
   1. The default MPA must include all MPU regions. However templates need not 
      include unused upper regions, because mp_MPULoad() automatically clears
      them, and the stack slot, MPU[7] is loaded by other means.
   2. USART for demo msgs.
*/
#endif /* SMX_CFG_SSMX */
