/*
* svc.c                                                     Version 6.0.0
*
* ARMM SVC system call shell functions. These are all system calls permitted
* from umode. Some have built-in restrictions. For better security, all unused
* service calls should be removed from this file. For even greater security,
* a custom set of shell functions can be defined within a partition.
*
* Copyright (c) 2016-2026 Micro Digital Inc.
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
#include "svc.h"

#if defined(MW_FATFS) && defined(SB_CPU_STM32)
#ifdef __cplusplus
extern "C" {
#endif
uint8_t BSP_SD_Init(void);
u8 sbu_BSP_SD_Init(void);
#ifdef __cplusplus
}
#endif
#endif

#if SMX_CFG_SSMX

/*===========================================================================*
*                         System Service Jump Table <1>                      *
*===========================================================================*/

/* main system service table indices */
enum ssndx {LIM, AS, ASL, BG, BM, BP, BR, BRA, BU, BPC, BPD, BPP, 
            EFP, EFS, EFT, EFTS, EGC, EGCR, EGD, EGP, EQCL, 
            EQC, EQCT, EQCR, EQD, EQP, EQS, HBP, HBS, HBSR, 
            HC, HCP, HF, HM, HP, HR,
           #if defined(SMX_DEBUG)
            HTA, HTD, HTGH, HTGN,
           #endif
            MB, MG, MM, MP, MR, MRS, MRL, MRLA, MS, MU, MXC, MXCR, MXD, MXP, 
            MUC, MUCR, MUD, MUF, MUG, MUGS, MUP, MUR, 
            PBGH, PBGP, PBM, PBRH, PBRP, 
            PIC, PICR, PID, PIG8, PIG8M, PIGP, PIGPW, PIGPWS, PIP8, PIP8M, PIPP, 
            PIPPW, PIPPWS, PIP, PIR, 
            PMGH, PMGP, PMM, PMR, PMRS, PMRL, PMRP, PMS, PMSB, 
            SC, SCR, SD, SP, SS, ST, STS, SPK, SPHC, SYT, SWI, 
            TB, TC, TCR, TD, TL, TLK, TLKC, TP, TR, TSET, TSL, TSLS, TS, TSN, 
            TSO, TSU, TU, TUQ, TY, 
            TMRD, TMRP, TMRR, TMRSP, TMRS, TMRSA, TMRSO,
           #if SMX_CFG_EVB
            ELU4, ELU5, ELU6, ELUP,
           #endif
            IRQC, IRQM, IRQU, PK, PTMG,
            MO, MPAC, MPACL, MPASM, 
           #if SMX_CFG_PORTAL
            FPC, FPO, FPR, FPS, FTPS, POEM, POL, POR, TPC,
            DTPO, DTPR, DTPS,
           #endif
           #if defined(MW_FATFS) && defined(SB_CPU_STM32)
            BSP_SDI,
           #endif
            EM,
            END
}; 

