/*
* xapip.h                                                   Version 5.4.0
*
* smx API functions for pmode, only. Reverses macros in xapiu.h to
* revert to direct (non-SVC) calls.
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
* Author: David Moore
*
*****************************************************************************/

#ifndef SMX_XAPIP_H
#define SMX_XAPIP_H

/* Undefine all macros in xapipu.h to restore standard API */

#undef smx_BlockGet
#undef smx_BlockMake
#undef smx_BlockPeek
#undef smx_BlockRel
#undef smx_BlockRelAll
#undef smx_BlockUnmake

#undef smx_BlockPoolCreate
#undef smx_BlockPoolDelete
#undef smx_BlockPoolPeek

#undef smx_EventFlagsPulse
#undef smx_EventFlagsSet
#undef smx_EventFlagsTest
#undef smx_EventFlagsTestStop
#undef smx_EventGroupClear
#undef smx_EventGroupCreate
#undef smx_EventGroupDelete
#undef smx_EventGroupPeek
#undef smx_EventGroupSet

#undef smx_EventQueueClear
#undef smx_EventQueueCount
#undef smx_EventQueueCountStop
#undef smx_EventQueueCreate
#undef smx_EventQueueDelete
#undef smx_EventQueuePeek
#undef smx_EventQueueSet
#undef smx_EventQueueSignal

#undef smx_HeapBinPeek
#undef smx_HeapBinScan
#undef smx_HeapBinSeed
#undef smx_HeapBinSort
#undef smx_HeapCalloc
#undef smx_HeapChunkPeek
#undef smx_HeapExtend
#undef smx_HeapFree
#undef smx_HeapInit
#undef smx_HeapMalloc
#undef smx_HeapPeek
#undef smx_HeapRealloc
#undef smx_HeapRecover
#undef smx_HeapScan
#undef smx_HeapSet

#undef smx_HTAdd
#undef smx_HTDelete
#undef smx_HTGetHandle
#undef smx_HTGetName
#undef smx_HTInit

#undef smx_LSRCreate
#undef smx_LSRDelete
#undef smx_LSRInvoke
#undef smx_LSRsOff
#undef smx_LSRsOn

#undef smx_MsgBump
#undef smx_MsgGet
#undef smx_MsgMake
#undef smx_MsgPeek
#undef smx_MsgReceive
#undef smx_MsgReceiveStop
#undef smx_MsgRel
#undef smx_MsgRelAll
#undef smx_MsgSend
#undef smx_MsgUnmake

#undef smx_MsgXchgClear
#undef smx_MsgXchgCreate
#undef smx_MsgXchgDelete
#undef smx_MsgXchgPeek
#undef smx_MsgXchgSet

#undef smx_MutexClear
#undef smx_MutexCreate
#undef smx_MutexDelete
#undef smx_MutexFree
#undef smx_MutexGet
#undef smx_MutexGetStop
#undef smx_MutexPeek
#undef smx_MutexSet
#undef smx_MutexRel

#undef smx_PipeClear
#undef smx_PipeCreate
#undef smx_PipeDelete
#undef smx_PipeGet8
#undef smx_PipeGet8M
#undef smx_PipeGetPkt
#undef smx_PipeGetPktWait
#undef smx_PipeGetPktWaitStop
#undef smx_PipePeek
#undef smx_PipePut8
#undef smx_PipePut8M
#undef smx_PipePutPkt
#undef smx_PipePutPktWait
#undef smx_PipePutPktWaitStop
#undef smx_PipeResume
#undef smx_PipeSet

#undef smx_PBlockGetHeap
#undef smx_PBlockGetPool
#undef smx_PBlockMake
#undef smx_PBlockRelHeap
#undef smx_PBlockRelPool
#undef smx_PMsgGetHeap
#undef smx_PMsgGetPool
#undef smx_PMsgMake
#undef smx_PMsgRel
#undef smx_PMsgReceive
#undef smx_PMsgReceiveStop
#undef smx_PMsgSend
#undef smx_PMsgSendB

#undef smx_SemClear
#undef smx_SemCreate
#undef smx_SemDelete
#undef smx_SemPeek
#undef smx_SemSet
#undef smx_SemSignal
#undef smx_SemTest
#undef smx_SemTestStop

#undef smx_SysPeek
#undef smx_SysPowerDown
#undef smx_SysPseudoHandleCreate
#undef smx_SysWhatIs

#undef smx_TaskBump
#undef smx_TaskCreate
#undef smx_TaskCurrent
#undef smx_TaskDelete
#undef smx_TaskLocate
#undef smx_TaskLock
#undef smx_TaskLockClear
#undef smx_TaskPeek
#undef smx_TaskResume
#undef smx_TaskSet
#undef smx_TaskSleep
#undef smx_TaskSleepStop
#undef smx_TaskStart
#undef smx_TaskStartNew
#undef smx_TaskStop
#undef smx_TaskSuspend
#undef smx_TaskUnlock
#undef smx_TaskUnlockQuick
#undef smx_TaskYield

#undef smx_TimerDup
#undef smx_TimerPeek
#undef smx_TimerReset
#undef smx_TimerSetLSR
#undef smx_TimerSetPulse
#undef smx_TimerStart
#undef smx_TimerStartAbs
#undef smx_TimerStop

#undef smxpp_ObjGet
#undef smxpp_ObjRel

#undef smx_EVBLogUser0
#undef smx_EVBLogUser1
#undef smx_EVBLogUser2
#undef smx_EVBLogUser3
#undef smx_EVBLogUser4
#undef smx_EVBLogUser5
#undef smx_EVBLogUser6
#undef smx_EVBLogUserPrint

#undef sb_IRQClear
#undef sb_IRQMask
#undef sb_IRQUnmask
#undef sb_Peek
#undef sb_PtimeGet
#if SB_CFG_CON
#undef sb_MsgDisplay
#undef sb_MsgOut
#endif
#undef sb_TMStart
#undef sb_TMEnd

#undef mp_FPortalClose
#undef mp_FPortalOpen
#undef mp_FPortalReceive
#undef mp_FPortalSend
#undef mp_FTPortalSend
#undef mp_MPACreate
#undef mp_MPASlotMove

#undef SMX_XAPIU_H /* allow xapiu.h to be included again */
#endif /* SMX_XAPIP_H */
