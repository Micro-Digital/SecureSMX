/*
* xblk.c                                                    Version 6.0.0
*
* smx Block Pool Functions
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
* Author: Ralph Moore
*
*****************************************************************************/

#include "xsmx.h"

/* internal subroutines */
static bool smx_BlockRel_F(BCB_PTR blk, u16 clrsz);

/* block pools */
BCB   bcb_pool[SMX_NUM_BLOCKS];
MCB   mcb_pool[SMX_NUM_MSGS];
PCB   pcb_pool[SMX_NUM_POOLS];

/*
*  smx_BlockGet()   SSR
*
*  Gets block from block pool and BCB from BCB pool, initializes the BCB,
*  loads block pointer into bpp, clears clrsz bytes of block, and returns
*  the block handle. If pool is invalid or out of BCBs, aborts and returns NULL.
*  If bpp == NULL, operation proceeds normally, but no block ptr is returned.
*
*  Notes:
*     1. For proper operation there must be at least as many BCBs as there
*        are active smx blocks in a system.
*     2. Interrupt safe wrt to sb_BlockGet() and sb_BlockRel() operating on
*        the same block pool.
*/
BCB_PTR smx_BlockGet(PCB_PTR pool, u8** bpp, u32 clrsz, BCB_PTR* bhp)
{
   BCB_PTR blk;
   u8     *bp;

   smx_SSR_ENTER4(SMX_ID_BLOCK_GET, pool, bpp, clrsz, bhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_BLOCK_GET, NULL);

   /* block multiple gets and verify current task has block get permission */
   if ((blk = (BCB_PTR)smx_ObjectCreateTestH((u32*)bhp)) && !smx_errno)
   {
      /* verify that pool is valid and that current task has access permission for pool */
      if (blk = (BCB_PTR)smx_PCBTest(pool, SMX_PRIV_LO))
      {
         blk = NULL;
         /* get block if pool not empty */
         if (pool->pn != NULL)
         {
            /* get BCB */
            if ((blk = (BCB_PTR)smx_bcbs.pn) == NULL)
               smx_ERROR_EXIT(SMXE_OUT_OF_BCBS, NULL, 0, SMX_ID_BLOCK_GET);
            smx_bcbs.pn = *(u8**)blk;

            /* get next block and update pcb.pn, with interrupts disabled */
            sb_INT_DISABLE();
            bp = pool->pn;
            pool->pn = *(u8**)bp;
            sb_INT_ENABLE();

            /* initialize BCB */
            blk->bp = bp;
            blk->ph = pool;
            blk->onr = (smx_clsr ? (TCB_PTR)smx_clsr : smx_ct);
            blk->bhp = bhp;

            /* clear clrsz bytes, up to block size */
            if (clrsz)
            {
               if (clrsz < pool->size)
                  memset(bp, 0, clrsz);
               else
                  memset(bp, 0, pool->size);
            }
            /* load block pointer and block handle */
            if (bpp)
               *bpp = bp;
            if (bhp)
               *bhp = blk;
         }
      }
   }
   return((BCB_PTR)smx_SSRExit((u32)blk, SMX_ID_BLOCK_GET));
}

