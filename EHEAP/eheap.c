/*
* eheap.c                                                   Version 5.4.0
*
* Embedded Heap Functions
*
* Copyright (c) 1989-2025 Micro Digital Inc.
* All rights reserved. www.smxrtos.com
*
* This software, documentation, and accompanying materials are made available
* under the Apache License, Version 2.0. You may not use this file except in
* compliance with the License. http://www.apache.org/licenses/LICENSE-2.0
*
* SPDX-License-Identifier: Apache-2.0
*
* This Work is protected by US Patents 10,318,198, 11,010,070, and one or more
* patents pending.
*
* A patent license is granted according to the License above.
* This entire comment block must be preserved in all copies of this file.
*
* Support services are offered by MDI. Inquire at support@smxrtos.com.
*
* Author: Ralph Moore
*
*****************************************************************************/

#include "eheap.h"
#include "bcfg.h"

#if /*SMX_CFG_SSMX &&*/ defined(__IAR_SYSTEMS_ICC__)
#pragma section_prefix = ".sys"
#endif

/*============================================================================
                                VARIABLES
============================================================================*/

EHV_PTR  eh_hvp[EH_NUM_HEAPS]; /* pointers to eh variable structures */
u32      eh_hvpn = 0;          /* next available eh_hvp slot */

/*===========================================================================*
                                  MACROS
*===========================================================================*/

#define BK_LNK(cp)      (CCB_PTR)((u32)cp->blf & (u32)~EH_FLAGS)
#define DEBUG_CHK(bp)   (((u32)*((u32*)bp - 1))&EH_DEBUG)

/*============================================================================
                           SUBROUTINE PROTOTYPES
============================================================================*/

static CCB_PTR  aligned_srch(u32 csz, u32 an, u32 hn);
static void     bin_dq(CCB_PTR cp, u32 hn);
static void     bin_nq(CCB_PTR cp, u32 hn);
static u32      bin_find(u32 csz, u32 hn);
static bool     bp_free(void* bp, u32 hn);
static void     bp_init(u32 hn);
static void*    bp_malloc(u32 sz, u32 an, u32 hn);
static void     chunk_merge_up(CCB_PTR cp, u32 hn);
static CCB_PTR  chunk_split(CCB_PTR cp, u32 csz, u32 hn, u32 iu = 0);
static void     chunk_unfree(CCB_PTR cp, u32 hn);
static void     debug_load(CCB_PTR cp);
static void     fix_ffl_bs(u32 binno, u32 num, u32 hn);
static void     fix_fl_bs(u32 num, u32 hn);
static CCB_PTR  fix_fl_sz(CCB_PTR cp, u32 hn);
static CCB_PTR  merge_space(CCB_PTR cp, u32 d, u32 hn);

#if SB_CPU_ARMM7
static CCB_PTR  dctc_get(CCB_PTR* dtcp, u32* dp, u32 csz, u32 an, u32 hn);
static CCB_PTR  region_srch(u32 csz, u32 an, u32 hn);
#endif

static void     eh_error(EH_ERRNO errnum, u32 level, u32 hn);

/*============================================================================
                             HEAP API FUNCTIONS
============================================================================*/

/*
*  eh_BinPeek()
*
*  Returns information from the heap bin.
*
*/
u32 eh_BinPeek(u32 binno, EH_PK_PAR par, u32 hn)
{
   u32     val = 0; /* value returned */
   CCB_PTR cp;

   /* check for invalid parameter */
   if (binno > eh_hvp[hn]->top_bin)
   {
      eh_error(EH_INV_PAR, 1, hn);
      return -1;
   }
   eh_hvp[hn]->errno = EH_OK;

   switch (par)
   {
      case EH_PK_COUNT:
         for (cp = eh_hvp[hn]->binp[binno].ffl; cp != NULL; cp = cp->ffl)
            val++;
         break;
      case EH_PK_FIRST:
         val = (u32)eh_hvp[hn]->binp[binno].ffl;
         break;
      case EH_PK_LAST:
         val = (u32)eh_hvp[hn]->binp[binno].fbl;
         break;
      case EH_PK_SIZE:
         val = eh_hvp[hn]->bszap[binno];
         break;
      case EH_PK_SPACE:
         for (cp = eh_hvp[hn]->binp[binno].ffl; cp != NULL; cp = cp->ffl)
            val += cp->sz;
         break;
      default:
         eh_error(EH_INV_PAR, 1, hn);
         val = -1;
   }
   return val;
}

/*
*  eh_BinScan() 
*
*  Scans the bin free list forward for a broken link. If eh_hvp[hn]->bsp == NULL, 
*  starts a new scan at bin list start, else, continues scan from eh_hvp[hn]->bsp. 
*  If broken ffl found, scans from the end of the bin to fix it, then resumes 
*  forward scan. If broken fbl found, fixes it. Also checks and fixes binx8 
*  fields in chunks and the bin fbl field in the bin. 
*  NOTE: To start scan set eh_hvp[hn]->bsp == NULL & bin_fwd == ON.
*/
bool eh_BinScan(u32 binno, u32 fnum, u32 bnum, u32 hn)
{ 
   CCB_PTR cp, ncp;
   CCB_PTR pi;
   CCB_PTR px;

   /* check for invalid parameters */
   if (binno > eh_hvp[hn]->top_bin || fnum == 0 || bnum == 0)
   {
      eh_error(EH_INV_PAR, 1, hn);
      return true;
   }
   eh_hvp[hn]->errno = EH_OK;
   pi = (CCB_PTR)eh_hvp[hn]->pi;
   px = (CCB_PTR)eh_hvp[hn]->px;

   if (!eh_hvp[hn]->mode.fl.bs_fwd)
   {
      fix_ffl_bs(binno, bnum, hn);
      return false;
   }

   if (eh_hvp[hn]->bsp == NULL) 
   {
      /* Start a new bin scan */
      cp = eh_hvp[hn]->binp[binno].ffl;

      /* return if bin is empty */
      if (cp == NULL)
      {
         /* if bin fbl is broken, fix it and report HEAP_FIXED */
         if (eh_hvp[hn]->binp[binno].fbl != NULL)
         {
            eh_hvp[hn]->binp[binno].fbl = NULL;
            eh_error(EH_HEAP_FIXED, 1, hn);
         }
         return true;
      }

      /* test if eh_hvp[hn]->binp[binno].ffl is broken */
      if (cp < pi || cp > px)
      {
         /* make bin empty, report HEAP_BRKN, and return */
         eh_hvp[hn]->binp[binno].ffl = NULL;
         eh_hvp[hn]->binp[binno].fbl = NULL;
         eh_hvp[hn]->bmap &= ~(1 << binno);
         eh_error(EH_HEAP_BRKN, 1, hn);
         return true;
      }

      /* eh_hvp[hn]->binp[binno].ffl is ok, fix cp->fbl, if broken */
      if (cp->fbl != NULL)
      {
         cp->fbl = NULL;
         eh_error(EH_HEAP_FIXED, 1, hn);
      }

      /* return if one-chunk bin */
      if (cp->ffl == NULL)
      {
         /* if bin fbl is broken, fix it and report HEAP_FIXED */
         if (eh_hvp[hn]->binp[binno].fbl != cp)
         {
            eh_hvp[hn]->binp[binno].fbl = cp;
            eh_error(EH_HEAP_FIXED, 1, hn);
         }
         /* test and fix cp->binx8, if wrong */
         if (cp->binx8 != binno*8)
         {
            cp->binx8 = binno*8;
            eh_error(EH_HEAP_FIXED, 1, hn);
         }
         return true;
      }
      eh_hvp[hn]->bsp = cp;
   }
   else
      /* continue previous bin scan */
      cp = eh_hvp[hn]->bsp;

   /* scan up to fnum chunks */
   for (; fnum > 0; fnum--)
   {
      ncp = cp->ffl;

      /* test and fix cp->binx8, if wrong */
      if (cp->binx8 != binno*8)
      {
         cp->binx8 = binno*8;
         eh_error(EH_HEAP_FIXED, 1, hn);
      }

      if (ncp == NULL) /* stop scan */
      {
         /* test and fix bin fbl, if wrong */
         if (eh_hvp[hn]->binp[binno].fbl != cp)
         {
            eh_hvp[hn]->binp[binno].fbl = cp;
            eh_error(EH_HEAP_FIXED, 1, hn);
         }
         eh_hvp[hn]->bsp = NULL;
         return true; /* done with this bin */
      }
         
      /* test if cp->ffl is broken */
      if (ncp < pi || ncp > px)
      {
         /* enable fix_fl_bs() to attempt to fix */
         eh_hvp[hn]->mode.fl.bs_fwd = OFF;
         eh_hvp[hn]->mode.fl.fbbs = ON;
         eh_hvp[hn]->bfp = eh_hvp[hn]->binp[binno].fbl;
         return false;
      }

      /* cp->ffl is ok, fix ncp->fbl, if broken */
      if (ncp->fbl != cp)
      {
         ncp->fbl = cp;
         eh_error(EH_HEAP_FIXED, 1, hn);
      }

      /* advance to next chunk */
      cp = ncp;
      eh_hvp[hn]->bsp = cp;
   }
   return false; /* continue scanning */
}

/*
*  eh_BinSeed() -- SSR
*
*  Gets a big enough chunk to divide into num chunks for blocks of size bsz and
*  puts them into the correct bin for their size.
*/
bool eh_BinSeed(u32 num, u32 bsz, u32 hn)
{
   CCB_PTR  cp, ncp;
   u32      bp, flags, i, sz;
   HMODE    hmode;

   /* check for invalid parameters */
   if (num == 0 || bsz == 0)
   {
      eh_error(EH_INV_PAR, 1, hn);
      return false;
   }
   eh_hvp[hn]->errno = EH_OK;
   hmode = eh_hvp[hn]->mode;

   /* get big chunk */
   sz = num*(bsz + EH_CHK_OVH) - EH_CHK_OVH;
   bp = (u32)eh_Malloc(sz, 0, hn);
   if (bp == NULL)
   {
      eh_error(EH_INSUFF_HEAP, 1, hn);
      return false;
   }

   /* split big chunk into num chunks with same flags and word below bp */
   cp = (CCB_PTR)(bp - EH_BP_OFFS);

   for (i = num; i > 1; i--)
   {
      ncp = (CCB_PTR)((u32)cp + bsz + EH_CHK_OVH);
      ncp->fl  = cp->fl;
      cp->fl   = ncp;
      ncp->blf = (CCB_PTR)((u32)cp | EH_INUSE);
      cp = ncp;
      if (eh_hvp[hn]->mode.fl.debug)
         debug_load(cp);
   }

   /* adjust blf pointer of next chunk (could be ec) */
   flags = (u32)cp->fl->blf & EH_FLAGS;
   cp->fl->blf = (CCB_PTR)((u32)cp | flags);

   /* turn off cmerge and free the chunks to bsz bin */
   eh_hvp[hn]->mode.fl.cmerge = OFF;
   for (i = num; i > 0; i--)
   {
      eh_Free((void*)bp, hn);
      bp = bp + bsz + EH_CHK_OVH;
   }
   /* restore cmerge */
   eh_hvp[hn]->mode = hmode;
   return true;
}

