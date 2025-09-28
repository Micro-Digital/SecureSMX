/*
* mpu.c                                                     Version 5.4.0
*
* ARMM MPU control functions for use by smx and init code.
*
* Copyright (c) 2016-2025 Micro Digital Inc.
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
#include "bsp.h"

#if SB_CPU_ARMM8
#include "mpatmplt.h"
#endif

#if SMX_CFG_SSMX

/* internal subroutines */
static void mp_MPALoad(u32* mp, MPA* tmp, u32 tmsk, u32 mpasz);
static void mp_MPARLoad(u32** mpp, u32** tpp, u32 i);
static u8*  mp_RegionGetHeap(u32 sz, u32 hn, u32* psrd=NULL);

#if MP_MPA_DEV && (MP_MPU_STATSZ > 0)
const char *mp_mpu_snames[MP_MPU_STATSZ];  /* static region names (for debugging) */
#endif

/* For inactive MPU regions <7> */
#pragma section = "sys_code"
#pragma section = "sys_data"

extern u32 scsz;
extern u32 sdsz;

u32         rbar;
#if SB_CPU_ARMM7
u32         rasr;
#else
u32         rlar;
#endif

/*===========================================================================*
                               API FUNCTIONS
*===========================================================================*/

/* 
*  mp_MPACreate() Task version
*
*  Allocates large enough MPA for regions in template selected by tp. Must be
*  at least MP_MPU_ACTVSZ slots. Loads template regions into MPA that are 
*  selected by 1's in tmsk.
*/
bool mp_MPACreate(TCB_PTR task, MPA* tmp, u32 tmsk, u32 mpasz)
{
   u32   i;
   u32*  mp;         /* MPA pointer */
   bool  pass;       /* pass TCB test */
   u32*  srp;        /* stack region pointer */
   MPA*  tp;         /* template pointer */
   u8    tsz = 0;    /* number of 1's in template */

   /* verify that task is valid and that current task has access permission */
   if (pass = smx_TCBTest(task, SMX_PRIV_HI))
   {
      /* abort if utask tries to change its own MPA */
      if (task->flags.umode == 1 && task == smx_ct)
      {
         smx_EM(SBE_PRIV_VIOL); /*<1>*/
         return false;
      }

      /* determine template pointer to use */
      if (task->parent == NULL)
         tp = tmp;
      else
         tp = task->parent->mpatp;
            
      /* if MPA prev allocated, save its stack region for new MPA, then free it */
      if (task->mpap != mpa_dflt)
      {
         i = MP_MPU_ACTVSZ - 1;
         srp = (u32*)task->mpap + MP_MPR_SZ*i;
         task->rv = *srp++ - i;
         task->sv = *srp;
         smx_HeapFree(task->mpap);
      }

      /* check parameters */
      if (tp == NULL || tmsk == 0 || mpasz == 0 || mpasz < tsz)
      {
         smx_EM(SBE_INV_PAR); /*<1>*/
         return false;
      }

      /* allocate new MPA from main heap */
      if ((mp = (u32*)smx_HeapMalloc(4*MP_MPR_SZ*mpasz)) == NULL)
         return false;

      /* save parameters in task TCB before they are changed */
      task->mpap  = (MPR*)mp;
      task->mpasz = mpasz;
      task->mpatp = tp;

      mp_MPALoad(mp, tp, tmsk, mpasz);

      /* load task stack region saved in tcb.rv and tcb.sv by smx_TaskCreate()
         or if MPA is being reloaded (see above). <2> */
      i = MP_MPU_ACTVSZ - 1; /*<10>*/
      srp = (u32*)task->mpap + MP_MPR_SZ*i;

     #if SB_CPU_ARMM7
      *srp++ = task->rv + i;
      *srp   = task->sv;
      #if MP_MPA_DEV
      *++srp = (u32)"stack";
      #endif

     #elif SB_CPU_ARMM8
      if (task->flags.umode || task->hn != 0)
      {
         *srp++ = task->rv;
         *srp   = task->sv;
         #if MP_MPA_DEV
         *++srp = (u32)"stack";
         #endif
      }
     #endif
   }
   return pass;
}

