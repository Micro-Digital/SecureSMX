/*
* bbase.c                                                   Version 6.0.0
*
* smxBase Functions.
*
* Copyright (c) 2005-2026 Micro Digital Inc.
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
* Authors: Ralph Moore, David Moore, Jianchi Chen
*
*****************************************************************************/

#include "bbase.h"
#include "xsmx.h"

/*============================================================================
                                VARIABLES
============================================================================*/

u8*         sb_isp;           /* interrupt stack pointer */

#if SB_CFG_TM
u32         sb_te[4];         /* ending times for time measurements */
u32         sb_ts1;           /* starting time for time measurement 1 */
u32         sb_ts2;           /* starting time for time measurement 2 */
u32         sb_TMCal;         /* TM calibration for pmode */
u32         sb_TMs;
u32         sb_tltsel = 0;    /* LSR type selector for sb_TMLsr() */
#if SMX_CFG_SSMX
/*............................. SECTION CHANGE .............................*/
#pragma default_variable_attributes = @ ".ucom.bss"
u32         sbu_TMCal;        /* TM calibration for umode */
u32         sbu_TMs;
/*............................. SECTION CHANGE .............................*/
#pragma default_variable_attributes =
#endif /* SMX_CFG_SSMX */
#endif /* SB_CFG_TM */

/*===========================================================================*
*                               DAR FUNCTIONS                                *
*===========================================================================*/

/* Allocate an aligned block from dar. sz is size in bytes. A minimum alignment
   of 4 (align >= 4) is required and align must be a power of 2 and not greater
   than 16 KB. sz should be a word multiple; if not, it will be rounded up.
*/
u8 *sb_DARAlloc(SB_DCB_PTR dar, u32 sz, u32 align)
{
   u8 *p;
   u32 a, x;

   if (sz < 4)
      smx_ERROR_RET(SBE_INV_SIZE, NULL, 0);

   if (dar == NULL || dar->pn < dar->pi || dar->pn > dar->px || dar->px == 0)
      smx_ERROR_RET(SBE_INV_DAR, NULL, 0);

   if (align < 4 || align % 4 || align > 0x4000)
   {
      smx_ERROR_RET(SBE_INV_ALIGN, NULL, 0);
   }
   else
   {
      for (a = align/4, x = 1; a > 0; a /= 2)
         x = x*2 + 1;
   }

   p = (u8 *)(((u32)dar->pn + x) & ~x); /* align p up */
   sz = (sz + 3) & 0xFFFFFFFC; /* sz = word multiple */

   if ((p + sz) <= dar->px && (p + sz) > p) /* 2nd test is for 4GB overflow */
   {
      dar->pl = dar->pn;
      dar->pn = p + sz;
      return p;
   }
   else
   {
      smx_ERROR_RET(SBE_INSUFF_DAR, NULL, 0);
   }
}

/* Free and refill the last block allocated from dar 
*/
bool sb_DARFreeLast(SB_DCB_PTR dar)
{
   u32 *p;
   if (dar == NULL || dar->pl < dar->pi || dar->pl > dar->px || dar->pl >= dar->pn)
      smx_ERROR_RET(SBE_INV_DAR, false, 0);

   for (p = (u32 *)dar->pl; p <= (u32 *)dar->pn; p++)  /* fill words from pl to pn */
      *p = *(u32 *)dar->px;
   dar->pn = dar->pl; /* reset pn */
   return true;
}

