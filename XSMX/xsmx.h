/*
* xsmx.h                                                    Version 5.4.0
*
* smx internal functions and macros. Not for use in application code.
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
* Authors: Alan Moore, Ralph Moore
*
*****************************************************************************/

#ifndef SMX_XSMX_H
#define SMX_XSMX_H

#include "smx.h"

#if SMX_CFG_SSMX
#pragma section_prefix = ".sys"
#endif

/*===========================================================================*
*                        INTERNAL FUNCTION PROTOTYPES                        *
*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/* control block test functions */
bool     smx_ObjectCreateTest(u32* hp);
u32      smx_ObjectCreateTestH(u32* hp);
bool     smx_ObjectDupTest(u32* hp);
bool     smx_BCBTest(BCB_PTR blk, bool priv);
bool     smx_EGCBTest(EGCB_PTR eg, bool priv);
bool     smx_EQCBTest(EQCB_PTR eq, bool priv);
bool     smx_LCBTest(LCB_PTR lsr, bool priv);
bool     smx_MCBTest(MCB_PTR msg, bool priv);
bool     smx_MCBOnrTest(MCB_PTR msg, bool priv);
bool     smx_MUCBTest(MUCB_PTR mtx, bool priv);
bool     smx_PCBTest(PCB_PTR pool, bool priv);
bool     smx_PICBTest(PICB_PTR pipe, bool priv);
bool     smx_SCBTest(SCB_PTR sem, bool priv);
bool     smx_TCBTest(TCB_PTR task, bool priv);
bool     smx_TMRCBTest(TMRCB_PTR tmr, bool priv);
bool     smx_XCBTest(XCB_PTR xchg, bool priv);

/* profiling functions */
#if SMX_CFG_PROFILE
void     smx_RTC_ISRStart(void);
void     smx_RTC_ISREnd(void);
void     smx_RTC_LSRStart(void);
#endif
#if SMX_CFG_PROFILE || SMX_CFG_RTLIM
void     smx_RTC_LSREnd(void);
void     smx_RTC_TaskStart(void);
void     smx_RTC_TaskStartID(void);
void     smx_RTC_TaskEnd(void);
#endif

/* queuing functions */
void     smx_DQMsg(MCB_PTR m);
MCB_PTR  smx_DQFMsg(CB_PTR q);
void     smx_NQMsg(CB_PTR q, MCB_PTR m);
void     smx_PNQMsg(CB_PTR q, MCB_PTR m, u32 cb);
TCB_PTR  smx_DQFTask(CB_PTR q);
void     smx_DQRQTask(TCB_PTR t);
void     smx_DQTask(TCB_PTR t);
u32      smx_DQTimer(TMRCB_PTR tmr);
void     smx_NQRQTask(TCB_PTR t);
void     smx_NQTask(CB_PTR q, TCB_PTR t);
void     smx_NQTimer(TMRCB_PTR tmr, u32 delay);
void     smx_PNQTask(CB_PTR q, TCB_PTR t, u32 cb);
bool     smx_ReQTask(TCB_PTR t);
bool     smx_QTest(CB_PTR q, SMX_CBTYPE qh_type, SMX_CBTYPE qm_type);

/* SSR enter functions */
void     smx_SSREnter0(u32 id);
void     smx_SSREnter1(u32 id, u32 p1);
void     smx_SSREnter2(u32 id, u32 p1, u32 p2);
void     smx_SSREnter3(u32 id, u32 p1, u32 p2, u32 p3);
void     smx_SSREnter4(u32 id, u32 p1, u32 p2, u32 p3, u32 p4);
void     smx_SSREnter5(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5);
void     smx_SSREnter6(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6);
void     smx_SSREnter7(u32 id, u32 p1, u32 p2, u32 p3, u32 p4, u32 p5, u32 p6, u32 p7);