/* 
*  mp_MPACreateLSR() LSR version.
*/
bool mp_MPACreateLSR(LCB_PTR lsr, MPA* tmp, u32 tmsk, u32 mpasz)
{
   u32   i;
   u32*  mp;         /* MPA pointer */
   bool  pass;
   u32*  srp;        /* stack region pointer */

   /* verify that lsr is valid, that call is not from an lsr, and that the 
      current task has create MPA permission for this lsr */
   if (pass = smx_LCBTest(lsr, SMX_PRIV_HI))
   {
      /* check parameters */
      if (tmp == NULL || tmsk == 0 || mpasz == 0 || mpasz > MP_MPU_ACTVSZ)
      {
         smx_EM(SBE_INV_PAR);
         return false;
      }
      /* free MPA if previously allocated */
      if (lsr->mpap != mpa_dflt)
         smx_HeapFree(lsr->mpap);

      /* allocate new MPA from main heap */
      if ((mp = (u32*)smx_HeapMalloc(4*MP_MPR_SZ*mpasz)) == NULL)
         return false;

      /* save parameters in lsr LCB before they are changed */
      lsr->mpap  = (MPR*)mp;
      lsr->mpasz = mpasz;

      mp_MPALoad(mp, tmp, tmsk, mpasz);

      /* load lsr stack region saved in lsr->sr by smx_LSRCreate() */
      i = MP_MPU_ACTVSZ - 1; /*<10>*/
      srp = (u32*)lsr->mpap + MP_MPR_SZ*i;

     #if SB_CPU_ARMM7
      *srp++ = lsr->sr.rbar + i;
      *srp   = lsr->sr.rasr;
      #if MP_MPA_DEV
      *++srp   = (u32)"stack";
      #endif

     #elif SB_CPU_ARMM8
      if (lsr->flags.umode) /*<4>*/
      {
         *srp++ = lsr->sr.rbar;
         *srp   = lsr->sr.rlar;
         #if MP_MPA_DEV
         *++srp = (u32)"stack";
         #endif
      }
     #endif
   }
   return pass;
}

/* 
*  mp_MPASlotMove() Moves contents of MPA[sn] to MPA[dn] and to MPU[dn+fas],
*  if dn < MP_MPU_ACTVSZ.  
*/
bool mp_MPASlotMove(u8 dn, u8 sn)
{
   u32* dsp;      /* destination slot pointer */
   u32* mp;       /* MPA slot pointer */
   u32* ssp;      /* source slot pointer */

   /* abort if parameters are out of range */
   if (dn >= smx_ct->mpasz || sn >= smx_ct->mpasz)
   {
      smx_EM(SBE_INV_PAR); /*<1>*/
      return false;
   }

   dsp = mp_MPA_PTR(smx_ct, dn);
   mp  = dsp;
   ssp = mp_MPA_PTR(smx_ct, sn);

   /* move MPA[sn] to MPA[dn] */
  #if SB_CPU_ARMM7
   *dsp++ = (*ssp++ & 0xFFFFFFF0) + MP_MPU_FAS + dn; /*<5>*/
  #elif SB_CPU_ARMM8
   *dsp++ = *ssp++;
  #endif
   *dsp   = *ssp;
  #if MP_MPA_DEV
   *++dsp = *++ssp;
  #endif

   /* move MPA[dn] to MPU[dn+fas] */
   if (dn < MP_MPU_ACTVSZ)
   {
      sb_INT_DISABLE();  
     #if SB_CPU_ARMM7
      *ARMM_MPU_RNR = dn;
      *ARMM_MPU_RASR = 0;  /* disable region dn */ 
      *ARMM_MPU_RBAR = (*mp++ & 0xFFFFFFF0) + MP_MPU_FAS + dn;
      *ARMM_MPU_RASR = *mp;
     #elif SB_CPU_ARMM8
      *ARMM_MPU_RNR  = dn;
      *ARMM_MPU_RLAR = 0;  /* disable region dn */ 
      *ARMM_MPU_RBAR = *mp++;
      *ARMM_MPU_RLAR = *mp;
     #endif
      __DSB();
      __ISB();
      sb_INT_ENABLE();
   }

   return true;
}

/* 
*  mp_MPUInit() Internal function. Called from main() if SMX_CFG_SSMX. 
*/
void mp_MPUInit(void)
{
   u32 n;

   /* Disable the MPU */
   __DMB();
   *ARMM_MPU_CTRL = 0;

  #if defined(SMX_DEBUG)
   /* Check number of actual MPU slots from register */
   if (MP_MPU_SZ != (*ARMM_MPU_TYPE >> 8) & 0xFF)
      sb_DEBUGTRAP();  /* MPU size configuration mismatch */
  #endif

   /* Clear all MPU slots */
   for (n = 0; n < MP_MPU_SZ; n++)
   {
      *ARMM_MPU_RNR  = n;
     #if SB_CPU_ARMM7
      *ARMM_MPU_RBAR = n;
      *ARMM_MPU_RASR = 0;
     #elif SB_CPU_ARMM8
      *ARMM_MPU_RLAR = 0;  /* must clear RLAR before RBAR to avoid fault due to overlap */
      *ARMM_MPU_RBAR = 0;
     #endif
   }

  #if SB_CPU_ARMM8
   /* Load memory attribute indirection registers */
   *ARMM_MPU_MAIR0 = (AF3 << 24) + (AF2 << 16) + (AF1 << 8) + AF0;
   *ARMM_MPU_MAIR1 = (AF7 << 24) + (AF6 << 16) + (AF5 << 8) + AF4;
  #endif

  #if SMX_CFG_MPU_ENABLE
   /* Enable the MPU with background region on */
   *ARMM_MPU_CTRL = 0x5;
   __DSB();
   __ISB();
  #endif
}

