/*
* xapi.h                                                    Version 5.4.0
*
* smx system service routines, application functions, and application macros. 
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
* Authors: Ralph Moore, David Moore
*
*****************************************************************************/

#ifndef SMX_XAPI_H
#define SMX_XAPI_H

/*===========================================================================*
*                        smx System Service Routines                         *
*===========================================================================*/

#ifdef __cplusplus
extern "C" {

/*------------------- for C++ with default parameters ----------------------*/

BCB_PTR  smx_BlockGet(PCB_PTR pool, u8** bpp, u32 clrsz=0, BCB_PTR* bhp=NULL);
BCB_PTR  smx_BlockMake(PCB_PTR pool, u8* bp, BCB_PTR* bhp=NULL);
u32      smx_BlockPeek(BCB_PTR blk, SMX_PK_PAR par);
bool     smx_BlockRel(BCB_PTR blk, u16 clrsz=0);
u32      smx_BlockRelAll(TCB_PTR task);
u8*      smx_BlockUnmake(PCB_PTR* pool, BCB_PTR blk);

PCB_PTR  smx_BlockPoolCreate(u8* p, u16 num, u16 size, const char* name=NULL, PCB_PTR* php=NULL);
u8*      smx_BlockPoolDelete(PCB_PTR* php);
u32      smx_BlockPoolPeek(PCB_PTR pool, SMX_PK_PAR par);

bool     smx_CBPoolsCreate(void);

bool     smx_EventFlagsPulse(EGCB_PTR eg, u32 pulse_mask);
bool     smx_EventFlagsSet(EGCB_PTR eg, u32 set_mask, u32 pre_clear_mask);
u32      smx_EventFlagsTest(EGCB_PTR eg, u32 test_mask, u32 mode, u32 post_clear_mask, u32 timeout=SMX_TMO_DFLT);
void     smx_EventFlagsTestStop(EGCB_PTR eg, u32 test_mask, u32 mode, u32 post_clear_mask, u32 timeout=SMX_TMO_DFLT);
bool     smx_EventGroupClear(EGCB_PTR eg, u32 init_mask);
EGCB_PTR smx_EventGroupCreate(u32 init_mask, const char* name=NULL, EGCB_PTR* eghp=NULL);
bool     smx_EventGroupDelete(EGCB_PTR* eghp);
u32      smx_EventGroupPeek(EGCB_PTR eg, SMX_PK_PAR par);
bool     smx_EventGroupSet(EGCB_PTR eg, SMX_ST_PAR par, u32 v1, u32 v2=0);

bool     smx_EventQueueClear(EQCB_PTR eq);
bool     smx_EventQueueCount(EQCB_PTR eq, u32 count, u32 timeout=SMX_TMO_DFLT);
void     smx_EventQueueCountStop(EQCB_PTR eq, u32 count, u32 timeout=SMX_TMO_DFLT);
EQCB_PTR smx_EventQueueCreate(const char* name=NULL, EQCB_PTR* eqhp=NULL);
bool     smx_EventQueueDelete(EQCB_PTR* eqhp);
u32      smx_EventQueuePeek(EQCB_PTR eq, SMX_PK_PAR par);
bool     smx_EventQueueSet(EQCB_PTR eq, SMX_ST_PAR par, u32 v1, u32 v2=0);
bool     smx_EventQueueSignal(EQCB_PTR eq);

void     smx_Go(void);

u32      smx_HeapBinPeek(u32 binno, EH_PK_PAR par, u32 hn=0);
bool     smx_HeapBinScan(u32 binno, u32 fnum, u32 bnum, u32 hn=0);
bool     smx_HeapBinSeed(u32 num, u32 bsz, u32 hn=0);
bool     smx_HeapBinSort(u32 binno, u32 fnum, u32 hn=0);
void*    smx_HeapCalloc(u32 num, u32 sz, u32 an=0, u32 hn=0);
u32      smx_HeapChunkPeek(void* vp, EH_PK_PAR par, u32 hn=0);
bool     smx_HeapExtend(u32 xsz, u8* xp, u32 hn=0);
bool     smx_HeapFree(void* bp, u32 hn=0);
u32      smx_HeapInit(u32 sz, u32 dcsz, u8* hp, EHV_PTR vp, u32* bszap, HBCB* binp, u32 mode, const char* name=NULL);
void*    smx_HeapMalloc(u32 sz, u32 an=0, u32 hn=0);
u32      smx_HeapPeek(EH_PK_PAR par, u32 hn=0);
void*    smx_HeapRealloc(void* cbp, u32 sz, u32 an=0, u32 hn=0);
bool     smx_HeapRecover(u32 sz, u32 num, u32 an=0, u32 hn=0);
bool     smx_HeapScan(CCB_PTR cp, u32 fnum, u32 bnum, u32 hn=0);
bool     smx_HeapSet(EH_ST_PAR par, u32 val, u32 hn=0);

bool     smx_HTAdd(void* h, const char* name);
bool     smx_HTDelete(void* h);
void*    smx_HTGetHandle(const char* name);
const char* smx_HTGetName(void* h);
void     smx_HTInit(void);

void     smx_ISREnter(void);
void     smx_ISRExit(void);

LCB_PTR  smx_LSRCreate(FUN_PTR fun, u32 flags=SMX_FL_TRUST, const char* name=NULL, TCB_PTR htask=NULL, u32 ssz=0, LCB_PTR* lhp=NULL);
bool     smx_LSRDelete(LCB_PTR* lhp);
bool     smx_LSRInvoke(LCB_PTR lsr, u32 par=0);
void     smx_LSRInvokeF(LCB_PTR lsr, u32 par=0);
void     smx_LSRsOff(void);
bool     smx_LSRsOn(void);

bool     smx_MsgBump(MCB_PTR msg, u8 pri);
MCB_PTR  smx_MsgGet(PCB_PTR pool, u8** bpp=NULL, u16 clrsz=0, MCB_PTR* mhp=NULL);
MCB_PTR  smx_MsgMake(u8* bp, u32 bs=-1, MCB_PTR* mhp=NULL);
u32      smx_MsgPeek(MCB_PTR msg, SMX_PK_PAR par);
MCB_PTR  smx_MsgReceive(XCB_PTR xchg, u8** bpp=NULL, u32 timeout=SMX_TMO_DFLT, MCB_PTR* mhp=NULL);
void     smx_MsgReceiveStop(XCB_PTR xchg, u8** bpp=NULL, u32 timeout=SMX_TMO_DFLT, MCB_PTR* mhp=NULL);
bool     smx_MsgRel(MCB_PTR msg, u16 clrsz=0);
u32      smx_MsgRelAll(TCB_PTR task);
bool     smx_MsgSend(MCB_PTR msg, XCB_PTR xchg, u8 pri=0, void* reply=NULL);
u8*      smx_MsgUnmake(MCB_PTR msg, u32* bsp=NULL);

bool     smx_MsgXchgClear(XCB_PTR xchg);
XCB_PTR  smx_MsgXchgCreate(SMX_XMODE mode, const char* name=NULL, XCB_PTR* xhp=NULL, u8 pi=0);
bool     smx_MsgXchgDelete(XCB_PTR* xhp);
u32      smx_MsgXchgPeek(XCB_PTR xchg, SMX_PK_PAR par);
bool     smx_MsgXchgSet(XCB_PTR xchg, SMX_ST_PAR par, u32 v1, u32 v2=0);

bool     smx_MutexClear(MUCB_PTR mtx);
MUCB_PTR smx_MutexCreate(u8 pi, u8 ceiling=0, const char* name=NULL, MUCB_PTR* muhp=NULL);
bool     smx_MutexDelete(MUCB_PTR* muhp);
bool     smx_MutexFree(MUCB_PTR mtx);
bool     smx_MutexGet(MUCB_PTR mtx, u32 timeout=SMX_TMO_DFLT);
bool     smx_MutexGetFast(MUCB_PTR mtx, u32 timeout=SMX_TMO_DFLT);
void     smx_MutexGetStop(MUCB_PTR mtx, u32 timeout=SMX_TMO_DFLT);
u32      smx_MutexPeek(MUCB_PTR mtx, SMX_PK_PAR par);
bool     smx_MutexRel(MUCB_PTR mtx);
void     smx_MutexRelFast(MUCB_PTR mtx);
bool     smx_MutexSet(MUCB_PTR mtx, SMX_ST_PAR par, u32 v1, u32 v2=0);

bool     smx_PipeClear(PICB_PTR pipe);
PICB_PTR smx_PipeCreate(void* ppb, u8 width, u16 length, const char* name=NULL, PICB_PTR* php=NULL);
void*    smx_PipeDelete(PICB_PTR* php);
bool     smx_PipeGet8(PICB_PTR pipe, u8* bp);
u32      smx_PipeGet8M(PICB_PTR pipe, u8* bp, u32 lim);
bool     smx_PipeGetPkt(PICB_PTR pipe, void* pdst);
bool     smx_PipeGetPktWait(PICB_PTR pipe, void* pdst, u32 timeout=SMX_TMO_DFLT);
void     smx_PipeGetPktWaitStop(PICB_PTR pipe, void* pdst, u32 timeout=SMX_TMO_DFLT);
u32      smx_PipePeek(PICB_PTR pipe, SMX_PK_PAR par);
bool     smx_PipePut8(PICB_PTR pipe, u8 b);
u32      smx_PipePut8M(PICB_PTR pipe, u8* bp, u32 lim);
bool     smx_PipePutPkt(PICB_PTR pipe, void* psrc);
bool     smx_PipePutPktWait(PICB_PTR pipe, void* psrc, u32 timeout=SMX_TMO_DFLT, SMX_PIPE_MODE mode=SMX_PUT_TO_BACK);
void     smx_PipePutPktWaitStop(PICB_PTR pipe, void* psrc, u32 timeout=SMX_TMO_DFLT, SMX_PIPE_MODE mode=SMX_PUT_TO_BACK);
bool     smx_PipeResume(PICB_PTR pipe);
bool     smx_PipeSet(PICB_PTR pipe, SMX_ST_PAR par, u32 v1, u32 v2=0);

#if SMX_CFG_SSMX
u8*      smx_PBlockGetHeap(u32 sz, u8 sn, u32 attr, const char* name=NULL, u32 hn=0);
u8*      smx_PBlockGetPool(PCB_PTR pool, u8 sn, u32 attr, const char* name=NULL);
bool     smx_PBlockMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name=NULL);
bool     smx_PBlockRelHeap(u8* bp, u8 sn, u32 hn=0);
bool     smx_PBlockRelPool(u8* bp, u8 sn, PCB_PTR pool=0, u32 clrsz=0);
MCB_PTR  smx_PMsgGetHeap(u32 sz, u8** bpp, u8 sn, u32 attr, u32 hn=0, MCB_PTR* mhp=NULL);
MCB_PTR  smx_PMsgGetPool(PCB_PTR pool, u8** bpp, u8 sn, u32 attr, MCB_PTR* mhp=NULL);
MCB_PTR  smx_PMsgMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name=NULL, MCB_PTR* mhp=NULL);
MCB_PTR  smx_PMsgReceive(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout=SMX_TMO_DFLT, MCB_PTR* mhp=NULL);
void     smx_PMsgReceiveStop(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout=SMX_TMO_DFLT, MCB_PTR* mhp=NULL);
bool     smx_PMsgRel(MCB_PTR* mhp, u16 clrsz=0);
bool     smx_PMsgReply(MCB_PTR pmsg);
bool     smx_PMsgSend(MCB_PTR pmsg, XCB_PTR xchg, u8 pri=0, void* reply=NULL);
bool     smx_PMsgSendB(MCB_PTR pmsg, XCB_PTR xchg, u8 pri=0, void* reply=NULL);
#endif

