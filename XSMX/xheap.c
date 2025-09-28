/*
* xheap.c                                                   Version 5.4.0
*
* smx Heap Functions
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

/* internal subroutines */
static bool smx_HeapEnter(u32 p1, u32 hn, u32 id);
static bool smx_HeapEnter(u32 p1, u32 p2, u32 hn, u32 id);
static bool smx_HeapEnter(u32 p1, u32 p2, u32 p3, u32 hn, u32 id);
static void smx_HeapExit(u32 rv, u32 hn, u32 id);

/* internal variables */
bool        smx_hmng;   /* run HeapManager */
u32         smx_htmo;   /* heap mutex timeout */

#if SB_CFG_TM
u32         smx_htm;    /* heap time measurement */
u32         smx_ts1;    /* time start 1 */
#endif

MUCB_PTR  smx_hmtx[EH_NUM_HEAPS];  /* heap mutex pointer array */

/* eheap error to smx error mapping table */
const u32 xerrno[] = {SMXE_OK, 
                      SMXE_HEAP_ALRDY_INIT, 
                      SMXE_HEAP_BRKN,
                      SMXE_HEAP_FIXED,
                      SMXE_HEAP_ERROR,
                      SMXE_HEAP_FENCE_BRKN,
                      SMXE_INSUFF_HEAP,
                      SMXE_INV_CCB,
                      SMXE_INV_PAR,
                      SMXE_HEAP_RECOVER,
                      SMXE_TOO_MANY_HEAPS,
                      SMXE_WRONG_HEAP,
                      };

/* heap mutex access names */
#define ACO_NAME_LEN  9  /* length of names including NUL */
#define ACO_NAME_MAX  6

/* heap names */
const char aco_names[] = "mheapmtx\0heap1mtx\0heap2mtx\0heap3mtx\0heap4mtx\0heap5mtx\0";

#if EH_NUM_HEAPS > ACO_NAME_MAX
#error Define more heap mutex names here in xheap.c.
#endif

/*
*  smx_HeapBinPeek()   Mutex-Protected Service
*
*  Returns information from the heap bin.
*/
u32 smx_HeapBinPeek(u32 binno, EH_PK_PAR par, u32 hn)
{
   u32 val;
   if (!smx_HeapEnter(binno, par, hn, SMX_ID_HEAP_BIN_PEEK))
      return 0;
   val = eh_BinPeek(binno, par, hn);
   if (eh_hvp[hn]->errno != 0)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(val, hn, SMX_ID_HEAP_BIN_PEEK);
   return val;
}

/*
*  smx_HeapBinScan()   Mutex-Protected Service
*
*  Scans the heap bin for errors, sorts it, and makes fixes when it can.
*/
bool smx_HeapBinScan(u32 binno, u32 fnum, u32 bnum, u32 hn)
{
   bool pass;
   if (!smx_HeapEnter(binno, fnum, bnum, hn, SMX_ID_HEAP_BIN_SCAN))
      return false;
   pass = eh_BinScan(binno, fnum, bnum, hn);
   if (eh_hvp[hn]->errno != 0)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(pass, hn, SMX_ID_HEAP_BIN_SCAN);
   return pass;
}
 
/*
*  smx_HeapBinSeed()   Mutex-Protected Service
*
*  Gets a big enough chunk to divide into num chunks for blocks of size bsz and
*  puts them into the correct bin for their size.
*/
bool smx_HeapBinSeed(u32 num, u32 bsz, u32 hn)
{
   bool pass;
   if (!smx_HeapEnter(num, bsz, hn, SMX_ID_HEAP_BIN_SEED))
      return false;
   pass = eh_BinSeed(num, bsz, hn);
   if (eh_hvp[hn]->errno != 0)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(pass, hn, SMX_ID_HEAP_BIN_SEED);
   return pass;
}

/*
*  smx_HeapBinSort()   Mutex-Protected Service
*
*  Sorts the heap bin free list by increasing chunk size.
*/
bool smx_HeapBinSort(u32 binno, u32 fnum, u32 hn)
{
   bool epass;   /* end pass detected */
   if (!smx_HeapEnter(binno, fnum, hn, SMX_ID_HEAP_BIN_SORT))
      return false;
   epass = eh_BinSort(binno, fnum, hn);
   if (eh_hvp[hn]->errno != 0)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(epass, hn, SMX_ID_HEAP_BIN_SORT);
   return epass;
}

/*
*  smx_HeapCalloc()   Mutex-Protected Service
*
*  Allocates a block from the heap of num*sz bytes with an alignment, and 
*  clears it.
*/
void* smx_HeapCalloc(u32 num, u32 sz, u32 an, u32 hn)
{
   void* bp;  
   if (!smx_HeapEnter(num, sz, an, hn, SMX_ID_HEAP_CALLOC))
      return NULL;
   bp = eh_Calloc(num, sz, an, hn);
   smx_HeapExit((u32)bp, hn, SMX_ID_HEAP_CALLOC);
   return bp;
}

