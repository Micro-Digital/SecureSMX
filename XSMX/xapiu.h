/*
* xapiu.h                                                   Version 6.0.0
*
* smx API functions for umode, only
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

#ifndef SMX_XAPIU_H
#define SMX_XAPIU_H

#if SMX_CFG_SSMX

#ifndef SMX_XAPIU1_H /* xapip.h undefs SMX_XAPIU_H but not these */
#define SMX_XAPIU1_H /* so prototypes only appear once */

#pragma default_variable_attributes = @ ".ucom.bss"
extern u32 sbu_TMCal;
#pragma default_variable_attributes =

/*===========================================================================*
*                             smxu Kernel API                                *
*===========================================================================*/

#ifdef __cplusplus  /*---------- for C++ with default parameters -----------*/
extern "C" {

BCB_PTR  smxu_BlockGet(PCB_PTR pool, u8** bpp, u32 clrsz=0, BCB_PTR* bhp=NULL);
BCB_PTR  smxu_BlockMake(PCB_PTR pool, u8* bp, BCB_PTR* bhp=NULL);
u32      smxu_BlockPeek(BCB_PTR blk, SMX_PK_PAR par);
bool     smxu_BlockRel(BCB_PTR blk, u16 clrsz=0);
u32      smxu_BlockRelAll(TCB_PTR task);
u8*      smxu_BlockUnmake(PCB_PTR* pool, BCB_PTR blk);

PCB_PTR  smxu_BlockPoolCreate(u8* p, u16 num, u16 size, const char* name=NULL, PCB_PTR* php=NULL);
u8*      smxu_BlockPoolDelete(PCB_PTR* php);
u32      smxu_BlockPoolPeek(PCB_PTR pool, SMX_PK_PAR par);

bool     smxu_EventFlagsPulse(EGCB_PTR eg, u32 pulse_mask);
bool     smxu_EventFlagsSet(EGCB_PTR eg, u32 set_mask, u32 pre_clear_mask);
u32      smxu_EventFlagsTest(EGCB_PTR eg, u32 test_mask, u32 mode, u32 post_clear_mask, u32 timeout=SMX_TMO_DFLT);
void     smxu_EventFlagsTestStop(EGCB_PTR eg, u32 test_mask, u32 mode, u32 post_clear_mask, u32 timeout=SMX_TMO_DFLT);
bool     smxu_EventGroupClear(EGCB_PTR eg, u32 init_mask);
EGCB_PTR smxu_EventGroupCreate(u32 init_mask, const char* name=NULL, EGCB_PTR* eghp=NULL);
bool     smxu_EventGroupDelete(EGCB_PTR* eghp);
u32      smxu_EventGroupPeek(EGCB_PTR eg, SMX_PK_PAR par);

bool     smxu_EventQueueClear(EQCB_PTR eq);
bool     smxu_EventQueueCount(EQCB_PTR eq, u32 count, u32 timeout=SMX_TMO_DFLT);
void     smxu_EventQueueCountStop(EQCB_PTR eq, u32 count, u32 timeout=SMX_TMO_DFLT);
EQCB_PTR smxu_EventQueueCreate(const char* name=NULL, EQCB_PTR* eqhp=NULL);
bool     smxu_EventQueueDelete(EQCB_PTR* eqhp);
u32      smxu_EventQueuePeek(EQCB_PTR eq, SMX_PK_PAR par);
bool     smxu_EventQueueSet(EQCB_PTR eq, SMX_ST_PAR par, u32 v1, u32 v2=0);
bool     smxu_EventQueueSignal(EQCB_PTR eq);

u32      smxu_HeapBinPeek(u32 binno, EH_PK_PAR par, u32 hn=0);
bool     smxu_HeapBinScan(u32 binno, u32 fnum, u32 bnum, u32 hn=0);
bool     smxu_HeapBinSort(u32 binno, u32 fnum, u32 hn=0);
void*    smxu_HeapCalloc(u32 num, u32 sz, u32 an=0, u32 hn=0);
u32      smxu_HeapChunkPeek(void* vp, EH_PK_PAR par, u32 hn=0);
bool     smxu_HeapFree(void* bp, u32 hn=0);
void*    smxu_HeapMalloc(u32 sz, u32 an=0, u32 hn=0);
u32      smxu_HeapPeek(EH_PK_PAR par, u32 hn=0);
void*    smxu_HeapRealloc(void* cbp, u32 sz, u32 an=0, u32 hn=0);

bool     smxu_HTAdd(void* h, const char* name);
bool     smxu_HTDelete(void* h);
void*    smxu_HTGetHandle(const char* name);
const char* smxu_HTGetName(void* h);

LCB_PTR  smxu_LSRCreate(FUN_PTR fun, u32 flags=SMX_FL_TRUST, const char* name=NULL, TCB_PTR htask=NULL, u32 ssz=0, LCB_PTR* lhp=NULL);
bool     smxu_LSRDelete(LCB_PTR* lhp);
bool     smxu_LSRInvoke(LCB_PTR lsr, u32 par=0);
void     smxu_LSRsOff(void);
bool     smxu_LSRsOn(void);

bool     smxu_MsgBump(MCB_PTR msg, u8 pri);
MCB_PTR  smxu_MsgGet(PCB_PTR pool, u8** bpp=NULL, u16 clrsz=0, MCB_PTR* mhp=NULL);
MCB_PTR  smxu_MsgMake(u8* bp, u32 bs=-1, MCB_PTR* mhp=NULL);
u32      smxu_MsgPeek(MCB_PTR msg, SMX_PK_PAR par);
MCB_PTR  smxu_MsgReceive(XCB_PTR xchg, u8** bpp=NULL, u32 timeout=SMX_TMO_DFLT, MCB_PTR* mhp=NULL);
void     smxu_MsgReceiveStop(XCB_PTR xchg, u8** bpp=NULL, u32 timeout=SMX_TMO_DFLT, MCB_PTR* mhp=NULL);
bool     smxu_MsgRel(MCB_PTR msg, u16 clrsz=0);
u32      smxu_MsgRelAll(TCB_PTR task);
bool     smxu_MsgSend(MCB_PTR msg, XCB_PTR xchg, u8 pri, void* reply=NULL);
u8*      smxu_MsgUnmake(MCB_PTR msg, u32* bsp=NULL);

bool     smxu_MsgXchgClear(XCB_PTR xchg);
XCB_PTR  smxu_MsgXchgCreate(SMX_XMODE mode, const char* name=NULL, XCB_PTR* xhp=NULL, u8 pi=0);
bool     smxu_MsgXchgDelete(XCB_PTR* xhp);
u32      smxu_MsgXchgPeek(XCB_PTR xchg, SMX_PK_PAR par);

bool     smxu_MutexClear(MUCB_PTR mtx);
MUCB_PTR smxu_MutexCreate(u8 pi, u8 ceiling=0, const char* name=NULL, MUCB_PTR* muhp=NULL);
bool     smxu_MutexDelete(MUCB_PTR* muhp);
bool     smxu_MutexFree(MUCB_PTR mtx);
bool     smxu_MutexGet(MUCB_PTR mtx, u32 timeout=SMX_TMO_DFLT);
void     smxu_MutexGetStop(MUCB_PTR mtx, u32 timeout=SMX_TMO_DFLT);
u32      smxu_MutexPeek(MUCB_PTR mtx, SMX_PK_PAR par);
bool     smxu_MutexRel(MUCB_PTR mtx);
bool     smxu_MutexSet(MUCB_PTR mtx, SMX_ST_PAR par, u32 v1, u32 v2=0);

bool     smxu_PipeClear(PICB_PTR pipe);
PICB_PTR smxu_PipeCreate(void* ppb, u8 width, u16 length, const char* name=NULL, PICB_PTR* php=NULL);
void*    smxu_PipeDelete(PICB_PTR* php);
bool     smxu_PipeGet8(PICB_PTR pipe, u8* bp);
u32      smxu_PipeGet8M(PICB_PTR pipe, u8* bp, u32 lim);
void     smxu_PipeGetPkt(PICB_PTR pipe, void* pdst);
bool     smxu_PipeGetPktWait(PICB_PTR pipe, void* pdst, u32 timeout=SMX_TMO_DFLT);
void     smxu_PipeGetPktWaitStop(PICB_PTR pipe, void* pdst, u32 timeout=SMX_TMO_DFLT);
u32      smxu_PipePeek(PICB_PTR pipe, SMX_PK_PAR par);
bool     smxu_PipePut8(PICB_PTR pipe, u8 b);
u32      smxu_PipePut8M(PICB_PTR pipe, u8* bp, u32 lim);
void     smxu_PipePutPkt(PICB_PTR pipe, void* psrc);
bool     smxu_PipePutPktWait(PICB_PTR pipe, void* psrc, u32 timeout=SMX_TMO_DFLT, SMX_PIPE_MODE mode=SMX_PUT_TO_BACK);
void     smxu_PipePutPktWaitStop(PICB_PTR pipe, void* psrc, u32 timeout=SMX_TMO_DFLT, SMX_PIPE_MODE mode=SMX_PUT_TO_BACK);
bool     smxu_PipeResume(PICB_PTR pipe);

u8*      smxu_PBlockGetHeap(u32 sz, u8 sn, u32 attr, const char* name=NULL, u32 hn=0);
u8*      smxu_PBlockGetPool(PCB_PTR pool, u8 sn, u32 attr, const char* name=NULL);
bool     smxu_PBlockMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name=NULL);
bool     smxu_PBlockRelHeap(u8* bp, u8 sn, u32 hn=0);
bool     smxu_PBlockRelPool(u8* bp, u8 sn, PCB_PTR pool=0, u32 clrsz=0);
MCB_PTR  smxu_PMsgGetHeap(u32 sz, u8** bpp, u8 sn, u32 attr, u32 hn=0, MCB_PTR* mhp=NULL);
MCB_PTR  smxu_PMsgGetPool(PCB_PTR pool, u8** bpp, u8 sn, u32 attr, MCB_PTR* mhp=NULL);
MCB_PTR  smxu_PMsgMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name=NULL, MCB_PTR* mhp=NULL);
MCB_PTR  smxu_PMsgReceive(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout=SMX_TMO_DFLT, MCB_PTR* mhp=NULL);
void     smxu_PMsgReceiveStop(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout=SMX_TMO_DFLT, MCB_PTR* mhp=NULL);
bool     smxu_PMsgRel(MCB_PTR* mhp, u16 clrsz=0);
bool     smxu_PMsgReply(MCB_PTR pmsg);
bool     smxu_PMsgSend(MCB_PTR pmsg, XCB_PTR xchg, u8 pri=0, void* reply=NULL);
bool     smxu_PMsgSendB(MCB_PTR pmsg, XCB_PTR xchg, u8 pri=0, void* reply=NULL);