bool     smx_SemClear(SCB_PTR sem);
SCB_PTR  smx_SemCreate(SMX_SEM_MODE mode, u8 lim, const char* name=NULL, SCB_PTR* shp=NULL);
bool     smx_SemDelete(SCB_PTR* shp);
u32      smx_SemPeek(SCB_PTR sem, SMX_PK_PAR par);
bool     smx_SemSet(SCB_PTR sem, SMX_ST_PAR par, u32 v1, u32 v2=0);
bool     smx_SemSignal(SCB_PTR sem);
bool     smx_SemTest(SCB_PTR sem, u32 timeout=SMX_TMO_DFLT);
void     smx_SemTestStop(SCB_PTR sem, u32 timeout=SMX_TMO_DFLT);

u32      smx_SysPeek(SMX_PK_PAR par);
bool     smx_SysPowerDown(u32 power_mode);
void*    smx_SysPseudoHandleCreate(void);
void     smx_SysTest(u32 test);
SMX_CBTYPE  smx_SysWhatIs(void* h);

bool     smx_TaskBump(TCB_PTR task, u8 pri);
TCB_PTR  smx_TaskCreate(FUN_PTR fun, u8 pri, u32 tlssz_ssz, u32 fl_hn, const char* name=NULL, u8* bp=NULL, TCB_PTR* thp=NULL);
TCB_PTR  smx_TaskCurrent(void);
bool     smx_TaskDelete(TCB_PTR* thp);
void*    smx_TaskLocate(const TCB_PTR task);
bool     smx_TaskLock(void);
bool     smx_TaskLockClear(void);
u32      smx_TaskPeek(TCB_PTR task, SMX_PK_PAR par);
bool     smx_TaskResume(TCB_PTR task);
bool     smx_TaskSet(TCB_PTR task, SMX_ST_PAR par, u32 v1=0, u32 v2=0);
bool     smx_TaskSleep(u32 time);
void     smx_TaskSleepStop(u32 time);
bool     smx_TaskStart(TCB_PTR task, u32 par=0);
bool     smx_TaskStartNew(TCB_PTR task, u32 par, u8 pri, FUN_PTR fun);
bool     smx_TaskStop(TCB_PTR task, u32 timeout=SMX_TMO_INF);
bool     smx_TaskSuspend(TCB_PTR task, u32 timeout=SMX_TMO_INF);
bool     smx_TaskUnlock(void);
bool     smx_TaskUnlockQuick(void);
#if SMX_CFG_SSMX
bool     smx_TaskYield(void);
#endif

