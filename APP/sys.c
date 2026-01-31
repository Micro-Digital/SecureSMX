/*
* sys.c                                                     Version 6.0.0
*
* System code running in pmode system partition, sys.
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
* Authors: Ralph Moore, David Moore
*
*        hmode = handler mode (no tasks, main stack)
*        pmode = privileged mode (tasks, task stacks)
*        umode = unprivileged mode (tasks, task stacks)
*
*        SMX_CFG_SSMX == 0 non-partitioned system -- see xcfg.h
*        SMX_CFG_SSMX == 1 partitioned system
*
*        <n> = footnote n
*****************************************************************************/

#include "smx.h"
#include "main.h"
#include "app.h"
#include "cpmap.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"  /* system regions <1>*/
#endif

/*============================================================================
                         IDLE TASK MAIN FUNCTION (pmode)
============================================================================*/

/* smx_IdleMain
*
*  The smx_Idle task runs in pmode when there is no other task to run. It 
*  performs background operations such as stack scans, message displays, heap 
*  management, runtime limit testing, etc.
*/
void smx_IdleMain(u32)
{
   static u32 hn = 0;

  #if CP_PORTAL
   /* open console portal for idle */
   mp_FPortalOpen(&cpcli_idle, CP_SLOT_IDLE, 80, 1, 5, "cp_rxchg");
  #endif

   while (1)
   {
     #if SMX_CFG_STACK_SCAN
      smx_StackScan();  /* scan task and main stacks to set high-water marks */

      /* do not add code here because smx_Idle may be running at high priority */
      if (smx_idleup)
      {
         smx_idleup = false;
         smx_NullSSR(); /* return to scheduler */
      }
     #endif

      smx_EtimeRollover();
      sb_MsgDisplay();

     #if SMX_CFG_PROFILE
      smx_ProfileDisplay();
     #endif

      if (smx_hmng) /* hmng is set by smx_KeepTimeLSR() */
      {
         /* call heap manager, if enabled for heap hn */
         if (eh_hvp[hn]->mgr)
            eh_hvp[hn]->mgr(hn);
         if (++hn >= eh_hvpn)
            hn = 0;
         smx_hmng = false;
      }

     #if SMX_CFG_RTLIM
      /* test for end of RTLIM frame <5> */
      if (++smx_Idle->idle_ctr >= SMX_IDLE_RTLIM)
      {
         if (smx_rtlimsem->fl != NULL)
         {
            smx_LSRsOff();
            /* resume all tasks at rtlimsem */
            smx_SemClear(smx_rtlimsem);

            /* clear all non-child RTLIM counters <6> */
            TCB* task = (TCB_PTR)smx_tcbs.pi;
            u32* limp = &task->rtlim; 
            u32* parp = (u32*)&task->parent;

            for (u32 n = 0; n < SMX_NUM_TASKS; n++)
            {
               if (*limp > 0 && *parp == 0)
                  *(limp + 1) = 0;
               limp += sizeof(TCB)/4;
               parp += sizeof(TCB)/4;
            }
            smx_LSRsOn();
         }
         smx_Idle->idle_ctr = 0;
      }
     #endif

      /* bump if another task waiting */
      if (smx_ct->fl != (CB_PTR)&smx_rq[PRI_MIN])
         smx_TaskBump(smx_ct, SMX_PRI_NOCHG);
      else
      {
        #if SMX_CFG_STACK_SCAN
         if (smx_scanstack == NULL)  /* no stacks waiting to be scanned */
        #endif
            smx_SysPowerDown(0); /* <2> */
      }
   }
}

/*============================================================================
                       MAIN HEAP and HEAP FUNCTIONS (pmode)
============================================================================*/
/* Large bin size array. The first 13 bins are small bins, spaced 8 bytes apart. 
   The next 16 bins are large bins spaced 112 bytes apart. The top bin contains 
   all chunks >= 2048 bytes. The array ends with 0xFFFFFFFF. */