bool     smxu_SemClear(SCB_PTR sem);
SCB_PTR  smxu_SemCreate(SMX_SEM_MODE mode, u8 lim, const char* name=NULL, SCB_PTR* shp=NULL);
bool     smxu_SemDelete(SCB_PTR* shp);
u32      smxu_SemPeek(SCB_PTR sem, SMX_PK_PAR par);
bool     smxu_SemSignal(SCB_PTR sem);
bool     smxu_SemTest(SCB_PTR sem, u32 timeout=SMX_TMO_DFLT);
void     smxu_SemTestStop(SCB_PTR sem, u32 timeout=SMX_TMO_DFLT);

u32      smxu_SysPeek(SMX_PK_PAR par);
void*    smxu_SysPseudoHandleCreate(void);
void     smxu_SysTest(u32 test);
SMX_CBTYPE  smxu_SysWhatIs(void* h);

bool     smxu_TaskBump(TCB_PTR task, u8 pri);
TCB_PTR  smxu_TaskCreate(FUN_PTR fun, u8 pri, u32 tlssz_ssz, u32 fl_hn, const char* name=NULL, u8* bp=NULL, TCB_PTR* thp=NULL);
TCB_PTR  smxu_TaskCurrent(void);
bool     smxu_TaskDelete(TCB_PTR* thp);
void*    smxu_TaskLocate(const TCB_PTR task);
bool     smxu_TaskLock(void);
bool     smxu_TaskLockClear(void);
u32      smxu_TaskPeek(TCB_PTR task, SMX_PK_PAR par);
bool     smxu_TaskResume(TCB_PTR task);
bool     smxu_TaskSet(TCB_PTR task, SMX_ST_PAR par, u32 val, u32 val2=0);
bool     smxu_TaskSleep(u32 time);
void     smxu_TaskSleepStop(u32 time);
bool     smxu_TaskStart(TCB_PTR task, u32 par);
bool     smxu_TaskStartNew(TCB_PTR task, u32 par, u8 pri, FUN_PTR fun);
bool     smxu_TaskStop(TCB_PTR task, u32 timeout=SMX_TMO_INF);
bool     smxu_TaskSuspend(TCB_PTR task, u32 timeout=SMX_TMO_INF);
bool     smxu_TaskUnlock(void);
bool     smxu_TaskUnlockQuick(void);
bool     smxu_TaskYield(void);