bool     smx_TimerDup(TMRCB_PTR* tmrbp, TMRCB_PTR tmra, const char* name=NULL);
u32      smx_TimerPeek(TMRCB_PTR tmr, SMX_PK_PAR par);
bool     smx_TimerReset(TMRCB_PTR tmr, u32* tlp=NULL);
bool     smx_TimerSetLSR(TMRCB_PTR tmr, LCB_PTR lsr, SMX_TMR_OPT opt, u32 par=0);
bool     smx_TimerSetPulse(TMRCB_PTR tmr, u32 period, u32 width);
bool     smx_TimerStart(TMRCB_PTR* tmhp, u32 delay, u32 period, LCB_PTR lsr, const char* name=NULL);
bool     smx_TimerStartAbs(TMRCB_PTR* tmhp, u32 time, u32 period, LCB_PTR lsr, const char* name=NULL);
bool     smx_TimerStop(TMRCB_PTR tmr, u32* tlp=NULL);
}
#else  /*--------------- for C without default parameters -------------------*/

BCB_PTR  smx_BlockGet(PCB_PTR pool, u8** bpp, u32 clrsz, BCB_PTR* bhp);
BCB_PTR  smx_BlockMake(PCB_PTR pool, u8* bp, BCB_PTR* bhp);
u32      smx_BlockPeek(BCB_PTR blk, SMX_PK_PAR par);
bool     smx_BlockRel(BCB_PTR blk, u16 clrsz);
u32      smx_BlockRelAll(TCB_PTR task);
u8*      smx_BlockUnmake(PCB_PTR* pool, BCB_PTR blk);

