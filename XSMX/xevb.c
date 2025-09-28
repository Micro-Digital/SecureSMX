/*
* xevb.c                                                    Version 5.4.0
*
* smx Event Buffer functions.
*
* Copyright (c) 2002-2025 Micro Digital Inc.
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
* Authors: Marty Cochran, David Moore, Ralph Moore
*
*****************************************************************************/

#include "xsmx.h"

#if SMX_CFG_EVB
#pragma section = "EVB"

u32   smx_sched_save;  /* to save the smx_sched flags for smx_EVB_LOG macros */

/*============================================================================
*                                 FUNCTIONS                                  *
============================================================================*/

/* Event Buffer Initialize. Space for EVB is allocated by the linker. Clears 
   EVB, intitializes smx_evb pointers, and sets smx_evben = flags, if EVB is 
   large enough.
*/
bool smx_EVBInit(u32 flags)
{
   u32 evbsz = (u32)__section_size("EVB")/4;
   if (evbsz >= SMX_EVB_MAX_REC)
   {
      smx_evbi = (u32 *)__section_begin("EVB");
      smx_evbn = smx_evbi;
      smx_evbx = smx_evbi + evbsz - SMX_EVB_MAX_REC;
      for (u32* p = smx_evbi; p < (smx_evbi + evbsz); p++)
         *p = 0;
      smx_evben = flags; /* enable logging of selected events */
      return(true);
   }
   else
   {
      smx_evben = 0; /* disable logging */
      return(false);
   }
}