/* Initialize DAR control block and fill DAR with fill pattern. sz is in bytes.
   DAR pointers are word-aligned for efficiency. If pi is misaligned or sz
   is not a word multiple, the DAR size will be slightly smaller than sz, to
   avoid overflowing into whatever follows. 
*/
bool sb_DARInit(SB_DCB_PTR dar, u8* pi, u32 sz, bool fill, u32 fillval)
{
   u32 *p;
   u32 i;

   if (dar == NULL || pi == NULL || sz < 4)
      return false;

   dar->pi = (u8 *)(((u32)pi + 3) & 0xFFFFFFFC);       /* -> first word of dar */
   dar->px = (u8 *)(((u32)pi + sz) & 0xFFFFFFFC) - 4;  /* -> last word of dar */
   dar->pn = dar->pi;
   dar->pl = dar->pi;

   if (fill)
   {
      i = (dar->px - dar->pi)/4 + 1;
      for (p = (u32 *)dar->pi; i != 0; i--, p++)  /* fill words from pi to px */
         *p = fillval;
   }

   return true;
}

/*===========================================================================*
*                         BASE BLOCK POOL FUNCTIONS                          *
*===========================================================================*/

/*
*  Block Pool Create
*
*  Creates a block pool consisting of num blocks of size bytes starting at pp.
*  Fails if parameters are not correct. Initializes the static PCB for this 
*  pool, clears the pool space, then links all blocks to pool->pn such that the
*  first word of each block contains a pointer to the next block and the last 
*  block contains NULL. Returns true if successful and false if not.
*
*  Notes:
*  1. Not interrupt safe -- DO NOT USE FROM ISRs.
*  2. ph must point to a valid PCB. This is not checked.
*  3. size must be at least one word.
*/
bool sb_BlockPoolCreate(u8* pp, PCB_PTR ph, u16 num, u16 size, const char* name)
{
   u32   i;
   u8   *bp;

   /* check parameters */
   if ((u32)pp % 4 || pp == NULL || ph == NULL || num == 0)
      smx_ERROR_RET(SBE_INV_BP, false, 0);
   if (size < 4 || size % 4)
      smx_ERROR_RET(SBE_INV_SIZE, false, 0);

   /* load PCB fields */
   ph->cbtype   = SB_CB_PCB;
   ph->num      = num;
   ph->num_used = 0;
   ph->size     = size;
   ph->pi = pp;
   ph->pn = pp;
   ph->px = pp + (num-1) * size;

   /* store name, if any */
   if (name && *name)
      ph->name = name;

   /* clear and link blocks -- leaves 0 ptr in last block and 0's in pads, if any */
   memset(pp, 0, num * size);
   for (i = 0, bp = pp; i < num - 1; i++)
   {
      *(u32*)bp = (u32)(bp + size);
      bp += size;
   }
   return true;
}

/*
*  Block Pool Create from DAR
*
*  Creates a block pool consisting of num blocks of size bytes from a DAR.
*  Fails if align parameter not a multiple of 4. If DAR space allocated,
*  calls sb_BlockPoolCreate(), which checks other parameters. If pool create
*  fails, returns DAR space. Returns true if successful and false if not.
*
*  Note: Not interrupt safe -- DO NOT USE FROM ISRs.
*/
bool sb_BlockPoolCreateDAR(SB_DCB* dar, PCB_PTR pool, u16 num, u16 size,
                                                   u16 align, const char* name)
{
   u8  *p;

   /* check parameters */
   if (size < 4 || size % 4)
      smx_ERROR_RET(SBE_INV_SIZE, false, 0);
   if (align%4)
      smx_ERROR_RET(SBE_INV_ALIGN, false, 0);

   /* get space from DAR for block pool */
   if ((p = sb_DARAlloc(dar, num*size, align)) == NULL)
      return false;

   /* create block pool */
   if (sb_BlockPoolCreate(p, pool, num, size, name))
      return true;

   /* return DAR space if create block pool fails */
   sb_DARFreeLast(dar); /* put DAR block back */
      return false;
}