bool     smxu_TimerDup(TMRCB_PTR* tmrbp, TMRCB_PTR tmra, const char* name=NULL);
u32      smxu_TimerPeek(TMRCB_PTR tmr, SMX_PK_PAR par);
bool     smxu_TimerReset(TMRCB_PTR tmr, u32* tlp=NULL);
bool     smxu_TimerSetLSR(TMRCB_PTR tmr, LSR_PTR lsr, SMX_TMR_OPT opt, u32 par=0);
bool     smxu_TimerSetPulse(TMRCB_PTR tmr, u32 period, u32 width);
bool     smxu_TimerStart(TMRCB_PTR* tmhp, u32 delay, u32 period, LSR_PTR lsr, const char* name=NULL);
bool     smxu_TimerStartAbs(TMRCB_PTR* tmhp, u32 time, u32 period, LSR_PTR lsr, const char* name=NULL);
bool     smxu_TimerStop(TMRCB_PTR tmr, u32* tlp=NULL);

void     smxu_EVBLogUser4(void* h, u32 p1, u32 p2, u32 p3, u32 p4);
void     smxu_EVBLogUser5(void* h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5);
void     smxu_EVBLogUser6(void* h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6);
void     smxu_EVBLogUserPrint(u32 time, u32 index);

bool     sbu_IRQClear(int irq_num);
bool     sbu_IRQMask(int irq_num);
bool     sbu_IRQUnmask(int irq_num);
u32      sbu_Peek(SB_PK_PAR par);
u32      sbu_PtimeGet(void);
#if SB_CFG_CON
void     sbu_MsgOut(u8 mtype, const char* mp);
#endif
void     sbu_TMInit(void);
void     sbu_TMStart(u32* pts);
void     sbu_TMEnd(u32 ts, u32* ptm, u32 cal=sbu_TMCal);

#if SMX_CFG_SSMX
bool     mpu_MPACreate(TCB_PTR task, MPA* tmp=NULL, u32 tmsk=MP_TMSK_DFLT, u32 mpasz=MP_MPU_ACTVSZ);
bool     mpu_MPACreateLSR(LCB_PTR lsr, MPA* tmp=NULL, u32 tmsk=MP_TMSK_DFLT, u32 mpasz=MP_MPU_ACTVSZ);
bool     mpu_MPASlotMove(u8 dn, u8 sn);

#if SMX_CFG_PORTAL
bool     mpu_FPortalClose(FPCS* pch, u8 xsn);
bool     mpu_FPortalOpen(FPCS* pch, u8 csn, u32 msz, u32 nmsg, u32 tmo=SMX_TMO_INF, const char* rxname=NULL);
MCB_PTR  mpu_FPortalReceive(FPCS* pch, u8** dpp);
bool     mpu_FPortalSend(FPCS* pch, MCB* pmsg);
bool     mpu_FTPortalSend(FPCS* pch, u8* bp, MCB* pmsg);
void     mpu_PortalEM(PS* ph, PERRNO errno, PERRNO* ep);
void     mpu_PortalLog(u32 id, u32 p1=0, u32 p2=0, u32 p3=0, u32 p4=0, u32 p5=0, u32 p6=0);
void     mpu_PortalRet(u32 id, u32 rv);
bool     mpu_TPortalClose(TPCS* pch);
bool     mpu_TPortalOpen(TPCS* pch, u32 msz, u32 thsz, u32 tmo, const char* ssname, const char* csname);
bool     mpu_TPortalReceive(TPCS* pch, u8* dp, u32 rqsz, u32 tmo);
bool     mpu_TPortalSend(TPCS* pch, u8* dp, u32 rqsz, u32 tmo);
#endif
#endif
}

#else  /*--------------- for C without default parameters -------------------*/

BCB_PTR  smxu_BlockGet(PCB_PTR pool, u8** bpp, u32 clrsz, BCB_PTR* bhp);
BCB_PTR  smxu_BlockMake(PCB_PTR pool, u8* bp, BCB_PTR* bhp);
u32      smxu_BlockPeek(BCB_PTR blk, SMX_PK_PAR par);
bool     smxu_BlockRel(BCB_PTR blk, u16 clrsz);
u32      smxu_BlockRelAll(TCB_PTR task);
u8*      smxu_BlockUnmake(PCB_PTR* pool, BCB_PTR blk);

PCB_PTR  smxu_BlockPoolCreate(u8* p, u16 num, u16 size, const char* name, PCB_PTR* php);
u8*      smxu_BlockPoolDelete(PCB_PTR* php);
u32      smxu_BlockPoolPeek(PCB_PTR pool, SMX_PK_PAR par);

bool     smxu_EventFlagsPulse(EGCB_PTR eg, u32 pulse_mask);
bool     smxu_EventFlagsSet(EGCB_PTR eg, u32 set_mask, u32 pre_clear_mask);
u32      smxu_EventFlagsTest(EGCB_PTR eg, u32 test_mask, u32 mode, u32 post_clear_mask, u32 timeout);
void     smxu_EventFlagsTestStop(EGCB_PTR eg, u32 test_mask, u32 mode, u32 post_clear_mask, u32 timeout);
bool     smxu_EventGroupClear(EGCB_PTR eg, u32 init_mask);
EGCB_PTR smxu_EventGroupCreate(u32 init_mask, const char* name, EGCB_PTR* eghp);
bool     smxu_EventGroupDelete(EGCB_PTR* eghp);
u32      smxu_EventGroupPeek(EGCB_PTR eg, SMX_PK_PAR par);

bool     smxu_EventQueueClear(EQCB_PTR eq);
bool     smxu_EventQueueCount(EQCB_PTR eq, u32 count, u32 timeout);
void     smxu_EventQueueCountStop(EQCB_PTR eq, u32 count, u32 timeout);
EQCB_PTR smxu_EventQueueCreate(const char* name, EQCB_PTR* eqhp);
bool     smxu_EventQueueDelete(EQCB_PTR* eqhp);
u32      smxu_EventQueuePeek(EQCB_PTR eq, SMX_PK_PAR par);
bool     smxu_EventQueueSet(EQCB_PTR eq, SMX_ST_PAR par, u32 v1, u32 v2);
bool     smxu_EventQueueSignal(EQCB_PTR eq);