/* other functions */
u32      smx_BlockRelAll_F(TCB_PTR task);
void     smx_EM(SMX_ERRNO errno, u8 sev=0);  /* error manager */
void     smx_EMC(SMX_ERRNO errno, u8 sev);   /* error manager call */
void     smx_EMClear(void);                  /* error manager clear */
void     smx_EMHook(SMX_ERRNO errno, void* t, u8 sev);
bool     smx_EMInit(void);                   /* initialize error mgr */
bool     smx_InMS(void);                     /* in main stack */
void     smx_KeepTimeLSRMain(u32 par);
MCB_PTR  smx_MsgReceive_F(XCB_PTR xchg, u8** bpp, u32 timeout, MCB_PTR* mhp);
bool     smx_MsgRel_F(MCB_PTR msg, u16 clrsz);
u32      smx_MsgRelAll_F(TCB_PTR task);
void     smx_RelPoolStack(TCB_PTR task);
bool     smx_SchedRunLSRs(u32 reload);    /* LSR scheduler */
void     smx_SchedRunTasks(void);         /* task scheduler */
u32      smx_SSRExit(u32 ret, u32 id);    /* SSR exit */
u32      smx_SSRExitIF(u32 ret);          /* SSR exit internal function */
void     smx_StackScan(void);             /* scan a stack to set HWM */
u32      smx_SVC(u32 ssr_id);             /* invoke SWI SSR */
void     smx_TaskDeleteLSRMain(u32 taskp);
void     smx_TaskTimeout(u32 etime);
void     smx_TimerTimeout(void);
void     smx_TimeoutLSRMain(u32 par);
void     smx_TimeoutSet(TCB_PTR task, u32 timeout);
#if SMX_CFG_TOKENS
bool     smx_TokenTest(TCB_PTR task, u32 hp, bool priv);  /* verify task has token */
#endif

#ifdef __cplusplus
}
#endif

/*===========================================================================*
*                             INTERNAL MACROS                                *
*===========================================================================*/

/* error macros */
#define smx_ERROR(errno, sev) smx_EMC(errno, sev);

#define smx_ERROR_EXIT(errno, exitval, sev, id)  /* for use in SSRs */ \
            { \
               smx_ERROR(errno, sev); \
               smx_SSRExit((u32)exitval, id); \
               return(exitval); /* <3> */ \
            }

#define smx_ERROR_RET(errno, val, sev)  /* for use in functions */ \
            { \
               smx_ERROR(errno, sev); \
               return(val); \
            }

#define smx_ERROR_RET_VOID(errno, sev)  /* for use in functions, or SSRs before smx_SSR_ENTER() */ \
            { \
               smx_ERROR(errno, sev); \
               return; \
            }

#if defined(SMX_DEBUG)
#define smx_EXIT_IF_IN_ISR(id, exitval)  /* for use in SSRs */ \
            { \
               if ((smx_GetPSR() & 0x1FF) >= 0xF) \
                  smx_ERROR_EXIT(SMXE_SSR_IN_ISR, exitval, 0, id) \
            }

#define smx_RET_IF_IN_ISR_VOID()  /* for use in SSRs before smx_SSR_ENTER() */ \
            { \
               if ((smx_GetPSR() & 0x1FF) >= 0xF) \
                  smx_ERROR_RET_VOID(SMXE_SSR_IN_ISR, 0) \
            }
#else
#define smx_EXIT_IF_IN_ISR(id, exitval)
#define smx_RET_IF_IN_ISR_VOID()
#endif

/* profile macros */
#if SMX_CFG_PROFILE || SMX_CFG_RTLIM
#define smx_RTC_TASK_START()    smx_RTC_TaskStart();
#define smx_RTC_TASK_START_ID() smx_RTC_TaskStartID();
#define smx_RTC_TASK_END()      smx_RTC_TaskEnd();
#define smx_RTC_LSR_END()       smx_RTC_LSREnd();
#else
#define smx_RTC_TASK_START()
#define smx_RTC_TASK_START_ID()
#define smx_RTC_TASK_END()
#define smx_RTC_LSR_END()
#endif
#if SMX_CFG_PROFILE
#define smx_RTC_ISR_START()     smx_RTC_ISRStart();
#define smx_RTC_ISR_END()       smx_RTC_ISREnd();
#define smx_RTC_LSR_START()     smx_RTC_LSRStart();
#else
#define smx_RTC_ISR_START()
#define smx_RTC_ISR_END()
#define smx_RTC_LSR_START()
#endif

/* stack macros */
#if SMX_CFG_SSMX
#define smx_PUT_RV_IN_EXR0(task) /*<1>*/ \
            { \
               if (task->sp) \
               task->flags.rv_r0 = 1; \
            }
#else
#define smx_PUT_RV_IN_EXR0(task)
#endif

/* system service enter and exit macros <2> */
#define smx_SSR_ENTER0(id) \
            { smx_SAVE_SUSPLOC(); \
              smx_SSREnter0(id); }
#define smx_SSR_ENTER1(id, p1) \
            { smx_SAVE_SUSPLOC(); \
              smx_SSREnter1(id, (u32)p1); }