/*
*  Block Pool Delete
*
*  Deletes a block pool created by sb_BlockPoolCreate(). Checks for valid
*  pool, then clears PCB and returns pointer to the start of pool space.
*
*  Notes:
*  1. Not interrupt safe -- DO NOT USE FROM ISRs.
*  2. Do not use for pools created by sb_BlockPoolCreateD().
*  3. Up to user to release or reuse pool space.
*/
u8* sb_BlockPoolDelete(PCB_PTR pool)
{
   u8  *p;

   /* check for valid pool */
   if (pool->cbtype != SB_CB_PCB)
      smx_ERROR_RET(SBE_INV_POOL, NULL, 0);

   /* clear PCB and return pointer to start of pool */
   p = pool->pi;
   memset(pool, 0, sizeof(PCB));
   return p;
}

/*
* Block Get
*
* Gets a block from pool, and clears the first clrsz bytes, up to the end of
* the block. Returns a pointer to the block, if successful, and NULL, if not.
*
* Notes:
*  1. Interrupt safe -- CAN BE USED IN ISRs.
*  2. Will not clear beyond end of block.
*  3. Always test that a valid block pointer has been returned before using it.
*/
u8* sb_BlockGet(PCB_PTR pool, u16 clrsz)
{
   u32   is;   /* interrupt state */
   u8   *bp;   /* block pointer */

   /* check for valid pool */
   if (pool->cbtype != SB_CB_PCB)
      smx_ERROR_RET(SBE_INV_POOL, NULL, 0);

   /* get next block and update pool->pn and pool->num_used, with interrupts 
      disabled */
   sb_INT_DISABLE_S(is);
   bp = pool->pn;
   if (bp != NULL)
   {
      pool->pn = *(u8**)bp;
      pool->num_used++;
   }
   sb_INT_ENABLE_R(is);

   /* clear clrsz bytes, up to block size */
   if ((clrsz != 0) && (bp != NULL))
   {
      if (clrsz < pool->size)
         memset(bp, 0, clrsz);
      else
         memset(bp, 0, pool->size);
   }
   return bp;
}

/*
*  sb_BlockPoolPeek()
*
*/
u32 sb_BlockPoolPeek(PCB_PTR pool, SB_PK_PAR par)
{
   u8 *pn;
   u32 val = 0;

   /* check for valid pool */
   if (pool->cbtype != SB_CB_PCB)
      smx_ERROR_RET(SBE_INV_POOL, NULL, 0);

   switch (par)
   {
      case SB_PK_NUM:
         val = (u32)pool->num;
         break;
      case SB_PK_NUM_USED:
         val = (u32)pool->num_used;
         break;
      case SB_PK_FREE:
         for (pn = pool->pn; pn != NULL; pn = *((u8 **)pn))
            val++;
         break;
      case SB_PK_FIRST:
         val = (u32)pool->pn;
         break;
      case SB_PK_MIN:
         val = (u32)pool->pi;
         break;
      case SB_PK_MAX:
         val = (u32)pool->px;
         break;
      case SB_PK_NAME:
         val = (u32)pool->name;
         break;
      case SB_PK_SIZE:
         val = (u32)pool->size;
         break;
      default:
         smx_ERROR_RET(SBE_INV_PAR, NULL, 0);
   }
   return val;
}

/*
*  Block Release
*
*  Returns block to pool if block pointer and pool are valid. Then clears
*  bytes 4 thru clrsz-1 up to the end of the block. Block pointer can point
*  anywhere in the block. Returns true if successful and false if not.
*
* Notes:
*  1. Interrupt safe -- CAN BE USED IN ISRs.
*  2. Will not clear beyond end of block.
*/
bool sb_BlockRel(PCB_PTR pool, u8* bp, u16 clrsz)
{
   u32   is;               /* interrupt state */
   u32   s = pool->size;   /* block space */

   /* check parameters */
   if (pool->cbtype != SB_CB_PCB)
      smx_ERROR_RET(SBE_INV_POOL, false, 0);
   if ((bp < pool->pi) || (bp >= pool->px + s))
      smx_ERROR_RET(SBE_INV_BP, false, 0);

   /* set bp to the start of the block */
   bp = bp - (bp - pool->pi)%s;

   /* link the block into the pool free list and decrement num_used with 
      interrupts disabled */
   sb_INT_DISABLE_S(is);
   *(u32*)bp = (u32)pool->pn;
   pool->pn = bp;
   pool->num_used = (pool->num_used ? --pool->num_used : 0);
   sb_INT_ENABLE_R(is);

   /* clear bytes 4 thru clrsz-1, up to block end */
   if (clrsz > 4)
   {
      if (clrsz < s)
         memset(bp + 4, 0, clrsz - 4);
      else
         memset(bp + 4, 0, pool->size - 4);
   }
   return true;
}