/*
*  eh_BinSort()
*
*  Sorts a heap bin free list in increasing chunk size.
*/
bool eh_BinSort(u32 binno, u32 fnum, u32 hn)
{
   u32      bsmap;   /* local bin sort map */
   s32      csbin;   /* local current sort bin */
   bool     epass;   /* end pass detected */
   CCB_PTR  ltpn;    /* ltp next */
   CCB_PTR  ncp;     /* next chunk pointer */

   /* check for invalid parameters */
   if (fnum == 0)
   {
      eh_error(EH_INV_PAR, 1, hn);
      return true;
   }
   eh_hvp[hn]->errno = EH_OK;
   bsmap = eh_hvp[hn]->bsmap;
   csbin = eh_hvp[hn]->csbin;
  
   if (csbin > eh_hvp[hn]->top_bin) /* start a new bin sort */
   {
      if (binno <= eh_hvp[hn]->top_bin)
      {
         if ((bsmap >> binno) & 1)
            csbin = binno;
         else
            return true;
      }
      else
      {
         if (bsmap)
         {
            /* find lowest bin to sort */
         #if USE_CLZ
            bsmap &= -bsmap;
            csbin = 31 - __CLZ(bsmap);
         #else
            u32 i = 0;
            for (bsmap >>= 1, i++; !(bsmap & 1) && i <= eh_hvp[hn]->top_bin; bsmap >>= 1, i++) {}
            csbin = i;
         #endif
         }
         else
         {
            /* no bins to sort */
            return true;
         }
      }
      eh_hvp[hn]->csbin = csbin;
   }

   /* start or restart a sort if bsmap bit is set */
   if ((eh_hvp[hn]->bsmap >> csbin) & 1)
   {
      eh_hvp[hn]->ccp = eh_hvp[hn]->binp[csbin].ffl;
      eh_hvp[hn]->ltp = eh_hvp[hn]->binp[csbin].fbl;
      eh_hvp[hn]->bsort = true;
      eh_hvp[hn]->bsmap &= ~(1 << csbin);
   }

   /* sort up to fnum loops */
   for (; fnum > 0; fnum--)
   {
      epass = (eh_hvp[hn]->ccp->ffl == eh_hvp[hn]->ltp) ? 1 : 0;

      /* insert last turtle if smaller than current chunk */
      if (eh_hvp[hn]->ltp->sz < eh_hvp[hn]->ccp->sz)
      {
         ltpn = eh_hvp[hn]->ltp->fbl; /* set ltp next */

         /* dequeue lt from bin free list */
         if (eh_hvp[hn]->ltp->ffl == NULL)
            eh_hvp[hn]->binp[csbin].fbl = ltpn;
         else
            eh_hvp[hn]->ltp->ffl->fbl = ltpn;
         ltpn->ffl = eh_hvp[hn]->ltp->ffl;
            
         /* move lt ahead of current chunk */
         if (eh_hvp[hn]->ccp->fbl == NULL)
            eh_hvp[hn]->binp[csbin].ffl = eh_hvp[hn]->ltp;
         else
            eh_hvp[hn]->ccp->fbl->ffl = eh_hvp[hn]->ltp;
         eh_hvp[hn]->ltp->fbl = eh_hvp[hn]->ccp->fbl;
         eh_hvp[hn]->ltp->ffl = eh_hvp[hn]->ccp;
         eh_hvp[hn]->ccp->fbl = eh_hvp[hn]->ltp;

         if (!epass)
         {
            eh_hvp[hn]->ltp = ltpn; /* move eh_hvp[hn]->ltp back to ltpn */
            eh_hvp[hn]->bsort = false; /* reset eh_hvp[hn]->bsort because a chunk has been moved */
         }
      }

      /* bubble sort */
      if (!epass)
      {
         ltpn = eh_hvp[hn]->ltp->fbl; /* set ltp next */
         ncp = eh_hvp[hn]->ccp->ffl;

         /* move next chunk ahead, if smaller than current chunk */
         if (ncp->sz < eh_hvp[hn]->ccp->sz)
         {
            /* dequeue next chunk from bin free list */
            if (ncp->ffl == NULL)
               eh_hvp[hn]->binp[csbin].fbl = eh_hvp[hn]->ccp;
            else
               ncp->ffl->fbl = eh_hvp[hn]->ccp;
            eh_hvp[hn]->ccp->ffl = ncp->ffl;

            /* move next chunk ahead of current chunk */
            if (eh_hvp[hn]->ccp->fbl == NULL)
               eh_hvp[hn]->binp[csbin].ffl = ncp;
            else
               eh_hvp[hn]->ccp->fbl->ffl = ncp;
            ncp->fbl = eh_hvp[hn]->ccp->fbl;
            ncp->ffl = eh_hvp[hn]->ccp;
            eh_hvp[hn]->ccp->fbl = ncp;

            if (ncp == eh_hvp[hn]->ltp)
               epass = true; /* pass is done */
            else
               eh_hvp[hn]->bsort = false; /* pass is not done and a chunk has been moved */  
         }
         else /* nc not moved */
            if (ncp == eh_hvp[hn]->ltp)
            {
               eh_hvp[hn]->ltp = ltpn;
               epass = true; /* pass is done */
            }
            else
               eh_hvp[hn]->ccp = ncp; /* pass is not done, advance eh_hvp[hn]->ccp */
      }

      /* test for sort done or new pass */
      if (epass)
      {
         if (eh_hvp[hn]->bsort)
         {
            /* sort is done */
            eh_hvp[hn]->csbin = -1;
            return true;
         }
         else
         {
            /* start a new pass for current bin */
            eh_hvp[hn]->ccp = eh_hvp[hn]->binp[csbin].ffl;
            eh_hvp[hn]->bsort = true;
            continue;
         }
      }
   }
   return false;
}

/*
*  eh_Calloc()
*
*  Allocates a block of num*sz bytes and clears it. Note: resets 
*  eh_hvp[hn]->mode.fl.fill before malloc and restores after memset.
*/
void* eh_Calloc(u32 num, u32 sz, u32 an, u32 hn)
{
   void* bp;

   eh_hvp[hn]->mode.fl.fill = OFF;
   eh_hvp[hn]->errno     = EH_OK;
   bp = eh_Malloc(num*sz, an, hn);
   if (bp > 0)   /* clear block */
      memset((void*)bp, 0, num*sz);
   eh_hvp[hn]->mode.fl.fill = ON;
   return bp;
}

/*
*  eh_ChunkPeek()
*
*  Returns information concerning a chunk, given a pointer to either the chunk 
*  or to the block within it.
*/
u32 eh_ChunkPeek(void* vp, EH_PK_PAR par, u32 hn)
{
   u32*     bp;
   CCB_PTR  cp;   
   CDCB_PTR dcp;
   u32      debug, inuse;
   u32      val = 0; /* returned value */
   u32      alt_debug, alt_inuse;

   eh_hvp[hn]->errno = EH_OK;

   /* verify that vp is in range, and 4-byte aligned */
   if (vp < eh_hvp[hn]->pi || vp > eh_hvp[hn]->px || (u32)vp & 3)
   {
      eh_error(EH_WRONG_HEAP, 1, hn);
      return -1;
   }

   bp = (u32*)vp;
   cp = (CCB_PTR)vp;
   dcp = (CDCB_PTR)vp;
   inuse = (u32)cp->blf&EH_INUSE;
   debug = (u32)dcp->blf&EH_DEBUG;
   alt_inuse = (u32)*(bp - 1)&EH_INUSE;
   alt_debug = (u32)*(bp - 1)&EH_DEBUG;

   switch (par)
   {
      case EH_PK_BINNO:
         if (inuse == 0) /* note: cp->binx8 == 0 for dc and tc */
            val = (u32)cp->binx8 >> 3;
         else
            val = 0;
         break;
      case EH_PK_BP:
         if (debug)
            val = (u32)cp + sizeof(CDCB) + 4*EH_NUM_FENCES;
         else if (inuse)
            val = (u32)cp + 8;
         break;
      case EH_PK_CP:
         if (alt_debug)
            val = (u32)bp - sizeof(CDCB) - 4*EH_NUM_FENCES;
         else if (alt_inuse)
            val = (u32)bp - 8;
         break;
      case EH_PK_NEXT:
         val = (u32)cp->fl;
         break;
      case EH_PK_NEXT_FREE:
         if (inuse == 0) /* note: cp->ffl == NULL for dc and tc */
            val = (u32)cp->ffl;
         break;
      case EH_PK_ONR:
         if (debug)
            val = dcp->onr;
         break;
      case EH_PK_PREV:
         val = (u32)cp->blf & (u32)~EH_FLAGS;
         break;
      case EH_PK_PREV_FREE:
         if (inuse == 0) /* note: cp->fbl == NULL for dc and tc */
            val = (u32)cp->fbl;
         break;
      case EH_PK_SIZE:
         val = (u32)cp->fl - (u32)cp;
         break;
      case EH_PK_TIME:
         if (debug)
            val = dcp->time;
         break;
      case EH_PK_TYPE:
         val = (u32)cp->blf&EH_FLAGS;
         break;
      default:
         eh_error(EH_INV_PAR, 1, hn);
         val = -1;
   }
   return val;
}

/*
*  eh_Extend()
*
*  Adds a memory extension to heap. Handles both the case where the extension is
*  adjacent to the current heap and where there is a gap. In the adjacent case,
*  merges extension with the top chunk. In the gap case, creates a permanent 
*  inuse chunk for the gap and the extension becomes the new top chunk. Loads 
*  previous tc into its correct bin. Updates eh_hvp[hn]->px. Fills the freed chunk and 
*  tc, if fill is enabled.
*/
bool eh_Extend(u32 xsz, u8* xp, u32 hn)
{
   HMODE    hmode;   /* internal heap mode */
   CCB_PTR  tcp;     /* internal top chunk pointer */
   CICB_PTR ecp;     /* end chunk pointer */
   CICB_PTR gcp;     /* gap chunk pointer */
   u32*     fp;      /* fill pointer */
   u32      i;

   /* check parameters */
   if (xsz == 0 || xp < (u8*)eh_hvp[hn]->px)
   {
      eh_error(EH_INV_PAR, 1, hn);
      return false;
   }
   eh_hvp[hn]->errno = EH_OK;
   hmode = eh_hvp[hn]->mode;
   tcp   = eh_hvp[hn]->tcp;

   /* increase xsz to 16-bytes or to next 8-byte boundary */
   if (xsz < 16)
      xsz = 16;
   else
      xsz = (xsz + 7)&0xFFFFFFF8;

   /* move xp to next 8-byte boundary */
   xp = (u8*)((u32)(xp+7)&0xFFFFFFF8);

   /* create new ec at the end of extension */
   ecp = (CICB_PTR)(xp + xsz - 8);
   ecp->fl = NULL;

   if (xp == (u8*)eh_hvp[hn]->px + 8) /* adjacent extension */
   {
      /* expand tc */
      tcp->fl = (CCB_PTR)ecp;
      tcp->sz += xsz;
   }
   else /* non-adjacent extension */
   {
      /* create an inuse chunk to cover the gap */
      gcp = eh_hvp[hn]->px;
      gcp->fl = (CCB_PTR)xp;
      gcp->blf = (CCB_PTR)((u32)tcp | EH_INUSE); 

      /* put old tc into a bin */
      bin_nq(tcp, hn);

      /* if fill enabled, load FREE pattern into data block of old tc */
      if (hmode.fl.fill)
      {
         fp = (u32*)gcp - 1;
         for (i = (tcp->sz - sizeof(CCB))/4; i > 0; i--, fp--)
            *fp = EH_FREE_FILL;
      }

      /* make extension the new tc */
      tcp = (CCB_PTR)xp;
      tcp->fl  = (CCB_PTR)ecp;
      tcp->blf = (CCB_PTR)gcp;
      tcp->sz = xsz - 8;
      tcp->fbl = tcp->ffl = NULL;
      tcp->binx8 = 0;
      eh_hvp[hn]->hused += 8;
   }
   ecp->blf = (CCB_PTR)((u32)tcp | EH_INUSE);
   eh_hvp[hn]->px = ecp;

   /* load fill pattern into data block of top chunk */
   if (hmode.fl.fill)
   {
      fp = (u32*)ecp - 1;
      for (i = (tcp->sz - sizeof(CCB))/4; i > 0; i--, fp--)
         *fp = EH_DTC_FILL;
   }
   eh_hvp[hn]->tcp = tcp;
   eh_hvp[hn]->hsz += xsz;

   return true;
}

