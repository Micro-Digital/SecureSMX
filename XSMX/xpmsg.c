/*
* xpmsg.c                                                   Version 5.4.0
*
* smx Protected Message Functions
*
* Copyright (c) 2018-2025 Micro Digital Inc.
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
* Authors: Ralph Moore
*
*****************************************************************************/

#include "xsmx.h"

#if SMX_CFG_SSMX

/* internal subroutines */
static u32 smx_PMsgLoadMPA(MCB* pmsg, TCB* rtask, u8 sn);

extern  MCB mcb_pool[SMX_NUM_MSGS];

#pragma section = "mheap"

/*
*  smx_PMsgGetHeap()   SSR
*
*  Gets a protected msg of sz bytes from heap hn and loads its region
*  into MPA[sn] of the current task, then into MPU[sn+fas]. Gets an MCB and
*  loads it and returns the pmsg handle. If bpp != NULL returns pmsg block ptr 
*  via bpp.
*/
MCB_PTR smx_PMsgGetHeap(u32 sz, u8** bpp, u8 sn, u32 attr, u32 hn, MCB_PTR* mhp)
{
   u8*     bp;
   u32*    mp;
   MCB_PTR pmsg;

   smx_SSR_ENTER6(SMX_ID_PMSG_GET_HEAP, sz, bpp, sn, attr, hn, mhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_PMSG_GET_HEAP, NULL);

   /* block multiple gets and verify current task has pmsg get permission */
   if ((pmsg = (MCB_PTR)smx_ObjectCreateTestH((u32*)mhp)) && !smx_errno)
   {
      /* abort if not valid slot number */
      if (sn >= smx_ct->mpasz)
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PMSG_GET_HEAP);

      /* get pdata block from heap <1> */
      bp = mp_RegionGetHeapT(smx_ct, sz, sn, attr, "pmsg", hn);

      /* abort if allocation fails or error */
      if (bp == NULL)
         return((MCB_PTR)smx_SSRExit(NULL, SMX_ID_PMSG_GET_HEAP));
      mp = mp_MPA_PTR(smx_ct, sn);

      /* if active region, load MPU[sn+fas] from MPA[sn] */
      if (sn + MP_MPU_FAS < MP_MPU_SZ)    /* in active region */
      {
         #if SB_CPU_ARMM7
         *ARMM_MPU_RBAR = *mp++;
         *ARMM_MPU_RASR = *mp--;
         #elif SB_CPU_ARMM8
         *ARMM_MPU_RNR  = sn + MP_MPU_FAS;
         *ARMM_MPU_RBAR = *mp++;
         *ARMM_MPU_RLAR = *mp--;
         #endif
      }

      /* get an MCB -- release pdata block and abort if fail */
      if ((pmsg = (MCB_PTR)sb_BlockGet(&smx_mcbs, 4)) == NULL)
      {
         smx_PBlockRelHeap(bp, sn, hn); /*<1>*/
         smx_ERROR_EXIT(SMXE_OUT_OF_MCBS, NULL, 0, SMX_ID_PMSG_GET_HEAP);
      }

      /* initialize MCB */
      pmsg->fl = NULL; /* clears free list link */
      pmsg->cbtype = SMX_CB_MCB;
      pmsg->pri = 0;
      pmsg->bp  = bp;
      pmsg->bs  = hn;
      pmsg->onr = (smx_clsr ? (TCB_PTR)smx_clsr : smx_ct);
      pmsg->con.osn = sn;
      pmsg->con.bnd = false;
      pmsg->rpx = 0xFF;
      pmsg->host = NULL;
     #if SB_CPU_ARMM8
      if (bp >= __section_begin("mheap") && bp <= __section_end("mheap"))
         pmsg->con.sb = 1;
      else
         pmsg->con.sb = 0;
      pmsg->shapxn = (attr & 0x1f);
     #endif
      pmsg->mhp = mhp;

      /* load pmsg block pointer and message handle */
      if (bpp)
         *bpp = bp;
      if (mhp)
         *mhp = pmsg;
   }
   return((MCB_PTR)smx_SSRExit((u32)pmsg, SMX_ID_PMSG_GET_HEAP));
}