/* main system service jump table */
u32 smx_sst[] = {
   (u32)END,
   (u32)smx_SchedAutoStop,
   (u32)smx_SchedAutoStopLSR,
   (u32)smx_BlockGet,
   (u32)smx_BlockMake,
   (u32)smx_BlockPeek,
   (u32)smx_BlockRel,
   (u32)smx_BlockRelAll,
   (u32)smx_BlockUnmake,
   (u32)smx_BlockPoolCreate,
   (u32)smx_BlockPoolDelete,
   (u32)smx_BlockPoolPeek,
   (u32)smx_EventFlagsPulse,
   (u32)smx_EventFlagsSet,
   (u32)smx_EventFlagsTest,
   (u32)smx_EventFlagsTestStop,
   (u32)smx_EventGroupClear,
   (u32)smx_EventGroupCreate,
   (u32)smx_EventGroupDelete,
   (u32)smx_EventGroupPeek,
   (u32)smx_EventQueueClear,  
   (u32)smx_EventQueueCount,
   (u32)smx_EventQueueCountStop,
   (u32)smx_EventQueueCreate,
   (u32)smx_EventQueueDelete,
   (u32)smx_EventQueuePeek,
   (u32)smx_EventQueueSignal,
   (u32)smx_HeapBinPeek,
   (u32)smx_HeapBinScan,
   (u32)smx_HeapBinSort,
   (u32)smx_HeapCalloc,
   (u32)smx_HeapChunkPeek,
   (u32)smx_HeapFree,
   (u32)smx_HeapMalloc,
   (u32)smx_HeapPeek,
   (u32)smx_HeapRealloc,
  #if defined(SMX_DEBUG)
   (u32)smx_HTAdd,
   (u32)smx_HTDelete,
   (u32)smx_HTGetHandle,
   (u32)smx_HTGetName,
  #endif
   (u32)smx_MsgBump,
   (u32)smx_MsgGet,
   (u32)smx_MsgMake,
   (u32)smx_MsgPeek,
   (u32)smx_MsgReceive,
   (u32)smx_MsgReceiveStop,
   (u32)smx_MsgRel,
   (u32)smx_MsgRelAll,
   (u32)smx_MsgSend,
   (u32)smx_MsgUnmake,
   (u32)smx_MsgXchgClear,
   (u32)smx_MsgXchgCreate,
   (u32)smx_MsgXchgDelete,
   (u32)smx_MsgXchgPeek,
   (u32)smx_MutexClear,   
   (u32)smx_MutexCreate,
   (u32)smx_MutexDelete,
   (u32)smx_MutexFree,
   (u32)smx_MutexGet,
   (u32)smx_MutexGetStop,
   (u32)smx_MutexPeek,
   (u32)smx_MutexRel,
   (u32)smx_PBlockGetHeap, 
   (u32)smx_PBlockGetPool,
   (u32)smx_PBlockMake,
   (u32)smx_PBlockRelHeap,
   (u32)smx_PBlockRelPool,
   (u32)smx_PipeClear,
   (u32)smx_PipeCreate,
   (u32)smx_PipeDelete,
   (u32)smx_PipeGet8,
   (u32)smx_PipeGet8M,
   (u32)smx_PipeGetPkt,
   (u32)smx_PipeGetPktWait, 
   (u32)smx_PipeGetPktWaitStop,
   (u32)smx_PipePut8,
   (u32)smx_PipePut8M,
   (u32)smx_PipePutPkt,
   (u32)smx_PipePutPktWait,
   (u32)smx_PipePutPktWaitStop,
   (u32)smx_PipePeek,
   (u32)smx_PipeResume,
   (u32)smx_PMsgGetHeap,
   (u32)smx_PMsgGetPool,
   (u32)smx_PMsgMake,
   (u32)smx_PMsgReceive,
   (u32)smx_PMsgReceiveStop,
   (u32)smx_PMsgRel,
   (u32)smx_PMsgReply,
   (u32)smx_PMsgSend,
   (u32)smx_PMsgSendB,
   (u32)smx_SemClear,
   (u32)smx_SemCreate,
   (u32)smx_SemDelete,
   (u32)smx_SemPeek,
   (u32)smx_SemSignal,
   (u32)smx_SemTest,
   (u32)smx_SemTestStop,
   (u32)smx_SysPeek,  
   (u32)smx_SysPseudoHandleCreate,
   (u32)smx_SysTest,
   (u32)smx_SysWhatIs,
   (u32)smx_TaskBump,
   (u32)smx_TaskCurrent,
   (u32)smx_TaskCreate,
   (u32)smx_TaskDelete,
   (u32)smx_TaskLocate,
   (u32)smx_TaskLock,
   (u32)smx_TaskLockClear,
   (u32)smx_TaskPeek,
   (u32)smx_TaskResume,
   (u32)smx_TaskSet,
   (u32)smx_TaskSleep,
   (u32)smx_TaskSleepStop,
   (u32)smx_TaskStart,
   (u32)smx_TaskStartNew,
   (u32)smx_TaskStop,
   (u32)smx_TaskSuspend,
   (u32)smx_TaskUnlock,
   (u32)smx_TaskUnlockQuick,
   (u32)smx_TaskYield,
   (u32)smx_TimerDup, 
   (u32)smx_TimerPeek,
   (u32)smx_TimerReset,
   (u32)smx_TimerSetPulse,
   (u32)smx_TimerStart,
   (u32)smx_TimerStartAbs,
   (u32)smx_TimerStop,
  #if SMX_CFG_EVB
   (u32)smx_EVBLogUser4,
   (u32)smx_EVBLogUser5,
   (u32)smx_EVBLogUser6,
   (u32)smx_EVBLogUserPrint,
  #endif
   (u32)sb_IRQClear,
   (u32)sb_IRQMask,
   (u32)sb_IRQUnmask,
   (u32)sb_Peek,
   (u32)sb_PtimeGet,
   (u32)sb_MsgOut,
   (u32)mp_MPACreate,
   (u32)mp_MPACreateLSR,
   (u32)mp_MPASlotMove,
  #if SMX_CFG_PORTAL
   (u32)mp_FPortalClose,
   (u32)mp_FPortalOpen,
   (u32)mp_FPortalReceive,
   (u32)mp_FPortalSend,
   (u32)mp_FTPortalSend,
   (u32)mp_PortalEM,
   (u32)mp_PortalLog,
   (u32)mp_PortalRet,
   (u32)mp_TPortalClose,
   (u32)mp_SetDAF,
   (u32)mp_SetDAF,
   (u32)mp_SetDAF,
  #endif
  #if defined(MW_FATFS) && defined(SB_CPU_STM32)
   (u32)BSP_SD_Init,
  #endif
   (u32)smx_EM,
};

