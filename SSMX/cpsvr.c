/*
* cpsvr.c                                                   Version 6.0.0
*
* Console Portal Server Code.
*
* Copyright (c) 2019-2026 Micro Digital Inc.
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

#include "xsmx.h" /* section_prefix = ".sys" */

#if CP_PORTAL

#pragma default_variable_attributes = @ ".cp.bss"
FPSS    cpsvr;                   /* cp portal server struct */
#pragma default_variable_attributes = 

extern const IRQ_PERM sb_irq_perm_uart[]; /* IRQ permissions */
extern FPCS*  cpcli_lst[];       /* permitted client list for cp portal */
extern u32    cpcli_lstsz;       /* pcl size */
TCB_PTR       cp_task;           /* portal server task */
const char* const cpname = "cp"; /* to force "cp" into .rodata <1> */

/* internal subroutines */
static void cp_heap_init(void);
static void cp_main(u32);
static void cp_server(FPSS* psh);

/*
*  cp_init()
*
*  Initialize console portal server.
*/
bool cp_init(u32 ssn)
{
   FPSS* cph = &cpsvr;

   if (mp_FPortalCreate(&cph, cpcli_lst, cpcli_lstsz, ssn, cpname, "cp_sxchg"))
   {
      cp_heap_init();

      /* create and start portal server task */
      cp_task = smx_TaskCreate(cp_main, PRI_MAX, 400, SMX_FL_UMODE, "cp_task");
      if (cp_task)
      {
        #if SB_CPU_ARMM7
         mp_MPACreate(cp_task, &mpa_tmplt_cp, 0xF, 8);
        #elif SB_CPU_ARMM8
         mp_MPACreate(cp_task, &mpa_tmplt_cp, 0xF, 9);
        #endif

        #if SB_CFG_UARTI && !defined(SMX_TSMX)
         smx_TaskSet(cp_task, SMX_ST_IRQ, (u32)sb_irq_perm_uart, 0);
        #endif

         cpsvr.stask = cp_task;
         smx_TaskStart(cp_task);
         return true;
      }
   }
   return false;
}

bool cp_exit(u32 csn)
{
   bool ret = true;

   ret &= mp_FPortalDelete(&cpsvr, cpcli_lst, cpcli_lstsz, csn);
   ret &= smx_TaskDelete(&cpsvr.stask);

   return ret;
}

/*============================================================================
                         CONSOLE PARTITION HEAP (cp_heap)
                         (Runs in umode console partition)
============================================================================*/
#pragma default_variable_attributes = @ ".cp.rodata"

/* Single bin size array. Bin 0 is the only bin. The array ends with 0xFFFFFFFF. */
u32 const cp_binsz[] =
/*bin  0       end */  
      {24, 0xFFFFFFFF};

#pragma data_alignment = SB_CACHE_LINE       /* cache align in DRAM */

#pragma default_variable_attributes = @ ".cp.bss"
HBCB        cp_bin[(sizeof(cp_binsz)/4)-1];  /* cp_heap bins */
u32         cp_hn;                           /* cp_heap number */
EHV         cp_hv;                           /* cp_heap variable struct */

#if EH_STATS
u32         cp_bnum[(sizeof(cp_binsz)/4)-1]; /* cp_heap number of chunks per bin */
u32         cp_bsum[(sizeof(cp_binsz)/4)-1]; /* cp_heap sum of chunk sizes per bin */
#endif
#pragma default_variable_attributes =

#pragma section = "cp_heap"
#pragma default_function_attributes = @ ".cp.text"


/*===========================================================================*
*                           INTERNAL SUBROUTINES                             *
*                           Do Not Call Directly                             * 
*===========================================================================*/

/* initialize cp_heap */
void cp_heap_init(void)
{
   /* get start and size for cp_heap allocated in linker command file */
   u8* hsa = (u8*)__section_begin("cp_heap"); /* heap starting address */
   u32 hsz = (u32)__section_size("cp_heap");  /* heap size */

   /* clear cp_heap variables structure */
   memset((void*)&cp_hv, 0, sizeof(EHV));

   /* initialize cp_heap */
   cp_hn = smx_HeapInit(hsz, 0, hsa, &cp_hv, (u32*)cp_binsz, (HBCB*)cp_bin, 
                                                         EH_NORM, "cp_heap");
   cp_hv.mode.fl.cmerge = ON;  /* always merge chunks */
}

#pragma default_function_attributes = @ ".cp.text"
/*
*  cp_main()
*
*  Main function for console portal server task.
*/
void cp_main(u32)
{
   FPSS* psh = &cpsvr;
   cp_server(psh);
}

#include "xapiu.h"

/*
*  cp_server()
*
*  console portal server.
*/
void cp_server(FPSS* psh)
{
   CPSH* shp;  /* service header pointer */
   MCB*  pmsg;
   u32   p1, p2, p3, p4, p5;
   bool  ret;

   while (pmsg = smx_PMsgReceive(psh->sxchg, (u8**)&shp, psh->ssn, SMX_TMO_INF, NULL))
   {
      switch (shp->fid)
      {
         case CP_CLR_SCREEN:
            sb_ConClearScreen();
            break;
         case CP_CLR_SCREEN_UNP:
            sb_ConClearScreenUnp();
            break;
         case CP_DBG_MODE:
            ret = sb_ConDbgMsgMode();
            shp->ret = ret;
            break;
         case CP_DBG_MODE_SET:
            p1 = shp->p1;
            sb_ConDbgMsgModeSet(p1);
            break;
         case CP_PUT_CHAR:
            p1 = shp->p1;
            sb_ConPutChar(p1);
            break;
         case CP_WRITE_CHAR:
            p1 = (shp->p1>>24)&0xFF;
            p2 = (shp->p1>>16)&0xFF;
            p3 = (shp->p1>>8)&0xFF;
            p4 = (shp->p1)&0xFF;
            p5 = (shp->p1)&0x80000000 ? 1 : 0;
            sb_ConWriteChar(p1, p2, p3, p4, p5,(char)shp->p2);
            break;
         case CP_WRITE_STRING:
            p1 = (shp->p1>>24)&0xFF;
            p2 = (shp->p1>>16)&0xFF;
            p3 = (shp->p1>>8)&0xFF;
            p4 = (shp->p1)&0xFF;
            p5 = (shp->p1)&0x80000000 ? 1 : 0;
            sb_ConWriteString(p1, p2, p3, p4, p5,(ccp)shp->dp);
            break;
         case CP_WRITE_STRING_NUM:
            p1 = (shp->p1>>24)&0xFF;
            p2 = (shp->p1>>16)&0xFF;
            p3 = (shp->p1>>8)&0xFF;
            p4 = (shp->p1)&0xFF;
            p5 = (shp->p1)&0x80000000 ? 1 : 0;
            sb_ConWriteStringNum(p1, p2, p3, p4, p5, (ccp)shp->dp, shp->p2);
            break;
      }
      smx_PMsgReply(pmsg);
   }
}

/* Notes:
   1. "cp" must be in .rodata so that --section .rodata=.svc.rodata in Extra
      Options works, thus allowing it to be accessed by cp clients. If "cp"
      is a parameter, it is put into .text.
*/
#endif /* CP_PORTAL */