/*
*  smx_PMsgGetPool()   SSR
*
*  Gets a protected msg from pool and loads its region into into MPA[sn] of 
*  the current task, then into MPU[sn+fas]. Gets an MCB and loads it and returns 
*  the pmsg handle and pmsg block ptr via bpp.
*  Note: For ARMM7, pool blocks must be full region sizes and aligned on 
*  region boundaries, else some blocks would span two regions.
*/
MCB_PTR smx_PMsgGetPool(PCB_PTR pool, u8** bpp, u8 sn, u32 attr, MCB_PTR* mhp)
{
   u8*      bp;    /* block pointer */
   u32      bsz;   /* block size = region size */
   u32*     mp;    /* MPA slot pointer */
   MCB_PTR  pmsg;  /* message handle */

   smx_SSR_ENTER5(SMX_ID_PMSG_GET_POOL, pool, bpp, sn, attr, mhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_PMSG_GET_POOL, NULL);

   /* block multiple gets and verify current task has pmsg get permission */
   if ((pmsg = (MCB_PTR)smx_ObjectCreateTestH((u32*)mhp)) && !smx_errno)
   {
      /* abort if not valid slot number */
      if (sn >= smx_ct->mpasz)
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PMSG_GET_POOL);

      bsz = pool->size;

      #if SB_CPU_ARMM7
      /* abort if pn and sz are not multiples of 2^rn or bsz < 32 */   
      u32 rn = 31-__CLZ(bsz);          /* power of two for region size */
      u32 amask = ~(0xFFFFFFFF << rn); /* alignment mask */
      if ((u32)pool->pn & amask || bsz & amask || bsz < 32)
         smx_ERROR_EXIT(SMXE_WRONG_POOL, NULL, 0, SMX_ID_PMSG_GET_POOL);
      #elif SB_CPU_ARMM8
      /* abort if pn and sz are not multiples of 32 or bsz < 32 */
      if ((u32)pool->pn & 0x1F || bsz & 0x1F || bsz < 32)
         smx_ERROR_EXIT(SMXE_WRONG_POOL, NULL, 0, SMX_ID_PMSG_GET_POOL);
      #endif

      /* abort if out of MCBs */
      if ((pmsg = (MCB_PTR)smx_mcbs.pn) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_MCBS, NULL, 0, SMX_ID_PMSG_GET_POOL);

      /* get block from pool and put its region into MPA[smx_ct][sn] */
      bp = mp_RegionGetPoolT(smx_ct, pool, sn, attr, "pmsg");

      /* abort if pool is empty or error */
      if (bp == NULL)
         smx_ERROR_EXIT(SMXE_POOL_EMPTY, NULL, 0, SMX_ID_PMSG_GET_POOL);

      if (sn + MP_MPU_FAS < MP_MPU_SZ) /* in active region */
      {
         /* load MPU[sn+fas] from MPA[sn]*/
         mp = mp_MPA_PTR(smx_ct, sn);
         #if SB_CPU_ARMM7
         *ARMM_MPU_RBAR = *mp++;
         *ARMM_MPU_RASR = *mp;
         #elif SB_CPU_ARMM8
         *ARMM_MPU_RNR  = sn + MP_MPU_FAS;
         *ARMM_MPU_RBAR = *mp++;
         *ARMM_MPU_RLAR = *mp;
         #endif
      }

      /* remove MCB from MCB pool and initialize it */
      smx_mcbs.pn = *(u8**)pmsg;
      smx_mcbs.num_used++;
      pmsg->fl = NULL; /* clears free list link */
      pmsg->cbtype = SMX_CB_MCB;
      pmsg->pri = 0;
      pmsg->bp  = bp;
      pmsg->bs  = (u32)pool;
      pmsg->onr = (smx_clsr ? (TCB_PTR)smx_clsr : smx_ct);
      pmsg->con.osn = sn;
      pmsg->con.bnd = false;
      pmsg->rpx = 0xFF;
      pmsg->host = NULL;
      #if SB_CPU_ARMM8
      pmsg->shapxn = attr & 0x1f;
      #endif
      pmsg->mhp = mhp;

      /* load pmsg block pointer and message handle */
      if (bpp)
         *bpp = bp;
      if (mhp)
         *mhp = pmsg;
   }
   return((MCB_PTR)smx_SSRExit((u32)pmsg, SMX_ID_PMSG_GET_POOL));
}