/*
*  smx_HeapChunkPeek()   Mutex-Protected Service
*
*  Returns information concerning a chunk, given a pointer to either it or to 
*  the block inside of it.
*/
u32 smx_HeapChunkPeek(void* vp, EH_PK_PAR par, u32 hn)
{
   u32 val;
   if (!smx_HeapEnter((u32)vp, par, hn, SMX_ID_HEAP_CHUNK_PEEK))
      return 0;
   val = eh_ChunkPeek(vp, par, hn);
   if (eh_hvp[hn]->errno != 0)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(val, hn, SMX_ID_HEAP_CHUNK_PEEK);
   return val;
}

/*
*  smx_HeapExtend()   Mutex-Protected Service
*
*  Adds memory extension to heap. Handles both the case where the extension is
*  adjacent to the current heap and where there is a gap.
*/
bool smx_HeapExtend(u32 xsz, u8* xp, u32 hn)
{
   bool pass;
   if (!smx_HeapEnter(xsz, (u32)xp, hn, SMX_ID_HEAP_EXTEND))
      return false;
   pass = eh_Extend(xsz, xp, hn);
   if (eh_hvp[hn]->errno != EH_OK)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(pass, hn, SMX_ID_HEAP_EXTEND);
   return pass;
}

/*
*  smx_HeapFree()   Mutex-Protected Service
*
*  Frees a block previously allocated from the heap.
*/
bool smx_HeapFree(void* bp, u32 hn) 
{
   bool pass;
   if (!smx_HeapEnter((u32)bp, hn, SMX_ID_HEAP_FREE))
      return false;
   pass = eh_Free(bp, hn);
   if (eh_hvp[hn]->errno != 0 && eh_hvp[hn]->mode.fl.em_en)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(pass, hn, SMX_ID_HEAP_FREE);
   return pass;
}

/*
*  smx_HeapInit()   Function
*
*  Intializes heap starting at hp and ending at hp + sz - 4.
*  Notes: mtx == NULL and reinitializing a preeemptible heap to be a
*  non-preemptible heap are not supported. hn == -1 if fail
*/
u32 smx_HeapInit(u32 sz, u32 dcsz, u8* hp, EHV_PTR vp, u32* bszap, HBCB* binp, 
                                                   u32 mode, const char* name)
{
   u32 hn;
   smx_ct->err = SMXE_OK;

  #if SMX_CFG_EVB
   smx_EVBLogSSR7(SMX_ID_HEAP_INIT, sz, dcsz, (u32)hp, (u32)vp, (u32)bszap, (u32)binp, mode);
  #endif

   hn = eh_Init(sz, dcsz, hp, vp, bszap, binp, mode, name);

   if (vp->errno != EH_OK)
   {
      smx_ERROR((SMX_ERRNO)xerrno[vp->errno], 0);
   }
   else
   {
      if (vp->mode.fl.pre) /* if preemption protected */
      {
         if (vp->mtx == NULL)
         {
            /* create hmtx if not already done */
            #if defined(SMX_TSMX) /* without priority promotion */
            vp->mtx = smx_MutexCreate(0, 0, aco_names+(hn*ACO_NAME_LEN));
            #else /* with priority promotion */
            vp->mtx = smx_MutexCreate(1, 0, aco_names+(hn*ACO_NAME_LEN));
            #endif
         }
         if (vp->mtx == NULL)
            hn = -1;
      }
      else
      {
         vp->mtx = NULL;
      }
      if (hn != -1)
         smx_hmtx[hn] = (MUCB_PTR)vp->mtx; /* for hmtx pretest shells and debug */
   }

  #if SMX_CFG_EVB
   smx_EVBLogSSRRet(hn, SMX_ID_HEAP_INIT);
  #endif
   return hn;
}

/*
*  smx_HeapMalloc()   Mutex-Protected Service
*
*  Allocates a block of sz bytes, aligned on 2^an, from heap hn.
*  Note: To request a v7M region search, add the EH_R flag to an.
*/
void* smx_HeapMalloc(u32 sz, u32 an, u32 hn)
{
   void* bp;
   if (!smx_HeapEnter(sz, an, hn, SMX_ID_HEAP_MALLOC))
      return NULL;
   bp = eh_Malloc(sz, an, hn);
   if (eh_hvp[hn]->errno != 0 && eh_hvp[hn]->mode.fl.em_en)
   {
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   }
   smx_HeapExit((u32)bp, hn, SMX_ID_HEAP_MALLOC);
   return bp;
}