u32      smxu_HeapBinPeek(u32 binno, EH_PK_PAR par, u32 hn);
bool     smxu_HeapBinScan(u32 binno, u32 fnum, u32 bnum, u32 hn);
bool     smxu_HeapBinSort(u32 binno, u32 fnum, u32 hn);
void*    smxu_HeapCalloc(u32 num, u32 sz, u32 an, u32 hn);
u32      smxu_HeapChunkPeek(void* vp, EH_PK_PAR par, u32 hn);
bool     smxu_HeapFree(void* bp, u32 hn);
void*    smxu_HeapMalloc(u32 sz, u32 an, u32 hn);
u32      smxu_HeapPeek(EH_PK_PAR par, u32 hn);
void*    smxu_HeapRealloc(void* cbp, u32 sz, u32 an, u32 hn);

bool     smxu_HTAdd(void* h, const char* name);
bool     smxu_HTDelete(void* h);
void*    smxu_HTGetHandle(const char* name);
const char* smxu_HTGetName(void* h);

LCB_PTR  smxu_LSRCreate(FUN_PTR fun, u32 flags, const char* name, TCB_PTR htask, u32 ssz, LCB_PTR* lhp);
bool     smxu_LSRDelete(LCB_PTR* lhp);
bool     smxu_LSRInvoke(LCB_PTR lsr, u32 par);
void     smxu_LSRsOff(void);
bool     smxu_LSRsOn(void);

bool     smxu_MsgBump(MCB_PTR msg, u8 pri);
MCB_PTR  smxu_MsgGet(PCB_PTR pool, u8** bpp, u16 clrsz, MCB_PTR* mhp);
MCB_PTR  smxu_MsgMake(u8* bp, u32 bs, MCB_PTR* mhp);
u32      smxu_MsgPeek(MCB_PTR msg, SMX_PK_PAR par);
MCB_PTR  smxu_MsgReceive(XCB_PTR xchg, u8** bpp, u32 timeout, MCB_PTR* mhp);
void     smxu_MsgReceiveStop(XCB_PTR xchg, u8** bpp, u32 timeout, MCB_PTR* mhp);
bool     smxu_MsgRel(MCB_PTR msg, u16 clrsz);
u32      smxu_MsgRelAll(TCB_PTR task);
bool     smxu_MsgSend(MCB_PTR msg, XCB_PTR xchg, u8 pri, void* reply);
u8*      smxu_MsgUnmake(MCB_PTR msg, u32* bsp);

bool     smxu_MsgXchgClear(XCB_PTR xchg);
XCB_PTR  smxu_MsgXchgCreate(SMX_XMODE mode, const char* name, XCB_PTR* xhp, u8 pi);
bool     smxu_MsgXchgDelete(XCB_PTR* xhp);
u32      smxu_MsgXchgPeek(XCB_PTR xchg, SMX_PK_PAR par);

bool     smxu_MutexClear(MUCB_PTR mtx);
MUCB_PTR smxu_MutexCreate(u8 pi, u8 ceiling, const char* name, MUCB_PTR* muhp);
bool     smxu_MutexDelete(MUCB_PTR* muhp);
bool     smxu_MutexFree(MUCB_PTR mtx);
bool     smxu_MutexGet(MUCB_PTR mtx, u32 timeout);
void     smxu_MutexGetStop(MUCB_PTR mtx, u32 timeout);
u32      smxu_MutexPeek(MUCB_PTR mtx, SMX_PK_PAR par);
bool     smxu_MutexRel(MUCB_PTR mtx);
bool     smxu_MutexSet(MUCB_PTR mtx, SMX_ST_PAR par, u32 v1, u32 v2);

bool     smxu_PipeClear(PICB_PTR pipe);
PICB_PTR smxu_PipeCreate(void* ppb, u8 width, u16 length, const char* name, PICB_PTR* php);
void*    smxu_PipeDelete(PICB_PTR* php);
bool     smxu_PipeGet8(PICB_PTR pipe, u8* bp);
u32      smxu_PipeGet8M(PICB_PTR pipe, u8* bp, u32 lim);
void     smxu_PipeGetPkt(PICB_PTR pipe, void* pdst);
bool     smxu_PipeGetPktWait(PICB_PTR pipe, void* pdst, u32 timeout);
void     smxu_PipeGetPktWaitStop(PICB_PTR pipe, void* pdst, u32 timeout);
u32      smxu_PipePeek(PICB_PTR pipe, SMX_PK_PAR par);
bool     smxu_PipePut8(PICB_PTR pipe, u8 b);
u32      smxu_PipePut8M(PICB_PTR pipe, u8* bp, u32 lim);
void     smxu_PipePutPkt(PICB_PTR pipe, void* psrc);
bool     smxu_PipePutPktWait(PICB_PTR pipe, void* psrc, u32 timeout, SMX_PIPE_MODE mode);
void     smxu_PipePutPktWaitStop(PICB_PTR pipe, void* psrc, u32 timeout, SMX_PIPE_MODE mode);
bool     smxu_PipeResume(PICB_PTR pipe);

u8*      smxu_PBlockGetHeap(u32 sz, u8 sn, u32 attr, const char* name, u32 hn);
u8*      smxu_PBlockGetPool(PCB_PTR pool, u8 sn, u32 attr, const char* name);
bool     smxu_PBlockMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name);
bool     smxu_PBlockRelHeap(u8* bp, u8 sn, u32 hn);
bool     smxu_PBlockRelPool(u8* bp, u8 sn, PCB_PTR pool, u32 clrsz);
MCB_PTR  smxu_PMsgGetHeap(u32 sz, u8** bpp, u8 sn, u32 attr, u32 hn, MCB_PTR* mhp);
MCB_PTR  smxu_PMsgGetPool(PCB_PTR pool, u8** bpp, u8 sn, u32 attr, MCB_PTR* mhp);
MCB_PTR  smxu_PMsgMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name, MCB_PTR* mhp);
MCB_PTR  smxu_PMsgReceive(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout, MCB_PTR* mhp);
void     smxu_PMsgReceiveStop(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout, MCB_PTR* mhp);
bool     smxu_PMsgRel(MCB_PTR* mhp, u16 clrsz);
bool     smxu_PMsgReply(MCB_PTR pmsg);
bool     smxu_PMsgSend(MCB_PTR pmsg, XCB_PTR xchg, u8 pri, void* reply);
bool     smxu_PMsgSendB(MCB_PTR pmsg, XCB_PTR xchg, u8 pri, void* reply);