/* 
*  mp_MPULoad() Internal function called from the scheduler. Loads new task's 
*  active MPA slots into the MPU. Sets PSPLIM and MPU_CTRL = 5 (BR ON, MPU ON).
*/
void mp_MPULoad(bool task)
{
   u32* mp;

   /* Disable the MPU */
   __DMB();
   *ARMM_MPU_CTRL = 0;

   #if SB_CPU_ARMM8
   /* set stack overflow limit to start of stack block */
   if (task)
      __set_PSPLIM((u32)smx_ct->spp);
   else
      __set_PSPLIM((u32)smx_clsr->stp);
   #endif

   /* set MPA pointer, mp, for ct or clsr mpap */
   if (task)
      mp = (u32*)smx_ct->mpap;
   else
      mp = (u32*)smx_clsr->mpap;

 #if MP_MPA_DEV /* load MPA[0..ACTVSZ-1] into MPU[fas..sz-1] skipping region names */
   for (u32 n = MP_MPU_FAS; n < MP_MPU_SZ; n++)
   {
    #if SB_CPU_ARMM7
      *ARMM_MPU_RBAR = *mp++; /* <5> */
      *ARMM_MPU_RASR = *mp++;
      mp++;                   /* skip region name in MPA */
    #elif SB_CPU_ARMM8
      *ARMM_MPU_RNR  = n;
      *ARMM_MPU_RBAR = *mp++;
      *ARMM_MPU_RLAR = *mp++;
      mp++;                   /* skip region name in MPA */
    #endif
   }

 #else  /* fast load MPA[0..ACTVSZ-1] into MPU[fas..sz-1] using LDMIA/STMIA <8> */

   __asm("push {r4-r9} \n\t");

  #if SB_CPU_ARMM7

   /* init pointers r0 = mp and r1 = MPU->RBAR */
   __asm("mov r0,%0 \n\t" :: "r"(mp) : "r0");
   __asm("movw r1,%L0 \n\t"
         "movt r1,%H0 \n\t" :: "i"(0xE000ED9CUL) : "r1");

   /* copy even multiples of 4 slots (each is 2 words) */
   #if MP_MPU_ACTVSZ >= 4
   __asm("ldmia r0!, {r2-r9} \n\t" ::: "r0");  /* read first 8 words from MPA (and update r0/mp) */
   __asm("stmia r1, {r2-r9} \n\t");            /* write first 8 words to MPU (no update r1) <6> */
   #endif
   #if MP_MPU_ACTVSZ >= 8
   __asm("ldmia r0!, {r2-r9} \n\t" ::: "r0");  /* read next 8 words from MPA (and update r0/mp) */
   __asm("stmia r1, {r2-r9} \n\t");            /* write next 8 words to MPU (no update r1) <6> */
   #endif
   #if MP_MPU_ACTVSZ >= 12
   __asm("ldmia r0!, {r2-r9} \n\t" ::: "r0");  /* read next 8 words from MPA (and update r0/mp) */
   __asm("stmia r1, {r2-r9} \n\t");            /* write next 8 words to MPU (no update r1) <6> */
   #endif
   #if MP_MPU_ACTVSZ >= 16
   __asm("ldmia r0!, {r2-r9} \n\t" ::: "r0");  /* read next 8 words from MPA (and update r0/mp) */
   __asm("stmia r1, {r2-r9} \n\t");            /* write next 8 words to MPU (no update r1) <6> */
   #endif

   /* copy remaining slots after cases above (each is 2 words) */
   #if (MP_MPU_ACTVSZ % 4) == 1
   __asm("ldmia r0!, {r2-r3} \n\t" ::: "r0");  /* read last 2 words from MPA (and update r0/mp) */
   __asm("stmia r1, {r2-r3} \n\t");            /* write last 2 words to MPU (no update r1) <6> */
   #elif (MP_MPU_ACTVSZ % 4) == 2
   __asm("ldmia r0!, {r2-r5} \n\t" ::: "r0");  /* read last 4 words from MPA (and update r0/mp) */
   __asm("stmia r1, {r2-r5} \n\t");            /* write last 4 words to MPU (no update r1) <6> */
   #elif (MP_MPU_ACTVSZ % 4) == 3
   __asm("ldmia r0!, {r2-r7} \n\t" ::: "r0");  /* read last 6 words from MPA (and update r0/mp) */
   __asm("stmia r1, {r2-r7} \n\t");            /* write last 6 words to MPU (no update r1) <6> */
   #endif /* MP_MPU_ACTVSZ */

  #elif SB_CPU_ARMM8
   mp_MPULoad_M8(mp);
  #endif /* SB_CPU_ARMM7 */
   __asm("pop {r4-r9} \n\t");
 #endif /* MP_MPA_DEV */

  #if SMX_CFG_MPU_ENABLE
   *ARMM_MPU_CTRL = 0x5; /* enable the MPU with background region on */
  #endif
   __DSB();
   __ISB();
   return;
}