/*
*  smx_HeapPeek()   Mutex-Protected Service
*
*  Returns information concerning the heap.
*/
u32 smx_HeapPeek(EH_PK_PAR par, u32 hn)
{
   u32 val;
   if (!smx_HeapEnter(par, hn, SMX_ID_HEAP_PEEK))
      return 0;
   val = eh_Peek(par, hn);
   if (eh_hvp[hn]->errno != 0)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(val,hn, SMX_ID_HEAP_PEEK);
   return val;
}

/*
*  smx_HeapRealloc()   Mutex-Protected Service
*
*  Allocates a new size/aligned heap block from an existing heap block, 
*  if possible, or gets a new block, copies the old block, and frees it.
*/
void* smx_HeapRealloc(void* cbp, u32 sz, u32 an, u32 hn)
{
   void* nbp;
   if (!smx_HeapEnter((u32)cbp, sz, an, hn, SMX_ID_HEAP_REALLOC))
      return NULL;
   nbp = eh_Realloc(cbp, sz, an, hn);
   if (eh_hvp[hn]->errno != 0 && eh_hvp[hn]->mode.fl.em_en)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit((u32)nbp, hn, SMX_ID_HEAP_REALLOC);
   return nbp;
}

/*
*  smx_HeapRecover()   Mutex-Protected Service
*
*  Scans the heap for adjacent free chunks that can be merged. If a large-enough
*  chunk can be made, merges them and puts result into a bin.
*/
bool smx_HeapRecover(u32 sz, u32 num, u32 an, u32 hn)
{
   bool pass;
   if (!smx_HeapEnter(sz, num, an, hn, SMX_ID_HEAP_RECOVER))
      return NULL;
   pass = eh_Recover(sz, num, an, hn);
   if (eh_hvp[hn]->errno != 0 && eh_hvp[hn]->mode.fl.em_en)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit((u32)pass, hn, SMX_ID_HEAP_RECOVER);
   return pass;
}

/*
*  smx_HeapScan()   Mutex-Protected Service
*
*  Scans the heap for errors and makes fixes when it can.
*/
bool smx_HeapScan(CCB_PTR cp, u32 fnum, u32 bnum, u32 hn)
{
   bool pass;
   if (!smx_HeapEnter((u32)cp, fnum, bnum, hn, SMX_ID_HEAP_SCAN))
      return true;
   pass = eh_Scan(cp, fnum, bnum, hn);
   if (eh_hvp[hn]->errno != 0)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(pass, hn, SMX_ID_HEAP_SCAN);
   return pass;
}

/*
*  smx_HeapSet()   Mutex-Protected Service
*
*  Sets the specified heap parameter to the specified value.
*  Not permitted in umode.
*/
bool smx_HeapSet(EH_ST_PAR par, u32 val, u32 hn)
{
   bool pass;
   if (!smx_HeapEnter(par, val, hn, SMX_ID_HEAP_SET))
      return false;
   #if SMX_CFG_SSMX
   /* a utask cannot make heap changes */
   if (smx_ct->flags.umode)
      smx_ERROR_EXIT(SMXE_PRIV_VIOL, false, 0, SMX_ID_HEAP_SET);
   #endif
   pass = eh_Set(par, val, hn);
   if (eh_hvp[hn]->errno != 0)
      smx_ERROR((SMX_ERRNO)xerrno[eh_hvp[hn]->errno], 0);
   smx_HeapExit(pass, hn, SMX_ID_HEAP_SET);
   return pass;
}

/*===========================================================================*
                             CALLBACK FUNCTIONS
                            Do Not Call Directly
*===========================================================================*/

u32 eh_onr(void)
{
   return (smx_clsr != NULL ? (u32)smx_clsr : (u32)smx_ct);
}

u32 eh_time(void)
{
   return smx_etime;
}

/*===========================================================================*
*                         HEAP TRANSLATION ROUTINES                          *
*===========================================================================*/

#if !defined(size_t)
#include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if (__MULTIPLE_HEAPS__)
void *__data_calloc(size_t nitems, size_t size)
#else
void *calloc(size_t nitems, size_t size)
#endif
{
   return(smx_HeapCalloc(nitems, size));
}

#if (__MULTIPLE_HEAPS__)
void *__data_malloc(size_t size)
#else
void *malloc(size_t size)
#endif
{
   return(smx_HeapMalloc(size));
}

#if (__MULTIPLE_HEAPS__)
void *__data_realloc(void *bp, size_t size)
#else
void *realloc(void *bp, size_t size)
#endif
{
   return(smx_HeapRealloc(bp, size));
}