bool     smxu_SemClear(SCB_PTR sem);
SCB_PTR  smxu_SemCreate(SMX_SEM_MODE mode, u8 lim, const char* name, SCB_PTR* shp);
bool     smxu_SemDelete(SCB_PTR* shp);
u32      smxu_SemPeek(SCB_PTR sem, SMX_PK_PAR par);
bool     smxu_SemSignal(SCB_PTR sem);
bool     smxu_SemTest(SCB_PTR sem, u32 timeout);
void     smxu_SemTestStop(SCB_PTR sem, u32 timeout);

u32      smxu_SysPeek(SMX_PK_PAR par);
void*    smxu_SysPseudoHandleCreate(void);
void     smxu_SysTest(u32 test);
SMX_CBTYPE  smxu_SysWhatIs(void* h);

bool     smxu_TaskBump(TCB_PTR task, u8 pri);
TCB_PTR  smxu_TaskCreate(FUN_PTR fun, u8 pri, u32 tlssz_ssz, u32 fl_hn, const char* name, u8* bp, TCB_PTR* thp);
TCB_PTR  smxu_TaskCurrent(void);
bool     smxu_TaskDelete(TCB_PTR* thp);
void*    smxu_TaskLocate(const TCB_PTR task);
bool     smxu_TaskLock(void);
bool     smxu_TaskLockClear(void);
u32      smxu_TaskPeek(TCB_PTR task, SMX_PK_PAR par);
bool     smxu_TaskResume(TCB_PTR task);
bool     smxu_TaskSet(TCB_PTR task, SMX_ST_PAR par, u32 val, u32 val2);
bool     smxu_TaskSleep(u32 time);
void     smxu_TaskSleepStop(u32 time);
bool     smxu_TaskStart(TCB_PTR task, u32 par);
bool     smxu_TaskStartNew(TCB_PTR task, u32 par, u8 pri, FUN_PTR fun);
bool     smxu_TaskStop(TCB_PTR task, u32 timeout);
bool     smxu_TaskSuspend(TCB_PTR task, u32 timeout);
bool     smxu_TaskUnlock(void);
bool     smxu_TaskUnlockQuick(void);
bool     smxu_TaskYield(void);

bool     smxu_TimerDup(TMRCB_PTR* tmrbp, TMRCB_PTR tmra, const char* name);
u32      smxu_TimerPeek(TMRCB_PTR tmr, SMX_PK_PAR par);
bool     smxu_TimerReset(TMRCB_PTR tmr, u32* tlp);
bool     smxu_TimerSetLSR(TMRCB_PTR tmr, LSR_PTR lsr, SMX_TMR_OPT opt, u32 par);
bool     smxu_TimerSetPulse(TMRCB_PTR tmr, u32 period, u32 width);
bool     smxu_TimerStart(TMRCB_PTR* tmhp, u32 delay, u32 period, LSR_PTR lsr, const char* name);
bool     smxu_TimerStartAbs(TMRCB_PTR* tmhp, u32 time, u32 period, LSR_PTR lsr, const char* name);
bool     smxu_TimerStop(TMRCB_PTR tmr, u32* tlp);

void     smxu_EVBLogUser4(void* h, u32 p1, u32 p2, u32 p3, u32 p4);
void     smxu_EVBLogUser5(void* h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5);
void     smxu_EVBLogUser6(void* h, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6);
void     smxu_EVBLogUserPrint(u32 time, u32 index);

bool     sbu_IRQClear(int irq_num);
bool     sbu_IRQMask(int irq_num);
bool     sbu_IRQUnmask(int irq_num);
u32      sbu_Peek(SB_PK_PAR par);
u32      sbu_PtimeGet(void);
#if SB_CFG_CON
void     sbu_MsgOut(u8 mtype, const char* mp);
#endif
void     sbu_TMInit(void);
void     sbu_TMStart(u32* pts);
void     sbu_TMEnd(u32 ts, u32* ptm, u32 cal);

#if SMX_CFG_SSMX
bool     mpu_MPACreate(TCB_PTR task, MPA* tmp, u32 tmsk, u32 mpasz);
bool     mpu_MPACreateLSR(LCB_PTR lsr, MPA* tmp, u32 tmsk, u32 mpasz);
bool     mpu_MPASlotMove(u8 dn, u8 sn);
#if SMX_CFG_PORTAL
bool     mpu_FPortalClose(FPCS* pch, u8 xsn);
bool     mpu_FPortalOpen(FPCS* pch, u8 csn, u32 msz, u32 nmsg, u32 tmo, const char* rxname);
MCB_PTR  mpu_FPortalReceive(FPCS* pch, u8** dpp);
bool     mpu_FPortalSend(FPCS* pch, MCB* pmsg);
bool     mpu_FTPortalSend(FPCS* pch, u8* bp, MCB* pmsg);
void     mpu_PortalEM(PS* ph, PERRNO errno, PERRNO* ep);
void     mpu_PortalLog(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6);
void     mpu_PortalRet(u32 id, u32 rv);
bool     mpu_TPortalClose(TPCS* pch);
bool     mpu_TPortalOpen(TPCS* pch, u32 msz, u32 thsz, u32 tmo, const char* ssname, const char* csname);
bool     mpu_TPortalReceive(TPCS* pch, u8* dp, u32 rqsz, u32 tmo);
bool     mpu_TPortalSend(TPCS* pch, u8* dp, u32 rqsz, u32 tmo);
#endif
#endif
#endif /* __cplusplus */
#endif /* SMX_XAPIU1_H */

#if SMX_CFG_MPU_ENABLE
/*===========================================================================*
*                         smx to smxu Mapping Macros                         *
*===========================================================================*/
         
#define smx_BlockGet(pool, bpp, clrsz, bhp)     smxu_BlockGet(pool, bpp, clrsz, bhp)
#define smx_BlockMake(pool, bp, bhp)            smxu_BlockMake(pool, bp, bhp)
#define smx_BlockPeek(blk, par)                 smxu_BlockPeek(blk, par)
#define smx_BlockRel(blk, clrsz)                smxu_BlockRel(blk, clrsz)
#define smx_BlockRelAll(task)                   smxu_BlockRelAll(task)
#define smx_BlockUnmake(pool, blk)              smxu_BlockUnmake(pool, blk)