#if MP_MPU_STATSZ != 0
/* 
*  mp_MPUSlotLoad() loads a static MPU slot from a template region. pmode only.
*/
bool mp_MPUSlotLoad(u8 dn, u32* rp)
{
   /* abort if dn is not a static slot */
   if (dn >= MP_MPU_FAS)
   {
      smx_EM(SBE_INV_PAR); /*<1>*/
      return false;
   }
   
   sb_INT_DISABLE();
   #if SB_CPU_ARMM7
   *ARMM_MPU_RNR = dn;
   *ARMM_MPU_RASR = 0;  /* disable region dn */
   *ARMM_MPU_RBAR = (*rp++ & 0xFFFFFFF0) + dn;
   *ARMM_MPU_RASR = *rp;
   #elif SB_CPU_ARMM8
   *ARMM_MPU_RNR  = dn;
   *ARMM_MPU_RLAR = 0;  /* disable region dn */
   *ARMM_MPU_RBAR = *rp++;
   *ARMM_MPU_RLAR = *rp;
   #endif
   sb_INT_ENABLE();

   #if MP_MPA_DEV && (MP_MPU_STATSZ > 0)
   /* Save static slot name in array for smxAware */
   if (dn < MP_MPU_STATSZ)
      mp_mpu_snames[dn] = (const char *)*++rp;
   #endif

   return true;
}
#endif /* MP_MPU_STATSZ */

/* 
*  mp_MPUSlotSwap() Swaps content of region at rp with MPU[dn]. 
*  Notes:
*     1. For use in pmode, only. 
*     2. dn and *rp are assumed to be correct, for speed. 
*     3. For use in ISRs, but not interrupt safe.
*/
bool mp_MPUSlotSwap(u8 dn, u32* rp)
{
   u32  mpu_ctrl; /* to save MPU CTRL */
   u32  rsav[2];  /* to temporarily save region from MPU */

   /* Disable the MPU */
   __DMB();
   mpu_ctrl = *ARMM_MPU_CTRL;
   *ARMM_MPU_CTRL = 0;

   *ARMM_MPU_RNR = dn;
   rsav[0] = *ARMM_MPU_RBAR;
   #if SB_CPU_ARMM7
   *ARMM_MPU_RBAR = (*rp & 0xFFFFFFF0) + dn;
   *rp++ = rsav[0];
   rsav[1] = *ARMM_MPU_RASR;
   *ARMM_MPU_RASR = *rp;
   #elif SB_CPU_ARMM8
   *ARMM_MPU_RBAR = *rp;
   *rp++ = rsav[0];
   rsav[1] = *ARMM_MPU_RLAR;
   *ARMM_MPU_RLAR = *rp;
   #endif
   *rp = rsav[1];

   /* Enable the MPU and return */
   *ARMM_MPU_CTRL = mpu_ctrl;
   __DSB();
   __ISB();
   return true;
}

/* 
*  mp_RegionGetHeapR() Gets region from heap hn, creates rbar and rasr or rlar 
*  for it, and loads them into region @ rp. Used to create a dynamic region. 
*/
u8* mp_RegionGetHeapR(MPR_PTR rp, u32 sz, u8 sn, u32 attr, const char* name, u32 hn)
{
   u8*   bp;   /* block pointer */

  #if SB_CPU_ARMM7
   u32   n;    /* power of 2 */
   u32   ra;   /* region address */
   u32   rsf;  /* region size field for rasr */
   u32   srd;  /* subregion disables */

   bp = mp_RegionGetHeap(sz, hn, &srd); 
   if (bp != NULL)
   {
      /* find power of two for lower region boundary and find rsf */
      #if USE_CLZ
      n = 32 - __CLZ(sz - 1);
      #else
      for (n = 5, sz >>= 5; sz > 0; sz >>= 1, n++) {}
      #endif
      ra = (u32)bp & (0xFFFFFFF << n);
      rsf = n - 1;

      /* load region */
      rp->rbar = ra | 0x10 | sn;
      rp->rasr = attr | srd << 8 | rsf << 1 | MP_EN;

     #if MP_MPA_DEV
      rp->name = name;
     #else
      (void)name;
     #endif
   }
  #elif SB_CPU_ARMM8
   bp = mp_RegionGetHeap(sz, hn);
   if (bp != NULL)
   {
      /* load region */
      sz = sz + 0x1F & 0xFFFFFFE0;
      rp->rbar = (u32)bp | (attr & 0x1F);
      rp->rlar = ((u32)(bp + sz - 1) & 0xFFFFFFE0) | attr >> 4 | MP_EN;

     #if MP_MPA_DEV
      rp->name = name;
     #else
      (void)name;
     #endif
   }
  #endif
   return bp;
}

