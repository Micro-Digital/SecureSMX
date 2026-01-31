/*
* xlsr.c                                                    Version 6.0.0
*
* smx LSR Functions
*
* Copyright (c) 1989-2026 Micro Digital Inc.
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
* Authors: Ralph Moore, Alan Moore
*
*****************************************************************************/

#include "xsmx.h"

/* pseudo handles for event buffer */
void* smx_InvokeH;
void* smx_TickISRH;

/*
*  smx_LSRCreate()   SSR
*
*  Gets an LSR control block and loads its fields. Gets an LSR stack from the
*  main heap. stack is in an MPU region for ARMM7. lsr->mpap is set to 
*  mpa_default. Returns LSR handle.
*
*/
LCB_PTR smx_LSRCreate(FUN_PTR fun, u32 flags, const char* name, TCB_PTR htask, u32 ssz, LCB_PTR* lhp)
{
   LCB_PTR  lsr;  /* lsr handle */
   bool     pass;
   u8*      stp;  /* stack top pointer */

   smx_SSR_ENTER6(SMX_ID_LSR_CREATE, fun, flags, name, htask, ssz, lhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_LSR_CREATE, NULL);

   /* block multiple creates and verify current task has LSR create permission */
   if (pass = smx_ObjectCreateTest((u32*)lhp))
   {
      /* check flags */
      if ((flags & SMX_FL_UMODE) && (flags & SMX_FL_TRUST))
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_LSR_CREATE);

      /* get an LSR control block */
      if ((lsr = (LCB_PTR)sb_BlockGet(&smx_lcbs, 4)) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_LCBS, NULL, 0, SMX_ID_LSR_CREATE);

      /* load basic LSR fields */
      lsr->fun = fun;
      lsr->lhp = lhp;
      lsr->cbtype = SMX_CB_LSR;
      lsr->name = (name && *name) ? name : "lsr";

     #if SMX_CFG_EVB
      lsr->flags.nolog = (flags & SMX_FL_NOLOG) ? 1 : 0;
     #endif

      if (flags & SMX_FL_TRUST)
      {
         /* set trust flag and clear rest of LCB */
         lsr->flags.trust = 1;
         u32* p = (u32*)&lsr->htask;
         for (u32 i = 0; i < (sizeof(LCB) - 16)/4; i++)
            *p++ = 0;
      }
      else
      {
         lsr->flags.trust = 0;

         /* allocate stack block from main heap */
        #if SMX_CFG_SSMX
         #if SB_CPU_ARMM7
         stp = mp_RegionGetHeapR(&(lsr->sr), ssz, MP_MPU_STATSZ, MP_DATARW);
         #else /* SB_CPU_ARMM8 */
         if (flags & SMX_FL_UMODE)
            stp = mp_RegionGetHeapR(&(lsr->sr), ssz, 0, MP_DATARW);
         else
         {
            stp = (u8*)smx_HeapMalloc(ssz, 5); /*<1>*/
            lsr->sr.rbar = 0;
            lsr->sr.rlar = 0;
         }
         #endif
        #else
         stp = (u8*)smx_HeapMalloc(ssz, 5);
        #endif
      
         if (stp)
         {
           #if SMX_CFG_SSMX
            lsr->flags.umode = (flags & SMX_FL_UMODE) ? 1 : 0;
            lsr->mpap  = (MPR*)mpa_dflt;
            lsr->mpasz = MP_MPU_ACTVSZ;
           #endif
            lsr->htask = htask;
            lsr->stp = stp;
            lsr->sbp = (u8*)((u32)(stp + ssz) & (u32)(0 - SB_STACK_ALIGN));
           #if SMX_CFG_STACK_SCAN
            /* fill stack with fill value */
            memset(lsr->stp, SB_STK_FILL_VAL, lsr->sbp - lsr->stp);
           #endif
         }
         else
         {
            /* stack allocation failed <2>*/
            sb_BlockRel(&smx_lcbs, (u8*)lsr, 0); /* free LCB */
            pass = false;
         }
      }
   }
   if (pass)
      return((LCB_PTR)smx_SSRExit((u32)lsr, SMX_ID_LSR_CREATE));
   else
      return((LCB_PTR)smx_SSRExit((u32)NULL, SMX_ID_LSR_CREATE));
}