u32 const mheap_binsz[]
/*bin  0    1     2     3     4     5     6     7     8     9     10    11 */
      {24,  32,   40,   48,   56,   64,   72,   80,   88,   96,   104,  112, \
/*bin 12    13    14    15    16    17    18    19    20    21    22    23 */
      120,  128,  256,  384,  512,  640,  768,  896,  1024, 1152, 1280, 1408, \
/*bin 24    25    26    27    28       end */
      1536, 1664, 1792, 1920, 2048, 0xFFFFFFFF};

HBCB        mheap_bin[(sizeof(mheap_binsz)/4)-1];  /* mheap bins */
u32         mheap_hn;                              /* mheap heap number */
EHV         mheap_hv;                              /* mheap variable struct */

#if EH_STATS
u32         mheap_bnum[(sizeof(mheap_binsz)/4)-1]; /* mheap number of chunks per bin */
u32         mheap_bsum[(sizeof(mheap_binsz)/4)-1]; /* mheap sum of chunk sizes per bin */
#endif

#pragma section = "mheap" /*<11>*/

/* mheap_init()
*
*  initialize main heap
*/
void mheap_init(void)
{
   /* get start and size for mheap allocated in linker command file */
   u8* hsa = (u8*)__section_begin("mheap"); /* heap starting address */
   u32 hsz = (u32)__section_size("mheap");  /* heap size */

   /* clear mheap variables structure */
   memset((void*)&mheap_hv, 0, sizeof(EHV));

   /* initialize mheap */
   mheap_hn = smx_HeapInit(hsz, 0, hsa, &mheap_hv, (u32*)mheap_binsz, 
                                          (HBCB*)mheap_bin, EH_NORM, "mheap");
   if (mheap_hn == -1)
      smx_EM(SMXE_HEAP_INIT_FAIL, 2);  /* stop system */

   mheap_hv.mgr = smx_HeapManager;     /* call heap manager for mheap */
   mheap_hv.mode.fl.amerge = ON;       /* automatic merge control */

  #if defined(SMX_DEBUG)
   mheap_hv.mode.fl.fill = ON;         /* load fill patterns into data blocks */
  #endif

  #if EH_STATS
   mheap_hv.bnump = (u32*)&mheap_bnum; /* record heap statistics */
   mheap_hv.bsump = (u32*)&mheap_bsum;
  #endif
}

/* smx_HeapManager
*
*  Optional heap management function to run from smx_IdleMain(). This can be 
*  enabled for any heap.
*/
void smx_HeapManager(u32 hn)
{
  #if SMX_CFG_SSMX
   #define HEAP_SCAN_CNT 1
  #else
   #define HEAP_SCAN_CNT 5
  #endif

   static u32  i = 0;
   static u32  heap_scan = HEAP_SCAN_CNT;
   CCB_PTR     tblcp;   /* top bin last chunk pointer */
   u32         tblcsz;  /* top bin last chunk size */
   u32         tcsz;    /* top chunk size */

   /* bin sort and heap plus bin scan <10> */
   if (smx_ct == smx_Idle && eh_hvp[hn]->mtx->onr == NULL)
   {
      /* do two sort iterations for the smallest bin with bsmap set */
      smx_HeapBinSort(100, 2, hn);

      /* heap scan and fix when heap scan is 0 */
      if (--heap_scan == 0)
      {
         smx_HeapScan(NULL, 4, 4, hn);
         heap_scan = HEAP_SCAN_CNT;
      }
      /* heap bin scan and fix */
      if (smx_HeapBinScan(i, 2, 10, hn))
         i = (i == eh_hvp[hn]->top_bin ? 0 : i + 1);
   }

   /* automatic heap chunk merge control */
   if (eh_hvp[hn]->mode.fl.amerge == ON)
   {
      tcsz = eh_hvp[hn]->tcp->sz;

      if ((eh_hvp[hn]->tbsz + tcsz) < (SMX_HEAP_SPACE/4))
         eh_hvp[hn]->mode.fl.cmerge = ON;
      else
      {
         tblcp = mheap_bin[eh_hvp[hn]->top_bin].fbl;
         tblcsz = (tblcp == NULL ? 0 : tblcp->sz);

         if (tblcsz < SMX_HEAP_CSZ_MAX && tcsz < SMX_HEAP_CSZ_MAX)
            eh_hvp[hn]->mode.fl.cmerge = ON;
         else
            eh_hvp[hn]->mode.fl.cmerge = OFF;
      }
   }
}