/*===========================================================================*
*                                uSSR Shells                                 *
*===========================================================================*/

#include "xapiu.h"

#pragma default_function_attributes = @ ".svc.text"

NI void smxu_SchedAutoStop(void)
{
   sb_SVC(AS)
}

NI void smxu_SchedAutoStopLSR(void)
{
   sb_SVC(ASL)
}

NI BCB_PTR smxu_BlockGet(PCB_PTR pool, u8 **bpp, u32 clrsz, BCB_PTR* bhp)
{
   sb_SVC(BG)
}

NI BCB_PTR smxu_BlockMake(PCB_PTR pool, u8 *bp, BCB_PTR* bhp)
{
   sb_SVC(BM)
}

NI u32 smxu_BlockPeek(BCB_PTR blk, SMX_PK_PAR par)
{
   sb_SVC(BP)
}

NI bool smxu_BlockRel(BCB_PTR blk, u16 clrsz)
{
   sb_SVC(BR)
}

NI u32 smxu_BlockRelAll(TCB_PTR task)
{
   sb_SVC(BRA)
}

NI u8* smxu_BlockUnmake(PCB_PTR *pool, BCB_PTR blk)
{
   sb_SVC(BU)
}

NI PCB_PTR smxu_BlockPoolCreate(u8 *p, u16 num, u16 size, const char *name, PCB_PTR* php)
{
   sb_SVCG4(BPC)
}

NI u8* smxu_BlockPoolDelete(PCB_PTR* php)
{
   sb_SVC(BPD)
}

NI u32 smxu_BlockPoolPeek(PCB_PTR pool, SMX_PK_PAR par)
{
   sb_SVC(BPP)
}

NI bool smxu_EventFlagsPulse(EGCB_PTR eg, u16 pulse_mask)
{
   sb_SVC(EFP)
}

NI bool smxu_EventFlagsSet(EGCB_PTR eg, u32 set_mask, u32 pre_clear_mask)
{
   sb_SVC(EFS)
}

NI u32 smxu_EventFlagsTest(EGCB_PTR eg, u32 test_mask, u32 mode, 
                                             u32 post_clear_mask, u32 timeout)
{
   sb_SVC(EFT)
}

NI void smxu_EventFlagsTestStop(EGCB_PTR eg, u32 test_mask, u32 mode,
                                             u32 post_clear_mask, u32 timeout)
{
   sb_SVC(EFTS)
}

NI bool smxu_EventGroupClear(EGCB_PTR eg, u16 init_mask)
{
   sb_SVC(EGC)
}

NI EGCB_PTR smxu_EventGroupCreate(u32 init_mask, const char *name, EGCB_PTR* eghp)
{
   sb_SVC(EGCR)
}

