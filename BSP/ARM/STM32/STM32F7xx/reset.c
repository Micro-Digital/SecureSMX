/*
* reset.c                                                   Version 6.0.0
*
* Reset vector and stack pointer for STM32F7xxxx. Put at start of every
* major code region in case it is biggest and MpuPacker locates it first.
* These must be at offset 0 of the boot flash. They are normally the first
* two entries of the EVT. __low_level_init sets NVIC_VTOR to locate the EVT.
*
* Copyright (c) 2020-2026 Micro Digital Inc.
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

#pragma language=extended  /* allow IAR extended syntax in this file */
#pragma segment="CSTACK"

#ifdef __cplusplus
extern "C" {
#endif
void __iar_program_start(void);  /* cmain.s (startup code in IAR dlib) */
#ifdef __cplusplus
}
#endif

/*
   A reset block needs to be created for any code block that could be biggest
   and ordered first by MpuPacker, except sys_code which has the full EVT.
*/

/* Needed since ucom_code is put at start of sys_code (which has full EVT). */
 #ifdef __cplusplus
__root extern const intvec_elem __ucom_reset[] @ ".ucom.reset" =
 #else
__root const intvec_elem __ucom_reset[] @ ".ucom.reset" =
 #endif
{
/*  0 */  { .__ptr = __sfe( "CSTACK" ) },  /* Initial Stack Pointer */
/*  1 */  __iar_program_start,             /* Reset Vector (Entry Point) */
};

#if defined(MW_FATFS)
 #ifdef __cplusplus
__root extern const intvec_elem __fs_reset[] @ ".fs.reset" =
 #else
__root const intvec_elem __fs_reset[] @ ".fs.reset" =
 #endif
{
/*  0 */  { .__ptr = __sfe( "CSTACK" ) },  /* Initial Stack Pointer */
/*  1 */  __iar_program_start,             /* Reset Vector (Entry Point) */
};
#endif