/*
*  eh_Free()
*
*  Frees a block previously allocated from heap hn.
*/
bool eh_Free(void* bp, u32 hn)
{
   HMODE    hmode;   /* internal heap mode */
   CCB_PTR  cp;      /* chunk pointer */
   u32*     fp;      /* fill pointer */
   u32      flags, i;
   CCB_PTR  cpn;     /* new chunk pointer */
   CCB_PTR  ncp;     /* next chunk pointer */
   CCB_PTR  pcp;     /* previous chunk pointer */
   CCB_PTR  pi;      /* pointer to heap start chunk */
   CCB_PTR  px;      /* pointer to heap end chunk */

   /* per ANSI standard */
   if (bp == NULL)
      return true;

   /* check that bp is in heap range */
   pi = (CCB_PTR)eh_hvp[hn]->pi;
   px = (CCB_PTR)eh_hvp[hn]->px;

   if ((CCB_PTR)bp <= pi || (CCB_PTR)bp >= px)
   {
      eh_error(EH_INV_PAR, 2, hn);
      return false;
   }

   #if EH_BP
   if (bp < eh_hvp[hn]->fhcp)
   {
      if (bp_free(bp, hn))
         return true;
   }
   #endif

   /* prevent double free */
   if ((*((u32*)bp-1)&EH_INUSE) == 0)
   {
      eh_error(EH_HEAP_ERROR, 2, hn);
      return false;
   }

   /* convert bp to chunk pointer */
   if (DEBUG_CHK(bp))
      cp = (CCB_PTR)((u32)bp - (sizeof(CDCB) + 4*EH_NUM_FENCES));
   else
      cp = (CCB_PTR)((u32)bp - 8);

   /* get previous chunk pointer and verify it is valid */
   pcp = (CCB_PTR)((u32)cp->blf & (u32)~EH_FLAGS);
   if (cp->fl <= pi || cp->fl > px || pcp < pi || pcp >= px)
   {
      eh_error(EH_INV_CCB, 1, hn);
      return false;
   }

   eh_hvp[hn]->errno = EH_OK;
   hmode   = eh_hvp[hn]->mode;
   cp->blf = pcp;
   ncp     = cp->fl;

   /* load cp->sz if inuse chunk */
   cp->sz = (u32)ncp - (u32)cp;

   /* reduce eh_hvp[hn]->hused */
   eh_hvp[hn]->hused -= cp->sz;

   #if EH_SS_MERGE
   /* merge spare space from inuse prechunk with freed chunk unless realloc */
   if (((u32)pcp->blf & EH_SSP) && !eh_hvp[hn]->mode.fl.realloc)
   {
      /* merge pcp spare space with current chunk */
      cpn = (CCB_PTR)*((u32*)pcp->fl - 1);
      cpn->fl  = ncp;
      cpn->blf = pcp; /* flags == 0 */
      cpn->sz  = (u32)cpn->fl - (u32)cpn;
      flags    = (u32)ncp->blf & EH_FLAGS; /* preserve ncp flags */
      ncp->blf = (CCB_PTR)((u32)cpn | flags);
      pcp->fl  = cpn;
      pcp->blf = (CCB_PTR)((u32)pcp->blf & ~EH_SSP);

      /* Adjust heap scan and fix pointers, if necessary */
      if (eh_hvp[hn]->hsp == cp)
         eh_hvp[hn]->hsp = cpn;
      if (eh_hvp[hn]->hfp == cp)
         eh_hvp[hn]->hfp = cpn;
      cp = cpn;
   }
   #endif

   if (hmode.fl.cmerge || hmode.fl.realloc)
   {
      /* merge lower free chunk if not donor chunk and it is free */
      if (pcp != eh_hvp[hn]->dcp && EH_IS_FREE(pcp))
      {
         /* merge pcp with current chunk */
         bin_dq(pcp, hn);
         pcp->fl = cp->fl;
         flags = (u32)ncp->blf & EH_FLAGS; /* preserve ncp flags */
         ncp->blf = (CCB_PTR)((u32)cp->blf | flags);
         pcp->sz += cp->sz;

         /* Adjust heap scan and fix pointers, if necessary */
         if (eh_hvp[hn]->hsp == cp)
            eh_hvp[hn]->hsp = pcp;
         if (eh_hvp[hn]->hfp == cp)
            eh_hvp[hn]->hfp = pcp;
         cp = pcp;
      }
      else
      {
         pcp = NULL; /*<4>*/
      }

      /* merge upper free chunk */
      if (EH_IS_FREE(ncp))
         chunk_merge_up(cp, hn);
      else
         ncp = NULL; /*<4>*/

      eh_hvp[hn]->ncp = ncp;
      eh_hvp[hn]->pcp = pcp;
   }

   /* if fill enabled, load free pattern into data block (dc and tc are
      filled in chunk_merge_up) */
   if (hmode.fl.fill && cp != eh_hvp[hn]->dcp && cp != eh_hvp[hn]->tcp)
   {
      fp = (u32*)cp + cp->sz/4 - 1;
      for (i = (cp->sz - sizeof(CCB))/4; i > 0; i--, fp--)
         *fp = EH_FREE_FILL;
   }

   /* put new free chunk into its bin, unless dc or tc */
   if (cp != eh_hvp[hn]->dcp && cp != eh_hvp[hn]->tcp)
      bin_nq(cp, hn);
   return true;
}

/*
*  eh_Init()
*
*  Intializes heap starting at hp and ending at hp + sz. Creates a start
*  chunk of 8 bytes, an end chunk of 8 bytes, a donor chunk, and a top chunk. 
*  Start and end chunks have 0 data bytes, the donor chunk has dcsz bytes,
*  and the top chunk has the remaining free space. Initializes the 
*  eh_hvp[hn]->hcb PCB, heap bins, heap modes, heap used & high-water mark, and 
*  returns true, if successful or if heap already initialized. Fails and 
*  returns false if parameters are invalid.
*/
u32 eh_Init(u32 sz, u32 dcsz, u8* hp, EHV_PTR vp, u32* bszap, HBCB* binp, 
                                              u32 mode, const char* name)
{
   HMODE    hmode;   /* internal heap mode */
   CCB_PTR  dc;      /* donor chunk */
   CICB_PTR ec;      /* end chunk */
   #if defined(EH_BT_DEBUG) || defined(SMX_DEBUG)
   u32*     fp;      /* fill pointer */
   #endif
   u32      hn;      /* heap number */
   u32      i;
   CICB_PTR sc;      /* start chunk */
   CCB_PTR  tc;      /* top chunk */

   /* exit if parameters are not valid */
   if (vp == NULL)
   {
      return -1;
   }

   if (sz < 32 || hp == NULL)
   {
      vp->errno = EH_INV_PAR;
      return -1;
   }

   /* fail if out of eh_hvp slots */
   if (eh_hvpn >= EH_NUM_HEAPS)
   {
      vp->errno = EH_TOO_MANY_HEAPS;
      return -1;
   }

   /* do not initialize more than once */
   if (vp->mode.fl.init)
   {
      vp->errno = EH_ALREADY_INIT;
      return -1;
   }
   vp->errno = EH_OK;

   /* adjust sz and dcsz to 8-byte multiples and hp to 8-byte boundary */
   if ((u32)hp & 0x7) /* if hp is misaligned */
      sz -= 8 - ((u32)hp & 0x7);
   sz   &= 0xFFFFFFF8;
   dcsz &= 0xFFFFFFF8;
   hp = (u8*)((u32)(hp+7)&0xFFFFFFF8);

   /* set hn and load vp into eh_hvp[hn] */
   hn = eh_hvpn;
   eh_hvp[eh_hvpn++] = vp;

   /* initialize start, end, and top chunks */
   sc      = (CICB_PTR)hp;
   sc->fl  = (CCB_PTR)(hp + 8);
   sc->blf = (CCB_PTR)(NULL | EH_INUSE);

   ec      = (CICB_PTR)(hp + sz - 8);
   tc      = (CCB_PTR)(hp + 8);
   ec->fl  = NULL;
   ec->blf = (CCB_PTR)((u32)tc | EH_INUSE);

   tc->blf = (CCB_PTR)sc;
   tc->fl  = (CCB_PTR)ec;
   tc->sz  = (u32)ec - (u32)tc;
   tc->fbl = tc->ffl = NULL;
   tc->binx8 = 0;

   /* setup for eh_Malloc */
   vp->binp  = binp;
   vp->bszap = bszap;
   vp->cbp   = NULL;
   vp->hsz   = sz;
   vp->mode.fl.use_dc = OFF;
   vp->tcp   = tc;

   /* initialize block pools, if enabled */
   #if EH_BP
   bp_init(hn);
   #endif

   /* initialize dc */
   if (dcsz < 24)
   {
      dc = (CCB_PTR)((u8*)eh_Malloc(16, 3, hn) - 8);
   }
   else
   {
      dc = (CCB_PTR)((u8*)eh_Malloc(dcsz-8, 3, hn) - 8);
   }
   tc = vp->tcp;
   dc->blf = (CCB_PTR)((u32)dc->blf & ~EH_FLAGS); 
   dc->sz = (u32)tc - (u32)dc;
   dc->fbl = dc->ffl = NULL;
   dc->binx8 = 0;

   /* initialize vp pointers */
   vp->dcp   = dc;
   #if EH_BP
   vp->fhcp  = dc;
   #endif

   /* set user-modifiable heap flags */
   hmode.wd = mode;

   /* set non-user-modifiable heap flags */
   hmode.fl.init   = ON;
   hmode.fl.hs_fwd = ON;
   hmode.fl.bs_fwd = ON;

   /* calculate vp->sba_top, vp->sba_top_sz, and vp->top_bin */
   for (i = 0; vp->bszap[i+1] - vp->bszap[i] == 8; i++){}

   if (i == 0) /* no SBA */
   {
      vp->sba_top = 0;
      vp->sba_top_sz = 0;
      hmode.fl.use_dc = OFF;
   }
   else /* SBA is present */
   {
      vp->sba_top = --i;
      vp->sba_top_sz = vp->bszap[i];
      if (dcsz <= 24)
         hmode.fl.use_dc = OFF;
      else
         hmode.fl.use_dc = ON;
   }

   /* find top bin */
   for ( ; vp->bszap[i+1] < 0xFFFFFFFF; i++){}
   vp->top_bin = i;

   /* clear the bin array and bin map */
   memset((void*)vp->binp, 0, 8*(vp->top_bin + 1));
   vp->bmap = 0;

   vp->pi = sc;
   vp->px = ec;

   /* initialize heap scan pointer to start of heap */
   vp->hsp = (CCB_PTR)sc;

   /* initialize bin sorting */
   vp->csbin = -1;
   vp->bsmap = 0;

   #if defined(EH_BT_DEBUG) || defined(SMX_DEBUG)
   /* load fill patterns into data blocks of top chunk and donor chunk */
   if (hmode.fl.fill)
   {
      fp = (u32*)ec - 1;
      for (i = (tc->sz - 24)/4; i > 0; i--, fp--)
      {
         *fp = EH_DTC_FILL;
      }
      if (hmode.fl.use_dc == ON)
      {
         fp = (u32*)tc - 1;
         for (i = (dc->sz - 24)/4; i > 0; i--, fp--)
         {
            *fp = EH_DTC_FILL;
         }     
      }
   }
   #endif

   /* initialize remaining vp fields */
   vp->acop  = NULL;
   vp->hhwm  = 32;
   vp->hused = 32;
   vp->mode  = hmode;
   vp->name  = name;
   vp->tbsz  = 0;
   return hn;
}

/*
*  eh_Malloc()
*
*  Allocates a block aligned on an bytes of size sz bytes from heap hn.
*/
void* eh_Malloc(u32 sz, u32 an, u32 hn)
{
   HMODE    hmode;   /* heap mode */
   u32      bmap;    /* bin map */
   void*    bp;      /* block pointer value to return */
   CCB_PTR  cp;      /* chunk pointer */
   u32      csz;     /* chunk size */
   u32*     fp;      /* free pointer */
   u32*     flp;     /* fill pointer */
   u32      i, j;    /* indices */
   CCB_PTR  ncp;     /* new chunk pointer */

   /* auto-init heap(s) here if desired or necessary for C++ initializers <12> */

   /* check for invalid size */
   if (sz == 0 || sz > eh_hvp[hn]->hsz)
   {
      eh_error(EH_INV_PAR, 2, hn);
      return NULL;
   }
   eh_hvp[hn]->errno = EH_OK;

   #if EH_BP
   if (sz <= 12)
   {
      if (bp = bp_malloc(sz, an, hn))
      {
         return bp;
      }
   }
   #endif

   hmode = eh_hvp[hn]->mode;
   cp    = NULL;

   /* round block size up to 16-bytes or next 8-byte boundary */
   if (sz < 16)
      sz = 16;
   else
      sz = (sz + 7)&0xFFFFFFF8;

   /* determine needed chunk size */
   csz = sz + EH_CHK_OVH;

   /* find and allocate chunk */
   if (an > 3)  /* ok that EH_R not masked */
   {
      #if EH_ALIGN
      cp = aligned_srch(csz, an, hn);
      if (cp == NULL)
      {
         eh_error(EH_INSUFF_HEAP, 2, hn);
         return NULL;
      }
      #else
      {
         eh_error(EH_INV_PAR, 2, hn);
         return NULL;
      }
      #endif
   }
   else
   {
retry:
      bmap  = eh_hvp[hn]->bmap;
      if (csz <= eh_hvp[hn]->sba_top_sz)
      {
         /* try exact-fit SBA bin */
         i = (csz/8) - 3;
         bmap >>= i;

         if (bmap & 1) /* get from SBA */
         {
            cp = eh_hvp[hn]->binp[i].ffl;
            bin_dq(cp, hn);
         }
         else
         {
            /* or calve from donor chunk if enabled and large enough */
            if (hmode.fl.use_dc && ((csz + sizeof(CCB) <= eh_hvp[hn]->dcp->sz)))
            {
               cp = eh_hvp[hn]->dcp;
               eh_hvp[hn]->dcp = chunk_split(cp, csz, hn); /* split from dc */
            }
         }
      }
      else /* or try to get from best-fit UBA bin */
      {
         i = bin_find(csz, hn);
         bmap >>= i;

         /* search bin[i] for first large-enough chunk */
         if (bmap & 1)
            for (cp = eh_hvp[hn]->binp[i].ffl; cp != NULL && csz > cp->sz; cp = cp->ffl){}
         if (cp != NULL)
            bin_dq(cp, hn); /* remove chunk from bin[i] */
      }
      if (cp == NULL) /* or try to get from next occupied bin above bin[i] <11> */
      {
         bmap >>= 1;
         if (bmap > 0)
         {
            /* find next higher occupied bin */
            #if USE_CLZ
            bmap &= -bmap;
            i += 32 - __CLZ(bmap);
            #else
            for (i++; !(bmap & 1) && i <= eh_hvp[hn]->top_bin; bmap >>= 1, i++) {}
            #endif

            /* get first chunk from bin[i] */
            cp = eh_hvp[hn]->binp[i].ffl;
            bin_dq(cp, hn);
         }
      }
      if (cp == NULL) /* or calve from top chunk if it is big enough */
      {
         if (csz + sizeof(CCB) <= eh_hvp[hn]->tcp->sz)
         {
            cp = eh_hvp[hn]->tcp;
            eh_hvp[hn]->tcp = chunk_split(cp, csz, hn); /* split from tc */
         } 
         else /* chunk not found in heap */
         {
            /* try auto-recovery if enabled */
            if (eh_hvp[hn]->mode.fl.auto_rec && eh_Recover(sz, -1, 0, hn))
            {
               eh_hvp[hn]->retries++;
               eh_error(EH_RECOVER, 2, hn);
               goto retry;
            }
            else
            {
               eh_error(EH_INSUFF_HEAP, 2, hn);
               return NULL;
            }
         }
      }
   }

   /* set EH_INUSE flag <9> */
   cp->blf = (CCB_PTR)((u32)cp->blf | EH_INUSE);

   /* copy current data block to new data block for eh_Realloc() <1> */
   bp = (void*)((u32)cp + EH_BP_OFFS);
   
   if (eh_hvp[hn]->cbp != NULL && (u32)bp != (u32)eh_hvp[hn]->cbp && sz > 16)
   {
      memcpy((void*)((u32)bp+16), (void*)((u32)eh_hvp[hn]->cbp+16), sz-16);
      eh_hvp[hn]->cbp = NULL;
   }

   /* split found chunk, if big enough, merge remnant with upper free 
      chunk, if any, and put new free chunk into a bin, unless it is dc
      or tc */
   if (cp->sz - csz >= EH_MIN_FRAG)
   {
      ncp = chunk_split(cp, csz, hn);
      if (hmode.fl.cmerge && EH_IS_FREE(ncp->fl)) 
         chunk_merge_up(ncp, hn);
      if (ncp != eh_hvp[hn]->dcp && ncp != eh_hvp[hn]->tcp)
         bin_nq(ncp, hn);
   }
   /* else if spare space exists after the block, put a pointer to its start
      at the end of the chunk and set the EH_SSP flag in blf. */
   else if (cp->sz - csz > 0)
   {  
      fp = (u32*)((u32)cp->fl - (cp->sz - csz));
      *(u32*)((u32)cp->fl - 4) = (u32)fp;
       cp->blf = (CCB_PTR)((u32)cp->blf | EH_SSP);
   }

   /* load debug information, if enabled */
   if (hmode.fl.debug)
      debug_load(cp);

   /* update heap_used and heap_hwm (must be ahead of block fill) */
   eh_hvp[hn]->hused += cp->sz;
   if (eh_hvp[hn]->hused > eh_hvp[hn]->hhwm)
      eh_hvp[hn]->hhwm = eh_hvp[hn]->hused;

   /* fill data block and spare space, if enabled.  */
   if (hmode.fl.fill)
   {
      /* load d's into data block */
      if (((u32)cp->blf & EH_SSP) == 0)
      {
         if (hmode.fl.debug)
         {
            flp = (u32*)cp->fl - EH_NUM_FENCES - 1;
            j = (cp->sz - sizeof(CDCB))/4 - 2*EH_NUM_FENCES;
         }
         else 
         {
            flp = (u32*)cp->fl - 1;
            j   = (cp->sz - 8)/4;
         }
         for (; j > 0; j--, flp--)
         {
            *flp = EH_DATA_FILL;
         }
      }
      else
      {
         /* load e's into ss after ssp */
         flp = (u32*)cp->fl - 2;
         j   = flp - fp + 1;
         for (; j > 0; j--, flp--)
         {
            *flp = EH_FREE_FILL;
         }

         /*  load d's into data block Note: overwrites sz. */
         if (hmode.fl.debug)
         {
            j = fp - (u32*)cp - sizeof(CDCB)/4 - 2*EH_NUM_FENCES;
            flp -= EH_NUM_FENCES;
         }
         else
         {
            j  = fp - (u32*)cp - 2;
         }

         for (; j > 0; j--, flp--)
         {
            *flp = EH_DATA_FILL;
         }
      }
   }
   else
   {
      if (!hmode.fl.debug)
         cp->sz = 0; /* done for flag recovery */
   }
   return bp;
}