/*
*  smx_BlockPeek()   SSR
*/
u32 smx_BlockPeek(BCB_PTR blk, SMX_PK_PAR par)
{
   u32 val;

   smx_SSR_ENTER2(SMX_ID_BLOCK_PEEK, blk, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_BLOCK_PEEK, 0);

   /* verify that blk is valid and that current task has access permission for blk */
   if (val = smx_BCBTest(blk, SMX_PRIV_LO))
   {
      val = 0;
      switch (par)
      {
         case SMX_PK_BP:
            val = (u32)blk->bp;
            break;
         case SMX_PK_NEXT:
            val = (u32)(blk->onr == 0 ? 0 : blk->bp);
            break;
         case SMX_PK_ONR:
            val = (u32)blk->onr;
            break;
         case SMX_PK_POOL:
            val = (u32)blk->ph;
            break;
         case SMX_PK_SIZE:
            val = (u32)blk->ph->size;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_BLOCK_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_BLOCK_PEEK);
}

/*
*  smx_BlockRel()   SSR
*
*  Releases an smx block obtained by smx_BlockGet(). Releases block and BCB
*  back to their pools. Clears clrsz bytes up to the end of the block. Returns
*  true if successful. Fails and returns false if blk handle is not valid or
*  block has already been released.
*/
bool smx_BlockRel(BCB_PTR blk, u16 clrsz)
{
   bool pass;

   smx_SSR_ENTER2(SMX_ID_BLOCK_REL, blk, clrsz);
   smx_EXIT_IF_IN_ISR(SMX_ID_BLOCK_REL, false);

   /* verify that blk is valid and that current task has access permission for blk */
   if (pass = smx_BCBTest(blk, SMX_PRIV_HI))
   {
      if (!(blk->onr))
         smx_ERROR_EXIT(SMXE_INV_BCB, false, 0, SMX_ID_BLOCK_REL);
      pass = smx_BlockRel_F(blk, clrsz);
   }
   return((bool)smx_SSRExit(pass, SMX_ID_BLOCK_REL));
}

/*
*  smx_BlockRelAll()   SSR
*
*  Releases all blocks belonging to task and returns number released.
*/
u32 smx_BlockRelAll(TCB_PTR task)
{
   u32   n;

   smx_SSR_ENTER1(SMX_ID_BLOCK_REL_ALL, task);
   smx_EXIT_IF_IN_ISR(SMX_ID_BLOCK_REL_ALL, 0);

   /* verify that task is valid and that current task has access permission */
   if (n = smx_TCBTest(task, SMX_PRIV_HI))
   {
      smx_TASK_OP_PERMIT_VAL(task, SMX_ID_BLOCK_REL_ALL);
      n = smx_BlockRelAll_F(task);
   }
   return(smx_SSRExit(n, SMX_ID_BLOCK_REL_ALL));
}

/*
*  smx_BlockMake()   SSR
*
*  Makes an smx block from a base block or a bare block. Gets BCB from BCB
*  pool, initializes it and returns its handle. The pool pointer is stored in
*  the BCB.
*/
BCB_PTR smx_BlockMake(PCB_PTR pool, u8* bp, BCB_PTR* bhp)
{
   BCB_PTR blk;

   smx_SSR_ENTER3(SMX_ID_BLOCK_MAKE, pool, bp, bhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_BLOCK_MAKE, NULL);

   /* block multiple makes and verify current task has block make permission */
   if ((blk = (BCB_PTR)smx_ObjectCreateTestH((u32*)bhp)) && !smx_errno)
   {
      /* test bp parameter */
      if (bp == NULL)
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_BLOCK_MAKE);

      if (pool)
         if (bp < pool->pi || bp >= pool->px + pool->size)
            smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_BLOCK_MAKE);

      /* get BCB */
      if ((blk = (BCB_PTR)smx_bcbs.pn) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_BCBS, NULL, 0, SMX_ID_BLOCK_MAKE);

      /* remove BCB from BCB pool and initialize it */
      smx_bcbs.pn = *(u8**)blk;
      blk->bp = bp;
      blk->ph = pool;
      blk->onr = (smx_clsr ? (TCB_PTR)smx_clsr : smx_ct);
      blk->bhp = bhp;

      /* load block handle */
      if (bhp)
         *bhp = blk;
   }
   return((BCB_PTR)smx_SSRExit((u32)blk, SMX_ID_BLOCK_MAKE));
}

/*
*  smx_BlockUnmake()   SSR
*
*  Reverses smx_BlockMake() by converting an smx block to a base block or a
*  bare block by releasing its BCB. Aborts with error and returns NULL, if
*  the BCB is invalid. Otherwise, returns the block's data pointer and loads
*  its pool handle, if any, into user-supplied pool handle.
*/
u8* smx_BlockUnmake(PCB_PTR* php, BCB_PTR blk)
{
   BCB_PTR* bhp;
   u8*      bp;

   smx_SSR_ENTER2(SMX_ID_BLOCK_UNMAKE, php, blk);
   smx_EXIT_IF_IN_ISR(SMX_ID_BLOCK_UNMAKE, NULL);

   /* verify that blk is valid and that current task has unmake permission for blk */
   if (bp = (u8*)smx_BCBTest(blk, SMX_PRIV_HI))
   {
      if (!(blk->onr))
         smx_ERROR_EXIT(SMXE_INV_BCB, NULL, 0, SMX_ID_BLOCK_UNMAKE);

      bp = blk->bp;
      if (php)
         *php = blk->ph;

      /* release BCB to the BCB pool and clear it and its handle */
      bhp = blk->bhp;
      if (sb_BlockRel(&smx_bcbs, (u8*)blk, sizeof(BCB)))
         if (bhp)
            *bhp = NULL;
   }
   return((u8*)smx_SSRExit((u32)bp, SMX_ID_BLOCK_UNMAKE));
}

/*
*  smx_BlockPoolCreate()   SSR
*
*  Gets PCB for block pool and creates block pool @ bp. Returns block pool 
*  handle or NULL if fail. 
*/
PCB_PTR smx_BlockPoolCreate(u8* bp, u16 num, u16 size, const char* name, PCB_PTR* php)
{
   PCB_PTR  pool;

   smx_SSR_ENTER5(SMX_ID_BP_CREATE, bp, num, size, name, php);
   smx_EXIT_IF_IN_ISR(SMX_ID_BP_CREATE, NULL);

   /* block multiple creates and verify current task has create permission */
   if ((pool = (PCB_PTR)smx_ObjectCreateTestH((u32*)php)) && !smx_errno)
   {
      /* test for valid parameters */
      if (bp == 0 || (u32)bp % 4 || num == 0 || size == 0 || size % 4)
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_BP_CREATE);

      /* get PCB and handle for this block pool */
      if ((pool = (PCB_PTR)sb_BlockGet(&smx_pcbs, 4)) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_PCBS, NULL, 0, SMX_ID_BP_CREATE);

      /* create block pool and return handle */
      if (!sb_BlockPoolCreate(bp, pool, num, size, name))
         smx_ERROR_EXIT(SMXE_INV_PAR, NULL, 0, SMX_ID_BP_CREATE);
      pool->php = php;

      /* load pool handle */
      if (php)
         *php = pool;
   }
   return((PCB_PTR)smx_SSRExit((u32)pool, SMX_ID_BP_CREATE));
}