/*
*  smx_PMsgMake()   SSR
*
*  Makes a protected msg from any block given a block pointer, sz, slot
*  number, and attributes, and and loads its region into MPU[sn+fas] from MPA[sn] 
*  of the current task. pmsg->bs is set to -1 to indicate that the pmsg block 
*  is standalone and is not to be released. 
*  Note: For ARMM7, block must be full region size and >= 32 bytes and aligned 
*  on a region boundary. For ARMM8, block size must be a multiple of 32 and 
*  >= 32 bytes and block must be aligned on a 32-byte boundary.
*/
MCB_PTR smx_PMsgMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name, MCB_PTR* mhp)
{
   u32*     mp;  /* MPA slot pointer */
   MCB_PTR  pmsg; /* message handle */

   smx_SSR_ENTER6(SMX_ID_PMSG_MAKE, bp, sz, sn, attr, name, mhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_PMSG_MAKE, NULL);

   /* block multiple gets and verify current task has pmsg make permission */
   if ((pmsg = (MCB_PTR)smx_ObjectCreateTestH((u32*)mhp)) && !smx_errno)
   {
      /* abort if not valid slot number */
      if (sn >= smx_ct->mpasz)
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PMSG_MAKE);

      #if SB_CPU_ARMM7
      /* abort if bp and sz are not multiples of 2^rn or sz < 32 */   
      u32 rn = 31-__CLZ(sz);           /* power of two for region size */
      u32 amask = ~(0xFFFFFFFF << rn); /* alignment mask */
      if ((u32)bp & amask || sz & amask || sz < 32)
         smx_ERROR_EXIT(SMXE_WRONG_POOL, NULL, 0, SMX_ID_PMSG_MAKE);
      #elif SB_CPU_ARMM8
      /* abort if bp and sz are not multiples of 32 or bsz < 32 */
      if ((u32)bp & 0x1F || sz & 0x1F || sz < 32)
         smx_ERROR_EXIT(SMXE_WRONG_POOL, NULL, 0, SMX_ID_PMSG_MAKE);
      #endif

      /* make block into pblock and put its region into MPA[smx_ct][sn] */
      if (!mp_RegionMakeT(bp, sz, sn, attr, name?name:"pmsg"))
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PMSG_MAKE);

      /* get MCB -- abort if out of MCBs */
      if ((pmsg = (MCB_PTR)smx_mcbs.pn) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_MCBS, NULL, 0, SMX_ID_PMSG_MAKE);

      if (sn + MP_MPU_FAS < MP_MPU_SZ) /* in active region */
      {
         /* load MPU[sn+fas] from MPA[sn]*/
         mp = mp_MPA_PTR(smx_ct, sn);
        #if SB_CPU_ARMM7
         *ARMM_MPU_RBAR = *mp++;
         *ARMM_MPU_RASR = *mp;
        #elif SB_CPU_ARMM8
         *ARMM_MPU_RNR  = sn + MP_MPU_FAS;
         *ARMM_MPU_RBAR = *mp++;
         *ARMM_MPU_RLAR = *mp;
        #endif
      }

      /* remove MCB from MCB pool and initialize it */
      smx_mcbs.pn = *(u8**)pmsg;
      smx_mcbs.num_used++;
      pmsg->fl = NULL; /* clears free list link */
      pmsg->cbtype = SMX_CB_MCB;
      pmsg->pri = 0;
      pmsg->bp  = bp;
      pmsg->bs  = -1;
      pmsg->onr = (smx_clsr ? (TCB_PTR)smx_clsr : smx_ct);
      pmsg->con.osn = sn;
      pmsg->con.bnd = false;
      pmsg->rpx = 0xFF;
      pmsg->host = NULL;
     #if SB_CPU_ARMM8
      pmsg->shapxn = attr & 0x1f;
     #endif
      pmsg->mhp = mhp;

      /* load message handle */
      if (mhp)
         *mhp = pmsg;
   }
   return((MCB_PTR)smx_SSRExit((u32)pmsg, SMX_ID_PMSG_MAKE));
}