NI bool smxu_EventGroupDelete(EGCB_PTR* eghp)
{
   sb_SVC(EGD)
}

NI u32 smxu_EventGroupPeek(EGCB_PTR eg, SMX_PK_PAR par)
{
   sb_SVC(EGP)
}

NI bool smxu_EventQueueClear(EQCB_PTR eq)
{
   sb_SVC(EQCL)
}

NI bool smxu_EventQueueCount(EQCB_PTR eq, u32 count, u32 timeout)
{
   sb_SVC(EQC)
}

NI void smxu_EventQueueCountStop(EQCB_PTR eq, u32 count, u32 timeout)
{
   sb_SVC(EQCT)
}

NI EQCB_PTR smxu_EventQueueCreate(const char *name, EQCB_PTR* eqhp)
{
   sb_SVC(EQCR)
}

NI bool smxu_EventQueueDelete(EQCB_PTR* eqhp)
{
   sb_SVC(EQD)
}

NI u32 smxu_EventQueuePeek(EQCB_PTR eq, SMX_PK_PAR par)
{
   sb_SVC(EQP)
}

NI bool smxu_EventQueueSignal(EQCB_PTR eq)
{
   sb_SVC(EQS)
}

NI u32 smxu_HeapBinPeek(u32 binno, EH_PK_PAR par, u32 hn)
{
   sb_SVCH(HBP)
}

NI bool smxu_HeapBinScan(u32 binno, u32 fnum, u32 bnum, u32 hn)
{
   sb_SVCH4(HBS)
}

NI bool smxu_HeapBinSort(u32 binno, u32 fnum, u32 hn)
{
   sb_SVCH(HBSR)
}

NI void* smxu_HeapCalloc(u32 num, u32 sz, u32 an, u32 hn)
{
   sb_SVCH4(HC)
}

NI u32 smxu_HeapChunkPeek(void* vp, EH_PK_PAR par, u32 hn)
{
   sb_SVCH(HCP)
}

NI bool smxu_HeapFree(void *bp, u32 hn)
{
   sb_SVCH(HF)
}

NI void* smxu_HeapMalloc(u32 sz, u32 an, u32 hn)
{
   sb_SVCH(HM)
}

NI u32 smxu_HeapPeek(EH_PK_PAR par, u32 hn)
{
   sb_SVCH(HP)
}

NI void* smxu_HeapRealloc(void *cbp, u32 sz, u32 an, u32 hn)
{
   sb_SVCH4(HR)
}

#if defined(SMX_DEBUG)
NI bool smxu_HTAdd(void *h, const char *name)
{
   sb_SVC(HTA)
}

NI bool smxu_HTDelete(void *h)
{
   sb_SVC(HTD)
}

NI void* smxu_HTGetHandle(const char *name)
{
   sb_SVC(HTGH)
}

NI const char* smxu_HTGetName(void *h)
{
   sb_SVC(HTGN)
}
#endif

NI bool smxu_LSRInvoke(LSR_PTR lsr, u32 par)
{
   sb_SVC(0xFF)
}

NI void smxu_LSRsOff(void)
{
   sb_SVC(0xFF)
}

NI bool smxu_LSRsOn(void)
{
   sb_SVC(0xFF)
}

NI bool smxu_MsgBump(MCB_PTR msg, u8 pri)
{
   sb_SVC(MB)
}

NI MCB_PTR smxu_MsgGet(PCB_PTR pool, u8 **bpp, u16 clrsz, MCB_PTR* mhp)
{
   sb_SVC(MG)
}

NI MCB_PTR smxu_MsgMake(u8 *bp, u32 bs, u8 pri, MCB_PTR* mhp)
{
   sb_SVC(MM)
}

NI u32 smxu_MsgPeek(MCB_PTR msg, SMX_PK_PAR par)
{
   sb_SVC(MP)
}

NI MCB_PTR smxu_MsgReceive(XCB_PTR xchg, u8 **bpp, u32 timeout, MCB_PTR* mhp)
{
   sb_SVC(MR)
}