/*===========================================================================*
*                         POWER MANAGEMENT FUNCTIONS                         *
*===========================================================================*/

/* Power Down processor. Set sleep mode, save tick counter, and issue
   power down instruction. On power return, determine counts lost, divide
   it by counts per tick, load remainder into tick counter, and return the
   quotient = number of ticks lost. Used by smx, and documented in the
   Power Management chapter of the smx User's Guide. Define sleep_mode
   values (levels) for your application.
*/
u32 sb_PowerDown(u32 sleep_mode)
{
   return 10; /* for testing */
}

/*===========================================================================*
 *                           MAIN STACK FUNCTIONS                            *
*===========================================================================*/

/* Main Stack Fill. Fills whole MS. */
void sb_MSFill(void)
{
   sb_MSFillResv(0);
}

/* Main Stack Fill Reserve. Fills MS to within resv bytes from the bottom.
   Offset must be < MS size.
*/

void sb_MSFillResv(u32 resv)
{
   u32 *p;
   u32 i, n;

   if ((p = (u32 *)sb_MS_GET_TOP()) != NULL)
   {
      n = sb_MS_GET_SIZE();
      n = (n - resv)/4;
      for (i = 0; i < n; i++)
         p[i] = SB_STK_FILL_VAL;
   }
}

/* Main Stack Scan. Called after startup code. Tests for SB_STK_FILL_VAL
   from the top of MS down to the first non-match and returns number of
   bytes filled. 
*/
u32 sb_MSScan(void)
{
   u32 n = 0, *p;

   if ((p = (u32 *)sb_MS_GET_TOP()) != NULL)
      for (; *p == (u32)SB_STK_FILL_VAL; p++)
         n++;
   return 4*n;
}

#if SB_CFG_TM
/*===========================================================================*
*                      TIME MEASUREMENT FUNCTIONS (PMODE)                    *
*===========================================================================*/

/* These versions directly call sb_PtimeGet(). */

/* Time Measurement Initialization determines overhead = sb_TMCal.
   Called automatically in main() via sb_TM_INIT(). 
*/
void sb_TMInit(void)
{
   sb_TMStart(&sb_TMs);
   sb_TMCal = 0;
   sb_TMEnd(sb_TMs, &sb_TMCal);
}

/* Time Measurement Start gets starting ptime and stores at pts */
void sb_TMStart(u32* pts)
{
   *pts = sb_PtimeGet();
}

/* Time Measurement End gets the ending ptime and calculates and stores the
   time measurement at ptm. Only good for up to sb_ticktmr_cntpt - 1 
*/
void sb_TMEnd(u32 ts, u32* ptm)
{
   u32 d;
   d = sb_PtimeGet();
   d = (d >= ts ? d - ts : d + sb_ticktmr_cntpt - ts);
   *ptm = (d > sb_TMCal ? d - sb_TMCal : 0);
}

/* Time measurement for tLSR, pLSR, and uLSR */
void sb_TMLsr(void)
{
   switch (sb_tltsel)
   {
      case 1:
         sb_TMEnd(sb_ts1, &sb_te[0]); /* sb_te[0] = tLSR time */
         break;
      case 2:
         sb_TMEnd(sb_ts1, &sb_te[1]); /* sb_te[1] = pLSR time */
         break;
      default:
         sb_TMEnd(sb_ts1, &sb_te[2]); /* sb_te[2] = uLSR time */
   }
}
#endif /* SB_CFG_TM */