/*
*  smx_PMsgReceive()   SSR
*
*  Protected message receive. For ARMM7: the MPA slot number rsn = dsn & Oxf. 
*  For ARMM8: rsn = dsn & 0xf if ct is a utask or if ct is a ptask, but pmsg 
*  block is not from sys_data (con.sb == 0). Otherwise rsn = dsn >> 4. In the 
*  first two cases rsn must be an active slot; in the third case it must be an 
*  auxiliary slot. Then MPA[rsn] is loaded, MPU[rsn+fas] is loaded if rsn is an
*  active slot, dsn is saved in ct, and the pmsg handle is returned.
*/
MCB_PTR smx_PMsgReceive(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout, MCB_PTR* mhp)
{
   MCB_PTR  pmsg;
   u32      rbar;
   u8       rsn;  /* receive slot number */

   smx_SSR_ENTER5(SMX_ID_PMSG_RECEIVE, xchg, bpp, dsn, timeout, mhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_PMSG_RECEIVE, NULL);

   /* verify that xchg is valid and that current task has access permission */
   if (pmsg = (MCB_PTR)smx_XCBTest(xchg, SMX_PRIV_LO))
   {
      /* load rsn and verify it is valid */
     #if SB_CPU_ARMM7
      rsn = dsn & 0xf;
      if (rsn >= smx_ct->mpasz)
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PMSG_RECEIVE);
     #elif SB_CPU_ARMM8
      pmsg = (MCB_PTR)xchg->fl;
      if (pmsg != NULL)
      {
         if (!smx_ct->flags.umode && pmsg->con.sb) /*<3>*/
         {
            rsn = dsn >> 4;  /* auxiliary slot */
            if (rsn < MP_MPU_ACTVSZ)
               smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PMSG_RECEIVE);
         }
         else
         {
            rsn = dsn & 0xf; /* active or auxiliary slot */
            if (rsn >= smx_ct->mpasz)
               smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PMSG_RECEIVE);
         }
      }
     #endif

      if (!(smx_clsr && timeout))
      {
         pmsg = smx_MsgReceive_F(xchg, bpp, timeout, mhp);
         if (pmsg == NULL && timeout)
            smx_sched = SMX_CT_SUSP;
         if (timeout)
            smx_lockctr = 0;
      }
      else
      {
         smx_ERROR_EXIT(SMXE_WAIT_NOT_ALLOWED, NULL, 0, SMX_ID_PMSG_RECEIVE);
      }

      if (pmsg != NULL)
      {
         /* load message region into MPA[rsn] for smx_ct */
         rbar = smx_PMsgLoadMPA(pmsg, smx_ct, rsn);

         /* load message region into MPU[rsn+fas] if active slot */
         if (rsn + MP_MPU_FAS < MP_MPU_SZ)
         {
            #if SB_CPU_ARMM7
            *ARMM_MPU_RBAR = rbar;
            *ARMM_MPU_RASR = pmsg->rasr;
            #elif SB_CPU_ARMM8
            *ARMM_MPU_RNR  = rsn + MP_MPU_FAS;
            *ARMM_MPU_RBAR = rbar;
            *ARMM_MPU_RLAR = pmsg->rlar;
            #endif
         }
         /* load pmsg->host and save rsn */
         pmsg->host = smx_ct; /*<5>*/
         if (pmsg->con.bnd)   /*<6>*/
            pmsg->con.hsn = rsn;
         else
            pmsg->con.osn = rsn;
      }
      else
         smx_ct->dsn = dsn; /* save dual slot number in smx_ct */
   }
   return((MCB_PTR)smx_SSRExit((u32)pmsg, SMX_ID_PMSG_RECEIVE));
}