/* EVB Log SSR functions for 0 to 6 parameters. Invoked by smx_SSR_ENTERn()
   macros -- see xapi.h.
*/
void smx_EVBLogSSR0(u32 id)
{
   if (smx_evben & (id & SMX_ID_MASK_SSRGRP))
   {
      u32 *p;
      sb_INT_DISABLE(); /* assumes interrupts are enabled */
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550004 | SMX_EVB_RT_SSR;
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = id;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogSSR1(u32 id, u32 p1)
{
   if (smx_evben & (id & SMX_ID_MASK_SSRGRP))
   {
      u32 *p;
      sb_INT_DISABLE(); /* assumes interrupts are enabled */
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550005 | SMX_EVB_RT_SSR;
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = id;
      *p++ = p1;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogSSR2(u32 id, u32 p1, u32 p2)
{
   if (smx_evben & (id & SMX_ID_MASK_SSRGRP))
   {
      u32 *p;
      sb_INT_DISABLE(); /* assumes interrupts are enabled */
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550006 | SMX_EVB_RT_SSR;
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = id;
      *p++ = p1;
      *p++ = p2;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogSSR3(u32 id, u32 p1, u32 p2, u32 p3)
{
   if (smx_evben & (id & SMX_ID_MASK_SSRGRP))
   {
      u32 *p;
      sb_INT_DISABLE(); /* assumes interrupts are enabled */
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550007 | SMX_EVB_RT_SSR;
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = id;
      *p++ = p1;
      *p++ = p2;
      *p++ = p3;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogSSR4(u32 id, u32 p1, u32 p2, u32 p3, u32 p4)
{
   if (smx_evben & (id & SMX_ID_MASK_SSRGRP))
   {
      u32 *p;
      sb_INT_DISABLE(); /* assumes interrupts are enabled */
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550008 | SMX_EVB_RT_SSR;
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = id;
      *p++ = p1;
      *p++ = p2;
      *p++ = p3;
      *p++ = p4;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogSSR5(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5)
{
   if (smx_evben & (id & SMX_ID_MASK_SSRGRP))
   {
      u32 *p;
      sb_INT_DISABLE(); /* assumes interrupts are enabled */
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550009 | SMX_EVB_RT_SSR;
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = id;
      *p++ = p1;
      *p++ = p2;
      *p++ = p3;
      *p++ = p4;
      *p++ = p5;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogSSR6(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6)
{
   if (smx_evben & (id & SMX_ID_MASK_SSRGRP))
   {
      u32 *p;
      sb_INT_DISABLE(); /* assumes interrupts are enabled */
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x5555000A | SMX_EVB_RT_SSR;
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = id;
      *p++ = p1;
      *p++ = p2;
      *p++ = p3;
      *p++ = p4;
      *p++ = p5;
      *p++ = p6;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogSSR7(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6, u32 p7)
{
   if (smx_evben & (id & SMX_ID_MASK_SSRGRP))
   {
      u32 *p;
      sb_INT_DISABLE(); /* assumes interrupts are enabled */
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x5555000B | SMX_EVB_RT_SSR;
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = id;
      *p++ = p1;
      *p++ = p2;
      *p++ = p3;
      *p++ = p4;
      *p++ = p5;
      *p++ = p6;
      *p++ = p7;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

/* EVB Log User functions for 0 to 6 parameters for use in user code. */
void smx_EVBLogUser0(void* h)
{
   if (smx_evben & SMX_EVB_EN_USER)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550004 | SMX_EVB_RT_USER;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (u32)h;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogUser1(void* h, u32 p1)
{
   if (smx_evben & SMX_EVB_EN_USER)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550005 | SMX_EVB_RT_USER;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (u32)h;
      *p++ = p1;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogUser2(void* h, u32 p1, u32 p2)
{
   if (smx_evben & SMX_EVB_EN_USER)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550006 | SMX_EVB_RT_USER;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (u32)h;
      *p++ = p1;
      *p++ = p2;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogUser3(void* h, u32 p1, u32 p2, u32 p3)
{
   if (smx_evben & SMX_EVB_EN_USER)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550007 | SMX_EVB_RT_USER;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (u32)h;
      *p++ = p1;
      *p++ = p2;
      *p++ = p3;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogUser4(void* h, u32 p1, u32 p2, u32 p3, u32 p4)
{
   if (smx_evben & SMX_EVB_EN_USER)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550008 | SMX_EVB_RT_USER;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (u32)h;
      *p++ = p1;
      *p++ = p2;
      *p++ = p3;
      *p++ = p4;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogUser5(void* h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5)
{
   if (smx_evben & SMX_EVB_EN_USER)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550009 | SMX_EVB_RT_USER;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (u32)h;
      *p++ = p1;
      *p++ = p2;
      *p++ = p3;
      *p++ = p4;
      *p++ = p5;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogUser6(void* h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6)
{
   if (smx_evben & SMX_EVB_EN_USER)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x5555000A | SMX_EVB_RT_USER;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (u32)h;
      *p++ = p1;
      *p++ = p2;
      *p++ = p3;
      *p++ = p4;
      *p++ = p5;
      *p++ = p6;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

/* Log entry into print ring buffer. Called by sa_Print(). */
void smx_EVBLogUserPrint(u32 time, u32 index)
{
   if (smx_evben & SMX_EVB_EN_USER)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550005 | SMX_EVB_RT_PRINT;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = time;
      *p++ = index;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

/* Log system events. */
void smx_EVBLogISR(void* isr)
{
   if (smx_evben & SMX_EVB_EN_ISR)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550004 | SMX_EVB_RT_ISR;
      *p++ = (u32)smx_ct;
      *p++ = (u32)isr;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogISRRet(void* isr)
{
   if (smx_evben & SMX_EVB_EN_ISR)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550003 | SMX_EVB_RT_ISR_RET;
      *p++ = sb_PtimeGet();
      *p++ = (u32)isr;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogLSR(void* lsr)
{
   if (smx_evben & SMX_EVB_EN_LSR && !((LCB_PTR)lsr)->flags.nolog)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550004 | SMX_EVB_RT_LSR;
      *p++ = (u32)smx_ct;
      *p++ = (u32)lsr;
      *p++ = sb_PtimeGet();
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogLSRRet(void* lsr)
{
   if (smx_evben & SMX_EVB_EN_LSR && !((LCB_PTR)lsr)->flags.nolog)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550003 | SMX_EVB_RT_LSR_RET;
      *p++ = sb_PtimeGet();
      *p++ = (u32)lsr;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogError(u32 errno, void* h)
{
   if (smx_evben & SMX_EVB_EN_ERR)
   {
      u32 istate, *p;
      istate = sb_IntStateSaveDisable();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550005 | SMX_EVB_RT_ERR;
      *p++ = sb_PtimeGet();
      *p++ = smx_etime;
      *p++ = errno;
      *p++ = (u32)h;
      smx_evbn = p;
      sb_IntStateRestore(istate);
   }
}

void smx_EVBLogInvoke(void* isr, LCB_PTR lsr, u32 par)
{
   if (smx_evben & SMX_EVB_EN_LSR)
   {
      u32 *p;
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550006 | SMX_EVB_RT_INVOKE;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (u32)isr;
      *p++ = (u32)lsr;
      *p++ = par;
      smx_evbn = p;
   }
}

void smx_EVBLogSSRRet(u32 rv, u32 id)
{
   if (smx_evben & (id & SMX_ID_MASK_SSRGRP))
   {
      u32 *p;
      sb_INT_DISABLE();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550005 | SMX_EVB_RT_SSR_RET;
      *p++ = sb_PtimeGet();
      *p++ = (smx_clsr != NULL) ? (u32)smx_clsr : (u32)smx_ct;
      *p++ = smx_sched;
      *p++ = rv;
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogTaskAutoStop(void)
{
   if (smx_evben & SMX_EVB_EN_TASK)
   {
      u32 *p;
      sb_INT_DISABLE();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550003 | SMX_EVB_RT_AUTOSTOP;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogTaskEnd(void)
{
   smx_sched_save = smx_sched;
}

void smx_EVBLogTaskResume(void)
{
   if (smx_evben & SMX_EVB_EN_TASK)
   {
      u32 *p;
      sb_INT_DISABLE();
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550004 | SMX_EVB_RT_RESUME;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (SMX_RESUMED | smx_sched_save);
      smx_evbn = p;
      sb_INT_ENABLE();
   }
}

void smx_EVBLogTaskStart(void)
{
   if (smx_evben & SMX_EVB_EN_TASK)
   {
      u32 *p;
      p = (smx_evbn > smx_evbx) ? smx_evbi : smx_evbn;
      *p++ = 0x55550004 | SMX_EVB_RT_START;
      *p++ = sb_PtimeGet();
      *p++ = (u32)smx_ct;
      *p++ = (SMX_STARTED | smx_sched_save);
      smx_evbn = p;
   }
}
#endif  /* SMX_CFG_EVB */