/*
*  eh_Peek()
*
*  Returns information concerning the heap.
*/
u32 eh_Peek(EH_PK_PAR par, u32 hn)
{
   HMODE hmode;   /* internal heap mode */
   u32   val = 0; /* value returned */

   eh_hvp[hn]->errno = EH_OK;
   hmode = eh_hvp[hn]->mode;

   switch (par)
   {
      case EH_PK_AUTO:
         val = hmode.fl.amerge;
         break;
      case EH_PK_BS_FWD:
         val = hmode.fl.bs_fwd;
         break;
      case EH_PK_DEBUG:
         val = hmode.fl.debug;
         break;
      case EH_PK_FILL:
         val = hmode.fl.fill;
         break;
      case EH_PK_HS_FWD:
         val = hmode.fl.hs_fwd;
         break;
      case EH_PK_INIT:
         val = hmode.fl.init;
         break;
      case EH_PK_MERGE:
         val = hmode.fl.cmerge;
         break;
      case EH_PK_USE_DC:
         val = hmode.fl.use_dc;
         break;
      default:
         eh_error(EH_INV_PAR, 1, hn);
         val = -1;
   }
   return val;
}

/*
*  eh_Realloc()
*
*  Allocates a new size heap block from an existing heap block, if possible.
*  Conforms to ANSI C Standard. Preserves debug mode, but temporarily suspends
*  fill mode, if it is set.
*/
void* eh_Realloc(void* cbp, u32 sz, u32 an, u32 hn)
{
   u32      amask;         /* alignment mask */
   CCB_PTR  ccp;           /* current chunk pointer */
   u32      ccsz;          /* current chunk size */
   u32      csz;           /* new chunk size */
   u32      data_sav[7];   /* save first 4 words of data block and last 3 of CDCB */
   u32*     dp;            /* data pointer for copying data */
   u32      flags_sav;     /* current chunk flags save */
   HMODE    hmode;         /* internal heap mode */
   u32      i;             /* index */
   void*    nbp;           /* new block pointer */
   CCB_PTR  ncp;           /* new chunk pointer */
   CCB_PTR  pi;            /* pointer to heap start chunk */
   CCB_PTR  px;            /* pointer to heap end chunk */

   if (cbp == NULL)
   {
      return eh_Malloc(sz, an, hn);
   }
   if (sz == 0)
   {
      eh_Free(cbp, hn);
      return NULL;
   }

   pi = (CCB_PTR)eh_hvp[hn]->pi;
   px = (CCB_PTR)eh_hvp[hn]->px;

   /* convert cbp to current chunk pointer */
   if (DEBUG_CHK(cbp))
      ccp = (CCB_PTR)((u32)cbp - (sizeof(CDCB) + 4*EH_NUM_FENCES));
   else
      ccp = (CCB_PTR)((u32)cbp - 8);

   /* avoid heap mangling */
   if (ccp <= pi || ccp >= px || (u32)ccp & 7)
   {
      eh_error(EH_WRONG_HEAP, 2, hn);
      return NULL;
   }
   eh_hvp[hn]->errno = EH_OK;
   hmode = eh_hvp[hn]->mode;
   nbp = cbp;
   ncp = NULL;
   amask = ~(0xFFFFFFFF << an);

   /* turn fill mode off */
   eh_hvp[hn]->mode.fl.fill = OFF;

   /* round block size to 16-bytes or next 8-byte boundary */
   if (sz < 16)
      sz = 16;
   else
      sz = (sz + 7)&0xFFFFFFF8;

   /* determine needed and current chunk sizes */
   csz = sz + EH_CHK_OVH;
   ccsz = (u32)ccp->fl - (u32)ccp;

   /* need different chunk if too small or not aligned enough */
   if (csz > ccsz || (u32)cbp & amask)
   {
      /* save first 4 words of block and current flags <3> */
      dp = (u32*)cbp;
      for (i = 0; i < 4; i++)
         data_sav[i] = *dp++;
      flags_sav = (u32)ccp->blf & EH_FLAGS;
      eh_hvp[hn]->cbp = cbp;

      if (DEBUG_CHK(cbp))
      {
         /* save last 3 words of CDCB <13> */
         dp = (u32*)ccp + 3;
         for (; i < 7; i++)
            data_sav[i] = *dp++;
      }

      /* free current chunk and merge with adjacent free chunks, if any */
      eh_hvp[hn]->mode.fl.realloc = ON;
      eh_Free(cbp, hn);
      eh_hvp[hn]->mode.fl.realloc = OFF;

      /* get new chunk <2> and copy data <2>, if necessary */
      if ((nbp = eh_Malloc(sz, an, hn)) != NULL)
      {
         /* got a new chunk. copy first 4 words of old block to new block */
         dp = (u32*)nbp;
         for (i = 0; i < 4; i++)
            *dp++ = data_sav[i];
      }
      else /* HeapRealloc() has failed */
      {
         /* unfree original chunk and clear eh_hvp[hn]->cbp */
         chunk_unfree(ccp, hn);
         eh_hvp[hn]->cbp = NULL;

         /* restore first 4 words of the current block and its flags */
         dp = (u32*)cbp;
         for (i = 0; i < 4; i++)
            *dp++ = data_sav[i];
         ccp->blf = (CCB_PTR)((u32)ccp->blf | flags_sav);

         if (DEBUG_CHK(cbp))
         {
            /* restore last 3 words of CDCB */
            dp = (u32*)ccp + 3;
            for (; i < 7; i++)
               *dp++ = data_sav[i];
         }
      }
   }
   else /* current chunk is sufficient */
   {
      /* split current chunk, if big enough, merge remnant with upper free 
         chunk, if any, put new free chunk into a bin, unless it is dc or tc,
         and reduce heap used. */
      if (ccsz >= csz + EH_MIN_FRAG)
      {
         /* split current chunk and update heap_used */
         eh_hvp[hn]->mode = hmode; /* restore fill mode */
         ncp = (CCB_PTR)((u8*)ccp + csz);
         ncp->fl = ccp->fl;
         ccp->fl = ncp;
         flags_sav = (u32)ncp->fl->blf & EH_FLAGS;
         ncp->fl->blf = (CCB_PTR)((u32)ncp | flags_sav);
         ncp->blf = ccp;
         ncp->sz = ccsz - csz;
         eh_hvp[hn]->hused -= ncp->sz;

         /* merge with upper free chunk */
         if (eh_hvp[hn]->mode.fl.cmerge && EH_IS_FREE(ncp->fl)) 
            chunk_merge_up(ncp, hn);

         /* put final chunk into a bin unless it is dc or tc */
         if (ncp != eh_hvp[hn]->dcp && ncp != eh_hvp[hn]->tcp)
            bin_nq(ncp, hn);
      }
   }
   eh_hvp[hn]->mode = hmode; /* restore fill mode */
   return nbp;
}

/*
*  eh_Recover()
*
*  Scans the heap for adjacent free chunks that can be merged to satisfy csz
*  and alignment, or region requirements. Searches lower heap for small chunks 
*  and upper heap for large chunks. If found, removes free chunks from bins, 
*  merges them, puts result into bin, dc, or tc and returns true, else returns 
*  false.
*/
bool  eh_Recover(u32 sz, u32 num, u32 an, u32 hn)
{
   CCB_PTR  ccp, ncp, pcp;
   u32      anr, flags, bp, csz, d, n, r;
   bool     space_found;

   /* check parameters */
   if (sz == 0 || num == 0)
   {
      eh_error(EH_INV_PAR, 2, hn);
      return false;
   }
   #if !EH_ALIGN
   if (an > 3)
   {
      eh_error(EH_INV_PAR, 2, hn);
      return false;
   }
   #endif

   eh_hvp[hn]->errno = EH_OK;
   space_found    = false;

   /* determine needed chunk size */
   csz = sz + EH_CHK_OVH;

   /* initialize ccp */
   if (csz <= eh_hvp[hn]->sba_top_sz)
   {
      ccp = (CCB_PTR)eh_hvp[hn]->pi; /* start search in lower heap */
   }
   else
   {
      ccp = eh_hvp[hn]->dcp; /* start search in upper heap */
   }

   /* search until enough free space is found, fnum chunks have been tested, 
      or the whole heap has been searched */
   for (n = 0; n < num && !space_found; n++)
   {
      ccp = ccp->fl;
      if (((u32)ccp->blf & EH_INUSE) == 0)
      {
         pcp = ccp;
         /* determine d for free chunk loop */
         if (an > 3)
         {
            bp = (u32)pcp + EH_BP_OFFS;

            if (an <= EH_MAX_AN) /* aligned allocation */
            {
               /* d = distance to next 2^an boundary */
               d  = -bp & ((1 << an) - 1);
            }
            else /* region allocation */
            {
               /* r = distance to next region boundary */
               anr = an & ~EH_R;
               r  = -bp & ((1 << (anr+3)) - 1);
               if (r < csz - EH_BP_OFFS)
               {
                  d = r; /* d to region boundary */
               }
               else
               {
                  d = -bp & ((1 << anr) - 1); /* d to next 2^anr boundary */
               }
            }
         }
         /* free chunk loop: loop until enough contiguous free space has been
            found or an inuse chunk has been found */
         do
         {
            if (an < 3) /* normal allocation */
            {
               if (csz <= ((u32)ccp->fl - (u32)pcp))
               {
                  space_found = true;
                  break;
               }
            }
            else /* aligned or region allocation */
            {
               if ((csz + d) <= ((u32)ccp->fl - (u32)pcp))
               {
                  if (!((ccp == eh_hvp[hn]->dcp) && (csz + d > eh_hvp[hn]->sba_top_sz))) /*<5>*/
                  {
                     space_found = true;
                     break;
                  }
               }
            }
            /* exit free chunk loop if dc <6> has been tested or end of heap */
            if ((ccp == eh_hvp[hn]->dcp) || (ccp > eh_hvp[hn]->tcp))
               break;

            /* advance to next chunk */
            ccp = ccp->fl;
            n++;
         } while (((u32)ccp->blf & EH_INUSE) == 0);
      }
      /* exit num loop if end of heap */
      if (ccp > eh_hvp[hn]->tcp)
      {
         break; 
      }
   }
   if (space_found)
   {
      /* dequeue found chunks */
      for (ncp = pcp; ncp <= ccp && ncp != eh_hvp[hn]->dcp && ncp != eh_hvp[hn]->tcp; 
                                                                  ncp = ncp->fl)
         bin_dq(ncp, hn);

      /* merge found chunks */
      pcp->sz = (u32)ccp->fl - (u32)pcp;
      pcp->fl = ccp->fl;
      flags = (u32)ccp->fl->blf & EH_FLAGS;
      ccp->fl->blf = (CCB_PTR)((u32)pcp | flags);

      /* put merged chunk into a bin, unless it is dc or tc, in which case, 
         update eh_hvp[hn]->dcp or eh_hvp[hn]->tcp. */
      if (ccp == eh_hvp[hn]->dcp)
         eh_hvp[hn]->dcp = pcp;
      else if (ccp == eh_hvp[hn]->tcp)
         eh_hvp[hn]->tcp = pcp;
      else
         bin_nq(pcp, hn);

      /* restart heap scan/fix, if in progress */
      eh_hvp[hn]->hsp = NULL;
      eh_hvp[hn]->mode.fl.hs_fwd = ON;

      return true;
   }
   else
   {
      return false;
   }
}