NI void smxu_MsgReceiveStop(XCB_PTR xchg, u8 **bpp, u32 timeout, MCB_PTR* mhp)
{
   sb_SVC(MRS)
}

NI bool smxu_MsgRel(MCB_PTR msg, u16 clrsz)
{
   sb_SVC(MRL)
}

NI u32 smxu_MsgRelAll(TCB_PTR task)
{
   sb_SVC(MRLA)
}

NI bool smxu_MsgSend(MCB_PTR msg, XCB_PTR xchg, u8 pri, void *reply)
{
   sb_SVC(MS)
}

NI u8* smxu_MsgUnmake(MCB_PTR msg, u32* bsp)
{
   sb_SVC(MU)
}

NI bool smxu_MsgXchgClear(XCB_PTR xchg)
{
   sb_SVC(MXC)
}

NI XCB_PTR smxu_MsgXchgCreate(SMX_XMODE mode, const char *name, XCB_PTR* xhp)
{
   sb_SVC(MXCR)
}

NI bool smxu_MsgXchgDelete(XCB_PTR* xhp)
{
   sb_SVC(MXD)
}

NI u32 smxu_MsgXchgPeek(XCB_PTR xchg, SMX_PK_PAR par)
{
   sb_SVC(MXP)
}

NI bool smxu_MutexClear(MUCB_PTR mtx)
{
   sb_SVC(MUC)
}

NI MUCB_PTR smxu_MutexCreate(u8 pi, u8 ceiling, const char *name, MUCB_PTR* muhp)
{
   sb_SVC(MUCR)
}

NI bool smxu_MutexDelete(MUCB_PTR* muhp)
{
   sb_SVC(MUD)
}

NI bool smxu_MutexFree(MUCB_PTR mtx)
{
   sb_SVC(MUF)
}

NI bool smxu_MutexGet(MUCB_PTR mtx, u32 timeout)
{
   sb_SVC(MUG)
}

NI void smxu_MutexGetStop(MUCB_PTR mtx, u32 timeout)
{
   sb_SVC(MUGS)
}

NI u32 smxu_MutexPeek(MUCB_PTR mtx, SMX_PK_PAR par)
{
   sb_SVC(MUP)
}

NI bool smxu_MutexRel(MUCB_PTR mtx)
{
   sb_SVC(MUR)
}

NI u8* smxu_PBlockGetHeap(u32 sz, u8 sn, u32 attr, const char* name, u32 hn)
{
   sb_SVCHG4(PBGH)
}

NI u8* smxu_PBlockGetPool(PCB_PTR pool, u8 sn, u32 attr, const char* name)
{
   sb_SVC(PBGP)
}

NI bool smxu_PBlockMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name)
{
   sb_SVCG4(PBM)
}

NI bool smxu_PBlockRelHeap(u8* bp, u8 sn, u32 hn)
{
   sb_SVCH(PBRH)
}

NI bool smxu_PBlockRelPool(u8* bp, u8 sn, PCB_PTR pool, u32 clrsz)
{
   sb_SVC(PBRP)
}

NI bool smxu_PipeClear(PICB_PTR pipe)
{
   sb_SVC(PIC)
}

NI PICB_PTR smxu_PipeCreate(void *ppb, u8 width, u16 length, const char *name, PICB_PTR* php)
{
   sb_SVCG4(PICR)
}

NI void* smxu_PipeDelete(PICB_PTR* php)
{
   sb_SVC(PID)
}

NI bool smxu_PipeGet8(PICB_PTR pipe, u8 *bp)
{
   sb_SVC(PIG8)
}

NI u32 smxu_PipeGet8M(PICB_PTR pipe, u8 *bp, u32 lim)
{
   sb_SVC(PIG8M)
}

NI void smxu_PipeGetPkt(PICB_PTR pipe, void *pdst)
{
   sb_SVC(PIGP)
}

NI bool smxu_PipeGetPktWait(PICB_PTR pipe, void *pdst, u32 timeout)
{
   sb_SVC(PIGPW)
}

NI void smxu_PipeGetPktWaitStop(PICB_PTR pipe, void *pdst, u32 timeout)
{
   sb_SVC(PIGPWS)
}