/*
*  smx_LSRDelete()   SSR
*  
*  Frees LSR's stack. If successful, clears MPA and frees it to mheap, if 
*  allocated, clears LCB and releases it to the LCB pool, and sets lsr = 
*  NULL.
*/
bool smx_LSRDelete(LCB_PTR* lhp)
{
   bool  pass;
   LCB_PTR  lsr = (lhp ? *lhp : NULL);

   smx_SSR_ENTER1(SMX_ID_LSR_DELETE, lhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_LSR_DELETE, false);

   /* verify that lsr is valid, that call is not from an lsr, and that the 
      current task has permission to delete this lsr */
   if (pass = smx_LCBTest(lsr, SMX_PRIV_HI))
   {
      /* free lsr stack <2> */
      pass = smx_HeapFree((void*)(lsr->stp)); 

      if (pass)
      {
        #if SMX_CFG_SSMX
         /* free MPA if allocated and not default */
         if (lsr->mpap && lsr->mpap != (MPR*)mpa_dflt)
            smx_HeapFree(lsr->mpap);
        #endif
         /* release LCB and set lsr = NULL */
         sb_BlockRel(&smx_lcbs, (u8*)lsr, sizeof(LCB));
         *lhp = NULL;
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_LSR_DELETE));
}

/*
*  smx_LSRInvokeF()   ISR-Safe Function for ISR use
*
*  Invokes an LSR by loading its handle into lq at smx_lqin and loading par 
*  after. smx_lqin is incremented cyclically around smx_lqx. If lq is full 
*  (smx_lqctr >= smx_cf.lq_size), reports SMXE_LQ_OVFL error.
*/
void smx_LSRInvokeF(LCB_PTR lsr, u32 par)
{
   CPU_FL prev_state;

   prev_state = sb_IntStateSaveDisable();
   smx_EVB_LOG_INVOKE(smx_InvokeH, lsr, par);
   if (smx_lqctr >= SMX_SIZE_LQ)
   {
      sb_IntStateRestore(prev_state);
      smx_ERROR(SMXE_LQ_OVFL, 0)
   }
   else
   {
      smx_lqctr++;
      smx_lqin->lsr = lsr;
      smx_lqin->par = par;
      smx_lqin++;
      if (smx_lqin > smx_lqx)
      {
         smx_lqin = smx_lqi;
      }
      sb_IntStateRestore(prev_state);
   }
}

/*
*  smx_LSRInvoke()   SSR for task use
*
*  Invokes an LSR like smx_LSRInvokeF(). Causes immediate preempt.
*/
bool smx_LSRInvoke(LCB_PTR lsr, u32 par)
{
   smx_SSR_ENTER2(SMX_ID_LSR_INVOKE, lsr, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_LSR_INVOKE, false);

   sb_INT_DISABLE()
   smx_EVB_LOG_INVOKE(smx_InvokeH, lsr, par);

   if (smx_lqctr >= SMX_SIZE_LQ)
   {
      sb_INT_ENABLE()
      smx_ERROR_EXIT(SMXE_LQ_OVFL, false, 0, SMX_ID_LSR_INVOKE);
   }
   else
   {
      smx_lqctr++;
      smx_lqin->lsr = lsr;
      smx_lqin->par = par;
      smx_lqin++;
      if (smx_lqin > smx_lqx)
      {
         smx_lqin = smx_lqi;
      }
   }
   return((bool)smx_SSRExit(true, SMX_ID_LSR_INVOKE));
}

/*
*  smx_LSRsOff()   Function
*
*  Disables LSRs from running.
*/
void smx_LSRsOff(void)
{
  #if SMX_CFG_EVB
   smx_EVBLogSSR0(SMX_ID_LSRS_OFF);
  #endif
  
   smx_srnest++; 
   
  #if SMX_CFG_EVB
   smx_EVBLogSSRRet(true, SMX_ID_LSRS_OFF);
  #endif
}

/*
*  smx_LSRsOn()   SSR
*
*  Reenables LSRs after being disabled by smx_LSRsOff(). SSR for use from 
*  tasks, only.
*/
bool smx_LSRsOn(void)
{
   smx_SSR_ENTER0(SMX_ID_LSRS_ON);
   smx_EXIT_IF_IN_ISR(SMX_ID_LSRS_ON, false);
   smx_srnest--;
   return((bool)smx_SSRExit(true, SMX_ID_LSRS_ON));
}

/* Notes:
   1. If the LSR is running in pmode, two of its MPU regions must be sys_data 
      and sys_code so it can make direct smx calls. Since the LSR stack must
      come from mheap, which is in sys_data, no stack region is created, for
      ARMM8 since it would overlap sys_data and cause an MMF.
   2. smx_HeapMalloc(), mp_RegionGetHeapR(), and smx_HeapFree() report failures.
*/
      