/*
*  eh_Scan()
*
*  Scans the heap forward for a broken link. If cp is NULL, starts scan from
*  eh_hvp[hn]->hsp. If cp is out of heap range, sets it to eh_hvp[hn]->pi. Scans up to
*  fnum chunks forward. If ncp = cp->fl is invalid, calls fix_fl_sz() to try 
*  to fix it. Aborts and enables fix_fl_bs() if not fixed. If fixed, tests
*  ncp->blf and fixes it, if broken. Checks and fixes flags, size, and fences.
*  If ncp is last active chunk, checks and fixes ec->blf and returns true.
*  NOTE: true indicates that a complete heap scan has been done. false means 
*  to continue scanning.
*/
bool eh_Scan(CCB_PTR cp, u32 fnum, u32 bnum, u32 hn)
{
   u32 flags, n, sz;
   bool free, debug;
   u32* fp;
   CCB_PTR ncp;   /* next chunk pointer */
   CCB_PTR pi;    /* pointer to heap start chunk */
   CCB_PTR px;    /* pointer to heap end chunk */

   pi = (CCB_PTR)eh_hvp[hn]->pi;
   px = (CCB_PTR)eh_hvp[hn]->px;

   /* check for wrong heap */
   if ((cp != NULL && cp < pi) || cp > px)
   {
      eh_error(EH_WRONG_HEAP, 1, hn);
      return true;
   }

   /* check for invalid parameters */
   if (fnum == 0 || bnum == 0)
   {
      eh_error(EH_INV_PAR, 1, hn);
      return true;
   }
   eh_hvp[hn]->errno = EH_OK;

   if (!eh_hvp[hn]->mode.fl.hs_fwd)
   {
      fix_fl_bs(bnum, hn);
      return false;
   }

   /* intialize chunk pointer */
   if (cp == NULL)
      cp = eh_hvp[hn]->hsp;

   /* if cp out of heap range, start a new heap scan */
   if (cp < pi || cp >= px)
      cp = pi;

   eh_hvp[hn]->hsp = cp;

   /* scan up to fnum chunks */
   for (; fnum > 0; fnum--)
   {
      ncp = cp->fl;

      /* determine if cp->fl is broken */
      if (ncp < cp || ncp > px)
         /* attempt to fix cp->fl. */
         if ((ncp = fix_fl_sz(cp, hn)) == NULL)
            return false;

      /* cp->fl is ok. If not at end of heap and ncp->blf is not valid, fix it */
      if (ncp != px && BK_LNK(ncp) != cp)
      {
         /* restore flags for ncp <10> */
         if (ncp->sz == (u32)ncp->fl - (u32)ncp)
         {
            if (ncp->binx8 == EH_FENCE_FILL)
               flags = EH_DEBUG + EH_INUSE;
            else
               flags = EH_FREE;
         }
         else
            flags = EH_INUSE;

         /* fix ncp->blf */
         ncp->blf = (CCB_PTR)((u32)cp | flags);
         eh_error(EH_HEAP_FIXED, 1, hn);
      }

      /* check and restore flags for cp <10> */
      flags = (u32)cp->blf & EH_FLAGS;
      if (flags == 2 || flags == 4 || flags == 6)
      {
         if (cp->sz == (u32)cp->fl - (u32)cp)
            if (cp->binx8 == EH_FENCE_FILL)
               flags = EH_DEBUG + EH_INUSE;
            else
               flags = EH_FREE;
         else
            flags = EH_INUSE;

         /* fix cp->blf */
         cp->blf = (CCB_PTR)(((u32)cp->blf & (u32)~EH_FLAGS) | flags);
         eh_error(EH_HEAP_FIXED, 1, hn);
      }
      debug = flags & EH_DEBUG;
      free = !(flags & EH_INUSE);

      /* check and fix cp size, if free or debug chunk */
      if (free || debug)
      {
         sz = (u32)cp->fl - (u32)cp;
         if (cp->sz != sz)
         {
            cp->sz = sz;
            eh_error(EH_HEAP_FIXED, 1, hn);
         }
      }

      /* check and fix cp fences, if debug chunk */
      if (debug)
      {
         #if defined(EH_BT_DEBUG) || defined(SMX_DEBUG)
         /* report broken lower fences */
         fp = (u32*)cp + 5;
         for (n = EH_NUM_FENCES + 1; n > 0; n--, fp++)
            if (*fp != EH_FENCE_FILL)
            {
               eh_error(EH_HEAP_FENCE_BRKN, 1, hn);
               return true;
            }
         /* report broken upper fences */
         if ((u32)cp->blf & EH_SSP)
            fp = (u32*)(*((u32*)cp->fl - 1)) - EH_NUM_FENCES;
         else
            fp = (u32*)cp->fl - EH_NUM_FENCES;
         for (n = EH_NUM_FENCES; n > 0; n--, fp++)
            if (*fp != EH_FENCE_FILL)
            {
               eh_error(EH_HEAP_FENCE_BRKN, 1, hn);
               return true;
            }
         #else
         /* fix broken lower fences */
         fp = (u32*)cp + 5;
         for (n = EH_NUM_FENCES + 1; n > 0; n--, fp++)
            if (*fp != EH_FENCE_FILL)
            {
               *fp = EH_FENCE_FILL;
               eh_error(EH_HEAP_FIXED, 1, hn);
            }
         /* fix broken upper fences */
         if ((u32)cp->blf & EH_SSP)
            fp = (u32*)(*((u32*)cp->fl - 1)) - EH_NUM_FENCES;
         else
            fp = (u32*)cp->fl - EH_NUM_FENCES;
         for (n = EH_NUM_FENCES; n > 0; n--, fp++)
            if (*fp != EH_FENCE_FILL)
            {
               *fp = EH_FENCE_FILL;
               eh_error(EH_HEAP_FIXED, 1, hn);
            }
         #endif
      }

      if (ncp == px) /* test and fix eh_hvp[hn]->px->blf and stop scan */
      {
         if (eh_hvp[hn]->px->blf != (CCB_PTR)((u32)cp | EH_INUSE))
         {
            eh_hvp[hn]->px->blf = (CCB_PTR)((u32)cp | EH_INUSE);
            eh_error(EH_HEAP_FIXED, 1, hn);
         }
         eh_hvp[hn]->hsp = pi;
         return true;
      }
      else  /* advance to next chunk */
      {
         cp = ncp;
         eh_hvp[hn]->hsp = cp;
      }
   }
   return false; /* heap scan not done */
}

/*
*  eh_Set()
*
*  Sets the specified heap parameter to the specified value.
*/
bool eh_Set(EH_ST_PAR par, u32 val, u32 hn)
{
   HMODE hmode; /* internal heap mode */

   eh_hvp[hn]->errno = EH_OK;
   hmode = eh_hvp[hn]->mode;

   if (val > 0) 
      val = 1;

   switch (par)
   {
      case EH_ST_AUTO:
         hmode.fl.amerge = val;
         break;
      case EH_ST_DEBUG:
         hmode.fl.debug = val;
         break;
      case EH_ST_FILL:
         hmode.fl.fill = val;
         break;
      case EH_ST_MERGE:
         hmode.fl.cmerge = val;
         break;
      default:
      {
         eh_error(EH_INV_PAR, 1, hn);
         return false;
      }
   }
   eh_hvp[hn]->mode = hmode;
   return true;
}

/*===========================================================================*
*                       ALIGNED ALLOCATION SUBROUTINES                       *
*                           Do Not Call Directly                             *
*===========================================================================*/
#if EH_ALIGN
/*
*  aligned_srch()
*
*  Called from eh_Malloc() to find a > 8-byte aligned block.
*/

static CCB_PTR aligned_srch(u32 csz, u32 an, u32 hn)
{ 
   u32      amask;   /* alignment mask */
   u32      bmap;    /* bin map */
   u32      bp;      /* current block pointer */
   CCB_PTR  cp;      /* current chunk pointer */
   u32      d;       /* distance to next 2^an boundary */
   u32      i, j;
   u32      tmap;    /* temporary bin map */

   #if SB_CPU_ARMM7
   if (an & EH_R)
   {
      cp = region_srch(csz, an, hn);
      return cp;
   }
   #elif SB_CPU_ARMM8
   an &= ~EH_R;
   #endif
   amask = ~(0xFFFFFFFF << an);
   cp    = NULL;

retry:
   bmap  = eh_hvp[hn]->bmap;
   if (csz <= eh_hvp[hn]->sba_top_sz)
   {
      /* find exact-fit SBA bin */
      i = (csz/8) - 3;
      bmap >>= i;

      /* try to get chunk from bin */
      if (bmap & 1)
      {
         cp = eh_hvp[hn]->binp[i].ffl;
         d = 0;

         /* search small bin[i] for aligned block */
         while (cp != NULL)
         {
            bp = (u32)cp + EH_BP_OFFS;
            if ((bp & amask) == 0)
            {
               bin_dq(cp, hn);
               return cp;
            }
            cp = cp->ffl; /* continue search */
         }
      }
      /* or calve from donor chunk if it is enabled and large enough */
      if (cp == NULL && eh_hvp[hn]->mode.fl.use_dc)
      {
         /* find distance, d, to next 2^an boundary */
         bp = (u32)eh_hvp[hn]->dcp + EH_BP_OFFS; /* add size of CICB or CDCB */
         d  = -bp & ((1 << an) - 1);

         /* if (csz + d) is a small chunk and dc is big enough get chunk 
            from dc <8> */
         if ((csz + d <= eh_hvp[hn]->sba_top_sz) && 
                           ((csz + d + sizeof(CCB)) <= eh_hvp[hn]->dcp->sz))
         {
            cp = eh_hvp[hn]->dcp;
            eh_hvp[hn]->dcp = chunk_split(cp, csz+d, hn); /* split from dc */
         }
      }
   }
   else /* or try to get from best-fit UBA bin */
   {
      i = bin_find(csz, hn);
      bmap >>= i;

      /* search bin[i] for large-enough chunk with aligned block */
      if (bmap & 1)
      {
         cp = eh_hvp[hn]->binp[i].ffl;
         while (cp != NULL)
         {
            if (cp->sz >= csz)
            {
               /* find distance, d, to next 2^an boundary */
               bp = (u32)cp + EH_BP_OFFS; /* add size of CICB or CDCB */
               d  = -bp & ((1 << an) - 1);

               /* if chunk is big enough get chunk */
               if (cp->sz >= (csz + d))
               {
                  bin_dq(cp, hn); /* remove chunk from bin[i] */
                  break;
               }
            }
            cp = cp->ffl; /* continue bin[i] search */
         }
      }
   }
   if (cp == NULL) /* or try to get from next occupied bin above bin[i] <11> */
   {
      bmap >>= 1;
      while (bmap > 0 && cp == NULL)
      {
         /* find next higher occupied bin */
         tmap = bmap;
        #if USE_CLZ
         tmap &= -tmap;
         j = 32 - __CLZ(tmap);
        #else
         for (j = 1; !(tmap & 1) && j <= eh_hvp[hn]->top_bin; tmap >>= 1, j++) {}
        #endif
         bmap >>= j;
         i += j;

         /* search bin[i] for large enough chunk with aligned block */
         cp = eh_hvp[hn]->binp[i].ffl;
         while (cp != NULL)
         {
            /* find distance, d, to next 2^an boundary */
            bp = (u32)cp + EH_BP_OFFS; /* add size of CICB or CDCB */
            d  = -bp & ((1 << an) - 1);

            /* if chunk is big enough get it */
            if (cp->sz >= (csz + d))
            {
               bin_dq(cp, hn); /* remove chunk from bin */
               break;
            }
            cp = cp->ffl; /* continue bin[i] search */
         }
      } /* go to next higher occupied bin */
   }
   if (cp == NULL)  /* or calve chunk from top chunk if it is big enough */
   {
      /* find distance, d, to next 2^an boundary */
      bp = (u32)eh_hvp[hn]->tcp + EH_BP_OFFS; /* add size of CICB or CDCB */
      d  = -bp & ((1 << an) - 1);

      /* if tc is big enough get chunk from it <8> */
      if (eh_hvp[hn]->tcp->sz >= (csz + d + sizeof(CCB)))
      {
         cp = eh_hvp[hn]->tcp;
         eh_hvp[hn]->tcp = chunk_split(cp, csz+d, hn); /* split from tc */            
      }
      else /* chunk not found in heap */
      {
         /* try auto-recovery if enabled */
         if (eh_hvp[hn]->mode.fl.auto_rec && eh_Recover(csz-EH_CHK_OVH, -1, an, hn))
         {
            eh_hvp[hn]->retries++;
            eh_error(EH_RECOVER, 2, hn);
            goto retry;
         }
         else
         {
            eh_error(EH_INSUFF_HEAP, 2, hn);
            return NULL;
         }
      }
   }

   /* deal with spare space if bp was not already aligned */
   if (cp != NULL && d > 0)
   {
      cp = merge_space(cp, d, hn);
   }
   return cp;
}