NI bool smxu_PipePut8(PICB_PTR pipe, u8 b)
{
   sb_SVC(PIP8)
}

NI u32 smxu_PipePut8M(PICB_PTR pipe, u8 *bp, u32 lim)
{
   sb_SVC(PIP8M)
}

NI void smxu_PipePutPkt(PICB_PTR pipe, void *psrc)
{
   sb_SVC(PIPP)
}

NI bool smxu_PipePutPktWait(PICB_PTR pipe, void *psrc, u32 timeout, u32 mode)
{
   sb_SVC(PIPPW)
}

NI void smxu_PipePutPktWaitStop(PICB_PTR pipe, void *psrc, u32 timeout, u32 mode)
{
   sb_SVC(PIPPWS)
}

NI u32 smxu_PipePeek(PICB_PTR pipe, SMX_PK_PAR par)
{
   sb_SVC(PIP)
}

NI bool smxu_PipeResume(PICB_PTR pipe)
{
   sb_SVC(PIR)
}

NI MCB_PTR smxu_PMsgGetHeap(u32 sz, u8** bpp, u8 sn, u32 attr, u32 hn, MCB_PTR* mhp)
{
   sb_SVCHG4(PMGH)
}

NI MCB_PTR smxu_PMsgGetPool(PCB_PTR pool, u8** bpp, u8 sn, u32 attr, MCB_PTR* mhp)
{
   sb_SVCG4(PMGP)
}

NI MCB_PTR smxu_PMsgMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name, MCB_PTR* mhp)
{
   sb_SVCG4(PMM)
}

NI MCB_PTR smxu_PMsgReceive(XCB_PTR xchg, u8 **bpp, u8 sn, u32 timeout, MCB_PTR* mhp)
{
   sb_SVCG4(PMR)
}

NI void smxu_PMsgReceiveStop(XCB_PTR xchg, u8 **bpp, u8 sn, u32 timeout, MCB_PTR* mhp)
{
   sb_SVCG4(PMRS)
}

NI bool smxu_PMsgRel(MCB_PTR* mhp, u16 clrsz)
{
   sb_SVCH(PMRL)
}

NI bool smxu_PMsgReply(MCB_PTR pmsg)
{
   sb_SVC(PMRP)
}

NI bool smxu_PMsgSend(MCB_PTR pmsg, XCB_PTR xchg, u8 pri, void *reply)
{
   sb_SVC(PMS)
}

NI bool smxu_PMsgSendB(MCB_PTR pmsg, XCB_PTR xchg, u8 pri, void *reply)
{
   sb_SVC(PMSB)
}

NI bool smxu_SemClear(SCB_PTR sem)
{
   sb_SVC(SC)
}

NI SCB_PTR smxu_SemCreate(SMX_SEM_MODE mode, u8 lim, const char *name, SCB_PTR* shp)
{
   sb_SVC(SCR)
}

NI bool smxu_SemDelete(SCB_PTR* shp)
{
   sb_SVC(SD)
}

NI u32 smxu_SemPeek(SCB_PTR sem, SMX_PK_PAR par)
{
   sb_SVC(SP)
}

NI bool smxu_SemSignal(SCB_PTR sem)
{
   sb_SVC(SS)
}

NI bool smxu_SemTest(SCB_PTR sem, u32 timeout)
{
   sb_SVC(ST)
}

NI void smxu_SemTestStop(SCB_PTR sem, u32 timeout)
{
   sb_SVC(STS)
}

NI u32 smxu_SysPeek(SMX_PK_PAR par)
{
   sb_SVC(SPK)
}

NI void* smxu_SysPseudoHandleCreate(void)
{
   sb_SVC(SPHC)
}

NI void smxu_SysTest(u32 test)
{
   sb_SVC(SYT)
}

NI SMX_CBTYPE smxu_SysWhatIs(void *h)
{
   sb_SVC(SWI)
}

NI bool smxu_TaskBump(TCB_PTR task, u8 pri)
{
   sb_SVC(TB)
}