/*===========================================================================*
*                               FAULT MANAGERS                               *
*===========================================================================*/

/* hard fault manager */
void sb_HFM(void)
{
   if (*ARMM_HFSR & ARMM_FL_FORCED) /* test for escalated fault */
   {
      if (*ARMM_MMFSR & (ARMM_FL_DACCVIOL || ARMM_FL_IACCVIOL))
      {
         /* report memory manage fault */
         smx_EM(SBE_CPU_MMF_VIOL, 2);
      }
      else
      {
         /* report hard fault */
         smx_EM(SBE_CPU_HF_VIOL, 2);
      }
   }
   else
   {
      /* report hard fault */
      smx_EM(SBE_CPU_HF_VIOL, 2);
   }
  #if !defined(SMX_TSMX)
   sb_Reboot();
  #endif
}

/* usage fault manager */
extern u32 msp_sav;
bool sb_UFM(void)
{
   bool  msplim = false;
  #if SB_CPU_ARMM8
   if (*ARMM_UFSR & ARMM_FL_STKOF)  /* test for stack overflow */
   {
      if (smx_TSOvfl())
      {
         /* inhibit double reporting by task scheduler */
         smx_ct->flags.stk_ovfl = 1;
         /* report task stack overflow and stop current task */
         smx_EM(SMXE_STK_OVFL, 1);
      }
      else
      {
         /* report main stack overflow and stop system */
         smx_EM(SMXE_MSTK_OVFL, 2);
         msplim = true;
      }
   }
   else
      smx_EM(SBE_CPU_UF_VIOL, 1);
  #else
      smx_EM(SBE_CPU_UF_VIOL, 1);
  #endif
   return msplim;
}

#if SMX_CFG_SSMX
/*+++++++++++++++++++++++ LIMITED SWI/SVC API (UMODE) ++++++++++++++++++++++*/
#include "xapiu.h"
/*............................. SECTION CHANGE .............................*/
#pragma default_function_attributes = @ ".ucom.text"

#if SB_CFG_TM
/*===========================================================================*
*                      TIME MEASUREMENT FUNCTIONS (UMODE)                    *
*===========================================================================*/

/* These versions call sb_PtimeGet() via SVC. See macros in xapiu.h,
   and see comments for pmode functions above. */

void sbu_TMInit(void)
{
   sbu_TMStart(&sbu_TMs);
   sbu_TMCal = 0;
   sbu_TMEnd(sbu_TMs, &sbu_TMCal);
}

void sbu_TMStart(u32* pts)
{
   *pts = sbu_PtimeGet();
}

void sbu_TMEnd(u32 ts, u32* ptm, u32 cal)
{
   u32 d;
   d = sbu_PtimeGet();
   d = (d >= ts ? d - ts : d + sbu_ticktmr_cntpt - ts);
   *ptm = (d > cal ? d - cal : 0);
}
#endif /* SB_CFG_TM */
#endif /* SMX_CFG_SSMX */


/*===========================================================================*
*                               TIME FUNCTIONS                               *
*===========================================================================*/

#if SB_CC_TIME_FUNCS  /* if compiler supports time functions */
#include <time.h>
#endif

#if SMX_CFG_SSMX

/* helper for sb_DelayUsec() */
#undef sb_PtimeGet
static u32 PtimeGet(void)
{
   if (sb_IN_UMODE())
      return sbu_PtimeGet();
   else
      return sb_PtimeGet();
}
#define sb_PtimeGet() PtimeGet()

#endif /* SMX_CFG_SSMX */