/*
*  smx_PMsgReceiveStop()    SSR
*
*  Protected message receive stop. Same as smx_PMsgReceive(), except ct is
*  stopped and the pmsg handle is passed as a parameter when it is restarted.
*  MPU[rsn+fas] is not loaded.
*
*/
void smx_PMsgReceiveStop(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout, MCB_PTR* mhp)
{
   bool     abort = false;      /* on error */
   MCB_PTR  pmsg = NULL;
   u8       rsn;        /* receive slot number */

   /* abort if call is from an LSR */
   if (smx_clsr)
      smx_ERROR_RET_VOID(SMXE_OP_NOT_ALLOWED, 0);
   smx_RET_IF_IN_ISR_VOID();
   smx_SSR_ENTER5(SMX_ID_PMSG_RECEIVE_STOP, xchg, bpp, dsn, timeout, mhp);

   /* verify that xchg is valid and that current task has access permission */
   if (pmsg = (MCB_PTR)smx_XCBTest(xchg, SMX_PRIV_LO))
   {
      /* load rsn and verify it is valid */
     #if SB_CPU_ARMM7
      rsn = dsn;
      if (rsn >= smx_ct->mpasz)
      {
         smx_ERROR(SMXE_INV_PAR, 0);
         abort = true;
      }

     #elif SB_CPU_ARMM8
      pmsg = (MCB_PTR)xchg->fl;
      if (pmsg != NULL)
      {
         if (!smx_ct->flags.umode && pmsg->con.sb) /*<3>*/
         {
            rsn = dsn >> 4;  /* auxiliary slot */
            if (rsn < MP_MPU_ACTVSZ)
            {
               smx_ERROR(SMXE_INV_PAR, 0);
               abort = true;
            }
         }
         else
         {
            rsn = dsn & 0xf; /* active slot */
            if (rsn >= MP_MPU_ACTVSZ)
            {
               smx_ERROR(SMXE_INV_PAR, 0);
               abort = true;
            }
         }
      }
     #endif

      /* verify that bpp does not point to ct stack */
      if ((smx_ct->sbp >= (u8*)bpp) && ((u8*)bpp <= smx_ct->spp))
      {
         smx_ERROR(SMXE_INV_PAR, 0);
         abort = true;
      }

      if (abort == false)
         pmsg = smx_MsgReceive_F(xchg, bpp, timeout, mhp);

      if (pmsg != NULL)
      {
         /* Load message region into MPA[rsn] for smx_ct */
         smx_PMsgLoadMPA(pmsg, smx_ct, rsn);

         /* load pmsg->host and save rsn */
         pmsg->host = smx_ct; /*<5>*/
         if (pmsg->con.bnd)   /*<6>*/
            pmsg->con.hsn = rsn;
         else
            pmsg->con.osn = rsn;
      }
      else
         smx_ct->dsn = dsn; /* save dual slot number in smx_ct */

      smx_sched = SMX_CT_STOP;
   }
   smx_SSRExit((u32)pmsg, SMX_ID_PMSG_RECEIVE_STOP);
}

/*
*  smx_PMsgRel()   SSR
* 
*  Releases a pmsg obtained by smx_PMsgGetHeap(), smx_PMsgGetPool(), or
*  smx_PMsgMake(). Clears host's MPA slot if pmsg is bound to sender and
*  pmsg region is still in the host slot. Clears MPA and MPU slots for ct. 
*/
bool smx_PMsgRel(MCB_PTR* pmsgp, u16 clrsz)
{
   bool     pass;
   MCB_PTR  pmsg; /* protected message handle */
   u32*     hmp;  /* host MPA slot pointer */
   u8       hsn;  /* host MPA slot number */
   u32*     omp;  /* owner MPA slot pointer */
   u8       osn;  /* owner slot number */

   pmsg = *pmsgp;

   smx_SSR_ENTER2(SMX_ID_PMSG_REL, pmsg, clrsz);
   smx_EXIT_IF_IN_ISR(SMX_ID_PMSG_REL, false);

   /* verify that msg is valid and that current task is its owner <2> and has 
      access permission */
   if (pass = smx_MCBOnrTest(pmsg, SMX_PRIV_HI))
   {
      /* dequeue pmsg if at an exchange */
      if (pmsg->fl != NULL)
          smx_DQMsg(pmsg);

      hsn = pmsg->con.hsn;
      osn = pmsg->con.osn;

      /* clear host's MPA slot if pmsg has been received by the host, is 
         bound, and the host slot is unchanged */
      if (pmsg->con.bnd && pmsg->host != NULL)
      {
         hmp = mp_MPA_PTR(pmsg->host, hsn);
         omp = mp_MPA_PTR(pmsg->onr, osn);
         if ((*omp & ~0xF) == (*hmp & ~0xF))
         {
            #if SB_CPU_ARMM7
            *hmp++ = 0x10 + hsn + MP_MPU_FAS;
            #elif SB_CPU_ARMM8
            *hmp++ = 0;
            #endif
            *hmp   = 0;
            #if MP_MPA_DEV
            *++hmp = 0;
            #endif
         }
      }
      /* clear pmsg owner's MPA and MPU slots, release pmsg and clear its handle  */
      smx_PBlockRelSlot(osn);
      pass = smx_MsgRel_F(pmsg, clrsz);
      *pmsgp = NULL;
   }
   return smx_SSRExit(pass, SMX_ID_PMSG_REL);
}

