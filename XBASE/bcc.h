/*
* bcc.h                                                     Version 6.2.0
*
* C Compiler Definitions and Run-Time Library Replacements include file.
* The main purposes of this file are to:
*
* 1. define macros to map onto some compiler keywords, for portability.
* 2. include C run-time library header files and definitions that MUST
*    precede smx header files when there is an issue such as the heap
*    functions vs. smx's translation macros.
* 3. supply prototypes for functions missing in some compilers' RTLs.
* 4. include common RTL headers so it's not necessary to add includes
*    to many files.
*
* Copyright (c) 2003-2026 Micro Digital Inc.
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

#ifndef SB_BCC_H
#define SB_BCC_H

/*===========================================================================*
*          C RTL Headers and Defines that Must Precede smx Headers           *
*===========================================================================*/

/*
*  Note: Including stdlib.h ahead of xapi.h, since otherwise, translation
*        macros in xapi.h would cause heap call names in stdlib.h to be
*        translated to smx names, causing build errors.
*/
#include <stdlib.h>


/*===========================================================================*
*                             Other C RTL Headers                            *
*===========================================================================*/
/*
   These do not need to precede smx headers, but are here for convenience.
*/

#include <string.h>     /* memcpy, memset, etc. */
#include <stdarg.h>

#if defined(SB_CPU_ARMM) && defined(__ICCARM__)
#include <intrinsics.h>
#endif


/*===========================================================================*
*                             Compiler Defines                               *
*===========================================================================*/

#if defined(__IAR_SYSTEMS_ICC__)
#include "ctype.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* packed aligns structure members tightly rather than padding to 32-bit boundaries */

#if defined(__IAR_SYSTEMS_ICC__)        /* IAR EWARM */
#define __inline__      inline
#define __interdecl
#define __interrupt
#define __packed        __packed
#define __packed_gnu
#define __packed_pragma 1
#define __short_enum_attr
#define __unaligned
int _stricmp(const char *__s1, const char *__s2);
int _strnicmp(const char *__s1, const char *__s2, size_t __n);
char * _strupr(char * s);
char * _ultoa(unsigned long v, char * str, int r);

#else
#error Define inline, packed, and similar macros for your compiler in bcc.h.
#define __inline__
#define __interdecl
#define __interrupt
#define __packed
#define __packed_gnu
#define __packed_pragma 1
#define __short_enum_attr
#define __unaligned
#endif

#ifdef __cplusplus
}
#endif

#define SB_PACKED_STRUCT_SUPPORT 1

#define SB_BYTES_TO_CHARS(size)  (size)


/*===========================================================================*
*                               Other RTL Stuff                              *
*===========================================================================*/

/* itoa(), ltoa(), etc. */

/* These functions are not supplied with some compilers so we define our
   own versions. Here we define prototypes in case the compiler doesn't.
*/

#ifdef __cplusplus
extern "C" {
#endif
char * _itoa(int val, char *str, int radix);
char * _ltoa(long val, char *str, int radix);
char * _ultoa(unsigned long val, char *str, int radix);
char * _strupr(char *str);
#ifdef __cplusplus
}
#endif

#define  itoa   _itoa
#define  ltoa   _ltoa
#define  ultoa  _ultoa
#define  strupr _strupr


/* Configuration: Specify whether to use C library functions (1) or ours (0).
   Set to 0 if the compiler does not supply the function (or it fails).
   XTOA represents itoa() and ltoa().
*/
#if defined(__IAR_SYSTEMS_ICC__)
#define SB_CC_XTOA      0
#define SB_CC_ULTOA     0
#define SB_CC_STRICMP   0
#define SB_CC_STRNICMP  1
#define SB_CC_STRUPR    0
#define SB_CC_TOUPPER   1
#define SB_CC_TIME_FUNC 1
#else
#error Define SB_CC macros for your compiler in bcc.h.
#endif


/*===========================================================================*
*                                  Undefines                                 *
*===========================================================================*/

/* Undefine symbols that conflict with our definitions. Global .h files should
   never #define macros with very short names. The substitutions cause strange
   compiler errors that are difficult to understand.
*/
#if defined(SCB)  /* ARM CMSIS core_*.h defines this */
#undef SCB
#endif

#endif /* SB_BCC_H */

