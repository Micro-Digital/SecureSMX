/*
* xeg.c                                                     Version 5.4.0
*
* smx Event Group Services
*
* Copyright (c) 1991-2025 Micro Digital Inc.
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

#include "xsmx.h"

/* internal subroutines */
static u32  andor_test(u32 flags, u32 mask);
static void smx_EventFlagsSearch_F(EGCB_PTR eg);
static u32  smx_EventFlagsTest_F(EGCB_PTR eg, u32 test_mask, u32 mode, 
                                             u32 post_clear_mask, u32 timeout);
static void smx_EventGroupResumeTasks_F(EGCB_PTR eg);

/*
*  smx_EventFlagsPulse()   SSR
*
*  Like smx_EventFlagsSet() except pulses event flags on and off that were
*  not already set. Post-clear masks of waiting tasks may clear already-set
*  event flags.
*/
bool smx_EventFlagsPulse(EGCB_PTR eg, u32 pulse_mask)
{
   u32 new_mask;
   bool pass;

   smx_SSR_ENTER2(SMX_ID_EF_PULSE, eg, pulse_mask);
   smx_EXIT_IF_IN_ISR(SMX_ID_EF_PULSE, false);

   /* verify that eg is valid and that current task has access permission */
   if (pass = smx_EGCBTest(eg, SMX_PRIV_LO))
   {
      /* set new flags mask and event flags */
      new_mask = pulse_mask & ~eg->flags; /* exclude flags already set */
      eg->flags |= new_mask;
      if (new_mask)
         smx_EventFlagsSearch_F(eg);
      eg->flags &= ~new_mask; /* clear flags not already set */
      /* callback */
      if (eg->cbfun)
         eg->cbfun((u32)eg);
   }
   return((bool)smx_SSRExit(pass, SMX_ID_EF_PULSE));
}

/*
*  smx_EventFlagsSet()   SSR
*
*  Pre-clears flags selected by pre_clear_mask in event group, then sets flags
*  selected by set_mask and searches for waiting tasks with matching test_masks.
*  Resumes any tasks found, then resets matching flags enabled by clear_masks
*  in resumed tasks. Does not search if set_mask has no new flags -- i.e. ones
*  not already set.
*/
bool smx_EventFlagsSet(EGCB_PTR eg, u32 set_mask, u32 pre_clear_mask)
{
   u32  new_mask;
   bool pass;

   smx_SSR_ENTER3(SMX_ID_EF_SET, eg, set_mask, pre_clear_mask);
   smx_EXIT_IF_IN_ISR(SMX_ID_EF_SET, false);

   /* verify that eg is valid and that current task has access permission */
   if (pass = smx_EGCBTest(eg, SMX_PRIV_LO))
   {
      /* pre-clear flags and set event flags */
      eg->flags &= ~pre_clear_mask;
      new_mask = set_mask & ~eg->flags; /* exclude flags already set */
      eg->flags |= new_mask;

      if (new_mask)
         smx_EventFlagsSearch_F(eg);

      /* callback */
      if (eg->cbfun)
         eg->cbfun((u32)eg);
   }
   return((bool)smx_SSRExit(pass, SMX_ID_EF_SET));
}

/*
*  smx_EventFlagsTest()   SSR
*
*  Suspend SSR version. Enters SSR, calls smx_EventFlagsTest_F(), and exits SSR.
*  Aborts if called from LSR and tmo != SMX_TMO_NOWAIT. Clears lockctr if
*  called from a task and tmo != SMX_TMO_NOWAIT.
*/
u32 smx_EventFlagsTest(EGCB_PTR eg, u32 test_mask, u32 mode, 
                                          u32 post_clear_mask, u32 timeout)
{
   u32 flags = 0;

   smx_SSR_ENTER5(SMX_ID_EF_TEST, eg, test_mask, mode, post_clear_mask, timeout);
   smx_EXIT_IF_IN_ISR(SMX_ID_EF_TEST, 0);
   if (!smx_clsr || timeout == SMX_TMO_NOWAIT)
   {
      if (timeout)
         smx_lockctr = 0;
      flags = smx_EventFlagsTest_F(eg, test_mask, mode, post_clear_mask, timeout);
   }
   else
      smx_ERROR(SMXE_WAIT_NOT_ALLOWED, 0);
   return(smx_SSRExit(flags, SMX_ID_EF_TEST));
}

