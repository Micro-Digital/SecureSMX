/*
* xem.c                                                     Version 5.4.0
*
* smx Error Manager
*
* Copyright (c) 1990-2025 Micro Digital Inc.
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
* Author: Ralph Moore
*
*****************************************************************************/

#include "xsmx.h"

#pragma section = "EB" /* error buffer -- size defined in tsmx.icf */

#if SMX_CFG_SSMX
extern void smxu_EM(SMX_ERRNO errno, u8 sev);  /* in svc.c */
#endif

const char *const smx_errmsgs[] =  /* (2nd const puts into ROM) */
{
   /* smx error messages */
   "smx OK",
   "smx TIMEOUT",
   "smx PRIVILEGE VIOLATION",

   "smx ABORT",
   "smx BLK IN USE",
   "smx BROKEN Q",
   "smx CLIB ABORT",
   "smx EXCESS LOCKS",
   "smx EXCESS UNLOCKS",

   "smx HEAP ALRDY INIT",
   "smx HEAP BRKN",
   "smx HEAP ERROR",
   "smx HEAP FENCE BKN",
   "smx HEAP FIXED",
   "smx HEAP INIT FAIL",
   "smx HEAP RECOVER",
   "smx HEAP TIMEOUT",

   "smx HT DUP",
   "smx HT FULL",

   "smx INIT MOD FAIL",
   "smx INSUFF HEAP",
   "smx INSUFF UNLOCKS",

   "smx INV BCB",
   "smx INV CCB",
   "smx INV EGCB",
   "smx INV EQCB",
   "smx INV FUNCTION",
   "smx INV LCB",
   "smx INV MCB",
   "smx INV MUCB",
   "smx INV OP",
   "smx INV PAR",
   "smx INV PCB",
   "smx INV PICB",
   "smx INV PRI",
   "smx INV SCB",
   "smx INV TCB",
   "smx INV TIME",
   "smx INV TMRCB",
   "smx INV XCB",

   "smx LQ OVFL",
   "smx LSR NOT OWN MTX",
   "smx MTX ALRDY FREE",
   "smx MTX NON ONR REL",
   "smx NO ISR",
   "smx NOT MSG OWNER",

   "smx OP NOT ALLOWED",

   "smx OUT OF BCBS",
   "smx OUT OF EGCBS",
   "smx OUT OF EQCBS",
   "smx OUT OF LCBS",
   "smx OUT OF MCBS",
   "smx OUT OF MUCBS",
   "smx OUT OF PCBS",
   "smx OUT OF PICBS",
   "smx OUT OF SCBS",
   "smx OUT OF STACKS",
   "smx OUT OF TCBS",
   "smx OUT OF TMRCBS",
   "smx OUT OF XCBS",

   "smx POOL EMPTY",
   "smx Q FIXED",
   "smx RQ ERROR",

   "smx SEM CTR OVFL",
   "smx SMX INIT FAIL",
   "smx SSR IN ISR",
   "smx STK OVFL",
   "smx MAIN STK OVFL",
   "smx TOKEN VIOLATION",
   "smx TOO MANY HEAPS",
   "smx UNKNOWN SIZE",

   "smx WAIT NOT ALLOWED",
   "smx WRONG HEAP",
   "smx WRONG MODE",
   "smx WRONG POOL",

   /* smxBase error messages */
   "sb INSUFF DAR",
   "sb INV ALIGN",
   "sb INV BP",
   "sb INV DAR",
   "sb INV LSR",
   "sb INV OFFSET",
   "sb INV POOL",
   "sb INV PRI",
   "sb INV SIZE",
   "sb LQ OVFL",
   "sb OUT OF LCBS",
   "sb OUT OF PCBS",
   "sb PRIV VIOL",
   "sb DAR INIT FAIL",
   "sb INV PAR",

   /* hardware faults */
   "CPU BUS FAULT",
   "CPU HARD FAULT",
   "CPU MEMORY MANAGE FAULT",
   "CPU USAGE FAULT",

   #if SMX_CFG_SSMX
   /* portal error messages */
   "CLIENT TIMEOUT",
   "INVALID TYPE",
   "INVALID COMMAND",
   "INVALID FUNCTION",
   "INVALID SID",
   "INVALID SSID",
   "INVALID SIZE",
   "NO PMSG",
   "PORTAL CLOSED",
   "PORTAL NOT EXIST",
   "PORTAL NOT OPEN",
   "SERVER TIMEOUT",
   "TRANSMISSION ERROR",
   "TRANSFER INCOMPLETE",
   #endif

   "end",
};