NI TCB_PTR smxu_TaskCreate(FUN_PTR fun, u8 pri, u32 tlssz_ssz, u32 fl_hn, 
                                    const char *name, u8* bp, TCB_PTR* thp)
{
   sb_SVCHG4(TCR)
}

NI TCB_PTR smxu_TaskCurrent(void)
{
   sb_SVC(TC)
}

NI bool smxu_TaskDelete(TCB_PTR* thp)
{
   sb_SVCH(TD)
}

NI void* smxu_TaskLocate(const TCB_PTR task)
{
   sb_SVC(TL)
}

NI bool smxu_TaskLock(void)
{
   sb_SVC(TLK)
}

NI bool smxu_TaskLockClear(void)
{
   sb_SVC(TLKC)
}

NI u32 smxu_TaskPeek(TCB_PTR task, SMX_PK_PAR par)
{
   sb_SVC(TP)
}

NI bool smxu_TaskResume(TCB_PTR task)
{
   sb_SVC(TR)
}

NI bool smxu_TaskSet(TCB_PTR task, SMX_ST_PAR par, u32 val1, u32 val2)
{
   sb_SVC(TSET)
}

NI bool smxu_TaskSleep(u32 time)
{
   sb_SVC(TSL)
}

NI void smxu_TaskSleepStop(u32 time)
{
   sb_SVC(TSLS)
}

NI bool smxu_TaskStart(TCB_PTR task, u32 par)
{
   sb_SVC(TS)
}

NI bool smxu_TaskStartNew(TCB_PTR task, u32 par, u8 pri, FUN_PTR fun)
{
   sb_SVC(TSN)
}

NI bool smxu_TaskStop(TCB_PTR task, u32 timeout)
{
   sb_SVC(TSO)
}

NI bool smxu_TaskSuspend(TCB_PTR task, u32 timeout)
{
   sb_SVC(TSU)
}

NI bool smxu_TaskUnlock(void)
{
   sb_SVC(TU)
}

NI bool smxu_TaskUnlockQuick(void)
{
   sb_SVC(TUQ)
}

NI bool smxu_TaskYield(void)
{
   sb_SVC(TY)
}

NI bool smxu_TimerDup(TMRCB_PTR *tmrbp, TMRCB_PTR tmra, const char *name)
{
   sb_SVC(TMRD)
}

NI u32 smxu_TimerPeek(TMRCB_PTR tmr, SMX_PK_PAR par)
{
   sb_SVC(TMRP)
}

NI bool smxu_TimerReset(TMRCB_PTR tmr, u32 *tlp)
{
   sb_SVC(TMRR)
}

NI bool smxu_TimerSetPulse(TMRCB_PTR tmr, u32 period, u32 width)
{
   sb_SVC(TMRSP)
}

NI bool smxu_TimerStart(TMRCB_PTR *tmhp, u32 delay, u32 period, LSR_PTR lsr, 
                                                               const char *name) 
{
   sb_SVCG4(TMRS)
}

NI bool smxu_TimerStartAbs(TMRCB_PTR *tmhp, u32 time, u32 period, LSR_PTR lsr, 
                                                               const char *name)
{
   sb_SVCG4(TMRSA)
}

NI bool smxu_TimerStop(TMRCB_PTR tmr, u32 *tlp)
{
   sb_SVC(TMRSO)
}

#if SMX_CFG_EVB
NI void smxu_EVBLogUser4(void *h, u32 p1, u32 p2, u32 p3, u32 p4)
{
   sb_SVCG4(ELU4)
}

NI void smxu_EVBLogUser5(void *h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5)
{
   sb_SVCG4(ELU5)
}

NI void smxu_EVBLogUser6(void *h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6)
{
   sb_SVCG4(ELU6)
}

NI void smxu_EVBLogUserPrint(u32 time, u32 index)
{
   sb_SVC(ELUP)
}
#endif

NI bool sbu_IRQClear(int irq_num)
{
   sb_SVC(IRQC)
}

NI bool sbu_IRQMask(int irq_num)
{
   sb_SVC(IRQM)
}

NI bool sbu_IRQUnmask(int irq_num)
{
   sb_SVC(IRQU)
}

NI u32 sbu_Peek(SB_PK_PAR par)
{
   sb_SVC(PK)
}