/*
*  smx_EventFlagsTestStop()   SSR
*
*  Stop SSR version. If called from an LSR, reports OP_NOT_ALLOWED error and
*  returns to LSR. Else, enters SSR, stops ct, and calls smx_EventFlagsTest_F().
*  Return value is passed via taskMain(par), when task restarts.
*/
void smx_EventFlagsTestStop(EGCB_PTR eg, u32 test_mask, u32 mode,
                                             u32 post_clear_mask, u32 timeout)
{
   u32 flags;

   if (smx_clsr)
      smx_ERROR_RET_VOID(SMXE_OP_NOT_ALLOWED, 0);
   smx_RET_IF_IN_ISR_VOID();
   smx_SSR_ENTER5(SMX_ID_EF_TEST_STOP, eg, test_mask, mode, post_clear_mask, timeout);
   flags = smx_EventFlagsTest_F(eg, test_mask, mode, post_clear_mask, timeout);
  #if SMX_CFG_TOKENS
   if (smx_ct->flags.tok_ok)
  #endif
      smx_sched = SMX_CT_STOP;
   smx_SSRExit(flags, SMX_ID_EF_TEST_STOP);
}

/*
*  smx_EventGroupCreate()   SSR
*
*  Allocates an event group control block from the EGCB pool and initializes it.
*  If allocation fails because no block is available, returns NULL. Otherwise
*  returns event group handle.
*/
EGCB_PTR smx_EventGroupCreate(u32 init_mask, const char* name, EGCB_PTR* eghp)
{
   EGCB_PTR  eg;

   smx_SSR_ENTER3(SMX_ID_EG_CREATE, init_mask, name, eghp);
   smx_EXIT_IF_IN_ISR(SMX_ID_EG_CREATE, NULL);

   /* block multiple creates and verify current task has create permission */
   if ((eg = (EGCB_PTR)smx_ObjectCreateTestH((u32*)eghp)) && !smx_errno)
   {
      /* get an event group control block */
      if ((eg = (EGCB_PTR)sb_BlockGet(&smx_egcbs, 4)) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_EGCBS, NULL, 0, SMX_ID_EG_CREATE);

      /* initialize EGCB */
      eg->cbtype = SMX_CB_EG;
      eg->flags = init_mask;
      eg->eghp = eghp;
      if (name && *name)
         eg->name = name;

      /* load event group handle */
      if (eghp)
         *eghp = eg;
   }
   return((EGCB_PTR)smx_SSRExit((u32)eg, SMX_ID_EG_CREATE));
}

/*
*  smx_EventGroupDelete()   SSR
*
*  Deletes an event group created by smx_EventGroupCreate(). Resumes all
*  waiting tasks with false return values. Clears and releases its EGCB,
*  removes its name from HT, and clears its handle.
*/
bool smx_EventGroupDelete(EGCB_PTR* eghp)
{
   EGCB_PTR eg = (eghp ? *eghp : NULL);
   bool pass;

   smx_SSR_ENTER1(SMX_ID_EG_DELETE, eg);  /* record actual handle */
   smx_EXIT_IF_IN_ISR(SMX_ID_EG_DELETE, false);

   /* verify that eg is valid and that current task has access permission */
   if (pass = smx_EGCBTest(eg, SMX_PRIV_HI))
   {
      /* resume all waiting tasks and delete name from HT */
      smx_EventGroupResumeTasks_F(eg);

      /* clear and release EGCB and set event group handle to nullcb */
      sb_BlockRel(&smx_egcbs, (u8*)eg, sizeof(EGCB));
      *eghp = NULL;
   }
   return (bool)smx_SSRExit(pass, SMX_ID_EG_DELETE);
}

/*
* smx_EventGroupClear()   SSR
*
*  Clears an event group by resuming all waiting tasks with 0 return values
*  and setting internal flags = init_mask.
*/
bool smx_EventGroupClear(EGCB_PTR eg, u32 init_mask)
{
   bool pass;

   smx_SSR_ENTER2(SMX_ID_EG_CLEAR, eg, init_mask);
   smx_EXIT_IF_IN_ISR(SMX_ID_EG_CLEAR, false);

   /* verify that eg is valid and that current task has access permission */
   if (pass = smx_EGCBTest(eg, SMX_PRIV_HI))
   {
      smx_EventGroupResumeTasks_F(eg);
      eg->flags = init_mask;
   }
   return (bool)smx_SSRExit(pass, SMX_ID_EG_CLEAR);
}