/* 
*  mp_RegionGetHeapT() Gets region from heap hn, creates rbar and rasr or rlar  
*  for it, and loads them into task->mpap[sn]. Used to get a private region for
*  a task. Called from smx_TaskCreate(), smx_PBlockGetHeap(), and 
*  smx_PMsgGetHeap(). 
*/
u8* mp_RegionGetHeapT(TCB_PTR task, u32 sz, u8 sn, u32 attr, const char* name, u32 hn)
{
   u8*   bp;   /* block pointer */
   u32*  mp;   /* MPA pointer */

   /* task is known to be good from caller */

   mp = mp_MPA_PTR(task, sn);

  #if SB_CPU_ARMM7
   u32   n;    /* power of 2 */
   u32   ra;   /* region address */
   u32   rsf;  /* region size field for rasr */
   u32   srd;  /* subregion disables */
   bp = mp_RegionGetHeap(sz, hn, &srd);
   if (bp != NULL)
   {
      /* find power of two for lower region boundary and find rsf */
      #if USE_CLZ
      n = 32 - __CLZ(sz - 1);
      #else
      for (n = 5, sz >>= 5; sz > 0; sz >>= 1, n++) {}
      #endif
      ra = (u32)bp & (0xFFFFFFF << n);
      rsf = n - 1;

      /* load region into task->mpap[sn] */
      if (sn < MP_MPU_ACTVSZ)
         *mp++ = ra | 0x10 | (sn + MP_MPU_FAS); /*<5>*/
      else
         *mp++ = ra | 0x10; /*<5>*/
      *mp   = attr | srd << 8 | rsf << 1 | MP_EN;
     #if MP_MPA_DEV
      *++mp = (u32)name;   
     #else
      (void)name;
     #endif
   }
  #elif SB_CPU_ARMM8
   if (task->flags.umode || sn >= MP_MPU_ACTVSZ || hn > 0) /*<11>*/
   {
      bp = mp_RegionGetHeap(sz, hn);
      if (bp != NULL)
      {
         /* load region into task->mpap[sn] */
         sz    = (sz + 0x1F) & 0xFFFFFFE0;
         *mp++ = (u32)bp | (attr & 0x1F);
         *mp   = ((u32)(bp + sz - 1) & 0xFFFFFFE0) | attr >> 4 | MP_EN;
        #if MP_MPA_DEV
         *++mp = (u32)name;   
        #else
         (void)name;
        #endif
      }
   }
   else
   {
      smx_EM(SBE_INV_PAR); /*<1>*/
      return NULL;      
   }
  #endif
   return bp;
}

/* 
*  mp_RegionGetPoolR() Gets region from a block pool, creates rbar and rlar or 
*  rasr for it, and loads them into region @ rp. Used to get a dynamic region.
*  NOTE: pool blocks are assumed to have proper size and alignment. 
*/
u8* mp_RegionGetPoolR(MPR_PTR rp, PCB_PTR pool, u8 sn, u32 attr, const char* name)
{
   u8* bp;  /* block pointer */

   /* get block if pool is not empty */
   bp = sb_BlockGet(pool, 0);

   if (bp != NULL)
   {
      /* load region into *rp */
     #if SB_CPU_ARMM7
      u32 rsf = 30 - __CLZ(pool->size);
      rp->rbar = (u32)bp | 0x10 | sn;
      rp->rasr = attr | rsf << 1 | MP_EN;
     #elif SB_CPU_ARMM8
      rp->rbar = (u32)bp | (attr & 0x1F);
      rp->rlar = (((u32)bp + pool->size - 1) & 0xFFFFFFE0) | attr >>4 | MP_EN;
     #endif

     #if MP_MPA_DEV
      rp->name = name;
     #else
      (void)name;
     #endif
   }
   return bp;
}

/*  
*  mp_RegionGetPoolT() Gets region from pool, creates rbar and rlar or rasr for 
*  it, and loads them into task->mpap[sn]. Used to get a private region for
*  task. Called from smx_PBlockGetPool() and smx_PMsgGetPool().
*  NOTE: pool blocks are assumed to have proper size and alignment. 
*/
u8* mp_RegionGetPoolT(TCB_PTR task, PCB_PTR pool, u8 sn, u32 attr, const char* name)
{
   u8*  bp;    /* block pointer */
   u32* mp;    /* MPA pointer */

   mp = mp_MPA_PTR(task, sn);

  #if SB_CPU_ARMM7
   bp = sb_BlockGet(pool, 0);
   if (bp != NULL)
   {
      /* load region into task->mpap[sn] */
      u32  rsf = 30 - __CLZ(pool->size);
      if (sn < MP_MPU_ACTVSZ)
         *mp++ = (u32)bp | 0x10 | (sn + MP_MPU_FAS); /*<5>*/
      else
         *mp++ = (u32)bp | 0x10; /*<5>*/
      *mp = attr | rsf << 1 | MP_EN;
     #if MP_MPA_DEV
      *++mp = (u32)name;
     #else
      (void)name;
     #endif
   }
  #elif SB_CPU_ARMM8
   if (task->flags.umode || (sn >= MP_MPU_ACTVSZ)) /*<11>*/
   {
      bp = sb_BlockGet(pool, 0);
      if (bp != NULL)
      {
         /* load region into task->mpap[sn] */
         *mp++ = (u32)bp | (attr & 0x1F);
         *mp   = (((u32)bp + pool->size - 1) & 0xFFFFFFE0) | attr >>4 | MP_EN;
        #if MP_MPA_DEV
         *++mp = (u32)name;   
        #else
         (void)name;
        #endif
      }
   }
   else
   {
      smx_EM(SBE_INV_PAR); /*<1>*/
      return NULL;      
   }
  #endif
   return bp;
}