/*------ sb_DelayUsec(num)
*
* Documented in bbsp.h.
*
* Differences from Spec: none
*
* Notes:
* 1. We use the same timer as the smx tick so more timers are
*    available to the application. This function cannot be used
*    until the tick timer has been initialized by sb_TickInit().
* 2. The routine is coded to allow passing very large values
*    in case that is useful. Normally, this should only be used
*    for short waits. For long waits you should normally do an smx
*    multitasking wait so equal and lower priority tasks can run.
*
*/
void sb_DelayUsec(u32 num)
{
   u32   num_left;
   u32   num_now;
   u32   remainder;
   u32   current;
   u32   target;
   u32   now;
   u32   clkhz;
   u32   cntpt;
   bool  tick_init_done;

   /* for testing long delays */
   //sb_LEDInit();
   //sb_LEDWriteRow(1);

   if (num == 0)
      return;

  #if SMX_CFG_SSMX
   if (sb_IN_UMODE())
   {
      clkhz = sb_Peek(SB_PK_TICKTMR_CLKHZ);
      cntpt = sb_Peek(SB_PK_TICKTMR_CNTPT);
      tick_init_done = sb_Peek(SB_PK_TICK_INIT_DONE);
   }
   else
  #endif
   {
      clkhz = sb_ticktmr_clkhz;
      cntpt = sb_ticktmr_cntpt;
      tick_init_done = sb_tick_init_done;
   }

   /* sb_DelayUsec() cannot be used until the tick has been initialized.
      If you must use this before sb_TickInit() is called, copy tick counter
      init here (not tick interrupt init).
   */
   if (!tick_init_done)
      while (1) { sb_DEBUGTRAP(); }

   current = sb_PtimeGet();
   if (current >= cntpt)
   {
      /* If this trap is hit, the tick timer reload value or
         sb_ticktmr_cntpt is set incorrectly. The timer reload value
         should be sb_ticktmr_cntpt-1. Check this and sb_TickInit()
         in bsp.c.
      */
      sb_DEBUGTRAP();
      current = cntpt-1;  /* adjust to work around the problem */
   }
   /* num_left = num * number of timer counts per usec */
   num_left = num * ((clkhz + 999999) / 1000000);
   num_now = num_left;
   if (num_now > cntpt/2)
      num_now = cntpt/2;
   /* num_now is the value used for all except the last loop possibly. */
   /* Uses /2 to leave half the range to stop the wait below for long delays. */

   while (num_left != 0)
   {
      if (num_now > num_left)
         num_now = num_left;

      /* calculate target */
      if ((cntpt-1 - current) >= num_now)
      {
         remainder = 0;
         target = current + num_now;
      }
      else
      {
         remainder = num_now - (cntpt-1 - current);
         target = remainder;
      }

      /* wait */
      if (!remainder)
         while ((now = sb_PtimeGet()) >= current && now < target) {}
      else
         while ((now = sb_PtimeGet()) >= current || now < target) {}

      /* update for next loop */
      current = target;
      num_left -= num_now;
   }

   /* for testing long delays */
   //sb_LEDWriteRow(0);
}

