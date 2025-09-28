/*
* cpsvr.c                                                   Version 5.4.0
*
* Console Portal Server Code.
*
* Copyright (c) 2019-2025 Micro Digital Inc.
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
   MCB*  pmsg = NULL;
   u32   p1, p2, p3, p4, p5;
   bool  ret;

   while (pmsg = smx_PMsgReceive(psh->sxchg, (u8**)&shp, psh->ssn, SMX_TMO_INF, &pmsg))
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