/*
*  merge_space()
*
*  Called from aligned_srch() or region_srch() to combine d bytes of unused 
*  space ahead of an aligned block with a preceding free chunk and move CCB 
*  ahead of the aligned block.
*/
static CCB_PTR merge_space(CCB_PTR cp, u32 d, u32 hn)
{
   HMODE    hmode;/* heap mode */
   u32*     fp;   /* fill pointer */
   u32      flags, n, nsz;
   CCB_PTR  ncp;  /* next chunk pointer */
   CCB_PTR  mcp;  /* moved chunk pointer */  
   CCB_PTR  pcp;  /* previous chunk pointer */
   u32*     ssp;  /* spare space pointer */

   hmode = eh_hvp[hn]->mode;

   /* move mcp up so it is before the aligned block */
   mcp     = (CCB_PTR)((u32)cp + d);
   mcp->sz = (cp->sz - d); /* needed by eh_Malloc() */
   pcp     = cp->blf;      /* flags == 0, since cp chunk is free */
   ncp     = cp->fl;

   /* link pcp <-> mcp <-> ncp */
   pcp->fl  = mcp;
   mcp->blf = pcp;   /* inuse flag is set later in eh_Malloc() */
   mcp->fl  = ncp;
   flags    = (u32)ncp->blf & EH_FLAGS; /* next chunk may be inuse */
   ncp->blf = (CCB_PTR)((u32)mcp | flags);

   /* adjust heap scan pointer and heap fix pointer, if necessary */
   if (eh_hvp[hn]->hsp == cp)
      eh_hvp[hn]->hsp = pcp;
   if (eh_hvp[hn]->hfp == cp)
      eh_hvp[hn]->hfp = pcp;

   if (EH_IS_FREE(pcp) && (pcp != eh_hvp[hn]->dcp)) /* prechunk is free and not dc*/
   {
      /* increase prechunk size */
      n = pcp->binx8 >> 3;
      nsz = pcp->sz + d;
      if (n == eh_hvp[hn]->top_bin || nsz < eh_hvp[hn]->bszap[n + 1])
      {
         pcp->sz = nsz; /* stays in same bin */
      }
      else /* requeue prechunk into a larger bin */
      {
         bin_dq(pcp, hn);
         pcp->sz = nsz;
         bin_nq(pcp, hn);
      }
   }
   if (pcp == eh_hvp[hn]->dcp) /* prechunk is dc */
   {
      if (d < EH_MIN_FRAG)
      {
         pcp->sz += d;
      }
      else
      {
         /* split free chunk from dc and enqueue it into a bin */
         cp = chunk_split(pcp, ((u32)cp - (u32)pcp), hn);
         bin_nq(cp, hn);
         d -= sizeof(CCB); /* protect new CCB (chunk is free) from fill */
      }
   }
   if (!(EH_IS_FREE(pcp))) /* prechunk is an inuse or debug chunk */
   {
      if ((u32)pcp->blf & EH_SSP)
      {
         /* update d if inuse prechunk already has spare space */
         ssp = (u32*)*(u32*)((u32)cp - 4);
         d   = ((u32)mcp - (u32)ssp);
      }
      else
      {
         /* add new spare space to pcp */
         ssp = (u32*)((u32)mcp - d);
      }

      if (d >= EH_MIN_FRAG)
      {
         /* split free chunk @ssp from inuse prechunk and enqueue it into a bin */
         cp = chunk_split(pcp, ((u32)ssp - (u32)pcp), hn, 1);
         bin_nq(cp, hn);
         pcp->blf = (CCB_PTR)((u32)pcp->blf & ~EH_SSP); /* reset EH_SSP flag, if set */
         /* fill only new spare space and protect new CCB from fill */
         d = (u32)mcp - (u32)cp - sizeof(CCB); 
      }
      else
      {
         /* load ssp at the end of prechunk, set EH_SSP, and update pcp->sz */
         *(u32*)((u32)mcp - 4) = (u32)ssp;
         pcp->blf = (CCB_PTR)((u32)pcp->blf | EH_SSP);
         if ((u32)pcp->blf & EH_DEBUG)
            pcp->sz += d;
      }
   }
   /* if fill enabled, load free pattern into spare/freed space */
   if (hmode.fl.fill)
   {
      if ((u32)pcp->blf & EH_SSP)
      {
         /* protect ssp from fill */
         fp = (u32*)((u32)mcp - 8);
         d -= 4;
      }
      else
      {
         fp = (u32*)((u32)mcp - 4);
      }
      for (n = d/4; n > 0; n--, fp--)
         *fp = EH_FREE_FILL;
   }
   return mcp;
}

#if SB_CPU_ARMM7
/*
*  dctc_get()
*
*  Called from region_srch() to get a chunk with aligned block inside of a 
   region from dc or tc.
*/
static CCB_PTR dctc_get(CCB_PTR* dtcp, u32* dp, u32 csz, u32 an, u32 hn)
{
   u32      bp;   /* current block pointer */
   CCB_PTR  cp;   /* current chunk pointer */
   u32      d;    /* distance to next 2^an boundary */
   u32      r;    /* distance to next region boundary */

   /* find distance, d, to next 2^an boundary and r to next region boundary */
   bp = (u32)(*dtcp) + EH_BP_OFFS; /* add size of CICB or CDCB */
   d  = -bp & ((1 << an) - 1);
   r  = -bp & ((1 << (an+3)) - 1);

   /* move block up, if necesary so it is inside of its region  */
   if ((r != 0) && (r < csz - EH_BP_OFFS))
   {
      /* move bp and d up to region boundary */
      bp += r;
      d   = r;
   }

   /* if dc or tc is large enough, take chunk <8> */
   if (((*dtcp)->sz) >= (csz + d + sizeof(CCB)))
   {
      cp = *dtcp;
      *dtcp = chunk_split(cp, csz+d, hn);
   }
   else
   {
      cp = NULL;
   }
   *dp = d;
   return cp;
}

/*
*  region_srch()
*
*  Called from aligned_search() to find an MPU region. 2^an is assumed
*  to be the subregion size. Thus the region size is 2^(an+3). Finds a 2^an
*  aligned block that is completely inside of the next region boundary.
*  Note: Use aligned_srch() for regions without subregions.
*/
static CCB_PTR region_srch(u32 csz, u32 an, u32 hn)
{ 
   u32      amask;   /* alignment mask */
   u32      bmap;    /* bin map */
   u32      bp;      /* current block pointer */
   CCB_PTR  cp;      /* current chunk pointer */
   u32      d;       /* distance to next 2^an boundary */
   u32      r;       /* distance to next region boundary = 2^(an+3) */
   u32      i, j;    /* indices */
   u32      tmap;    /* temporary bin map */

   an &= ~EH_R;
   if (an > EH_MAX_AN)
   {
      eh_error(EH_INV_PAR, 1, hn);
      return NULL;
   }
   amask = ~(0xFFFFFFFF << an);
   cp    = NULL;

retry:
   bmap  = eh_hvp[hn]->bmap;
   if (csz <= eh_hvp[hn]->sba_top_sz)
   {
      /* find exact-fit SBA bin */
      i = (csz/8) - 3;
      bmap >>= i;

      /* try to get chunk from bin */
      if (bmap & 1)
      {
         cp = eh_hvp[hn]->binp[i].ffl;
         d = 0;

         /* search small bin[i] for region block */
         while (cp != NULL)
         {
            bp = (u32)cp + EH_BP_OFFS;
            if ((bp & amask) == 0)
            {
               /* if block is inside region, dq and return it */
               r  = -bp & ((1 << (an+3)) - 1);
               if (r >= csz - EH_BP_OFFS || r == 0)
               {
                  bin_dq(cp, hn);
                  return cp;
               }
            }
            cp = cp->ffl; /* continue search */
         }
      }
      /* or calve from donor chunk if it is enabled and large enough */
      if (cp == NULL && eh_hvp[hn]->mode.fl.use_dc)
      {
         cp = dctc_get(&eh_hvp[hn]->dcp, &d, csz, an, hn);
      }
   }
   if (cp == NULL) /* or try to get from best-fit UBA bin */
   {
      i = bin_find(csz, hn);
      bmap >>= i;

      if (bmap & 1)
      {
         cp = eh_hvp[hn]->binp[i].ffl; 
         while (cp != NULL)
         {
            if (cp->sz >= csz)
            {
               /* find distance d to next subregion boundary and distance r to 
                  next region boundary */
               bp = (u32)cp + EH_BP_OFFS;
               d  = -bp & ((1 << an) - 1);
               r  = -bp & ((1 << (an+3)) - 1);

               /* move block up, if necesary, so it is inside of its region  */
               if ((r != 0) && (r < csz - EH_BP_OFFS))
               {
                  /* move bp and d up to region boundary */
                  bp += r - d;
                  d   = r;
               }
               /* if chunk is large enough, get it */
               if (cp->sz >= (csz + d))
               {
                  bin_dq(cp, hn);
                  break;
               }
            }
            cp = cp->ffl; /* continue bin[i] search */
         }
      }
   }
   if (cp == NULL) /* or try to get from a higher bin <11> */
   {
      bmap >>= 1;
      while (bmap > 0 && cp == NULL)
      {
         /* find next higher occupied bin */
         tmap = bmap;
        #if USE_CLZ
         tmap &= -tmap;
         j = 32 - __CLZ(tmap);
        #else
         for (j = 1; !(tmap & 1) && j <= eh_hvp[hn]->top_bin; tmap >>= 1, j++) {}
        #endif
         bmap >>= j;
         i += j;
         cp = eh_hvp[hn]->binp[i].ffl; 
         while (cp != NULL)
         {
            /* find distance d to next subregion boundary and distance r to 
               next region boundary */
            bp = (u32)cp + EH_BP_OFFS;
            d  = -bp & ((1 << an) - 1);
            r  = -bp & ((1 << (an+3)) - 1);

            /* move block up, if necesary, so it is inside of its region  */
            if ((r != 0) && (r < csz - EH_BP_OFFS))
            {
               /* move bp and d up to region boundary */
               bp += r - d;
               d   = r;
            }
            /* if chunk is large enough, get it */
            if (cp->sz >= (csz + d))
            {
               bin_dq(cp, hn);
               break;
            }
            cp = cp->ffl; /* continue bin[i] search */
         }
      } /* go to next higher occupied bin */
   }
   if (cp == NULL)  /* or calve chunk from top chunk if it is big enough */
   {
      cp = dctc_get(&eh_hvp[hn]->tcp, &d, csz, an, hn);
   }
   if (cp == NULL) /* chunk not found in heap */
   {
      /* try auto-recovery if enabled */
      if (eh_hvp[hn]->mode.fl.auto_rec && eh_Recover(csz-EH_CHK_OVH, -1, an + EH_R, hn))
      {
         eh_hvp[hn]->retries++;
         eh_error(EH_RECOVER, 2, hn);
         goto retry;
      }
      else
      {
         eh_error(EH_INSUFF_HEAP, 2, hn);
      }
   }

   /* deal with unused space if bp was not already aligned */
   if (cp != NULL && d > 0)
   {
      cp = merge_space(cp, d, hn);
   }
   return cp;
}
#endif
#elif SB_CPU_ARMM8
static CCB_PTR aligned_srch(u32 csz, u32 an, u32 hn)
{
   return NULL;
}
#endif