PCB_PTR  smx_BlockPoolCreate(u8* p, u16 num, u16 size, const char* name, PCB_PTR* php);
u8*      smx_BlockPoolDelete(PCB_PTR* php);
u32      smx_BlockPoolPeek(PCB_PTR pool, SMX_PK_PAR par);

bool     smx_CBPoolsCreate(void);

bool     smx_EventFlagsPulse(EGCB_PTR eg, u32 pulse_mask);
bool     smx_EventFlagsSet(EGCB_PTR eg, u32 set_mask, u32 pre_clear_mask);
u32      smx_EventFlagsTest(EGCB_PTR eg, u32 test_mask, u32 mode, u32 post_clear_mask, u32 timeout);
void     smx_EventFlagsTestStop(EGCB_PTR eg, u32 test_mask, u32 mode, u32 post_clear_mask, u32 timeout);
bool     smx_EventGroupClear(EGCB_PTR eg, u32 init_mask);
EGCB_PTR smx_EventGroupCreate(u32 init_mask, const char* name, EGCB_PTR* eghp);
bool     smx_EventGroupDelete(EGCB_PTR* eghp);
u32      smx_EventGroupPeek(EGCB_PTR eg, SMX_PK_PAR par);
bool     smx_EventGroupSet(EGCB_PTR eg, SMX_ST_PAR par, u32 v1, u32 v2);

bool     smx_EventQueueClear(EQCB_PTR eq);
bool     smx_EventQueueCount(EQCB_PTR eq, u32 count, u32 timeout);
void     smx_EventQueueCountStop(EQCB_PTR eq, u32 count, u32 timeout);
EQCB_PTR smx_EventQueueCreate(const char* name, EQCB_PTR* eqhp);
bool     smx_EventQueueDelete(EQCB_PTR* eqhp);
u32      smx_EventQueuePeek(EQCB_PTR eq, SMX_PK_PAR par);
bool     smx_EventQueueSet(EQCB_PTR eq, SMX_ST_PAR par, u32 v1, u32 v2);
bool     smx_EventQueueSignal(EQCB_PTR eq);