#if (__MULTIPLE_HEAPS__)
void __data_free(void *bp)
#else
void free(void *bp)
#endif
{
   smx_HeapFree(bp);
}

#ifdef __cplusplus
}
#endif
#ifdef __cplusplus

/*----------------------- NEW AND DELETE OPERATORS ------------------------*/

/* These are necessary to ensure that uses of new() and delete() are mapped
   to smx heap calls rather than using the C run time library routines.
   Compilers translate these into malloc() and free(), which we have handled
   in the translation routines above, but there may be other functionality,
   such as exception handling, that is also done that would be a problem if
   the C RTL routines were used.

   set_new_handler() is used to specify a routine to run if new() fails.
   new_handler() could be implemented to try to free up some memory or
   allocate it somewhere else. Or it can return 0, in which case the new()
   call will fail (the resulting object pointer will be NULL), and the
   application code can determine an appropriate action.

   Author: Ken Booth
*/

static PNHX _new_handler = NULL;

PNHX set_new_handler(PNHX a_new_handler)
{
   PNHX old_pnh = _new_handler;
   _new_handler = a_new_handler;
   return old_pnh;
}

void * newx(size_t s)
{
   void * p;

   while ((p = smx_HeapMalloc(s)) == NULL)
      if (_new_handler == NULL || !_new_handler(s))
         break;

   return p;
}

void * operator new[](size_t s)
{
   return newx(s);
}

void * operator new(size_t s)
{
   return newx(s);
}

void operator delete(void * b)
{
   smx_HeapFree(b);
}
#endif

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

static bool smx_HeapEnter(u32 p1, u32 hn, u32 id)
{
   MUCB_PTR hmtx;

   /* verify hn is ok before using it */
   if (hn >= eh_hvpn)
   {
      smx_ERROR(SMXE_INV_PAR, 0);
      return false;
   }

   hmtx = (MUCB_PTR)eh_hvp[hn]->mtx;

   /* attempt to gain heap access */
   if ((hmtx != NULL) && ((hmtx->onr != smx_ct) || smx_clsr)) /*<1>*/
   {
      if (!smx_MutexGetFast(hmtx, smx_htmo))
         return false;
   }

   smx_ct->err = SMXE_OK;

  #if SMX_CFG_EVB
   /* log start of heap operation */
   smx_EVBLogSSR2(id, p1, hn);
  #endif

   return true;
}

static bool smx_HeapEnter(u32 p1, u32 p2, u32 hn, u32 id)
{
   MUCB_PTR hmtx;

   /* verify hn is ok before using it */
   if (hn >= eh_hvpn)
   {
      smx_ERROR(SMXE_INV_PAR, 0);
      return false;
   }

   hmtx = (MUCB_PTR)eh_hvp[hn]->mtx;

   /* attempt to gain heap access */
   if ((hmtx != NULL) && ((hmtx->onr != smx_ct) || smx_clsr)) /*<1>*/
   {
      if (!smx_MutexGetFast(hmtx, smx_htmo))
         return false;
   }

   smx_ct->err = SMXE_OK;

  #if SMX_CFG_EVB
   /* log start of heap operation */
   smx_EVBLogSSR3(id, p1, p2, hn);
  #endif

   return true;
}

static bool smx_HeapEnter(u32 p1, u32 p2, u32 p3, u32 hn, u32 id)
{
   MUCB_PTR hmtx;

   /* verify hn is ok before using it */
   if (hn >= eh_hvpn)
   {
      smx_ERROR(SMXE_INV_PAR, 0);
      return false;
   }

   hmtx = (MUCB_PTR)eh_hvp[hn]->mtx;

   /* attempt to gain heap access */
   if ((hmtx != NULL) && ((hmtx->onr != smx_ct) || smx_clsr)) /*<1>*/
   {
      if (!smx_MutexGetFast(hmtx, smx_htmo))
         return false;
   }

   smx_ct->err = SMXE_OK;

  #if SMX_CFG_EVB
   /* log start of heap operation */
   smx_EVBLogSSR4(id, p1, p2, p3, hn);
  #endif

   return true;
}

static void smx_HeapExit(u32 rv, u32 hn, u32 id)
{
   if (eh_hvp[hn]->mtx != NULL)
      smx_MutexRelFast((MUCB_PTR)eh_hvp[hn]->mtx);

   #if SMX_CFG_EVB
   smx_EVB_LOG_SSR_RET(rv, id);
   #endif
}

/* Notes:
   1. This covers both a call from an SSR and from the SVC handler. This also 
      allows LSR access if the hn mutex is free. If preemption is not allowed
      for the heap (hpv[n]->mode.fl.eh_pre == 0), hmtx == NULL.
   2. smx_HeapEnter() can be called twice if first called from umode, then
      the SSR is called from pmode.
*/