/*===========================================================================*
*                           BLOCK POOL SUBROUTINES                           *
*                            Do Not Call Directly                            *
*===========================================================================*/
#if EH_BP
/*
*  bp_free()
*
*  Frees a block previously allocated from a block pool.
*/
static bool bp_free(void* bp, u32 hn)
{
   BPCB_PTR bpcbp;   /* block pool control block pointer */

   /* return if bpcbp is NULL */
   bpcbp = eh_hvp[hn]->bpcbp;
   if (bpcbp == NULL)
   {
      return false;
   }

   if (bp <= bpcbp->px) /* bp in pb8 */
   {
      *(u32*)bp = (u32)bpcbp->pn;
      bpcbp->pn = bp;
      bpcbp->inuse--;
   }
   else /* bp in pb12 */
   {
      bpcbp++;
      *(u32*)bp = (u32)bpcbp->pn;
      bpcbp->pn = bp;
      bpcbp->inuse--;
   }
   return true;
}

/*
*  bp_init() -- Create and initialize block pools if enabled.
*/
static void bp_init(u32 hn)
{
   u32** bp;         /* pointer to block pointer in block */
   u32   bsz;        /* block size */
   u32   i;          /* pool index */
   u32   n;          /* number of blocks / reverse block index */
   u32*  nbp;        /* next block pointer */
   u32   psz;        /* pool size */
   BPCB_PTR bpcbp;   /* block pool control block pointer */

   /* create pools */
   if (eh_hvp[hn]->bpcbp)
   {
      for (i = 0; i < 2; i++)
      {
         bpcbp = eh_hvp[hn]->bpcbp + i;
         n     = bpcbp->num_blks;
         if (n > 0)
         {
            /* allocate pool from heap */
            bsz  = 4 * i + 8;
            psz  = bsz * n;
            if ((bpcbp->pn = (u8*)eh_Malloc(psz, 3, hn)) == NULL)
            {
               return;
            }

            /* clear pool and link all of its blocks together */
            memset (bpcbp->pn, 0, psz);
            for (bp = (u32**)bpcbp->pn; n > 1; n--)
            {
               nbp = (u32*)((u32)bp + bsz);
               *bp = nbp;
               bp  = (u32**)nbp;
            }
            *bp = NULL; /* last block has NULL pointer */

            /* initialize block pool control block */
            bpcbp->inuse  = 0;
            bpcbp->maxuse = 0;
            bpcbp->pi = bpcbp->pn;
            bpcbp->px = (void*)((u8*)bpcbp->pi + psz - bsz);
         }
         else
         {
            bpcbp->pn = 0;
         }
      }
   }
}

/*
*  bp_malloc() -- Allocate block from block pool.
*/
static void* bp_malloc(u32 sz, u32 an, u32 hn)
{
   void*    bp;      /* pointer to next block in pool */
   BPCB_PTR bpcbp;   /* block pool control block pointer */

   bpcbp = eh_hvp[hn]->bpcbp;

   /* return if alignment > 8 or bpcbp is NULL */
   if (an > 3 || bpcbp == NULL)
   {
      return NULL;
   }
   if (sz <= 8) /* get block from 8-byte pool */
   {
      if ((bp = bpcbp->pn) != NULL)
      {
         bpcbp->pn = *(void**)bp;
      }
   }
   else /* get block from 12-byte pool */
   {
      if ((bp = (++bpcbp)->pn) != NULL)
      {
         if (an <= 2 || (u32)bp & 0x7 == 0)
         {
            bpcbp->pn = *(void**)bp;
         }
         else
         {
            bp = NULL;
         }
      }
   }

   /* update inuse and maxuse */
   if (bp != NULL)
   {
      bpcbp->inuse++;
      if (bpcbp->inuse > bpcbp->maxuse)
      {
         bpcbp->maxuse = bpcbp->inuse;
      }
   }
   return bp;
}
#endif

/*===========================================================================*
*                            GENERAL SUBROUTINES                             *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  bin_dq() -- Remove chunk from its free bin list and clear the
*  ith bin map bit if the bin becomes empty. If the chunk is removed from a
*  bin being sorted and the bin has at least two chunks remaining, its bin 
*  sort map bit is set so that the sort will start over. If the bin now has
*  less than two chunks, its bsmap bit is reset and, if a sort was in progress,
*  it is cancelled. If the bin was being scanned and cp equals eh_hvp[hn]->bsp or 
*  eh_hvp[hn]->bfp, the scan is started over.
*
*  Note: Chunk must be in a bin free list.
*/
static void bin_dq(CCB_PTR cp, u32 hn)
{
   HBCB_PTR bin = eh_hvp[hn]->binp;
   u32 i        = cp->binx8 >> 3;

   /* dequeue cp from bin i */
   cp->binx8 = 0;  /* will be restored to CDCB fence if block un-freed in eh_Realloc() */
   if (cp->fbl == NULL)
      bin[i].ffl = cp->ffl;
   else
      cp->fbl->ffl = cp->ffl;

   if (cp->ffl == NULL)
      bin[i].fbl = cp->fbl;
   else
      cp->ffl->fbl = cp->fbl;

   /* clear eh_hvp[hn]->bmap.i if bin i is empty */
   if (bin[i].ffl == NULL)
      eh_hvp[hn]->bmap &= ~(1 << i);

   /* adjust eh_hvp[hn]->bsmap.i and eh_hvp[hn]->csbin for chunk removal */
   if (bin[i].ffl == NULL || bin[i].ffl->ffl == NULL)
   {
      eh_hvp[hn]->bsmap &= ~(1 << i);
      if (eh_hvp[hn]->csbin == i)
         eh_hvp[hn]->csbin = -1;
   }
   else
      if (eh_hvp[hn]->csbin == i)
         eh_hvp[hn]->bsmap |= (1 << i);

   /* restart bin scan if eh_hvp[hn]->bsp or eh_hvp[hn]->bfp has been impacted */
   if (cp == eh_hvp[hn]->bsp || cp == eh_hvp[hn]->bfp)
   {
      eh_hvp[hn]->bsp = NULL;
      eh_hvp[hn]->mode.fl.bs_fwd = ON;
   }

   /* reduce tbsz if i is the top bin */
   if (i == eh_hvp[hn]->top_bin)
      eh_hvp[hn]->tbsz -= cp->sz;

   #if EH_STATS
   if (eh_hvp[hn]->bnump)
      eh_hvp[hn]->bnump[i]--;
   if (eh_hvp[hn]->bsump)
      eh_hvp[hn]->bsump[i] -= cp->sz;
   #endif
}

/*
*  bin_find() -- Does binary search of upper bins based upon chunk size.
*/
static u32 bin_find(u32 csz, u32 hn)
{
   u32 i  = eh_hvp[hn]->top_bin;
   u32 hi = eh_hvp[hn]->top_bin;
   u32 lo = eh_hvp[hn]->sba_top;

   if (csz < eh_hvp[hn]->bszap[hi])
   {
      while (hi - lo > 1)
      {
         i = lo + (hi - lo)/2;
         if (csz == eh_hvp[hn]->bszap[i])
         {
            lo = i;
            break;
         }
         if (csz > eh_hvp[hn]->bszap[i])
            lo = i;
         else
            hi = i;
      }
      i = lo;
   }
   return i;
}

/*
*  bin_nq() -- If bin is empty, link chunk to its ffl and fbl and set its 
*  bmap bit. Otherwise, link the chunk to the start of free list, if smaller
*  than first chunk, or to back of free list, if not. Load the (bin number)*8
*  into its binx8 field.
*  Note: Chunk must be free.
*/
static void bin_nq(CCB_PTR cp, u32 hn)
{
   HBCB_PTR bin = eh_hvp[hn]->binp;
   u32 i;

   if (cp->sz <= eh_hvp[hn]->sba_top_sz)
      i = (cp->sz/8) - 3;
   else
      i = bin_find(cp->sz, hn);

   if (bin[i].ffl == NULL) /* bin i is empty */
   {
      bin[i].ffl = cp;
      bin[i].fbl = cp;
      cp->ffl = NULL;
      cp->fbl = NULL;
      eh_hvp[hn]->bmap |= 1 << i;
   }
   else
   {
      if (cp->sz <= bin[i].ffl->sz) /* if <=, put at start */
      {
         cp->ffl = bin[i].ffl;
         cp->fbl = NULL;
         bin[i].ffl->fbl = cp;
         bin[i].ffl = cp;
      }
      else /* if larger, put at end and set bsmap bit */
      {
         bin[i].fbl->ffl = cp;
         cp->ffl = NULL;
         cp->fbl = bin[i].fbl;
         bin[i].fbl = cp;
         if (bin[i].ffl->ffl != cp) /* at least 3 chunks */
            eh_hvp[hn]->bsmap |= 1 << i;
      }
   }
   cp->binx8 = i << 3;

   if (i == eh_hvp[hn]->top_bin)
      eh_hvp[hn]->tbsz += cp->sz;

   #if EH_STATS
   if (eh_hvp[hn]->bnump)
      eh_hvp[hn]->bnump[i]++;
   if (eh_hvp[hn]->bsump)
      eh_hvp[hn]->bsump[i] += cp->sz;
   #endif
}

/*
*  chunk_merge_up()
*
*  Merge chunk with upper free chunk and adjust affected pointers.
*  Note: upper chunk assumed to be free and merge enabled.
*/
static void chunk_merge_up(CCB_PTR cp, u32 hn)
{
   u32      flags, i;
   u32*     fp;
   CCB_PTR  ncp = cp->fl;

   /* remove ncp from bin unless it is tc or dc */
   if (ncp == eh_hvp[hn]->tcp)
      eh_hvp[hn]->tcp = cp;
   else 
      if (ncp == eh_hvp[hn]->dcp)
         eh_hvp[hn]->dcp = cp;
      else  
         bin_dq(ncp, hn); /* remove chunk from bin */

   /* merge ncp and cp */
   flags = (u32)ncp->fl->blf & EH_FLAGS;
   cp->fl = ncp->fl;
   ncp->fl->blf = (CCB_PTR)((u32)cp | flags);
   cp->sz += ncp->sz;

   /* if fill enabled, load DTC fill pattern into added dc or tc space,
      clear ncp CCB and clear cp ffl, fbl, and binx8 */
   if (eh_hvp[hn]->mode.fl.fill && (cp == eh_hvp[hn]->dcp || cp == eh_hvp[hn]->tcp))
   {
      fp = (u32*)ncp + 5;
      if (cp == eh_hvp[hn]->dcp && fp >= (u32*)eh_hvp[hn]->tcp)
         fp = (u32*)eh_hvp[hn]->tcp - 1;
      for (i = ((u32)fp - (u32)cp - sizeof(CCB) + 4)/4; i > 0; i--, fp--)
         *fp = EH_DTC_FILL;
      for (i = 3; i > 0; i--, fp--)
         *fp = 0;
   }

   /* Adjust scan and fix pointers, if necessary */
   if (eh_hvp[hn]->hsp == ncp)
      eh_hvp[hn]->hsp = cp;
   if (eh_hvp[hn]->hfp == ncp)
      eh_hvp[hn]->hfp = cp;
}

/*
*  chunk_split()
*
*  Split chunk: cp -> chunk, csz is its intended size. returns -> remnant.
*  Note: chunk is assumed to be big enough to split.
*/
static CCB_PTR chunk_split(CCB_PTR cp, u32 csz, u32 hn, u32 iu)
{
   u32      flags;
   CCB_PTR  ncp;
   u32      ccsz; /* current chunk size */

   ccsz = (u32)cp->fl - (u32)cp;
   ncp = (CCB_PTR)((u8*)cp + csz);
   ncp->fl = cp->fl;
   cp->fl = ncp;
   flags = (u32)ncp->fl->blf & EH_FLAGS;
   ncp->fl->blf = (CCB_PTR)((u32)ncp | flags);
   ncp->blf = cp;
   ncp->sz = ccsz - csz;
   if (cp == eh_hvp[hn]->dcp || cp == eh_hvp[hn]->tcp)
   {
      ncp->fbl = ncp->ffl = NULL;
      ncp->binx8 = 0;
   }
   if (iu == 0)
   {
      cp->sz = csz;
   }
   return ncp;
}