void     smx_Go(void);

u32      smx_HeapBinPeek(u32 binno, EH_PK_PAR par, u32 hn);
bool     smx_HeapBinScan(u32 binno, u32 fnum, u32 bnum, u32 hn);
bool     smx_HeapBinSeed(u32 num, u32 bsz, u32 hn);
bool     smx_HeapBinSort(u32 binno, u32 fnum, u32 hn);
void*    smx_HeapCalloc(u32 num, u32 sz, u32 an, u32 hn);
u32      smx_HeapChunkPeek(void* vp, EH_PK_PAR par, u32 hn);
bool     smx_HeapExtend(u32 xsz, u8* xp, u32 hn);
bool     smx_HeapFree(void* bp, u32 hn);
u32      smx_HeapInit(u32 sz, u32 dcsz, u8* hp, EHV_PTR vp, u32* bszap, HBCB* binp, u32 mode, const char* name);
void*    smx_HeapMalloc(u32 sz, u32 an, u32 hn);
u32      smx_HeapPeek(EH_PK_PAR par, u32 hn);
void*    smx_HeapRealloc(void* cbp, u32 sz, u32 an, u32 hn);
bool     smx_HeapRecover(u32 sz, u32 num, u32 an, u32 hn);
bool     smx_HeapScan(CCB_PTR cp, u32 fnum, u32 bnum, u32 hn);
bool     smx_HeapSet(EH_ST_PAR par, u32 val, u32 hn);

bool     smx_HTAdd(void* h, const char* name);
bool     smx_HTDelete(void* h);
void*    smx_HTGetHandle(const char* name);
const char* smx_HTGetName(void* h);
void     smx_HTInit(void);

LCB_PTR  smx_LSRCreate(FUN_PTR fun, u32 flags, TCB_PTR htask, u32 ssz, const char* name, LCB_PTR* lhp);
bool     smx_LSRDelete(LCB_PTR* lhp);
bool     smx_LSRInvoke(LCB_PTR lsr, u32 par);
void     smx_LSRInvokeF(LCB_PTR lsr, u32 par);
void     smx_LSRsOff(void);
bool     smx_LSRsOn(void);

bool     smx_MsgBump(MCB_PTR msg, u8 pri);
MCB_PTR  smx_MsgGet(PCB_PTR pool, u8** bpp, u16 clrsz, MCB_PTR* mhp);
MCB_PTR  smx_MsgMake(u8* bp, u32 bs, MCB_PTR* mhp);
u32      smx_MsgPeek(MCB_PTR msg, SMX_PK_PAR par);
MCB_PTR  smx_MsgReceive(XCB_PTR xchg, u8** bpp, u32 timeout, MCB_PTR* mhp);
void     smx_MsgReceiveStop(XCB_PTR xchg, u8** bpp, u32 timeout, MCB_PTR* mhp);
bool     smx_MsgRel(MCB_PTR msg, u16 clrsz);
u32      smx_MsgRelAll(TCB_PTR task);
bool     smx_MsgSend(MCB_PTR msg, XCB_PTR xchg, u8 pri, void* reply);
u8*      smx_MsgUnmake(MCB_PTR msg, u32* bsp);

bool     smx_MsgXchgClear(XCB_PTR xchg);
XCB_PTR  smx_MsgXchgCreate(SMX_XMODE mode, const char* name, XCB_PTR* xhp);
bool     smx_MsgXchgDelete(XCB_PTR* xhp);
u32      smx_MsgXchgPeek(XCB_PTR xchg, SMX_PK_PAR par);
bool     smx_MsgXchgSet(XCB_PTR xchg, SMX_ST_PAR par, u32 v1, u32 v2);

bool     smx_MutexClear(MUCB_PTR mtx);
MUCB_PTR smx_MutexCreate(u8 pi, u8 ceiling, const char* name, MUCB_PTR* muhp);
bool     smx_MutexDelete(MUCB_PTR* muhp);
bool     smx_MutexFree(MUCB_PTR mtx);
bool     smx_MutexGet(MUCB_PTR mtx, u32 timeout);
bool     smx_MutexGetFast(MUCB_PTR mtx, u32 timeout);
void     smx_MutexGetStop(MUCB_PTR mtx, u32 timeout);
bool     smx_MutexRel(MUCB_PTR mtx);
u32      smx_MutexPeek(MUCB_PTR mtx, SMX_PK_PAR par);
void     smx_MutexRelFast(MUCB_PTR mtx);
bool     smx_MutexSet(MUCB_PTR mtx, SMX_ST_PAR par, u32 v1, u32 v2);