#define smx_BlockPoolCreate(pp, num, sz, name, php)  smxu_BlockPoolCreate(pp, num, sz, name, php)
#define smx_BlockPoolDelete(php)                smxu_BlockPoolDelete(php)
#define smx_BlockPoolPeek(pool, par)            smxu_BlockPoolPeek(pool, par)

#define smx_EventFlagsPulse(eg, pmsk)           smxu_EventFlagsPulse(eg, pmsk)
#define smx_EventFlagsSet(eg, smsk, pcmsk)      smxu_EventFlagsSet(eg, smsk, pcmsk)
#define smx_EventFlagsTest(eg, tmsk, mode, pcmsk, tmo)  smxu_EventFlagsTest(eg, tmsk, mode, pcmsk, tmo)
#define smx_EventFlagsTestStop(eg, tmsk, mode, pcmsk, tmo)  smxu_EventFlagsTestStop(eg, tmsk, mode pcmsk, tmo)
#define smx_EventGroupClear(eg, imsk)           smxu_EventGroupClear(eg, imsk)
#define smx_EventGroupCreate(imsk, name, eghp)  smxu_EventGroupCreate(imsk, name, eghp)
#define smx_EventGroupDelete(eghp)              smxu_EventGroupDelete(eghp)
#define smx_EventGroupPeek(eg, par)             smxu_EventGroupPeek(eg, par)
#define smx_EventGroupSet(eg, par, v1, v2)      _Pragma("error\"smx_EventGroupSet() not available in umode\"")

#define smx_EventQueueClear(eq)                 smxu_EventQueueClear(eq)
#define smx_EventQueueCount(eq, cnt, tmo)       smxu_EventQueueCount(eq, cnt, tmo)
#define smx_EventQueueCountStop(eq, cnt, tmo)   smxu_EventQueueCountStop(eq, cnt, tmo)
#define smx_EventQueueCreate(name, eqhp)        smxu_EventQueueCreate(name, eqhp)
#define smx_EventQueueDelete(eqhp)              smxu_EventQueueDelete(eqhp)
#define smx_EventQueuePeek(eq, par)             smxu_EventQueuePeek(eq, par)
#define smx_EventQueueSet(eq, par, v1, v2)      _Pragma("error\"smx_EventQueueSet() not available in umode\"")
#define smx_EventQueueSignal(eq)                smxu_EventQueueSignal(eq)

#define smx_HeapBinPeek(binno, par, hn)         smxu_HeapBinPeek(binno, par, hn)
#define smx_HeapBinScan(binno, fnum, bnum, hn)  smxu_HeapBinScan(binno, fnum, bnum, hn)
#define smx_HeapBinSeed(num, bsz)               _Pragma("error\"smx_HeapBinSeed() not available in umode\"")
#define smx_HeapBinSort(binno, fnum, hn)        smxu_HeapBinSort(binno, fnum, hn)
#define smx_HeapCalloc(num, sz, an, hn)         smxu_HeapCalloc(num, sz, an, hn)
#define smx_HeapChunkPeek(vp, par, hn)          smxu_HeapChunkPeek(vp, par, hn)
#define smx_HeapExtend(xsz, xp)                 _Pragma("error\"smx_HeapExtend() not available in umode\"")
#define smx_HeapFree(bp, hn)                    smxu_HeapFree(bp, hn)
#define smx_HeapInit(sz, hp)                    _Pragma("error\"smx_HeapInit() not available in umode\"")
#define smx_HeapMalloc(sz, an, hn)              smxu_HeapMalloc(sz, an, hn)
#define smx_HeapPeek(par, hn)                   smxu_HeapPeek(par, hn)
#define smx_HeapRealloc(cbp, sz, an, hn)        smxu_HeapRealloc(cbp, sz, an, hn)
#define smx_HeapRecover(sz, fnum)               _Pragma("error\"smx_HeapRecover() not available in umode\"")
#define smx_HeapScan(cp, fnum, bnum)            _Pragma("error\"smx_HeapScan() not available in umode\"")
#define smx_HeapSet(par, val)                   _Pragma("error\"smx_HeapSet() not available in umode\"")

#define smx_HTAdd(h, name)                      smxu_HTAdd(h, name)
#define smx_HTDelete(h)                         smxu_HTDelete(h)
#define smx_HTGetHandle(name)                   smxu_HTGetHandle(name)
#define smx_HTGetName(h)                        smxu_HTGetName(h)
#define smx_HTInit()                            _Pragma("error\"smx_HTInit() not available in umode\"")

#define smx_LSRCreate(fun, flags, htask, ssz, name, lhp)  _Pragma("error\"smx_LSRCreate() not available in umode\"")
#define smx_LSRDelete(lhp)                      _Pragma("error\"smx_LSRDelete() not available in umode\"")
#define smx_LSRInvoke(lsr, par)                 _Pragma("error\"smx_LSRInvoke() not available in umode\"")
#define smx_LSRsOff(void)                       _Pragma("error\"smx_LSRsOff() not available in umode\"")
#define smx_LSRsOn(void)                        _Pragma("error\"smx_LSRsOn() not available in umode\"")

#define smx_MsgBump(msg, pri)                   smxu_MsgBump(msg, pri)
#define smx_MsgGet(pool, bpp, clrsz, mhp)       smxu_MsgGet(pool, bpp, clrsz, mhp)
#define smx_MsgMake(bp, bs, mhp)                smxu_MsgMake(bp, bs, mhp)
#define smx_MsgPeek(msg, par)                   smxu_MsgPeek(msg, par)
#define smx_MsgReceive(xchg, bpp, tmo, mhp)     smxu_MsgReceive(xchg, bpp, tmo, mhp)
#define smx_MsgReceiveStop(xchg, bpp, tmo, mhp) smxu_MsgReceiveStop(xchg, bpp, tmo, mhp)
#define smx_MsgRel(msg, clrsz)                  smxu_MsgRel(msg, clrsz)
#define smx_MsgRelAll(task)                     smxu_MsgRelAll(task)
#define smx_MsgSend(msg, xchg, pri, reply)      smxu_MsgSend(msg, xchg, pri, reply)
#define smx_MsgUnmake(msg, bsp)                 smxu_MsgUnmake(msg, bsp)

#define smx_MsgXchgClear(xchg)                  smxu_MsgXchgClear(xchg)
#define smx_MsgXchgCreate(mode, name, xhp)      smxu_MsgXchgCreate(mode, name, xhp)
#define smx_MsgXchgDelete(xhp)                  smxu_MsgXchgDelete(xhp)
#define smx_MsgXchgPeek(xchg, par)              smxu_MsgXchgPeek(xchg, par)
#define smx_MsgXchgSet(xchg, par, v1, v2)       _Pragma("error\"smx_MsgXchgSet() not available in umode\"")