/*
*  smx_PMsgReply()
*
*  Sends pmsg to its reply exchange from its slot number and with its priority.
*  If pmsg->rpx = 0xFF, releases pmsg.
*  Note: Do not use to release bound messages from receiver.
*/
bool smx_PMsgReply(MCB_PTR pmsg)
{
   bool pass;
   XCB_PTR rxchg;

   if (pmsg->rpx < 0xFF)
   {
      rxchg = (XCB_PTR)(smx_xcbs.pi) + pmsg->rpx;
      pass = smx_PMsgSend(pmsg, rxchg, pmsg->pri, NULL);
   }
   else
   {
      pass = smx_PMsgRel(&pmsg, 0);
   }
   return pass;
}

/*
*  smx_PMsgSend()   SSR
*
*  Protected message send. Load MPA[ssn].rasr into pmsg->rasr. Send a protected 
*  message from MPA[ssn] to xchg. If send is successful and pmsg is not bound to
*  client, clear client MPA[ssn] and clear MPU[ssn+fas], if ssn is an active 
*  slot. If rtask is waiting at xchg, get dsn from task TCB and determine rsn 
*  and load MPA[rsn] as for PMsgReceive(). Load rsn and task handle into pmsg. 
*  Note: if rsn is an active region, MPU[rsn+fas] will be loaded when rtask is 
*  started or resumed. 
*/
bool smx_PMsgSend(MCB_PTR pmsg, XCB_PTR xchg, u8 pri, void* reply)
{
   u32*     mp;
   bool     pass;
   TCB_PTR  rtask = NULL;  /* task to be resumed, if !NULL */
   u8       rsn;           /* receiver slot number */
   u8       ssn;           /* sender slot number */

   smx_SSR_ENTER4(SMX_ID_PMSG_SEND, pmsg, xchg, pri, reply);
   smx_EXIT_IF_IN_ISR(SMX_ID_PMSG_SEND, false);

   /* verify that pmsg is valid and that current task is its owner <2> and has 
      access permission */
   if (pass = smx_MCBOnrTest(pmsg, SMX_PRIV_LO))
   {
      /* verify that xchg is valid and that current task has access permission */
      if (pass = smx_XCBTest(xchg, SMX_PRIV_LO))
      {
         ssn = pmsg->con.osn;

         /* abort if ssn not valid */
         if (ssn >= smx_ct->mpasz)
            smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PMSG_SEND);

         mp = mp_MPA_PTR(smx_ct, ssn) + 1;
        #if SB_CPU_ARMM7
         /* load MPA[ssn].RASR into pmsg.rasr */
         pmsg->rasr = *mp;
        #elif SB_CPU_ARMM8
         /* load MPA[ssn].RLAR into pmsg.rlar */
         pmsg->rlar = *mp;
        #endif

         /* get handle of first task waiting at xchg */
         if (xchg->flags.tq)
            rtask = (TCB_PTR)xchg->fl;

         pass = smx_MsgSend(pmsg, xchg, pri, reply);
         if (pass)
         {
            if (!(pmsg->con.bnd))
            {
               /* if free pmsg, clear MPA[ssn] and clear MPU[ssn+fas], if in MPU */
               mp = mp_MPA_PTR(smx_ct, ssn);
              #if SB_CPU_ARMM7
               *mp++ = 0x10 + ssn + MP_MPU_FAS;
               *mp   = 0;
               if (ssn + MP_MPU_FAS < MP_MPU_SZ) /* in active region */
               {
                  *ARMM_MPU_RBAR = 0x10 + ssn + MP_MPU_FAS;
                  *ARMM_MPU_RASR = 0;
               }
              #elif SB_CPU_ARMM8
               *mp++ = 0;
               *mp   = 0;
               if (ssn + MP_MPU_FAS < MP_MPU_SZ) /* in active region */
               {
                  *ARMM_MPU_RNR  = ssn + MP_MPU_FAS;
                  *ARMM_MPU_RLAR = 0;  /* must clear RLAR before RBAR to avoid fault due to overlap */
                  *ARMM_MPU_RBAR = 0;
               }
              #endif

              #if MP_MPA_DEV
               *++mp = 0;
              #endif
            }
            if (rtask != NULL)
            {
               /* determine receiver slot number <4> */
               #if SB_CPU_ARMM7
               rsn = rtask->dsn & 0xf;    /* active slot */
               #elif SB_CPU_ARMM8
               if (!rtask->flags.umode && pmsg->con.sb) /*<3>*/
                  rsn = rtask->dsn >> 4;  /* auxiliary slot */
               else
                  rsn = rtask->dsn & 0xf; /* active slot */
               #endif

               /* load pmsg->host and save rsn */
               pmsg->host = rtask;  /*<5>*/
               if (pmsg->con.bnd)   /*<6>*/
                  pmsg->con.hsn = rsn;
               else
                  pmsg->con.osn = rsn;

               /* Load pmsg region into MPA[rsn] for rtask */
               smx_PMsgLoadMPA(pmsg, rtask, rsn);
            }
            else
               pmsg->host = NULL; /*<5>*/
         }
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_PMSG_SEND));
}