/*============================================================================
                         CONSOLE PORTAL VARIABLES
============================================================================*/
#if SB_CFG_CON /* true if SMX_CFG_SSMX -- see xcfg.h */
#if CP_PORTAL

#if SB_CPU_ARMM7
#define CP_SLOT_OPCON 3   /* pmsg slot in mpa_tmplt_opcon */
#elif SB_CPU_ARMM8
#define CP_SLOT_OPCON 8   /* pmsg slot in mpa_tmplt_opcon <3> */
#endif

/* portal client structures for console portal */
FPCS    cpcli_opcon;      /* client struct for opcon */
FPCS    cpcli_idle;       /* client struct for idle */

#pragma default_variable_attributes = @ ".cp.data" /*<4>*/

/* console portal server permitted client list <4a> */
FPCS*   cpcli_lst[] = {&cpcli_idle, &cpcli_opcon,
#if SB_FPU_DEMO 
                                    &cpcli_fpu, 
#endif
#if MW_FATFS_DEMO
                                    &cpcli_fpd
#endif
                                              };

u32     cpcli_lstsz = sizeof(cpcli_lst)/4;  /* size of cpcli_lst */

#pragma default_variable_attributes =
#endif /* CP_PORTAL */

/*============================================================================
                             OPERATION CONTROL
============================================================================*/
TCB_PTR  opcon;         /* opcon task */
PICB_PTR opcon_pipe;    /* opcon pipe */ 
u8       pipe_buf[8];   /* buffer for opcon_pipe */

/* opcon_init  (pmode)
* 
*  operation contol initialization 
*/
void opcon_init(void)
{
   opcon = smx_TaskCreate(opcon_main, PRI_MAX, 0, SMX_FL_NONE, "opcon");
   opcon_pipe = smx_PipeCreate(&pipe_buf, 1, 8, "opcon_pipe");

 #if SMX_CFG_SSMX
  #if SB_CPU_ARMM7
   mp_MPACreate(opcon, &mpa_tmplt_opcon, 0x7, 8);
  #else
   mp_MPACreate(opcon, &mpa_tmplt_opcon, 0xF, 9);
  #endif
 #endif

   smx_TaskStart(opcon);
}

/* opcon_main  (pmode)
*
*  The interrupt-driven UART driver puts ASCII characters from the console 
*  keyboard into opcon_pipe. Otherwise, the UART is polled.
*
*  SSMX: This is an example of a pmode partition that receives characters via a
*  pipe and accesses another partition (console partition) via its portal in
*  order to display messages. <12>
*/
void opcon_main(u32)
{
   char key;
   bool appscr = true;

  #if CP_PORTAL
   #define CP_PCH &cpcli_opcon /* define client structure for cp portal calls <7> */

   /* open console portal for opcon */
   mp_FPortalOpen(&cpcli_opcon, CP_SLOT_OPCON, 80, 1, 5, "cp_rxchg");
  #endif

   while (1)
   {
     #if SB_CFG_UARTI
      smx_PipeGetPktWait(opcon_pipe, &key, SMX_TMO_INF); /* get key from opcon_pipe */
     #else
      while ((key = sb_UartGetCharPoll()) == 0) /* poll for key */
      {
         smx_TaskSuspend(SMX_CT, smx_ConvMsecToTicks(100));
      }
     #endif

      if (key == 0)
         continue;

      if (key == Esc)   /* shut down */
      {
         if (appscr)
         {
          #if SMX_CFG_SSMX
           #if CP_PORTAL
            cp_ConPortalClose(&cpcli_opcon, 0);
           #endif
            /* change Idle task to use mpa_tmplt_init */
           #if SB_CPU_ARMM8
            mp_MPACreate(smx_Idle, &mpa_tmplt_init, 0x7, 11);
           #else
            mp_MPACreate(smx_Idle, &mpa_tmplt_init, 0xF, 10);
           #endif
          #endif
            smx_lockctr = 0;
            /* restart idle with aexit() main function and max priority */
            smx_TaskStartNew(smx_Idle, SMXE_OK, PRI_SYS, (FUN_PTR)aexit);
         }
         else
         {
            appscr = true;
            sb_ConDbgMsgModeSet(false);
            sb_ConClearScreen();
            display_protosystem_msgs();
         }
      }
      else if (key == Ctrl_D)
      {
         /* Ctrl-D for debug mode */
         appscr = false;
         sb_ConClearScreen();
         sb_ConDbgMsgModeSet(true);
         sb_ConWriteString(ESC_COL, 0, SB_CLR_WHITE, SB_CLR_BLACK, !SB_CON_BLINK, "Esc to App");
      }
      else if (key == Ctrl_E)
      {
         if (smx_errctr != 0)
         {
            /* Ctrl-E on error screen refreshes it */
            appscr = false;
            sb_ConClearScreen();
            smx_EBDisplay();
            sb_ConWriteString(ESC_COL, 0, SB_CLR_WHITE, SB_CLR_BLACK, !SB_CON_BLINK, "Esc to App");
         }
         else
         {
            /* error buffer is empty */
            appscr = false;
            sb_ConClearScreen();
            sb_ConWriteString(ESC_COL, 0, SB_CLR_WHITE, SB_CLR_BLACK, !SB_CON_BLINK, "No errors");
         }
      }
   }
}
#endif /* SB_CFG_CON */