bool     smx_PipeClear(PICB_PTR pipe);
PICB_PTR smx_PipeCreate(void* ppb, u8 width, u16 length, const char* name, PICB_PTR* php);
void*    smx_PipeDelete(PICB_PTR* php);
bool     smx_PipeGet8(PICB_PTR pipe, u8* bp);
u32      smx_PipeGet8M(PICB_PTR pipe, u8* bp, u32 lim);
bool     smx_PipeGetPkt(PICB_PTR pipe, void* pdst);
bool     smx_PipeGetPktWait(PICB_PTR pipe, void* pdst, u32 timeout);
void     smx_PipeGetPktWaitStop(PICB_PTR pipe, void* pdst, u32 timeout);
u32      smx_PipePeek(PICB_PTR pipe, SMX_PK_PAR par);
bool     smx_PipePut8(PICB_PTR pipe, u8 b);
u32      smx_PipePut8M(PICB_PTR pipe, u8* bp, u32 lim);
bool     smx_PipePutPkt(PICB_PTR pipe, void* psrc);
bool     smx_PipePutPktWait(PICB_PTR pipe, void* psrc, u32 timeout, u32 mode);
void     smx_PipePutPktWaitStop(PICB_PTR pipe, void* psrc, u32 timeout, u32 mode);
bool     smx_PipeResume(PICB_PTR pipe);
bool     smx_PipeSet(PICB_PTR pipe, SMX_ST_PAR par, u32 v1, u32 v2);

#if SMX_CFG_SSMX
u8*      smx_PBlockGetHeap(u32 sz, u8 sn, u32 attr, const char* name, u32 hn);
u8*      smx_PBlockGetPool(PCB_PTR pool, u8 sn, u32 attr, const char* name);
bool     smx_PBlockMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name);
bool     smx_PBlockRelHeap(u8* bp, u8 sn, u32 hn);
bool     smx_PBlockRelPool(u8* bp, u8 sn, PCB_PTR pool, u32 clrsz);
MCB_PTR  smx_PMsgGetHeap(u32 sz, u8** bpp, u8 sn, u32 attr, u32 hn, MCB_PTR* mhp);
MCB_PTR  smx_PMsgGetPool(PCB_PTR pool, u8** bpp, u8 sn, u32 attr, MCB_PTR* mhp);
MCB_PTR  smx_PMsgMake(u8* bp, u32 sz, u8 sn, u32 attr, const char* name, MCB_PTR* mhp);
MCB_PTR  smx_PMsgReceive(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout, MCB_PTR* mhp);
void     smx_PMsgReceiveStop(XCB_PTR xchg, u8** bpp, u8 dsn, u32 timeout, MCB_PTR* mhp);
bool     smx_PMsgRel(MCB_PTR* mhp, u16 clrsz);
bool     smx_PMsgReply(MCB_PTR pmsg);
bool     smx_PMsgSend(MCB_PTR pmsg, XCB_PTR xchg, u8 pri, void* reply);
bool     smx_PMsgSendB(MCB_PTR pmsg, XCB_PTR xchg, u8 pri, void* reply);
#endif

bool     smx_SemClear(SCB_PTR sem);
SCB_PTR  smx_SemCreate(SMX_SEM_MODE mode, u8 lim, const char* name, SCB_PTR* shp);
bool     smx_SemDelete(SCB_PTR* shp);
u32      smx_SemPeek(SCB_PTR sem, SMX_PK_PAR par);
bool     smx_SemSet(SCB_PTR sem, SMX_ST_PAR par, u32 v1, u32 v2);
bool     smx_SemSignal(SCB_PTR sem);
bool     smx_SemTest(SCB_PTR sem, u32 timeout);
void     smx_SemTestStop(SCB_PTR sem, u32 timeout);

u32      smx_SysPeek(SMX_PK_PAR par);
bool     smx_SysPowerDown(u32 power_mode);
void*    smx_SysPseudoHandleCreate(void);
SMX_CBTYPE  smx_SysWhatIs(void* h);