/* error manager clear */
void smx_EMClear(void)
{
   u32 *p = (u32*)smx_ebi;
   u8  *pb;
   u32  i;
   u32  ebsz = (u32)__section_size("EB");

   for (i = ebsz/4; i > 0; i--)
      *p++ = 0;
   pb = smx_errctrs;
   for (i = SMX_NUM_ERRORS; i > 0; i--)
      *pb++ = 0;
   smx_errctr = 0;
   smx_errno = SMXE_OK;
}

/* error manager 
   Note: not reentrant, smx_srnest must be > 0 when called. */
void smx_EM(SMX_ERRNO errno, u8 sev)
{
   void*    h = NULL;
   LCB_PTR  l = NULL;
   TCB_PTR  t = NULL;
   
   if (smx_clsr)
   {
      l = smx_clsr;
      h = (void*)smx_clsr;
      l->err = errno;
   }
   else
   {
      t = smx_ct;
      h = (void*)smx_ct;
      t->err = errno;
   }

   smx_errno = errno;
   smx_errctr++;
   smx_errctrs[errno]++;

   if (smx_eben)
   {
      /* save error information in error buffer */
      CPU_FL istate = sb_IntStateSaveDisable();
      EREC_PTR p = (smx_ebn > smx_ebx ? smx_ebi : smx_ebn);
      p->etime = smx_etime;
      p->err = errno;
      p->handle = h;
      smx_ebn = p + 1;
      sb_IntStateRestore(istate);
   }
   smx_EVB_LOG_ERROR(errno, h);
   sb_MsgOut(SB_MSG_ERR, smx_errmsgs[errno]);
   smx_EMHook(errno, h, sev);
}

/* necessary to insure that r0 = errno and r1 = h. */
#pragma inline = never
void smx_EM_SVC(SMX_ERRNO errno, u8 sev)
{
   sb_SVC(0);  /* trigger EM SVC Handler */
}

/* error manager control */ 
void smx_EMC(SMX_ERRNO errno, u8 sev)
{
  #if !defined(SMX_DEBUG)
   if (!smx_InMS())
   #if SMX_CFG_SSMX
      smxu_EM(errno, sev);
   #else
      smx_EM_SVC(errno, sev);
   #endif
   else
  #endif
      smx_EM(errno, sev);
}

/* error manager initialize */
bool smx_EMInit(void)
{
   bool pass = true;
   u32  ebsz = (u32)__section_size("EB")/sizeof(struct EREC);
   smx_ebi   = (EREC*)__section_begin("EB");

   if (ebsz)
   {
      if (smx_ebi)
      {
         smx_ebn = smx_ebi;
         smx_ebx = smx_ebi + ebsz - 1;
         smx_EMClear();
         smx_eben = true;
      }
      else
      {
         smx_eben = 0;
         pass = false;
      }
   }
   return(pass);
}

/* error buffer display */
void smx_EBDisplay(void)
{
   EREC *ebp = smx_ebi;
   u32  row;

   for (row = 1; ebp <= smx_ebx; ebp++, row++)
   {
      if (ebp->err == SMXE_OK)
         return;
      if (ebp == smx_ebn - 1)
         sb_ConWriteChar(0, row, SB_CLR_WHITE, SB_CLR_BLACK, !SB_CON_BLINK, '*'); /* mark last error */

      sb_ConWriteString(1, row, SB_CLR_LIGHTRED, SB_CLR_BLACK, !SB_CON_BLINK, smx_errmsgs[ebp->err]);
      if (row == (SB_CON_ROWS_MAX) - 1) /* scroll to the top */
         row = 1;
   }
}