#define smx_MutexClear(mtx)                     smxu_MutexClear(mtx)
#define smx_MutexCreate(pi, ceiling, name, muhp)  smxu_MutexCreate(pi, ceiling, name, muhp)
#define smx_MutexDelete(muhp)                   smxu_MutexDelete(muhp)
#define smx_MutexFree(mtx)                      smxu_MutexFree(mtx)
#define smx_MutexGet(mtx, tmo)                  smxu_MutexGet(mtx, tmo)
#define smx_MutexGetStop(mtx, tmo)              smxu_MutexGetStop(mtx, tmo)
#define smx_MutexPeek(mtx, par)                 smxu_MutexPeek(mtx, par)
#define smx_MutexRel(mtx)                       smxu_MutexRel(mtx)
#define smx_MutexSet(mtx, par, v1, v2)          _Pragma("error\"smx_MutexSet() not available in umode\"")

#define smx_PipeClear(pipe)                     smxu_PipeClear(pipe)
#define smx_PipeCreate(ppb, w, l, name, php)    smxu_PipeCreate(ppb, w, l, name, php)
#define smx_PipeDelete(php)                     smxu_PipeDelete(php)
#define smx_PipeGet8(pipe, bp)                  smxu_PipeGet8(pipe, bp)
#define smx_PipeGet8M(pipe, bp, lim)            smxu_PipeGet8M(pipe, bp, lim)
#define smx_PipeGetPkt(pipe, pdst)              smxu_PipeGetPkt(pipe, pdst)
#define smx_PipeGetPktWait(pipe, pdst, tmo)     smxu_PipeGetPktWait(pipe, pdst, tmo)
#define smx_PipeGetPktWaitStop(pipe, pdst, tmo) smxu_PipeGetPktWaitStop(pipe, pdst, tmo)
#define smx_PipePeek(pipe, par)                 smxu_PipePeek(pipe, par)
#define smx_PipePut8(pipe, b)                   smxu_PipePut8(pipe, b)
#define smx_PipePut8M(pipe, bp, lim)            smxu_PipePut8M(pipe, bp, lim)
#define smx_PipePutPkt(pipe, psrc)              smxu_PipePutPkt(pipe, psrc)
#define smx_PipePutPktWait(pipe, psrc, tmo, mode)  smxu_PipePutPktWait(pipe, psrc, tmo, mode)
#define smx_PipePutPktWaitStop(pipe, psrc, tmo, mode)  smxu_PipePutPktWaitStop(pipe, psrc, tmo, mode)
#define smx_PipeResume(pipe)                    smxu_PipeResume(pipe)
#define smx_PipeSet(pipe, par, v1, v2)          _Pragma("error\"smx_PipeSet() not available in umode\"")

#define smx_PBlockGetHeap(sz, sn, attr, name, hn) smxu_PBlockGetHeap(sz, sn, attr, name, hn)
#define smx_PBlockGetPool(pool, sn, attr, name) smxu_PBlockGetPool(pool, sn, attr, name)
#define smx_PBlockMake(bp, sz, sn, attr, name)  smxu_PBlockMake(bp, sz, sn, attr, name)
#define smx_PBlockRelHeap(bp, sn, hn)           smxu_PBlockRelHeap(bp, sn, hn)
#define smx_PBlockRelPool(bp, sn, pool, clrsz)  smxu_PBlockRelPool(bp, sn, pool, clrsz)
#define smx_PMsgGetHeap(sz, bpp, sn, attr, hn, mhp)  smxu_PMsgGetHeap(sz, bpp, sn, attr, hn, mhp)
#define smx_PMsgGetPool(pool, bpp, sn, attr, mhp)  smxu_PMsgGetPool(pool, bpp, sn, attr, mhp)
#define smx_PMsgMake(bp, sz, sn, attr, name, mhp)  smxu_PMsgMake(bp, sz, sn, attr, name, mhp)
#define smx_PMsgReceive(xchg, bpp, dsn, tmo, mhp)  smxu_PMsgReceive(xchg, bpp, dsn, tmo, mhp)
#define smx_PMsgReceiveStop(xchg, bpp, dsn, tmo, mhp) smxu_PMsgReceiveStop(xchg, bpp, dsn, tmo, mhp)
#define smx_PMsgRel(mhp, clrsz)                 smxu_PMsgRel(mhp, clrsz)
#define smx_PMsgReply(msg)                      smxu_PMsgReply(msg)
#define smx_PMsgSend(msg, xchg, pri, reply)     smxu_PMsgSend(msg, xchg, pri, reply)
#define smx_PMsgSendB(msg, xchg, pri, reply)    smxu_PMsgSendB(msg, xchg, pri, reply)

#define smx_SemClear(sem)                       smxu_SemClear(sem)
#define smx_SemCreate(mode, lim, name, shp)     smxu_SemCreate(mode, lim, name, shp)
#define smx_SemDelete(shp)                      smxu_SemDelete(shp)
#define smx_SemPeek(sem, par)                   smxu_SemPeek(sem, par)
#define smx_SemSet(sem, par, v1, v2)            _Pragma("error\"smx_SemSet() not available in umode\"")
#define smx_SemSignal(sem)                      smxu_SemSignal(sem)
#define smx_SemTest(sem, tmo)                   smxu_SemTest(sem, tmo)
#define smx_SemTestStop(sem, tmo)               smxu_SemTestStop(sem, tmo)

#define smx_SysPeek(par)                        smxu_SysPeek(par)
#define smx_SysPowerDown(power_mode)            _Pragma("error\"smx_SysPowerDown() not available in umode\"")
#define smx_SysPseudoHandleCreate()             smxu_SysPseudoHandleCreate()
#define smx_SysWhatIs(h)                        smxu_SysWhatIs(h)