bool     smx_TaskBump(TCB_PTR task, u8 pri);
TCB_PTR  smx_TaskCreate(FUN_PTR fun, u8 pri, u32 tlssz_ssz, u32 fl_hn, const char* name, u8* bp, TCB_PTR* thp);
TCB_PTR  smx_TaskCurrent(void);
bool     smx_TaskDelete(TCB_PTR* thp);
void*    smx_TaskLocate(const TCB_PTR task);
bool     smx_TaskLock(void);
bool     smx_TaskLockClear(void);
u32      smx_TaskPeek(TCB_PTR task, SMX_PK_PAR par);
bool     smx_TaskResume(TCB_PTR task);
bool     smx_TaskSet(TCB_PTR task, SMX_ST_PAR par, u32 v1, u32 v2);
bool     smx_TaskSleep(u32 time);
void     smx_TaskSleepStop(u32 time);
bool     smx_TaskStart(TCB_PTR task, u32 par);
bool     smx_TaskStartNew(TCB_PTR task, u32 par, u8 pri, FUN_PTR fun);
bool     smx_TaskStop(TCB_PTR task, u32 timeout);
bool     smx_TaskSuspend(TCB_PTR task, u32 timeout);
bool     smx_TaskUnlock(void);
bool     smx_TaskUnlockQuick(void);
#if SMX_CFG_SSMX
bool     smx_TaskYield(void);
#endif
bool     smx_TimerDup(TMRCB_PTR* tmrbp, TMRCB_PTR tmra, const char* name);
u32      smx_TimerPeek(TMRCB_PTR tmr, SMX_PK_PAR par);
bool     smx_TimerReset(TMRCB_PTR tmr, u32* tlp);
bool     smx_TimerSetLSR(TMRCB_PTR tmr, LCB_PTR lsr, SMX_TMR_OPT opt, u32 par);
bool     smx_TimerSetPulse(TMRCB_PTR tmr, u32 period, u32 width);
bool     smx_TimerStart(TMRCB_PTR* tmhp, u32 delay, u32 period, LCB_PTR lsr, const char* name);
bool     smx_TimerStartAbs(TMRCB_PTR* tmhp, u32 time, u32 period, LCB_PTR lsr, const char* name);
bool     smx_TimerStop(TMRCB_PTR tmr, u32* tlp);
#endif /* __cplusplus */ 

/*===========================================================================*
*                          Application Functions                             *
*===========================================================================*/

void     ainit(u32 par);            /* application initialization */
void     aexit(SMX_ERRNO errno);    /* application exit */
void     smx_EBDisplay(void);       /* error buffer display */

void     smx_EtimeRollover(void);
void     smx_ErrorLQOvf(void);      /* reports SMXE_LQ_OVFL, for assembly files */
#ifdef __cplusplus
extern "C" u32 smx_GetPSR(void);
#else
u32      smx_GetPSR(void);
#endif
void     smx_NullF(void);           /* NOP function */
bool     smx_NullSSR(void);         /* NOP SSR */
#if SMX_CFG_PROFILE
void     smx_ProfileDisplay(void);
void     smx_ProfileInit(void);
void     smx_ProfileLSRMain(u32 par);
#endif
void     smx_TickISR(void);

#if SMX_CFG_SSMX

#ifdef __cplusplus  /*---------- for C++ with default parameters -----------*/

bool     mp_MPACreate(TCB_PTR task, MPA* tmp=NULL, u32 tmsk=MP_TMSK_DFLT, u32 mpasz=MP_MPU_ACTVSZ);
bool     mp_MPACreateLSR(LCB_PTR lsr, MPA* tmp=NULL, u32 tmsk=MP_TMSK_DFLT, u32 mpasz=MP_MPU_ACTVSZ);
bool     mp_MPASlotMove(u8 dn, u8 sn);
void     mp_MPUInit(void);
extern "C" 
void     mp_MPULoad(bool task);  /* internal use */
bool     mp_MPUSlotLoad(u8 dn, u32* rp);
bool     mp_MPUSlotSwap(u8 dn, u32* rp);
u8*      mp_RegionGetHeapR(MPR_PTR rp, u32 sz, u8 sn, u32 attr, const char* name=NULL, u32 hn=0);
u8*      mp_RegionGetHeapT(TCB_PTR task, u32 sz, u8 sn, u32 attr, const char* name=NULL, u32 hn=0);
u8*      mp_RegionGetPoolR(MPR_PTR rp, PCB_PTR pool, u8 sn, u32 attr, const char* name=NULL);
u8*      mp_RegionGetPoolT(TCB_PTR task, PCB_PTR pool, u8 sn, u32 attr, const char* name=NULL);
bool     mp_RegionMakeR(MPR_PTR rp, u8* bp, u32 sz, u8 sn, u32 attr, const char* name=NULL);
bool     mp_RegionMakeT(u8* bp, u32 sz, u8 sn, u32 attr, const char* name=NULL);