/*------ sb_StimeSet()
*
* Documented in bbsp.h.
*
* Differences from Spec: none
*
*/
bool sb_StimeSet(void)
{
#if !SB_CC_TIME_FUNCS
   smx_stime = 0;  /* Just set to 0 for compilers that don't have time functions. */

#else
   struct tm time;
   static const char * months[12] = {"Jan","Feb","Mar","Apr","May","Jun",
                                     "Jul","Aug","Sep","Oct","Nov","Dec"};
   int    month;

   /* USER: Replace this case with appropriate code to set the time
      structure from your hardware clock or by prompting user, etc.

      Below is just an example that sets the time to either a fixed time
      or to the date and time of compilation of this module. Neither
      are appropriate for a real system, but are fine for demos.
   */

  #if 1
   /* Use fixed time and date; using time and date string can confuse the
      user since it will appear to be almost correct the first time they run. */
   static char datestr[15] = "Jan 01 2001";
   static char timestr[10] = "00:00:00";
  #else
   static char datestr[15];
   static char timestr[10];
   strcpy(datestr,__DATE__);  /* Get date of compilation in MMM DD YYYY (e.g. Jun 10 1988) */
   strcpy(timestr,__TIME__);  /* Get time of compilation in hh:mm:ss format (e.g. 15:32:10) */
  #endif

   //_strupr(datestr);
   for (month = 0; month < 12; ++month)    /* Search for matching month string. */
      if (memcmp(months[month],datestr,3) == 0)
         break;

   memset(&time, 0, sizeof(struct tm));    /* clear time structure */
   time.tm_mon  = month;                   /* Month  (0 - 11) */
   time.tm_mday = atoi(datestr+4);         /* Day of month (1 - 31) */
   time.tm_year = atoi(datestr+7) - 1900;  /* Year since 1900 */
   time.tm_hour = atoi(timestr);           /* Hour   (0 - 23) */
   time.tm_min  = atoi(timestr+3);         /* Minute (0 - 59) */
   time.tm_sec  = atoi(timestr+6);         /* Second (0 - 59) */

   /* The time structure has been set. Now initialize smx_stime from it. */

   smx_stime = mktime(&time);
   if (smx_stime == (u32) -1)
   {
      smx_stime = 0;
      sb_ConWriteString(0,8,SB_CLR_LIGHTRED,SB_CLR_BLACK,!SB_CON_BLINK,"time/date conversion failed in init_stime()");
      return(false);
   }
#endif /* SB_CC_TIME_FUNCS */

   return(true);
}

#if SMX_CFG_SSMX
/*+++++++++++++++++++++++ LIMITED SWI/SVC API (UMODE) ++++++++++++++++++++++*/
#include "xapiu.h"
/*............................. SECTION CHANGE .............................*/
#pragma default_function_attributes = @ ".ucom.text"
#endif

void sb_GetLocalTime(DATETIME* pDateTime)
{
#if !SB_CC_TIME_FUNCS  /* if compiler does not support time functions */
    pDateTime->wYear = 21;
    pDateTime->wMonth = 1;
    pDateTime->wDay = 1;
    pDateTime->wHour = 12;
    pDateTime->wMinute = 0;
    pDateTime->wSecond= 0;
#else
    u32 t;
    struct tm *timeptr;
    t = smx_SysPeek(SMX_PK_STIME);
    timeptr = localtime((time_t *) &t);
    if (timeptr)
    {
        if (timeptr->tm_year >= 80)
            pDateTime->wYear = (u16)(timeptr->tm_year - 80) ;
        else
            pDateTime->wYear = 0 ;
        pDateTime->wMonth = (u16)(timeptr->tm_mon + 1);
        pDateTime->wDay = (u16)(timeptr->tm_mday);
        pDateTime->wHour = (u16)(timeptr->tm_hour);
        pDateTime->wMinute = (u16)(timeptr->tm_min);
        pDateTime->wSecond= (u16)(timeptr->tm_sec);
    }
    else
    {
        memset((u8 *)pDateTime, 0, sizeof(DATETIME));
    }
#endif /* SB_CC_TIME_FUNCS */
}

/*===========================================================================*
*                        MISCELLANEOUS FUNCTIONS (UMODE)                     *
*===========================================================================*/

u32 sb_read32_unaligned(u8*addr)
{
   /* if address is already 4-byte aligned do simple read */
   if (!((u32)addr & 0x3))
   {
      return (*(u32*)addr);
   }
   else
   {
     #if (!SB_CPU_BIG_ENDIAN)
      return (((u32)addr[0]) | ((u32)addr[1] << 8) |
              ((u32)addr[2] << 16) | ((u32)addr[3] << 24));
     #else
      return (((u32)addr[3]) | ((u32)addr[2] << 8) |
              ((u32)addr[1] << 16) | ((u32)addr[0] << 24));
     #endif
   }
}

