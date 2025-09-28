/*
* xpblk.c                                                   Version 5.4.0
*
* smx Protected Block Functions
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
/* smx_PBlockRelSlot() in xarmm.h since shared */

/*
*  smx_PBlockGetHeap() 
*
*  Gets a protected block of sz bytes from heap hn and loads its region into
*  MPA[sn] of the current task, then into MPU[sn+fas]. Returns block pointer.
*/
u8* smx_PBlockGetHeap(u32 sz, u8 sn, u32 attr, const char* name, u32 hn)
{
   u8*  bp;
   u32* mp;

   smx_SSR_ENTER5(SMX_ID_PBLK_GET_HEAP, sz, sn, attr, name, hn);
   smx_EXIT_IF_IN_ISR(SMX_ID_PBLK_GET_HEAP, NULL);

   /* abort if not valid slot number */
   if (sn >= smx_ct->mpasz)
      smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PBLK_GET_HEAP);

   /* get region from heap hn <1> */
   bp = mp_RegionGetHeapT(smx_ct, sz, sn, attr, name?name:"pblock", hn);
   if (bp == NULL)
      return (u8*)smx_SSRExit(NULL, SMX_ID_PBLK_GET_HEAP);

   if (sn + MP_MPU_FAS < MP_MPU_SZ) /* in active region */
   {
      /* load MPU[sn+fas] from MPA[sn] */
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
   return (u8*)smx_SSRExit((u32)bp, SMX_ID_PBLK_GET_HEAP);
}

/*
*  smx_PBlockGetPool()   SSR
*
*  Gets a protected block from pool and loads its region into MPA[sn] of the 
*  current task, then into MPU[sn+fas], if sn is an active slot number. Pool 
*  blocks must meet MPU size and alignment requirements. Returns block pointer.
*  Note: For ARMM7, pool blocks must be full region sizes and aligned on 
*  region boundaries, else some blocks would span two regions.
*/
u8* smx_PBlockGetPool(PCB_PTR pool, u8 sn, u32 attr, const char* name)
{
   u8*  bp;    /* block pointer */
   u32  bsz;   /* block size = region size */
   u32* mp;    /* MPA slot pointer */

   smx_SSR_ENTER4(SMX_ID_PBLK_GET_POOL, pool, sn, attr, name);
   smx_EXIT_IF_IN_ISR(SMX_ID_PBLK_GET_POOL, NULL);

   /* abort if not valid slot number */
   if (sn >= smx_ct->mpasz)
      smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PBLK_GET_POOL);

   bsz = pool->size;

   #if SB_CPU_ARMM7
   /* abort if pn and sz are not multiples of 2^rn or bsz < 32 */   
   u32 rn = 31-__CLZ(bsz);          /* power of two for region size */
   u32 amask = ~(0xFFFFFFFF << rn); /* alignment mask */
   if ((u32)pool->pn & amask || bsz & amask || bsz < 32)
      smx_ERROR_EXIT(SMXE_WRONG_POOL, NULL, 0, SMX_ID_PBLK_GET_POOL);
   #elif SB_CPU_ARMM8
   /* abort if pn and sz are not multiples of 32 or bsz < 32 */
   if ((u32)pool->pn & 0x1F || bsz & 0x1F || bsz < 32)
      smx_ERROR_EXIT(SMXE_WRONG_POOL, NULL, 0, SMX_ID_PBLK_GET_POOL);
   #endif

   /* get block from pool and put its region into MPA[smx_ct][sn] */
   bp = mp_RegionGetPoolT(smx_ct, pool, sn, attr, name?name:"pblock");

   /* abort if pool was empty */
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
   return (u8*)smx_SSRExit((u32)bp, SMX_ID_PBLK_GET_POOL);
}