/*
*  smx_PMsgSendB()
*
*  Sends a bound pmsg. A bound pmsg continues to be owned by the sender, not
*  the exchange nor the receiver. Only the owner can release a pmsg.
*/
bool smx_PMsgSendB(MCB_PTR pmsg, XCB_PTR xchg, u8 pri, void* reply)
{   
   pmsg->con.bnd = true;
   if (smx_PMsgSend(pmsg, xchg, pri, reply))
      return(true);
   else
   {
      pmsg->con.bnd = false;
      return(false);
   }
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  smx_PMsgLoadMPA()
*
*  Load pmsg region into MPA[sn] for rtask.
*/
u32 smx_PMsgLoadMPA(MCB* pmsg, TCB* rtask, u8 sn)
{
   u32*  mp;
   u32   rbar;

   /* calculate rbar */
   #if SB_CPU_ARMM7
   u32 n = (pmsg->rasr>>1 & 0x1f) + 1; 
   u32 msk = 0xFFFFFFFF<<n;
   rbar = ((u32)pmsg->bp & msk) | 0x10 | (sn + MP_MPU_FAS);
   #elif SB_CPU_ARMM8
   rbar = (u32)pmsg->bp + (u32)pmsg->shapxn;
   #endif

   /* load message region into rtask->mpap[sn] */
   mp = mp_MPA_PTR(rtask, sn);
   *mp++ = rbar;
  #if SB_CPU_ARMM7
   *mp   = pmsg->rasr;
  #elif SB_CPU_ARMM8
   *mp   = pmsg->rlar;
  #endif

   #if MP_MPA_DEV
   *++mp = (u32)"pmsg";
   #endif
   return(rbar);
}

/* Notes:
   1. Current task will be suspended for up to smx_htmo ticks if heap hn is
      busy. Operation aborts with smx HEAP TIMEOUT error if timeout occurs.
   2. Only the task that owns a pmsg can send it or release it. If the bnd flag 
      is set, then the task that sent the pmsg is still its owner.
   3. For ARMM8, auxiliary, xsn, and active, asn, slot numbers are determined 
      by the user. Then dsn (dual slot number) = xsn<<4 + asn. dsn is passed in  
      smx_PMsgReceive() when it is called by a receiving task. If no pmsg is 
      waiting, dsn is stored in the task's dsn field to be used when a pmsg 
      does arrive. The auxiliary slot is used if the task is a ptask and pmsg 
      data block is from sys_data (pmsg->con.sb == 1). Otherwise the active
      slot is used.
   4. Since task->dsn is loaded only by smx_PMsgReceive() and is checked by it
      it is not necessary to check here.
   5. Used by mp_PMsgRel() to determine if a bound pmsg has been received by 
      the host.
   6. If pmsg is not bound to the sender, the host becomes the owner. Otherwise
      the sender remains the owner.
*/

#endif /* SMX_CFG_SSMX */