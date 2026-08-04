/*
* bcc.c                                                     Version 6.2.0
*
* C Compiler and Run-Time Library replacement routines. Add C RTL routines
* here that are either not present for some compilers or that are buggy.
* bcc.h sets SB_CC_ preprocessor symbols used below.
*
* Copyright (c) 1996-2026 Micro Digital Inc.
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
* Authors: David Moore, Marty Cochran
*
*****************************************************************************/

#include "bbase.h"

#if SMX_CFG_SSMX && defined(__IAR_SYSTEMS_ICC__)
#pragma section_prefix = ".ucom"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void reverse(char *s);

/* Run-Time Library Replacement Routines */

#if !SB_CC_XTOA
char * _itoa(int val, char *str, int radix)
{
   return _ltoa(val, str, radix);
}

char * _ltoa(long val, char *str, int radix)
{
   bool  neg;
   char *ptr;
   char  ch;
   unsigned long uval;

   if (radix < 2 || radix > 36)
   {
      if (str)
         *str = '\0';
      return str;
   }
   ptr = str;
   /* val can only be negative if radix is 10 */
   if (radix == 10 && val < 0)
   {
      neg = true;
      uval = (unsigned long) -val;
   }
   else
   {
      neg = false;
      uval = (unsigned long) val;
   }
   /* generate digits in reverse order */
   do {
      ch = (char)(uval % radix);  /* result <= 36 (this is modulo not div) */
      *ptr++ = (char)(ch < 10 ? ch + '0' : ch - 10 + 'A');
   } while ((uval = uval / radix) > 0);
   if (neg)
      *ptr++ = '-';
   *ptr = '\0';
   reverse(str);
   return str;
}
#endif /* !SB_CC_XTOA */

#if !SB_CC_ULTOA
char * _ultoa(unsigned long uval, char *str, int radix)
{
   char *ptr;
   char  ch;

   if (radix < 2 || radix > 36)
   {
      if (str)
         *str = '\0';
      return str;
   }
   ptr = str;
   /* generate digits in reverse order */
   do {
      ch = (char)(uval % radix);  /* result <= 36 (this is modulo not div) */
      *ptr++ = (char)(ch < 10 ? ch + '0' : ch - 10 + 'A');
   } while ((uval = uval / radix) > 0);
   *ptr = '\0';
   reverse(str);
   return str;
}
#endif /* !SB_CC_ULTOA */

#if !SB_CC_STRICMP
void chrupr(char * c)
{
    if(*c >= 'a' && *c <= 'z')
    {
        *c -= 'a' - 'A';
    }
}

int _stricmp(const char *__s1, const char *__s2)
{
    int i;
    int result = 0;
    char c1 = 0, c2 = 0;
    i = 0;
    while(__s1[i] != 0 && __s2[i] != 0)
    {
        c1 = __s1[i];
        c2 = __s2[i];
        chrupr(&c1);
        chrupr(&c2);
        result = (int)(c1 - c2);
        if(result != 0)
            break;
        i++;
    }
    if( __s1[i] != 0 || __s2[i] != 0)
        return (int)(__s1[i] - __s2[i]);
    else
        return result;
}
#endif /* SB_CC_STRICMP */

#if !SB_CC_STRNICMP
void __chrupr(char * c)
{
    if(*c >= 'a' && *c <= 'z')
    {
        *c -= 'a' - 'A';
    }
}

int _strnicmp(const char *__s1, const char *__s2, size_t __n)
{
    int i;
    int result = 0;
    char c1 = 0, c2 = 0;
    i = 0;
    while(__s1[i] != 0 && __s2[i] != 0)
    {
        c1 = __s1[i];
        c2 = __s2[i];
        __chrupr(&c1);
        __chrupr(&c2);
        result = (int)(c1 - c2);
        if(result != 0)
            break;
        i++;
        if (__n == i) return 0;     /* strings matched up to this point */
    }
    if( __s1[i] != 0 || __s2[i] != 0)
        return (int)(__s1[i] - __s2[i]);
    else
        return result;
}
#endif /* SB_CC_STRNICMP */

#if !SB_CC_STRUPR
char * _strupr(char *string)
{
   for (; *string; string++)
   {
      if (*string >= 'a' && *string <= 'z')
         *string -= 0x20;
   }
   return(string);
}
#endif /* !SB_CC_STRUPR */

#if !SB_CC_TOUPPER
int toupper(int c)
{
   if(c >= 'a' && c <= 'z')
   {
      c -= 0x20;
   }
   return c;
}
#endif /* !SB_CC_TOUPPER */

int putchar(int c)
{
#if (SB_CON_OUT)
   return sb_ConPutChar(c);
#else
   return -1;
#endif
}

/***** Helper Routines *****/

#if !SB_CC_XTOA || !SB_CC_ULTOA
void reverse(char *s)  /* reverse the string in place */
{
   char c;
   int i, j;

   for (i = 0, j = strlen(s)-1; i < j; i++, j--)
   {
      c    = s[i];
      s[i] = s[j];
      s[j] = c;
   }
}
#endif /* !SB_CC_XTOA || !SB_CC_ULTOA */

#ifdef __cplusplus
}
#endif
