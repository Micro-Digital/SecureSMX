/*
* reset.c                                                   Version 5.4.0
*
* Reset vector and stack pointer for STM32F7xxxx. Put at start of every
* major code region in case it is biggest and MpuPacker locates it first.
* These must be at offset 0 of the boot flash. They are normally the first
* two entries of the EVT. __low_level_init sets NVIC_VTOR to locate the EVT.
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