/*
*  chunk_unfree() -- Internal subroutine used by eh_Realloc(). cp points to
*  the chunk that was freed. It may have been merged with the chunk below or
*  the chunk above, or both, if either or both were free. eh_hvp[hn]->pcp and 
*  eh_hvp[hn]->ncp are left over from eh_Free(); they and their blf's are in their 
*  original states. Realloc() must save and restore overwritten data and flags
*  in the original chunk.
*/
static void chunk_unfree(CCB_PTR cp, u32 hn)
{
   u32 flags;
   CCB_PTR ncp = eh_hvp[hn]->ncp;
   CCB_PTR pcp = eh_hvp[hn]->pcp;

   /* process previous chunk, if merged <4>, and put it into its bin. */
   if (pcp)
   {
      /* remove pcp from its bin unless it is dc or tc. In the latter case,
         update eh_hvp[hn]->dcp or eh_hvp[hn]->tcp to cp */
      if (pcp == eh_hvp[hn]->dcp)
         eh_hvp[hn]->dcp = cp;
      else 
         if (pcp == eh_hvp[hn]->tcp)
            eh_hvp[hn]->tcp = cp;
         else  
            bin_dq(pcp, hn);

      /* split pcp from cp */
      cp->fl = pcp->fl;
      pcp->fl = cp;
      flags = (u32)cp->fl->blf & EH_FLAGS;
      cp->fl->blf = (CCB_PTR)((u32)cp | flags);
      pcp->sz = (u32)cp - (u32)pcp;

      /* return pcp to its bin */
      bin_nq(pcp, hn);
   }
   else /* remove free chunk from its bin, unless it is dc or tc */
      if (cp != eh_hvp[hn]->dcp && cp != eh_hvp[hn]->tcp)
         bin_dq(cp, hn);

   /* split next chunk, if merged <4>, and put it into its bin unless it is dc 
      or tc, in which case, restore eh_hvp[hn]->dcp or eh_hvp[hn]->tcp */
   if (ncp)
   {
      /* split cp from ncp */
      ncp->fl = cp->fl;
      cp->fl = ncp;
      flags = (u32)ncp->blf & EH_FLAGS;
      ncp->blf = (CCB_PTR)((u32)cp | flags);
      flags = (u32)ncp->fl->blf & EH_FLAGS;
      ncp->fl->blf = (CCB_PTR)((u32)ncp | flags);
      ncp->sz = (u32)ncp->fl - (u32)ncp;

      /* restore eh_hvp[hn]->dcp or eh_hvp[hn]->tcp or return ncp to its bin */
      if (cp == eh_hvp[hn]->dcp)
         eh_hvp[hn]->dcp = ncp;
      else if (cp == eh_hvp[hn]->tcp)
         eh_hvp[hn]->tcp = ncp;
      else
         bin_nq(ncp, hn);
   }
   /* restore cp size and eh_hvp[hn]->hused */
   cp->sz = (u32)cp->fl - (u32)cp;
   eh_hvp[hn]->hused += cp->sz;
}

/*
*  debug_load()
*
*  Sets EH_DEBUG flag, loads CDCB, and fills fences with fence fill pattern.
*/
static void debug_load(CCB_PTR cp)
{
   CDCB_PTR dcbp;
   u32      j;
   u32*     fp;   /* word fill pointer */

   /* set EH_DEBUG flag */
   cp->blf = (CCB_PTR)((u32)cp->blf | EH_DEBUG);

   /* load heap debug control block */
   dcbp = (CDCB_PTR)(cp);
   dcbp->time = eh_time();
   dcbp->onr  = eh_onr();
   dcbp->fence = EH_FENCE_FILL;

   /* load upper fence fill */
   if ((u32)cp->blf & EH_SSP)
   {
      fp = (u32*)*((u32*)cp->fl - 1) - 1;
   }
   else
   {
      fp = (u32*)cp->fl - 1;
   }
   for (j = EH_NUM_FENCES; j > 0; j--, fp--)
      *fp = EH_FENCE_FILL;

   /* load lower fence fill */
   fp = (u32*)cp + sizeof(CDCB)/4 + EH_NUM_FENCES - 1;
   for (j = EH_NUM_FENCES; j > 0; j--, fp--)
      *fp = EH_FENCE_FILL;
}

/*
*  fix_ffl_bs() -- Fixes broken ffl by scanning backward from bin list end, 
*  num chunks at a time. Stops when it reaches eh_hvp[hn]->bsp and sets eh_hvp[hn]->mode.fl.bs_fwd 
*  ON. If a bad back link is encountered, bridges the gap to chunk at eh_hvp[hn]->bsp,
*  aborts back scanning, sets eh_hvp[hn]->mode.fl.bs_fwd ON, reports EH_HEAP_BRKN, and 
*  returns. NOTE: Must be started with efsp -> last bin chunk and 
*  eh_hvp[hn]->mode.fl.hs_fwd = OFF.
*/
static void fix_ffl_bs(u32 binno, u32 num, u32 hn)
{
   CCB_PTR bfp = eh_hvp[hn]->bfp;
   CCB_PTR bsp = eh_hvp[hn]->bsp;
   CCB_PTR cp  = eh_hvp[hn]->bfp;
   CCB_PTR pcp;
   CCB_PTR pi = (CCB_PTR)eh_hvp[hn]->pi;
   CCB_PTR px = (CCB_PTR)eh_hvp[hn]->px;

   /* if bfp is not in range, bridge free list, report HEAP_BRKN, and return */
   if (bfp < pi || bfp > px)
   {
      eh_hvp[hn]->mode.fl.bs_fwd = ON;
      eh_hvp[hn]->bsp->ffl = NULL;
      eh_hvp[hn]->binp[binno].fbl = eh_hvp[hn]->bsp;
      eh_error(EH_HEAP_BRKN, 1, hn);
      return;
   }
   
   /* if first back scan, test for valid end chunk */
   if (eh_hvp[hn]->mode.fl.fbbs == ON && cp->ffl != NULL)
   {
      /* if invalid, bridge the gap, report HEAP_BRKN, and abort */
      eh_hvp[hn]->binp[binno].fbl = bsp;
      eh_hvp[hn]->bsp->ffl = NULL;
      if (bsp == bfp)
         eh_error(EH_HEAP_FIXED, 1, hn);
      else
         eh_error(EH_HEAP_BRKN, 1, hn);
      eh_hvp[hn]->mode.fl.bs_fwd = ON;
      return;
   }
   eh_hvp[hn]->mode.fl.fbbs = OFF;

   /* scan up to num chunks */
   for (; num > 0; num--)
   {
      pcp = cp->fbl;

      /* test if cp->fbl is broken */
      if (pcp < pi || pcp > px)
      {
         /* bridge the gap */
         cp->fbl = bsp;
         bsp->ffl = cp;

         /* enable forward scan and return */
         eh_hvp[hn]->mode.fl.bs_fwd = ON;
         eh_error(EH_HEAP_BRKN, 1, hn);
         return;
      }

      /* cp->fbl is ok, fix pcp->ffl, if broken */
      if (pcp->ffl != cp)
      {
         pcp->ffl = cp;
         eh_error(EH_HEAP_FIXED, 1, hn);
      }

      /* check if done */
      if (pcp == bsp)
      {
         /* enable forward scan and return */
         eh_hvp[hn]->mode.fl.bs_fwd = ON;
         return;
      }

      /* else advance to next chunk */
      cp = pcp;
      eh_hvp[hn]->bfp = cp;
   }
   return;
}

/*
*  fix_fl_bs() -- Fixes broken fl by scanning backward from heap end, 
*  num chunks, at a time. Stops when it reaches eh_hvp[hn]->hsp and turns 
*  eh_hvp[hn]->mode.fl.hs_fwd ON. If a bad back link is encountered, reports 
*  EH_HEAP_BRKN, bridges the gap between hsp and hfp, turns scan_fwd ON, and 
*  returns. Fixes all broken forward links encountered.
*/
static void fix_fl_bs(u32 num, u32 hn)
{
   u32 flags;
   CCB_PTR cp  = eh_hvp[hn]->hfp;
   CCB_PTR pcp = BK_LNK(cp);
   CCB_PTR hsp = eh_hvp[hn]->hsp;
   CCB_PTR pi  = (CCB_PTR)eh_hvp[hn]->pi;
   CCB_PTR px  = (CCB_PTR)eh_hvp[hn]->px;

   /* if px->blf is broken: bridge the gap, report HEAP_BRKN, and abort */
   if (cp == px && (pcp < pi || pcp > px))
   {
      eh_hvp[hn]->px->blf = (CCB_PTR)((u32)hsp + EH_INUSE);
      eh_hvp[hn]->hsp->fl = px;
      eh_error(EH_HEAP_BRKN, 1, hn);
      eh_hvp[hn]->mode.fl.hs_fwd = ON;
      return;
   }

   /* back scan up to num chunks */
   for (; num > 0; num--)
   {
      /* test if cp->blf is broken */
      if (pcp < pi || pcp > px || pcp >= cp)
      {
         /* restore flags for cp */
         if (cp->sz == ((u32)cp->fl - (u32)cp))
            if (cp->binx8 == EH_FENCE_FILL)
               flags = EH_DEBUG + EH_INUSE;
            else
               flags = EH_FREE;
         else
            flags = EH_INUSE;

         /* bridge the gap, report HEAP_BRKN, set scan_fwd, and return */
         cp->blf = (CCB_PTR)((u32)hsp | flags);
         eh_hvp[hn]->hsp->fl = cp;
         eh_error(EH_HEAP_BRKN, 1, hn);
         eh_hvp[hn]->mode.fl.hs_fwd = ON;
         return;
      }

      /* check pcp->fl. (cp->blf is ok) */
      if (pcp->fl != cp)
      {
         /* fix pcp->fl and report HEAP_FIXED */
         pcp->fl = cp;
         eh_error(EH_HEAP_FIXED, 1, hn);
      }

      /* check if done */
      if (pcp == eh_hvp[hn]->hsp)
      {
         /* enable forward scan and return */
         eh_hvp[hn]->mode.fl.hs_fwd = ON;
         return;
      }

      /* else go to previous chunk */
      cp = pcp;
      pcp = BK_LNK(cp);
      eh_hvp[hn]->hfp = cp;
   }  
   return;
}

/*
*  fix_fl_sz() -- Fixes broken fl, using cp->sz, if valid. Otherwise 
*  enables fix_fl_bs() to start back scan.
*/

static CCB_PTR fix_fl_sz(CCB_PTR cp, u32 hn)
{
   u32 flags = (u32)cp->blf & EH_FLAGS;
   CCB_PTR ncp;
   CCB_PTR pi = (CCB_PTR)eh_hvp[hn]->pi;
   CCB_PTR px = (CCB_PTR)eh_hvp[hn]->px;

   /* attempt to fix using cp->sz */
   if (!(flags & EH_INUSE) || flags & EH_DEBUG)
   {
      ncp = (CCB_PTR)((u32)cp + cp->sz);

      /* verify ncp is a valid chunk */
      if (ncp == px || (ncp->fl > pi && ncp->fl <= px))
      {
         cp->fl = ncp;
         eh_error(EH_HEAP_FIXED, 1, hn);
         return ncp;
      }
   }

   /* otherwise enable fix_fl_bs() to begin back scan */
   eh_hvp[hn]->mode.fl.hs_fwd = OFF;
   eh_hvp[hn]->hfp = px;
   return NULL;
}

/*
*  eh_error() -- Report error.
*/

static void eh_error(EH_ERRNO errnum, u32 level, u32 hn)
{
   if (eh_hvp[hn]->mode.fl.ed_en >= level);
   {
      eh_hvp[hn]->errno = errnum;
   }
}

/* Notes:
   1. memcpy() is assumed to work correctly when the new data block is
      overlapped by the current data block, as long as bp < cbp. Note that
      eh_Realloc() preserves the first 16-bytes of the data block.
   2. This may be the freed chunk if it is big enough, after merging with
      adjacent free chunks, or it may be another chunk that is a better fit.
      Copying the old data block to the new data block, if necessary, is done
      in eh_Malloc(), before chunk_split(), which may overwrite the old data
      block with CCB.
   3. In non-debug mode, these words are overwritten by the CCB and thus must
      be preserved before calling eh_Free(). They must be restored after
      eh_Malloc() and chunk_unfree(), else they will overwrite CCBs that they
      use.
   4. If NULL, indicates to chunk_unfree() that this chunk was not merged.
   5. Allocation searches skip dc if the resulting chunk would be too big for 
      the SBA. eh_Recover() must do the same or an infinite loop will result.
   6. Prevents combining free chunks after dc with dc.
   7. The CLZ instruction was not introduced until ARM Architecture v5. ARM7 
      and some ARM9 processors are Architecture v4.
   8. dc and tc must not be less than sizeof(CCB) = 24 bytes.
   9. cp is treated as a CICB_PTR after this point because aligned_srch() 
      overwrites bytes after the first 8.
   10. EH_SSP cannot be restored, so it is set to 0, which may result in lost
      spare space; setting EH_SSP to 1 might cause a heap error.
   11. The next larger occupied bin can be in SBA or UBA.
   12. Code can be added at the start of eh_Malloc() to auto-init the heap(s),
       especially any that are used by C++ initializers. Initializers are
       called after the compiler startup code does data init, before main(),
       so eh_Init() cannot be called by the assembly startup because then its
       initialized variables will be cleared by data init, and it cannot be
       called from main() because that's after the initializers. Some
       compilers, such as IAR, allow hooking into the startup routines.
       Otherwise, you can auto-init from the start of eh_Malloc(). Either check
       if heap hn has been initialized or check if (eh_hvpn == 0) to know that
       no heap has been initialized, and call a function to initialize them.
   13. Necessary only if prior chunk is not free. However that will usually be
       the case and testing this requires several instructions, so better to
       not test.
*/