void sb_write32_unaligned(u8*addr, u32 val)
{
   /* if address is already 4-byte aligned do simple write */
   if (!((u32)addr & 0x3))
   {
      *(u32*)addr = val;
   }
   else
   {
     #if (!SB_CPU_BIG_ENDIAN)
      addr[0] = (u8) val;
      addr[1] = (u8) (val >> 8);
      addr[2] = (u8) (val >> 16);
      addr[3] = (u8) (val >> 24);
     #else
      addr[3] = (u8) val;
      addr[2] = (u8) (val >> 8);
      addr[1] = (u8) (val >> 16);
      addr[0] = (u8) (val >> 24);
     #endif
   }
}

/*===========================================================================*
*                      MISSING C LIBRARY FUNCTIONS (UMODE)                   *
*                            NOT IN IAR C LIBRARY                            *
*===========================================================================*/

char* itoa(int val, char *str, int radix)
{
   return ltoa(val, str, radix);
}

char* ltoa(long val, char *str, int radix)
{
   bool          neg;
   char         *ptr;
   char          ch;
   unsigned long uval;

   if (radix < 2 || radix > 36)
   {
      if (str)
         *str = '\0';
      return str;
   }
   ptr = str;
   /* val can only be negative if radix is 10 */
   if (radix == 10 && val < 0)
   {
      neg = true;
      uval = (unsigned long) -val;
   }
   else
   {
      neg = false;
      uval = (unsigned long) val;
   }
   /* generate digits in reverse order */
   do {
      ch = (char)(uval % radix);  /* result <= 36 (this is modulo not div) */
      *ptr++ = (char)(ch < 10 ? ch + '0' : ch - 10 + 'A');
   } while ((uval = uval / radix) > 0);
   if (neg)
      *ptr++ = '-';
   *ptr = '\0';
   reverse(str);
   return str;
}

int putchar(int c) /*<1>*/
{
#if (SB_CFG_CON)
   return sb_ConPutChar(c);
#else
   return -1;
#endif
}

char* ultoa(unsigned long uval, char *str, int radix)
{
   char *ptr;
   char  ch;

   if (radix < 2 || radix > 36)
   {
      if (str)
         *str = '\0';
      return str;
   }
   ptr = str;
   /* generate digits in reverse order */
   do {
      ch = (char)(uval % radix);  /* result <= 36 (this is modulo not div) */
      *ptr++ = (char)(ch < 10 ? ch + '0' : ch - 10 + 'A');
   } while ((uval = uval / radix) > 0);
   *ptr = '\0';
   reverse(str);
   return str;
}

void reverse(char* s)  /* reverse the string in place */
{
   char c;
   int i, j;

   for (i = 0, j = strlen(s)-1; i < j; i++, j--)
   {
      c    = s[i];
      s[i] = s[j];
      s[j] = c;
   }
}

/*===========================================================================*
*                        MISCELLANEOUS FUNCTIONS (PMODE)                     *
*===========================================================================*/

#if SMX_CFG_SSMX
/*+++++++++++++++++++++++++ FULL DIRECT API (PMODE) ++++++++++++++++++++++++*/
#include "xapip.h"
/*............................. SECTION CHANGE .............................*/
#pragma default_function_attributes =
#endif

u32 sb_Peek(SB_PK_PAR par)
{
   switch (par)
   {
      case SB_PK_TICK_INIT_DONE:
         return(sb_tick_init_done);

      case SB_PK_TICKTMR_CLKHZ:
         return(sb_ticktmr_clkhz);

      case SB_PK_TICKTMR_CNTPT:
         return(sb_ticktmr_cntpt);

      default:
         break;
   }
   smx_ERROR_RET(SBE_INV_PAR, 0, 0);
}

/* Notes;
   1. Necessary to keep out putchar() from IAR library, which requires __word() 
      to be supplied.
*/