/* 
*  mp_RegionMakeR() Makes a region for a block from any source given a block
*  pointer and sz. Creates rbar and rlar or rasr for the block, and loads them 
*  into region @ rp. Block must be at least 32 bytes and of proper size and 
*  alignment.
*/
bool mp_RegionMakeR(MPR_PTR rp, u8* bp, u32 sz, u8 sn, u32 attr, const char* name)
{
   /* test that block size is at least 32 bytes */
   if (sz < 32)
      return false;

  #if SB_CPU_ARMM7
   u32 an;    /* alignment number */
   u32 amask; /* alignment mask */
   u32 rsf;   /* region size field */

   an = 32 - __CLZ(sz - 1);
   amask = ~(0xFFFFFFFF << an);

   /* test that size is a power of 2, and block is aligned on its size */
   if (sz != (1 << an) || (u32)bp & amask)
   {
      return false;
   } 

   /* load region into *rp and return true */
   rsf = 30 - __CLZ(sz);
   rp->rbar = (u32)bp | 0x10 | sn;
   rp->rasr = attr | rsf << 1 | MP_EN;
  #elif SB_CPU_ARMM8
   rp->rbar = (u32)bp | (attr & 0x1F);
   rp->rlar = ((u32)(bp + sz - 1) & 0xFFFFFFE0) | attr >> 4 | MP_EN;
  #endif

  #if MP_MPA_DEV
   rp->name = name;
  #else
   (void)name;
  #endif
   return true;
}