/*
*  smx_BlockPoolDelete()   SSR
*
*  Deletes a block pool created by smx_BlockPoolCreate(). Aborts if pool is 
*  invalid or one or more blocks are still in use. Otherwise deletes pool
*  name from handle table, releases and clears PCB, sets pool handle = 
*  NULL, and returns a pointer to the start of the released pool block.
*
*  Note: User is responsible for dealing with pool block. If it came from
*  the heap it can be freed, but otherwise it must be reused.
*/
u8* smx_BlockPoolDelete(PCB_PTR* php)
{
   PCB_PTR pool = (php ? *php : NULL);
   u32 bctr;
   u32 *bp;

   smx_SSR_ENTER1(SMX_ID_BP_DELETE, pool);  /* record actual handle */
   smx_EXIT_IF_IN_ISR(SMX_ID_BP_DELETE, NULL);

   /* verify that pool is valid and that current task has access permission for pool */
   if (bp = (u32*)smx_PCBTest(pool, SMX_PRIV_HI))
   {
      /* report error and exit if any blocks are in use */
      for (bctr = pool->num, bp = (u32*)pool->pn; bctr && bp; bctr--)
         bp = (u32*)(*bp);
      if (bctr > 0)
         smx_ERROR_EXIT(SMXE_BLK_IN_USE, NULL, 0, SMX_ID_BP_DELETE);

      /* get pointer to start of pool, clear and release PCB, and set pool handle to nullcb */
      bp = (u32*)pool->pi;
      sb_BlockRel(&smx_pcbs, (u8*)pool, sizeof(PCB));
      *php = NULL;
   }
   return((u8*)smx_SSRExit((u32)bp, SMX_ID_BP_DELETE));
}

/*
*  smx_BlockPoolPeek()   SSR
*/
u32 smx_BlockPoolPeek(PCB_PTR pool, SMX_PK_PAR par)
{
   u32 val;

   smx_SSR_ENTER2(SMX_ID_BP_PEEK, pool, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_BP_PEEK, 0);

   /* verify that pool is valid and that current task has access permission for pool */
   if (val = smx_PCBTest(pool, SMX_PRIV_LO))
   {
      if ((val = sb_BlockPoolPeek(pool, (SB_PK_PAR)par)) == 0)
         smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_BP_PEEK);
   }
   return (u32)smx_SSRExit(val, SMX_ID_BP_PEEK);
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            * 
*===========================================================================*/

/*
*  smx_BlockRel_F()
*
*  If pool type is SB_CB_PCB, releases block back to the pool it came from,
*  and clears clrsz bytes up to the end of the block. Then releases BCB back
*  to its pool and clears it. Returns true unless BCB's pool or bp fields are
*  invalid. Shared between smx_BlockRel() and smx_BlockRelAll().
*
*  NOTE: Interrupt safe wrt to sb_BlockGet() and sb_BlockRel() operating on
*  the same block pool.
*/
bool smx_BlockRel_F(BCB_PTR blk, u16 clrsz)
{
   BCB_PTR* bhp;
   bool     pass = true;
   PCB_PTR  pool;

   pool = blk->ph;

   /* release block to its pool and clear, if pool is non-zero and valid */
   if (pool != 0 && pool->cbtype == SB_CB_PCB)
      pass = sb_BlockRel(pool, blk->bp, clrsz);

   /* release BCB to its pool and clear it and its handle */
   bhp = blk->bhp;
   pass &= sb_BlockRel(&smx_bcbs, (u8*)blk, sizeof(BCB));
   if (pass)
   {
      if (bhp)
         *bhp = NULL;
      return true;
   }
   else
      smx_ERROR_RET(SMXE_INV_PAR, false, 0);
}

/*
*  smx_BlockRelAll_F()
*
*  Search entire smx_bcbs.pi to smx_bcbs.px and release all blocks belonging to
*  this task. Returns number of blocks released.
*/
u32 smx_BlockRelAll_F(TCB_PTR task)
{
   BCB_PTR  blk;
   int      n = 0;

   if (smx_bcbs.pi)
      for (blk = (BCB_PTR)smx_bcbs.px; blk >= (BCB_PTR)smx_bcbs.pi; blk--)
         if (blk->onr == task)
         {
            smx_BlockRel_F(blk, 0);
            n++;
         }
   return n;
}