/*
* smx_EventGroupPeek()   SSR
*
*  Returns the value of the argument specified.
*/
u32 smx_EventGroupPeek(EGCB_PTR eg, SMX_PK_PAR par)
{
   u32 val;
   CB_PTR nxt;

   smx_SSR_ENTER2(SMX_ID_EG_PEEK, eg, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_EG_PEEK, 0);

   /* verify that eg is valid and that current task has access permission */
   if (val = (u32)smx_EGCBTest(eg, SMX_PRIV_LO))
   {
      val = 0;
      switch (par)
      {
         case SMX_PK_FLAGS:
            val = eg->flags;
            break;
         case SMX_PK_TASK:
            if (eg->fl != 0)
               for (nxt = (CB_PTR)eg->fl; nxt != (CB_PTR)eg; nxt = nxt->fl)
                  val++;
            break;
         case SMX_PK_FIRST:
            val = (u32)eg->fl;
            break;
         case SMX_PK_NAME:
            if (eg->fl != NULL)
               val = (u32)(((TCB_PTR)(eg->fl))->name);
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_EG_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_EG_PEEK);
}

/*
*  smx_EventGroupSet()   SSR
*
*  Sets the specified event group parameter to the specified value.
*  Not permitted in umode.
*/
bool smx_EventGroupSet(EGCB_PTR eg, SMX_ST_PAR par, u32 v1, u32 v2)
{
   bool pass;

   smx_SSR_ENTER4(SMX_ID_EG_SET, eg, par, v1, v2);
   smx_EXIT_IF_IN_ISR(SMX_ID_EG_SET, false);

   /* verify that eg is valid and that current task has access permission */
   if (pass = smx_EGCBTest(eg, SMX_PRIV_HI))
   {
      #if SMX_CFG_SSMX
      /* a utask cannot make event group changes */
      if (smx_ct->flags.umode)
         smx_ERROR_EXIT(SMXE_PRIV_VIOL, false, 0, SMX_ID_EG_SET);
      #endif

      /* perform operation on event group */
      switch (par)
      {
         case SMX_ST_CBFUN:
            eg->cbfun = (CBF_PTR)v1;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_EG_SET);
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_EG_SET));
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            * 
*===========================================================================*/

/*
*  andor_test()
*
*  Tests for and/or match of test mask and flags. Probes mask from lsb to find
*  first 1 in mask. Creates an AND term submask from it plus consecutive 1's in
*  mask. AND's term with flags and compares to term. If match, saves term
*  in save. Removes term from flags and mask. Shifts mask 1 bit right to remove
*  spacer bit and continues until either flags or mask is 0. Returns all
*  matching terms in save.
*/
u32 andor_test(u32 flags, u32 mask)
{
   u32   probe = 1;  /* probe for and/or */
   u32   term = 0;
   u32   save = 0;

   while (flags && mask)
   {
      while (!(mask & probe))
      {
         probe <<= 1;
      }
      while (mask & probe)
      {
         term |= probe;
         probe <<= 1;
      }
      if ((flags & term) == term)
         save |= term;
      flags &= ~term;
      mask &= ~term;
      mask >>= 1;
      term = 0;
   }
   return save;
}

/*
*  smx_EventFlagsSearch_F()      
*
*  Searches eg task wait queue for matches and resumes matching tasks. Clears
*  flags causing matches, if enabled.
*/
void smx_EventFlagsSearch_F(EGCB_PTR eg)
{
   CB_PTR   next;
   TCB_PTR  task;
   bool     ef_and;     /* AND flag */
   bool     ef_andor;   /* AND/OR flag */
   u32      ttmsk;      /* task test mask */
   u32      tcmsk;      /* task clear mask */
   u32      sflags;     /* selected flags */
   u32      ccmsk = 0;  /* cumulative clear mask */
   bool     match = false;

   /* return if no tasks waiting */
   if (eg->fl == NULL)
      return;

   /* search eg wait queue for tasks that match flags */
   for (task = (TCB_PTR)eg->fl; task->cbtype == SMX_CB_TASK; task = (TCB_PTR)next)
   {
      ttmsk = task->sv;
      tcmsk = task->sv2;
      next = task->fl;
      ef_and = task->flags.ef_and;
      ef_andor = task->flags.ef_andor;

      /* test task for match */
      if (ef_andor)
         sflags = andor_test(eg->flags, ttmsk);
      else
      {
         sflags = eg->flags & ttmsk;  /* select flags to test */
         sflags = ((ef_and && (sflags == ttmsk)) || (!ef_and && sflags)) ? sflags : 0;
      }

      if (sflags) /* match found */
      {
         task->rv = sflags;         /* return flags causing match */
         smx_PUT_RV_IN_EXR0(task)
         task->flags.ef_and = 0;
         task->flags.ef_andor = 0;
        #if defined(SMX_TXPORT)
         if (task->afp != NULL)
            *task->afp = eg->flags; /* load event group flags */
        #endif
         ccmsk |= (sflags & tcmsk); /* accumulate flags to reset */
         smx_DQTask(task);          /* put task into ready queue */
         smx_NQRQTask(task);
         smx_timeout[task->indx] = SMX_TMO_INF;
         match = true;
      }
   }
   if (match)
      smx_DO_CTTEST();  /* test for task resume */
   eg->flags &= ~ccmsk; /* clear all flags causing matches */
}

/*
* smx_EventFlagsTest_F()      
*
*  Tests flags in event group vs. test_mask. If match, clears matching
*  flags selected by post_clear_mask, continues task, and returns flags which
*  caused match. If no match and timeout > 0, saves test_mask and
*  post_clear_mask in ct->sv and ct->sv2, FIFO enqueues task in eg wait queue, 
*  loads timeout, and suspends ct. Suspend not allowed for LSR. If no match 
*  and no timeout, fails.
*/
u32 smx_EventFlagsTest_F(EGCB_PTR eg, u32 test_mask, u32 mode, u32 post_clear_mask,
                                u32 timeout)
{
   u32      ttmsk;         /* task test mask */
   bool     ef_and;        /* AND flag */
   bool     ef_andor;      /* AND/OR flag */
   u32      sflags;        /* selected flags */
   TCB_PTR  ct = smx_ct;   /* globals optimization */

   /* verify that eg is valid and that current task has access permission */
   if (sflags = (u32)smx_EGCBTest(eg, SMX_PRIV_LO))
   {
      ttmsk = test_mask;
      if (ttmsk == 0)
         smx_ERROR_RET(SMXE_INV_PAR, 0, 0);

      ef_and = (mode == SMX_EF_AND ? true : false);
      ef_andor = (mode == SMX_EF_ANDOR ? true : false);

      /* test task for match */
      if (ef_andor)
         sflags = andor_test(eg->flags, ttmsk);
      else
      {
         sflags = eg->flags & ttmsk;  /* select flags to test */
         sflags = ((ef_and && (sflags == ttmsk)) || (!ef_and && sflags)) ? sflags : 0;
      }
      if (sflags)
         eg->flags &= ~(sflags & post_clear_mask); /* post clear selected flags */
      else
         if (timeout) /* if no match and timeout > 0, wait */
         {
            ct->sv = ttmsk;
            ct->sv2 = post_clear_mask;
            ct->flags.ef_and = ef_and;
            ct->flags.ef_andor = ef_andor;
            smx_DQRQTask(ct);
            smx_NQTask((CB_PTR)eg, ct);
            smx_sched = SMX_CT_SUSP;
            smx_TimeoutSet(ct, timeout);
         }
   }
   return sflags;
}

/*
* smx_EventGroupResumeTasks_F()      
*
*  Resumes all tasks waiting at event group with 0 return values.
*/
void smx_EventGroupResumeTasks_F(EGCB_PTR eg)
{
   TCB_PTR  task = NULL;

   /* resume all waiting tasks */
   while (eg->fl)
   {
      task = smx_DQFTask((CB_PTR)eg);
      smx_NQRQTask(task);
      task->rv = 0;
      smx_PUT_RV_IN_EXR0(task)
      task->sv = 0;
      smx_timeout[task->indx] = SMX_TMO_INF;
   }
   if (task)
      smx_DO_CTTEST();
}