/* 
*  mp_RegionMakeT() Makes a region for block and loads it into smx_ct->mpap[sn] 
*  provided that the block meets MPU size and alignment requirements. Called 
*  from smx_PBlockMake() or smx_PMsgMake(). If call is from umode and block
*  is not a data block wholly within current task MPA regions, returns false.
*/
bool mp_RegionMakeT(u8* bp, u32 sz, u8 sn, u32 attr, const char* name)
{
   u32   rattr;   /* region attributes */
   u32*  mp;      /* MPA pointer */
   u8*   rbase;   /* region base */
   u32   rsize;   /* region size */     
   u32   n;
   bool  pass;

   /* test that block size is at least 32 bytes */
   if (sz < 32)
      return false;

   /* if umode caller, test that block is valid <12> */
   if (sb_IN_SVC())
   {
      pass = false;
      mp = (u32*)smx_ct->mpap;
      for (n = 0; n < MP_MPU_ACTVSZ-1; n++)
      {
        #if SB_CPU_ARMM7
         rbase = (u8*)(*mp++ & 0xFFFFFFE0);
         rattr = (*mp & 0xFFFF0000);
         rsize = 1 << (1+(*mp & 0x0000003E)/2);
         u32 srd = (*mp & 0x0000FF00) >> 8; /* subregion disables */
         switch (srd)
         {
            case 0xE0:
               rsize = 5*rsize/8;
               break;
            case 0xC0:
               rsize = 6*rsize/8;
               break;
            case 0x80:
               rsize = 7*rsize/8;
         }
        #elif SB_CPU_ARMM8
         rbase = (u8*)(*mp & 0xFFFFFFE0);
         rattr = (*mp++ & 0x0000001F);
         rsize = (u32)((*mp & 0xFFFFFFE0) + 0x20 - (u32)rbase);
        #endif

         if (n == sn || bp < rbase || (bp + sz) >= (rbase + rsize) ||
                                      rattr != MP_DATARW)                       
         {
            #if MP_MPA_DEV
            mp = mp + 2;
            #else
            mp = mp + 1;
            #endif
         }
         else
         {
            pass = true;
            break;
         }
      }
      if (pass == false)
         return false;
   }

  #if SB_CPU_ARMM7
   u32  an;    /* alignment number */
   u32  amask; /* alignment mask */
   u32  rsf;   /* region size field */

   mp = mp_MPA_PTR(smx_ct, sn);
   an = 32 - __CLZ(sz - 1);
   amask = ~(0xFFFFFFFF << an);

   /* test that block size is at least 32 bytes, size is a power of 2, block 
      is aligned on its size and bp is not NULL */
   if (an < 5 || sz != (1 << an) || (u32)bp & amask || bp == NULL)
   {
      return false;
   }
   /* load region into smx_ct->mpap[sn] and return true */
   rsf = 30 - __CLZ(sz);
   if (sn < MP_MPU_ACTVSZ)
      *mp++ = (u32)bp | 0x10 | (sn + MP_MPU_FAS); /*<5>*/
   else
      *mp++ = (u32)bp | 0x10; /*<5>*/
   *mp   = attr | rsf << 1 | MP_EN;

  #if MP_MPA_DEV
   *++mp = (u32)name;
  #else
   (void)name;
  #endif

  #elif SB_CPU_ARMM8
   if (sn >= MP_MPU_ACTVSZ) /*<11>*/
   {
      /* load region into smx_ct->mpap[sn] */
      mp = mp_MPA_PTR(smx_ct, sn);
      *mp++ = (u32)bp | (attr & 0x1F);
      *mp   = (((u32)bp + sz - 1) & 0xFFFFFFE0) | 1;

     #if MP_MPA_DEV
      *++mp = (u32)name;   
     #else
      (void)name;
     #endif
   }
   else
   {
      smx_EM(SBE_INV_PAR); /*<1>*/
      return false;      
   }
  #endif
   return true;
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/* 
*  mp_MPALoad() Called by task and LSR versions of MPACreate() to load MPA
*  from template.
*/
void mp_MPALoad(u32* mp, MPA* tmp, u32 tmsk, u32 mpasz)
{
   u32   i;
   u32   tm  = tmsk; /* template mask */
   u8    tsz = 0;    /* number of 1's in template */

   /* determine number of 1's in template mask */
   while (tm > 0)
   {
      tsz += tm & 1;
      tm = tm >> 1;
   }
   tm = tmsk;

   /* load template into MPA */
   for (i = 0; i < tsz;)
   {
      if (tm & 1)
      {
         /* load selected template region and advance i */
         mp_MPARLoad(&mp, (u32**)&tmp, i);
         i++;
      }
      else
      {
         /* skip unselected template region, i is unchanged */
         tmp = (MPA*)((u32*)tmp + MP_MPR_SZ);
      }
      tm = tm >> 1; /* right shift template mask */
   }

   /* clear unused active slots above template regions */
   for (; i < mpasz; i++)
   {
      #if SB_CPU_ARMM7
      *mp++ = MP_V + i + MP_MPU_FAS; /*<3><9>*/
      #elif SB_CPU_ARMM8
      *mp++ = 0;
      #endif
      *mp++ = 0;
      #if MP_MPA_DEV
      *mp++ = 0;
      #endif
   }
}

/* 
*  mp_MPARLoad() Loads a static region @ tp into an MPA slot @ mp or loads a 
*  dynamic region pointed to by *tp if MP_DRT.
*/
//#if !defined(SMX_DEBUG)
#pragma inline=forced   /* cannot see local variables */
//#endif
void mp_MPARLoad(u32** mpp, u32** tpp, u32 i)
{
   u32* drp;         /* dynamic region array pointer */
   u32* mp = *mpp;   /* MPA pointer */
   u32* tp = *tpp;   /* template pointer */

   #if SB_CPU_ARMM7
   if ((*(tp+1) & MP_DRT) == 0)
   {
      /* load template region into MPA[i] */
      if (i < MP_MPU_ACTVSZ)
         *mp++ = (*tp++ & 0xFFFFFFF0) + i + MP_MPU_FAS; /*<5>*/
      else
         *mp++ = (*tp++ & 0xFFFFFFF0); /*<5>*/
      *mp++ = *tp++;
     #if MP_MPA_DEV
      *mp++ = *tp++;  /* load region name */
     #endif
   }
   #elif SB_CPU_ARMM8
   if (*(tp+1) != MP_DRT)
   {
      /* load template region into MPA[i] */
      *mp++ = *tp++;
      *mp++ = *tp++;
     #if MP_MPA_DEV
      *mp++ = *tp++;  /* load region name */
     #endif
   }
   #endif
   else
   {
      /* load dynamic region into MPA[i] */
      drp = *(u32**)tp;
      *mp++ = *drp++;
      *mp++ = *drp;
     #if MP_MPA_DEV
      if (*++drp == 0)
         *mp++ = (u32)"Dynamic";
      else
         *mp++ = *drp; /* load region name */
     #endif
      tp += MP_MPR_SZ;
   }
   /* pass back updated pointers */
   *mpp = mp;
   *tpp = tp;
}

/* 
*  mp_RegionGetHeap() Internal function. Gets a region from heap hn. For ARMM7 
*  loads srd into *psrd, if not NULL. returns bp. sz must be >= 32 bytes.
*/
u8* mp_RegionGetHeap(u32 sz, u32 hn, u32* psrd)
{

   u32  rn;  /* power of two for rs */
   u32  rs;  /* region size */
   u8*  bp;  /* block pointer */

   if (sz < 32)
      return 0;

  #if SB_CPU_ARMM7
   u32  i;
   u8   msk; /* mask */
   u32  nsi; /* number of subregions in block */
   u32  rd;  /* distance to next region boundary above bp */
   u8   srd; /* subregion disables */
   u32  ss;  /* subregion size */

   for (rs = 32, rn = 5; rs < sz; rn++)
   {
      rs <<= 1;
   }

   if (rs <= 128) /* region has no subregions */
   {
      bp = (u8*)smx_HeapMalloc(rs, rn, hn);
      srd = 0;
   }
   else  /* region has subregions */
   {
      /* get aligned block within region */
      ss = rs/8;
      for (nsi = 1; nsi*ss < sz; nsi++) {}
      bp = (u8*)smx_HeapMalloc(nsi*ss, EH_R + rn - 3, hn);

      if (bp != NULL)
      {
         /* find distance to next region boundary */
         rd  = -(u32)bp & ((1 << rn) - 1);

         /* set all subregion disables */
         srd = 0xFF;
         msk = 1;

         if (rd > 0)
         {
            msk = msk << (8 - rd/ss);
         }

         /* clear disables for subregions containing block */
         for (i = nsi; i > 0; i--)
         {
            srd ^= msk;
            msk <<= 1;
         }
      }
   }
   *psrd = srd;
  #elif SB_CPU_ARMM8
   rs = (sz + 0x1F) & 0xFFFFFFE0;
   rn = 5;
   bp = (u8*)smx_HeapMalloc(rs, rn, hn);
  #endif
   return bp;
}

/* Notes:
   1. Enable sb_DEBUGTRAP() in smx_EM() during debug. Tests should stay in to 
      avoid loading garbage into an MPA or the MPU.
   2. Task stack attributes = DATARW (see mparmm.h). Ignore the stack slot 
      for a new task without a permanent stack, since it will be overwriten 
      by smx_GetPoolStack() when the task is dispatched the first time. For
      a ptask under ARMM8, the top MPU slot is not used for the task stack 
      region and may be used for another region.
   3. For ARMM7, MPA active areas must have V set in all slots.  
   4. For ARMM8 pmode, the task stack is in sys_data. See Note 1 in xlsr.c.
   5. MPA slots 0 to MP_MPU_ACTVSZ-1 correspond to MPU slots MP_MPU_FAS to 
      MP_MPU_SZ-1. For ARMM7: MPA slot number fields contain the corresponding 
      MPU slot numbers; MPU slots below MP_MPU_FAS can contain static regions 
      loaded by mp_MPUSlotLoad(); and Auxiliary MPA slots above MP_MPU_ACTVSZ-1
      have no slot numbers.
   6. Writes successive alias registers which actually write RBAR and RASR.
      No base update so R1 remains pointing to RBAR.
   7. If two inactive MPU slots are available, sys_code and sys_data are
      loaded into them. These regions are not accessible by utasks, but are 
      accessible by ISRs and other pmode code. In this case, BR is OFF in umode.
      sys_code, scsz, sys_data, and sdsz are defined by the linker command 
      file and made accessible by #pragmas and externs above. If only one
      inactive MPU slot is available, sys_code is loaded into it, but for BR
      to be OFF in umode, ISR modifications would be necessary.
   8. MPU load is done inline rather than separate asm function because it:
      a. avoids the need to also define MP_MPA_DEV in .inc and create mpua.s
      file just for this.
      b. is simpler to understand the rest of the routine.
      ARMM7 allows writing up to 4 starting at any slot, since the slot number
      is indicated in each region.
      ARMM8 requires setting RNR to 0, 4, 8, etc. and writing only the slots
      in that group. Skipping any at the start of the group requires writing
      to the corresponding alias address. For example, skipping 0 and 1 in
      the group requires writing starting at RBAR_A2. Note: This algorithm
      cannot skip region names.
      Based upon Yiu p. 370.
   9. Necessary to clear MPU slot by MPULoad().
  10. Task stack slot is always the top MPA slot in the MPU active region for 
      ARMM7 or for an ARMM8 utask.
  11. ARMM8 does not permit active regions to overlap, so if task is a ptask, 
      it must use an aux slot not an active slot for this block or not mheap, 
      since ptask has sys_data region, which holds the main heap.
  12. sb_IN_SVC() is the right check not sb_IN_UMODE() because the need is to
      check the mode of the caller, not the current execution mode, which will
      be pmode if called from umode since we're in the SVC handler. Note that
      pmode callers can also do SVC calls (unnecessarily), so the same checks
      (restrictions) are done on the block. To make an unrestricted region,
      a pmode caller must make a direct call not an SVC call.
*/
#endif /* SMX_CFG_SSMX */