#else  /*--------------- for C without default parameters -------------------*/

bool     mp_MPACreate(TCB_PTR task, MPA* tmp, u32 tmsk, u32 mpasz);
bool     mp_MPACreateLSR(LCB_PTR lsr, MPA* tmp, u32 tmsk, u32 mpasz);
bool     mp_MPASlotMove(u8 dn, u8 sn);
void     mp_MPUInit(void);
void     mp_MPULoad(bool task);  /* internal use */
bool     mp_MPUSlotLoad(u8 dn, u32* rp);
bool     mp_MPUSlotSwap(u8 dn, u32* rp);
u8*      mp_RegionGetHeapR(MPR_PTR rp, u32 sz, u8 sn, u32 attr, const char* name, u32 hn);
u8*      mp_RegionGetHeapT(TCB_PTR task, u32 sz, u8 sn, u32 attr, const char* name, u32 hn);
u8*      mp_RegionGetPoolR(MPR_PTR rp, PCB_PTR pool, u8 sn, u32 attr, const char* name);
u8*      mp_RegionGetPoolT(TCB_PTR task, PCB_PTR pool, u8 sn, u32 attr, const char* name);
bool     mp_RegionMakeR(MPR_PTR rp, u8* bp, u32 sz, u8 sn, u32 attr, const char* name);
bool     mp_RegionMakeT(u8* bp, u32 sz, u8 sn, u32 attr, const char* name);
#endif /* __cplusplus */
#endif /* SMX_CFG_SSMX */

/*===========================================================================*
*                             Application Macros                             *
*===========================================================================*/

/* conversion macros <1> */
#define  smx_ConvMsecToTicks(ms)      (((ms)*SMX_TICKS_PER_SEC + 999) / 1000) /*<2>*/
#define  smx_ConvMsecToTicksRound(ms) (((ms)*SMX_TICKS_PER_SEC + 500) / 1000) /*<3>*/
#define  smx_ConvTicksToMsec(t)       ((1000*(t) + SMX_TICKS_PER_SEC-1) / SMX_TICKS_PER_SEC) /*<2>*/
#define  smx_ConvTicksToMsecRound(t)  ((1000*(t) + SMX_TICKS_PER_SEC/2) / SMX_TICKS_PER_SEC) /*<3>*/

/* handle table */
#if defined(SMX_DEBUG)
#define  smx_HT_ADD(h, name)        smx_HTAdd(h, name)
#define  smx_HT_DELETE(h)           smx_HTDelete(h)
#else
#define  smx_HT_ADD(h, name)
#define  smx_HT_DELETE(h)
#endif

/* miscellaneous */
#define  smx_DelayMsec(ms)          smx_TaskSuspend(SMX_CT, smx_ConvMsecToTicks(ms)+1)
#define  smx_DelaySec(s)            smx_TaskSuspend(SMX_CT, ((s)*(SMX_TICKS_PER_SEC))+1)
#define  smx_DelayTicks(t)          smx_TaskSuspend(SMX_CT, (t)+1)
#define  smx_LSR_INVOKE(lsr, par)   smx_LSRInvokeF(lsr, par)

#if !SMX_CFG_SSMX
#define  smx_TaskYield()            smx_TaskBump(smx_ct, SMX_PRI_NOCHG)
#endif

/* C++ new and delete replacements <5> */
#ifdef __cplusplus
void* newx(size_t s);
typedef int (* PNHX)( size_t );
#if !defined(SMX_TXPORT)  /* <4> */
void* operator new(size_t s);
void operator delete(void* b);
#endif

PNHX set_new_handler(PNHX a_new_handler);

#endif /* __cplusplus */
#endif /* SMX_XAPI_H */

/* Notes:
   1. Some compilers report a warning if you do not use the return value
      of macros that return a value. Basically, the compiler is reporting 
      that the statement seems to be useless. The text of the message mentions 
      "side effects" or says the code has no effect.
   2. Rounds up. Precision depends on tick rate. See smx Reference Manual.
   3. Rounds to nearest.
   4. Compiler errors that linkage specification is incompatible due to
      ThreadX .h files wrapping around whole includes with extern "C" so this
      whole file gets wrapped including these new/delete operators, causing
      the error. They need to fix their .h files.
   5. These are function prototypes and inline functions for C++ memory
      allocation using global new and delete operators. It is necessary to
      use these rather than the versions provided with the compiler,
      to ensure that smx heap functions are used and that no unwanted behavior
      is present. Also see XSMX\xheap.c for implementation of these operators.
*/
