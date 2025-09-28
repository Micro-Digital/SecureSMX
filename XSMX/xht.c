/*
* xht.c                                                     Version 5.4.0
*
* smx Handle Table functions.
*
* These functions create, add to, and search the handle table. A handle table 
* is necessary for smxAware. Usually, only smx_HTInit() and smx_HTAdd() 
* (called by the smx_HT_ADD() macro) are used by application code, but the 
* others can be used if desired.
*
* Copyright (c) 1991-2025 Micro Digital Inc.
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
* Authors: Ron Schow, David Moore
*
*****************************************************************************/

#include "xsmx.h"

#if defined(SMX_DEBUG)  /* conditions out the whole file if not defined */

static HTREC_PTR firstfree;  /* points at or below the first free entry */

/*
*  smx_HTAdd()
*
*  Adds an entry to the handle table. Do not directly call this
*  function. Instead, call smx_HT_ADD() which calls this function
*  when SMX_DEBUG is defined.
*/
bool smx_HTAdd(void* h, const char* name)
{
   HTREC_PTR  p;

   if (!h || !name)
      return(false);

   if (!smx_htpres)   /* auto-initialize in case here from global constructor */
      smx_HTInit();

   smx_htchg = true;  /* tells smxAware it needs to re-read HT (smxAware sets false) */

   /* Check if name is already in the table due to user error. */
   for (p = smx_hti; p < smx_htn; p++)
   {
      if (strcmp(p->name, name) == 0)
         smx_ERROR_RET(SMXE_HT_DUP, false, 0);
   }

   /* firstfree may be pointing below first free entry, so scan if nec. */
   for (p = firstfree; p <= smx_htx; p++)
   {
      if (p->h == NULL)
      {
         smx_LSRsOff();     /* protect against reentrancy by another task or LSR */
         if (p->h == NULL)  /* ensure still NULL */
         {
            p->h = h;
            smx_LSRsOn();   /* once handle is set, slot is unavailable */
            p->name = name;
            if (p == smx_htn)
               smx_htn++;
            firstfree = p+1;
            break;
         }
         else
         {
            smx_LSRsOn();
         }
      }
   }

   if (p > smx_htx)
      smx_ERROR_RET(SMXE_HT_FULL, false, 0);

   return(true);
}

/*
*  smx_HTDelete()
*
*  Removes a handle from the handle table. Searches for the target
*  handle and deletes the handle and associated ASCII string.
*/
bool smx_HTDelete(void* h)
{
   HTREC_PTR  p;

   for (p = smx_hti; p < smx_htn; p++)
   {
      if (p->h == h)
      {
         p->name = "empty";
         p->h = NULL;        /* change last since this field reserves the slot */
         if (p < firstfree)
            firstfree = p;
         smx_htchg = true;   /* tells smxAware it needs to re-read HT (smxAware sets false) */
         return(true);
      }
   }
   return(false);
}

/*
*  smx_HTGetHandle()
*
*  Searches the handle table for name and returns the associated handle,
*  if found. Returns NULL if name not found.
*/
void* smx_HTGetHandle(const char* name)
{
   HTREC_PTR  p;

   if (strcmp(name, "smx_ct") == 0)  /* smx_ct needed by DLMs to get their own handle */
      return(smx_ct);

   for (p = smx_hti; p < smx_htn; p++)
   {
      if (strcmp(p->name, name) == 0)
         return(p->h);
   }
   return(NULL);
}

/*
*  smx_HTGetName()
*
*  Searches the handle table for handle h and returns a pointer to the
*  associated name, if found. Returns null string if handle not found.
*/
const char * smx_HTGetName(void* h)
{
   HTREC_PTR  p;

   for (p = smx_hti; p < smx_htn; p++)
   {
      if (p->h == h)
         return(p->name);
   }
   return("");
}

/*
*  smx_HTInit()
*
*  Allocates and initializes the handle table. Called by smx_Go().
*/
void smx_HTInit(void)
{
  #if SMX_SIZE_HT == 0
   return;  /* (smx_hti, smx_htn, smx_htx, smx_htpres are statically init to 0) */

  #else
   if (smx_htpres)
      return;

   if ((smx_hti = (HTREC_PTR)smx_HeapMalloc(SMX_SIZE_HT * sizeof(HTREC))) != 0)
   {
      smx_htx = smx_hti + SMX_SIZE_HT - 1;

      /* Clear handle table. Note that the name field should normally be
         set to the null string ("") in free entries, but it is ok to set
         it to 0 initially, since all routines below scan only up to smx_htn.
         That is, they only scan through the last entry that was ever used
         and those will either be in use or if free, will have been freed
         by smx_HT_DELETE() which sets the name field to the null string.
      */
      memset((u8 *)smx_hti, 0, SMX_SIZE_HT * sizeof(HTREC));

      firstfree = smx_htn = smx_hti;
      smx_htpres = true;

      /* Add smx objects to handle table. */
     #if SMX_CFG_EVB
      /* create some pseudo handles to log these events in event buffer */
      smx_InvokeH   = smx_SysPseudoHandleCreate();  /* for LSR invoke events */
      smx_TickISRH  = smx_SysPseudoHandleCreate();
      smx_HT_ADD(smx_InvokeH, "smx_Invoke");
      smx_HT_ADD(smx_TickISRH, "smx_TickISR");
     #endif
   }
  #endif /* SMX_SIZE_HT */
}
#endif /* SMX_DEBUG */