/*============================================================================
                              OTHER FUNCTIONS
============================================================================*/

/* smx_EMHook   (pmode)
*
*  Callback function from Error Manager. 
*/
void smx_EMHook(SMX_ERRNO errno, void* h, u8 sev)
{
   TCB_PTR t;

   if ((h != NULL) && (h != (void*)smx_clsr))
      t = (TCB_PTR)h;

   switch (sev)
   {
      case 0:  /* continue operation */
         break;
      case 1:  /* stop task */
         if ((t != NULL) && (t != smx_Idle))
         {
            smx_TaskStop(t);  /*<8>*/
            sb_MsgOut(SB_MSG_WARN, "TASK STOPPED");
         }
         break;
      case 2: /* stop system */
         sb_MsgOut(SB_MSG_WARN, "SYSTEM ABORT");
         aexit(errno);  /*<9>*/
         break;
      default:
         break;
   }
}

/* Notes:
   1. section_prefix = ".sys" overrides the standard section names:
            .text   -> .sys.text
            .rodata -> .sys.rodata
            .data   -> .sys.data
            .bss    -> .sys.bss
      See the linker command file (.icf), for how sections are used to form
      regions and see mpa*.c for how regions are used to form MPA templates.
   2. Do not specify sleep mode unless processor actually stops.
   3. ARMM8 does not permit region overlap with sys_data, so if pmsg is from
      mheap, it is put into MPA[8], which is an auxiliary slot not loaded into 
      the MPU. If pmsg is from another heap, slot 5 should be used, instead.
   4. #pragma default_variable_attributes = @ ".cp.data" overrides any previous
      data section name with .cp.data until #pragma default_variable_attributes = 
      is encountered. See the linker command file (.icf) for how section names
      are used.
   4a.Only these clients are permitted to access console partition portal.
   5. An RTLIM frame ends after the Idle task has run SMX_IDLE_RTLIM times.
   6. Child task rtlim pointers are not cleared. Prior to clearing parent 
      rtl_ctrs, parent tasks that have hit rtlim could be logged.
   7. See cpmap.h and cpcli.c.
   8. Because smx_srnest == 2, smx_SSRExit() at the end of smx_TaskStop(), 
      skips PendSVH(), decrements smx_srnest to 1, and returns to the task 
      scheduler. If smx_sched was SUSP or TEST, it is now STOP and smx_ct
      is stopped.
   9. Because smx_srnest > 0, aexit cannot be preempted by tasks nor LSRs.
   10. These are skipped for idle if the heap mutex is busy, because causing 
       idle to wait would cause inaccurate profiling.
   11. Although #pragma section = "mheap" says section, it actually defines 
       mheap block defined in linker command file (.icf).
   12. See mpa7.c or mpa8.c for MPA template and <processor>app_mpu.icf for 
       linker command file.
*/