#define smx_SSR_ENTER2(id, p1, p2) \
            { smx_SAVE_SUSPLOC(); \
              smx_SSREnter2(id, (u32)p1, (u32)p2); }
#define smx_SSR_ENTER3(id, p1, p2, p3) \
            { smx_SAVE_SUSPLOC();  \
              smx_SSREnter3(id, (u32)p1, (u32)p2, (u32)p3); }
#define smx_SSR_ENTER4(id, p1, p2, p3, p4) \
            { smx_SAVE_SUSPLOC();  \
              smx_SSREnter4(id, (u32)p1, (u32)p2, (u32)p3, (u32)p4); }
#define smx_SSR_ENTER5(id, p1, p2, p3, p4, p5) \
            { smx_SAVE_SUSPLOC();  \
              smx_SSREnter5(id, (u32)p1, (u32)p2, (u32)p3, (u32)p4, (u32)p5); }
#define smx_SSR_ENTER6(id, p1, p2, p3, p4, p5, p6) \
            { smx_SAVE_SUSPLOC();  \
              smx_SSREnter6(id, (u32)p1, (u32)p2, (u32)p3, (u32)p4, (u32)p5, (u32)p6); }
#define smx_SSR_ENTER7(id, p1, p2, p3, p4, p5, p6, p7) \
            { smx_SAVE_SUSPLOC();  \
              smx_SSREnter7(id, (u32)p1, (u32)p2, (u32)p3, (u32)p4, (u32)p5, (u32)p6, (u32)p7); }

/* test macros */
#define smx_TEST_BRKNQ(q) \
   ((q) != 0 && ((((q)->cbtype >= SMX_CB_RQ) && ((q)->cbtype <= SMX_CB_MTX)) || \
    ((q)->cbtype == SMX_CB_PIPE) || ((q)->cbtype == SMX_CB_EG)))

#define smx_TEST_PRIORITY(pri) \
   (((u32)(pri) < SMX_PRI_NUM) || ((u32)(pri) == SMX_PRI_NOCHG))

#if SMX_CFG_TOKENS
#define smx_TOKEN_TEST(task, hp, priv) smx_TokenTest(task, hp, priv)
#else
#define smx_TOKEN_TEST(task, hp, priv) true
#endif

/* other macros */
#define smx_DO_CTTEST() \
            { \
               if (smx_lockctr == 0 && smx_sched == 0) \
               smx_sched = SMX_CT_TEST; \
            }

#define smx_PIPE_EMPTY(p, rp) \
            ((!(p)->flags.full)&&((p)->wp>=rp)&&(((p)->wp - rp)<(p)->width) ? true : false)

#define smx_PIPE_FULL(p, wp)  ((p)->flags.full)

#if SMX_CFG_SSMX
#define smx_TASK_OP_PERMIT(task, id) \
            if (!smx_TaskOpPermit(task)) \
               smx_ERROR_EXIT(SMXE_PRIV_VIOL, 0, 0, id); /*<3>*/
#define smx_TASK_OP_PERMIT_VAL(task, id) \
            if (!smx_TaskOpPermit(task)) \
               smx_ERROR_EXIT(SMXE_PRIV_VIOL, 0, 0, id); /*<3>*/
#else
#define smx_TASK_OP_PERMIT(task, id)
#define smx_TASK_OP_PERMIT_VAL(task, id)
#endif

/* Notes:
   1. This causes the correct return value to be passed to a task that suspended
      on a uSSR (called using the SVC handler). It is called by the complementary
      SSR that satisfies the waiting condition. For example, if a task suspends
      on smxu_SemTest() then smx_SemSignal() calls this to change the pre-set
      false return to true. This is a complexity that is needed when smx_SSRExit()
      runs in the SVC handler, because in that case, the smx_PENDSVH() trigger
      does not trigger there, but instead after the SVC handler exits, which
      is after the point of return(GetCTRV()). Normally, the task suspends
      right there where smx_PENDSVH() is called, and then the complementary
      SSR resumes it there, and return(GetCTRV()) gets the updated value.
   2. smx_SAVE_SUSPLOC() must be in the macro not inside the smx_SSREnter()
      function, because calling smx_SSREnter() overwrites LR, which is what 
      needs to be saved.
   3. return((cast)smx_SSR_EXIT(exitval, id)) is not used in smx_ERROR(),
      since it would be necessary to typecast the return value to whatever
      type the SSR returns. This would necessitate having a different
      version of this macro for each possible SSR return type.
*/
#endif /* SMX_XSMX_H */