#define smx_TaskBump(task, pri)                 smxu_TaskBump(task, pri)
#define smx_TaskCreate(fun, pri, tlssz_ssz, fl_hn, name, bp, thp)  smxu_TaskCreate(fun, pri, tlssz_ssz, fl_hn, name, bp, thp)
#define smx_TaskCurrent()                       smxu_TaskCurrent()
#define smx_TaskDelete(thp)                     smxu_TaskDelete(thp)
#define smx_TaskLocate(task)                    smxu_TaskLocate(task)
#define smx_TaskLock()                          _Pragma("error\"smx_TaskLock() not available in umode\"")
#define smx_TaskLockClear()                     _Pragma("error\"smx_TaskLockClear() not available in umode\"")
#define smx_TaskPeek(task, par)                 smxu_TaskPeek(task, par)
#define smx_TaskResume(task)                    smxu_TaskResume(task)
#define smx_TaskSet(task, par, val, val2)       _Pragma("error\"smx_TaskSet() not available in umode\"")
#define smx_TaskSleep(time)                     smxu_TaskSleep(time)
#define smx_TaskSleepStop(time)                 smxu_TaskSleepStop(time)
#define smx_TaskStart(task, par)                smxu_TaskStart(task, par)
#define smx_TaskStartNew(task, par, pri,  fun)  smxu_TaskStartNew(task, par, pri,  fun)
#define smx_TaskStop(task, tmo)                 smxu_TaskStop(task, tmo)
#define smx_TaskSuspend(task, tmo)              smxu_TaskSuspend(task, tmo)
#define smx_TaskUnlock()                        _Pragma("error\"smx_TaskUnlock() not available in umode\"")
#define smx_TaskUnlockQuick()                   _Pragma("error\"smx_TaskUnlockQuick() not available in umode\"")
#define smx_TaskYield()                         smxu_TaskYield()

#define smx_TimerDup(tmrbp, tmra, name)         smxu_TimerDup(tmrbp, tmra, name)
#define smx_TimerPeek(tmr, par)                 smxu_TimerPeek(tmr, par)
#define smx_TimerReset(tmr, tlp)                smxu_TimerReset(tmr, tlp)
#define smx_TimerSetLSR(tmr, lsr, opt, par)     _Pragma("error\"smx_TimerSetLSR() not available in umode\"")
#define smx_TimerSetPulse(tmr, per, width)      smxu_TimerSetPulse(tmr, per, width)
#define smx_TimerStart(tmhp, dly, per, lsr, name)  smxu_TimerStart(tmhp, dly, per, lsr, name)
#define smx_TimerStartAbs(tmhp, time, per, lsr, name) smxu_TimerStartAbs(tmhp, time, per, lsr, name)
#define smx_TimerStop(tmr, tlp)                 smxu_TimerStop(tmr, tlp)

#define mp_MPACreate(task, tmp, tmsk, mpasz)    mpu_MPACreate(task, tmp, tmsk, mpasz)
#define mp_MPACreateLSR(lsr, tmp, tmsk, mpasz)  mpu_MPACreateLSR(lsr, tmp, tmsk, mpasz)
#define mp_MPASlotMove(dn, sn)                  mpu_MPASlotMove(dn, sn)

#define mp_FPortalClose(pch, xsn)               mpu_FPortalClose(pch, xsn)
#define mp_FPortalOpen(pch, csn, msz, nmsg, tmo, rxname)  mpu_FPortalOpen(pch, csn, msz, nmsg, tmo, rxname)
#define mp_FPortalReceive(pch, dpp)             mpu_FPortalReceive(pch, dpp)
#define mp_FPortalSend(pch, pmsg)               mpu_FPortalSend(pch, pmsg)
#define mp_FTPortalSend(pch, bp, pmsg)          mpu_FTPortalSend(pch, bp, pmsg)
#define mp_PortalEM(ph, errno, ep)              mpu_PortalEM(ph, errno, ep)
#define mp_PortalLog(id, p1, p2, p3, p4, p5, p6)  mpu_PortalLog(id, p1, p2, p3, p4, p5, p6)
#define mp_PortalRet(id, rv)                    mpu_PortalRet(id, rv)
#define mp_TPortalClose(pch)                    mpu_TPortalClose(pch)
#define mp_TPortalOpen(pch, msz, thsz, tmo, ssname, csname)  mpu_TPortalOpen(pch, msz, thsz, tmo, ssname, csname)
#define mp_TPortalReceive(pch, dp, rqsz, tmo)   mpu_TPortalReceive(pch, dp, rqsz, tmo)
#define mp_TPortalSend(pch, dp, rqsz, tmo)      mpu_TPortalSend(pch, dp, rqsz, tmo)

/* only implemented User4 to save IDs, since only 256 for SVC, and > 5 pars not supported yet */
#define smx_EVBLogUser0(h)                      smxu_EVBLogUser4(h, 0, 0, 0, 0)
#define smx_EVBLogUser1(h, p1)                  smxu_EVBLogUser4(h, p1, 0, 0, 0)
#define smx_EVBLogUser2(h, p1, p2)              smxu_EVBLogUser4(h, p1, p2, 0, 0)
#define smx_EVBLogUser3(h, p1, p2, p3)          smxu_EVBLogUser4(h, p1, p2, p3, 0)
#define smx_EVBLogUser4(h, p1, p2, p3, p4)      smxu_EVBLogUser4(h, p1, p2, p3, p4)
#define smx_EVBLogUser5(h, p1, p2, p3, p4, p5)  smxu_EVBLogUser5(h, p1, p2, p3, p4, p5)
#define smx_EVBLogUser6(h, p1, p2, p3, p4, p5, p6) smxu_EVBLogUser6(h, p1, p2, p3, p4, p5, p6)
#define smx_EVBLogUserPrint(time, index)        smxu_EVBLogUserPrint(time, index)

#define sb_IRQClear(irq)                        sbu_IRQClear(irq)
#define sb_IRQMask(irq)                         sbu_IRQMask(irq)
#define sb_IRQUnmask(irq)                       sbu_IRQUnmask(irq)
#define sb_Peek(par)                            sbu_Peek(par)
#define sb_PtimeGet()                           sbu_PtimeGet()
#if SB_CFG_CON
#define sb_MsgOut(mtype, mp)                    sbu_MsgOut(mtype, mp)
#endif
#define sb_TMInit()                             sbu_TMInit()         /* in bbase.c; not SVC call */
#define sb_TMStart(pts)                         sbu_TMStart(pts)     /* in bbase.c; not SVC call */
#define sb_TMEnd(ts, ptm)                       sbu_TMEnd(ts, ptm, cal)   /* in bbase.c; not SVC call */
#endif /* SMX_CFG_MPU_ENABLE */

#undef SMX_XAPIP_H /* allow xapip.h to be included again */

#endif /* SMX_CFG_SSMX */
#endif /* SMX_XAPIU_H */