/*
*  smx_PBlockMake()   SSR
*
*  Makes a protected block from any block given a block pointer, size, slot
*  number, and attributes, loads its region into MPA[sn] of the current task, 
*  then into MPU[sn+fas], if sn is an active slot number.
*  Note: For ARMM7, block must be full region size and >= 32 bytes and aligned 
*  on a region boundary. For ARMM8, block size must be a multiple of 32 and 
*  >= 32 bytes and block must be aligned on a 32-byte boundary.
*/
bool smx_PBlockMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name)
{
   u32* mp;    /* MPA slot pointer */

   smx_SSR_ENTER5(SMX_ID_PBLK_MAKE, bp, sz, sn, attr, name);
   smx_EXIT_IF_IN_ISR(SMX_ID_PBLK_MAKE, false);

   /* abort if not valid slot number */
   if (sn >= smx_ct->mpasz)
      smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PBLK_MAKE);

   #if SB_CPU_ARMM7
   /* abort if bp and sz are not multiples of 2^rn or bsz < 32 */   
   u32 rn = 31-__CLZ(sz);           /* power of two for region size */
   u32 amask = ~(0xFFFFFFFF << rn); /* alignment mask */
   if ((u32)bp & amask || sz & amask || sz < 32)
      smx_ERROR_EXIT(SMXE_WRONG_POOL, NULL, 0, SMX_ID_PBLK_GET_POOL);
   #elif SB_CPU_ARMM8
   /* abort if bp and sz are not multiples of 32 or bsz < 32 */
   if ((u32)bp & 0x1F || sz & 0x1F || sz < 32)
      smx_ERROR_EXIT(SMXE_WRONG_POOL, NULL, 0, SMX_ID_PBLK_GET_POOL);
   #endif

   /* make block into pblock and put its region into MPA[sn] */
   if (!mp_RegionMakeT(bp, sz, sn, attr, name?name:"pblock"))
      smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_PBLK_MAKE);

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
   return smx_SSRExit(true, SMX_ID_PBLK_MAKE);
}

/*
*  smx_PBlockRelHeap()   SSR
*
*  Releases a protected block to heap hn. Clears MPA[sn] for smx_ct and 
*  MPU[sn+fas], if block released.
*/
bool smx_PBlockRelHeap(u8* bp, u8 sn, u32 hn)
{
   smx_SSR_ENTER3(SMX_ID_PBLK_REL_HEAP, bp, sn, hn);
   smx_EXIT_IF_IN_ISR(SMX_ID_PBLK_REL_HEAP, false);

   /* abort if not valid slot number */
   if (sn >= smx_ct->mpasz)
      smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_PBLK_REL_HEAP);

   /* free block to heap hn <1> */
   if (!smx_HeapFree((void*)bp, hn))
      return smx_SSRExit(false, SMX_ID_PBLK_REL_HEAP);

   /* clear MPA[sn] and MPU[sn+fas] and return */
   smx_PBlockRelSlot(sn);
   return smx_SSRExit(true, SMX_ID_PBLK_REL_HEAP);
}

/*
*  smx_PBlockRelPool()   SSR
*
*  Releases a protected block to its pool, if not NULL. If pool is NULL, the 
*  block is a standalone block and is not released. Clears MPA[sn] for smx_ct 
*  and MPU[sn].
*/
bool smx_PBlockRelPool(u8* bp, u8 sn, PCB_PTR pool, u32 clrsz)
{
   bool  pass = true; /* for standalone block */

   smx_SSR_ENTER4(SMX_ID_PBLK_REL_POOL, bp, sn, pool, clrsz);
   smx_EXIT_IF_IN_ISR(SMX_ID_PBLK_REL_POOL, false);

   /* abort if not valid slot number */
   if (sn >= smx_ct->mpasz)
      smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_PBLK_REL_POOL);

   if (pool != NULL && (pass = sb_BlockRel(pool, bp, clrsz)));
      smx_PBlockRelSlot(sn);

   return smx_SSRExit(pass, SMX_ID_PBLK_REL_POOL);
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  smx_PBlockRelSlot()
*  Clears smx_ct->mpap[sn] and clears MPU[sn+fas] if sn is an active slot.
*  Called from smx_PBlockRelHeap(), smx_PBlockRelPool(), and smx_PMsgRel().
*  For ARMM8 must be utask to clear MPU[sn+fas].
*/
void smx_PBlockRelSlot(u8 sn)
{
   u32*  mp;   /* MPA slot pointer */

   /* clear smx_ct->mpap[sn] */
   mp = mp_MPA_PTR(smx_ct, sn);
  #if SB_CPU_ARMM7
   *mp++ = 0x10 | (sn + MP_MPU_FAS);
  #elif SB_CPU_ARMM8
   *mp++ = 0;
  #endif
   *mp   = 0;
  #if MP_MPA_DEV
   *++mp = 0;
  #endif

   if (sn + MP_MPU_FAS < MP_MPU_SZ) /* in active region */
   {
      /* clear MPU[sn+fas] */
     #if SB_CPU_ARMM7
      *ARMM_MPU_RBAR = 0x10 | (sn + MP_MPU_FAS);
      *ARMM_MPU_RASR = 0;
     #elif SB_CPU_ARMM8
      *ARMM_MPU_RNR  = sn + MP_MPU_FAS;
      *ARMM_MPU_RLAR = 0;  /* clear RLAR first to avoid region overlap */
      *ARMM_MPU_RBAR = 0;
     #endif
   }
}
#endif /* SMX_CFG_SSMX */

/* Notes:
   1. Current task will be suspended for up to smx_htmo ticks if heap hn is
      busy. Operation aborts with smx HEAP TIMEOUT error if timeout occurs.
*/