NI u32 sbu_PtimeGet(void)
{
   sb_SVC(PTMG)
}

NI void sbu_MsgOut(u8 mtype, const char *mp)
{
   sb_SVC(MO)
}

NI bool mpu_MPACreate(TCB_PTR task, MPA* tmp, u32 tmsk, u32 mpasz)
{
   sb_SVC(MPAC)
}

NI bool mpu_MPACreateLSR(LCB_PTR lsr, MPA* tmp, u32 tmsk, u32 mpasz)
{
   sb_SVC(MPACL)
}

NI bool mpu_MPASlotMove(u8 dn, u8 sn)
{
   sb_SVC(MPASM)
}

#if SMX_CFG_PORTAL
NI bool mpu_FPortalClose(FPCS* pch, u8 xsn)
{
   sb_SVCH(FPC)
}

NI bool mpu_FPortalOpen(FPCS* pch, u8 csn, u32 msz, u32 nmsg,
                                                u32 tmo, const char* rxname)
{
   sb_SVCHG4(FPO)
}

NI MCB_PTR mpu_FPortalReceive(FPCS* pch, u8** dpp)
{
   sb_SVC(FPR)
}

NI bool mpu_FPortalSend(FPCS* pch, MCB* pmsg)
{
   sb_SVC(FPS)
}

NI bool mpu_FTPortalSend(FPCS* pch, u8* bp, MCB* pmsg)
{
   sb_SVC(FTPS)
}

NI void mpu_PortalEM(PS* ph, PERRNO errno, PERRNO* ep)
{
   sb_SVC(POEM)
}

NI void mpu_PortalLog(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6)
{
   sb_SVCG4(POL)
}

NI void mpu_PortalRet(u32 id, u32 rv)
{
   sb_SVC(POR)
}

NI bool mpu_TPortalClose(TPCS* pch)
{
   sb_SVC(TPC)
}

NI bool mpu_TPortalOpen(TPCS* pch, u32 msz, u32 thsz, u32 tmo, 
                                       const char* ssname, const char* csname)
{
   sb_SVCG4da(DTPO)
}

NI bool mpu_TPortalReceive(TPCS* pch, u8* dp, u32 rqsz, u32 tmo)
{
   sb_SVCda(DTPR)
}

NI bool mpu_TPortalSend(TPCS* pch, u8* dp, u32 rqsz, u32 tmo)
{
   sb_SVCda(DTPS)
}

#endif /* SMX_CFG_PORTAL */

#if defined(MW_FATFS) && defined(SB_CPU_STM32)
NI u8 sbu_BSP_SD_Init(void)
{
   sb_SVC(BSP_SDI)
}
#endif

NI void smxu_EM(SMX_ERRNO errno, u8 sev)
{
   sb_SVC(EM)
}

/* set deferred action function */
void mp_SetDAF(u32 n)
{
   switch (n)
   {
      case DTPO:
         smx_ct->daf = (u32)&mp_TPortalOpen;
         break;
      case DTPR:
         smx_ct->daf = (u32)&mp_TPortalReceive;
         break;
      case DTPS:
         smx_ct->daf = (u32)&mp_TPortalSend;
         break;
   }
   smx_ct->flags.da_enter = 1;
   smx_PENDSVH();          /* trigger PSVH */
}

/* 
   Notes:
   1. When adding new services:
      a. Add unique acronym to ssndx enum, wherever convenient.
      b. Put function name into smx_sst[] in same place as ssndx.
      c. Add smxu_ shell function using above acronym.
      d. Put function prototype into xapiu.h followed by mapping macro.
      f. Add #undef for mapping macro to xapip.h.
   2. smxu_Heap calls assume that the heap mutex is free. If it is, the call
      executes and returns. If not, and if no timeout is specified, the call 
      fails and returns 0. If a timeout is specified, the current task is 
      suspended on the heap mutex. If the timeout elapses, the call fails and
      returns 0. If the mutex is gained, -2 is returned and a recall executes 
      and returns. (-2 is used because -1 is a valid return for some heap 
      functions.)
*/
#endif /* SMX_CFG_SSMX */
