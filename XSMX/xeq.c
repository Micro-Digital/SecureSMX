/*
* xeq.c                                                     Version 5.4.0
*
* smx Event Queue Functions
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
* Author: Ralph Moore
*
*****************************************************************************/

#include "xsmx.h"

/* internal subroutines */
static bool smx_EventQueueClear_F(EQCB_PTR eq);
static bool smx_EventQueueCount_F(EQCB_PTR eq, u32 count, u32 timeout);
static void smx_EventQueueSignal_F(EQCB_PTR eq);

/*
*  smx_EventQueueClear()   SSR
*
*  Clears an event queue by resuming all waiting tasks with NULL return
*  values. Returns true, if successful.
*/
bool smx_EventQueueClear(EQCB_PTR eq)
{
   bool pass;

   smx_SSR_ENTER1(SMX_ID_EQ_CLEAR, eq);
   smx_EXIT_IF_IN_ISR(SMX_ID_EQ_CLEAR, false);
   pass = smx_EventQueueClear_F(eq);
   return (bool)smx_SSRExit(pass, SMX_ID_EQ_CLEAR);
}

/*
*  smx_EventQueueCount()   SSR
*
*  Suspend SSR version. If called from an LSR, reports OP_NOT_ALLOWED
*  and returns to LSR. If timeout == 0, reports OP_NOT_ALLOWED and
*  returns to LSR. Else, enters SSR, clears lockctr and calls 
*  smx_EventQueueCount_F()
*/
bool smx_EventQueueCount(EQCB_PTR eq, u32 count, u32 timeout)
{
   bool pass;

   if (smx_clsr || timeout == 0)
      smx_ERROR_RET(SMXE_OP_NOT_ALLOWED, false, 0);

   smx_SSR_ENTER3(SMX_ID_EQ_COUNT, eq, count, timeout);
   smx_EXIT_IF_IN_ISR(SMX_ID_EQ_COUNT, false);
   smx_lockctr = 0;
   pass = smx_EventQueueCount_F(eq, count, timeout);
   return((bool)smx_SSRExit(pass, SMX_ID_EQ_COUNT));
}

/*
*  smx_EventQueueCountStop()   SSR
*
*  Stop SSR version. If called from an LSR, reports OP_NOT_ALLOWED and
*  returns to LSR. If timeout == 0, reports OP_NOT_ALLOWED and
*  returns to task. Else, enters SSR, sets sched = STOP, calls
*  smx_EventQueueCount_F(), then exits SSR. Return value is passed via
*  taskMain(par), when task restarts.
*/
void smx_EventQueueCountStop(EQCB_PTR eq, u32 count, u32 timeout)
{
   bool pass;

   if (smx_clsr || timeout == 0)
      smx_ERROR_RET_VOID(SMXE_OP_NOT_ALLOWED, 0);
   smx_RET_IF_IN_ISR_VOID();
   smx_SSR_ENTER3(SMX_ID_EQ_COUNT_STOP, eq, count, timeout);
   pass = smx_EventQueueCount_F(eq, count, timeout);
  #if SMX_CFG_TOKENS
   if (smx_ct->flags.tok_ok)
  #endif
      smx_sched = SMX_CT_STOP;
   smx_SSRExit(pass, SMX_ID_EQ_COUNT_STOP);
}

/*
*  smx_EventQueueCreate()   SSR
*
*  Allocates an event queue control block from the EQCB pool and initializes it.
*  If allocation fails because no block is available, returns NULL. Otherwise
*  returns event queue handle.
*/
EQCB_PTR smx_EventQueueCreate(const char* name, EQCB_PTR* eqhp)
{
   EQCB_PTR  eq;

   smx_SSR_ENTER2(SMX_ID_EQ_CREATE, name, eqhp);
   smx_EXIT_IF_IN_ISR(SMX_ID_EQ_CREATE, NULL);

   /* block multiple creates and verify current task has create permission */
   if ((eq = (EQCB_PTR)smx_ObjectCreateTestH((u32*)eqhp)) && !smx_errno)
   {
      /* get an event queue control block */
      if ((eq = (EQCB_PTR)sb_BlockGet(&smx_eqcbs, 4)) == NULL)
         smx_ERROR_EXIT(SMXE_OUT_OF_EQCBS, NULL, 0, SMX_ID_EQ_CREATE);

      /* initialize EQCB */
      eq->cbtype = SMX_CB_EQ;
      eq->eqhp = eqhp;
      if (name && *name)
         eq->name = name;

      /* load event queue handle */
      if (eqhp)
         *eqhp = eq;
   }
   return((EQCB_PTR)smx_SSRExit((u32)eq, SMX_ID_EQ_CREATE));
}

/*
*  smx_EventQueueDelete()   SSR
*
*  This function deletes an event queue created by smx_EventQueueCreate. Resumes
*  all waiting tasks with false return values. Clears and releases its EQCB,
*  and sets its handle to NULL.
*/
bool smx_EventQueueDelete(EQCB_PTR* eqhp)
{
   bool pass;
   EQCB_PTR eq = (eqhp ? *eqhp : NULL);

   smx_SSR_ENTER1(SMX_ID_EQ_DELETE, eq);  /* record actual handle */
   smx_EXIT_IF_IN_ISR(SMX_ID_EQ_DELETE, false);

   /* resume all waiting tasks */
   if (pass = smx_EventQueueClear_F(eq))
   {
      /* release and clear EQCB and set eqhp */
      sb_BlockRel(&smx_eqcbs, (u8*)eq, sizeof(EQCB));
      *eqhp = NULL;
   }
   return((bool)smx_SSRExit(pass, SMX_ID_EQ_DELETE));
}

/*
*  smx_EventQueuePeek()   SSR
*
*  Return the specified information about eq.
*/
u32 smx_EventQueuePeek(EQCB_PTR eq, SMX_PK_PAR par)
{
   u32 val = 0;

   smx_SSR_ENTER2(SMX_ID_EQ_PEEK, eq, par);
   smx_EXIT_IF_IN_ISR(SMX_ID_EQ_PEEK, 0);

   /* verify that eq is valid and that current task has access permission */
   if (val = smx_EQCBTest(eq, SMX_PRIV_LO))
   {
      switch (par)
      {
         case SMX_PK_FIRST:
            val = (u32)eq->fl;
            break;
         case SMX_PK_LAST:
            val = (u32)eq->bl;
            break;
         case SMX_PK_NAME:
            val = (u32)eq->name;
            break;
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, 0, 0, SMX_ID_EQ_PEEK);
      }
   }
   return (u32)smx_SSRExit(val, SMX_ID_EQ_PEEK);
}

/*
*  smx_EventQueueSet()   SSR
*
*  Sets the specified event queue parameter to the specified value.
*  Not permitted in umode.
*/
bool smx_EventQueueSet(EQCB_PTR eq, SMX_ST_PAR par, u32 v1, u32 v2)
{
   bool pass;
   smx_SSR_ENTER4(SMX_ID_EQ_SET, eq, par, v1, v2);
   smx_EXIT_IF_IN_ISR(SMX_ID_EQ_SET, false);

   /* verify that eq is valid and that current task has access permission */
   if (pass = smx_EQCBTest(eq, SMX_PRIV_HI))
   {
      #if SMX_CFG_SSMX
      /* a utask cannot make event queue changes */
      if (smx_ct->flags.umode == 1)
         smx_ERROR_EXIT(SMXE_PRIV_VIOL, false, 0, SMX_ID_EQ_SET);
      #endif

      /* perform set operation on event queue */
      switch (par)
      {
         default:
            smx_ERROR_EXIT(SMXE_INV_PAR, false, 0, SMX_ID_EQ_SET);
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_EQ_SET));
}

/*
*  smx_EventQueueSignal()   SSR
*
*  If eq is a valid event queue and it is not empty, decrement the sv field of
*  first task and if 0, call smx_EventQueueSignal_F()
*/
bool smx_EventQueueSignal(EQCB_PTR eq)
{
   bool pass;
   smx_SSR_ENTER1(SMX_ID_EQ_SIGNAL, eq);
   smx_EXIT_IF_IN_ISR(SMX_ID_EQ_SIGNAL, false);

   /* verify that eq is valid and that current task has access permission */
   if (pass = smx_EQCBTest(eq, SMX_PRIV_LO))
   {
      /* if eq is not empty, decrement sv field of first task; if 0 call */
      if (eq->fl != NULL)
      {
         ((TCB_PTR)eq->fl)->sv--;
         if (((TCB_PTR)eq->fl)->sv == 0)
            smx_EventQueueSignal_F(eq);
      }
   }
   return((bool)smx_SSRExit(pass, SMX_ID_EQ_SIGNAL));
}

/*===========================================================================*
*                            INTERNAL SUBROUTINES                            *
*                            Do Not Call Directly                            *
*===========================================================================*/

/*
*  smx_EventQueueClear_F()
*
*  Resumes all tasks waiting at eq, with NULL return values. Returns true, if
*  successful.
*/
static bool smx_EventQueueClear_F(EQCB_PTR eq)
{
   bool pass;
   TCB_PTR t = NULL;

   /* verify that eq is valid and that current task has access permission */
   if (pass = smx_EQCBTest(eq, SMX_PRIV_HI))
   {
      while (eq->fl)  /* clear task queue */
      {
         t = smx_DQFTask((CB_PTR)eq);
         t->sv = 0;
         t->flags.in_eq = 0;
         smx_NQRQTask(t);
         smx_DO_CTTEST();
         smx_timeout[t->indx] = SMX_TMO_INF;
      }
   }
   return pass;
}

/*
*  smx_EventQueueCount_F()
*
*  If timeout and count are nonzero, suspend ct on eq in order by count. Load
*  the calculated differential count into ct->sv. Always returns false.
*/
static bool smx_EventQueueCount_F(EQCB_PTR eq, u32 count, u32 timeout)
{
   TCB_PTR  ct = smx_ct;          /* globals optimization */
   TCB_PTR  nt = (TCB_PTR)eq->fl; /* next task eq queue */
   bool  pass;

   /* verify that eq is valid and that current task has access permission */
   if (pass = smx_EQCBTest(eq, SMX_PRIV_LO))
   {
      if (count)
      {
         smx_DQRQTask(ct);
         if (smx_sched != SMX_CT_STOP)
            smx_sched = SMX_CT_SUSP;

         if (nt)  /* eq is not empty */
         {
            /* find position for ct */
            do
            {
               if (nt->flags.in_eq == 0)
                  smx_ERROR_RET(SMXE_BROKEN_Q, false, 0);
               if (count < nt->sv) /* put ct ahead of nt */
               {
                  nt->sv -= count;
                  break;
               }
               else /* continue search */
               {
                  count -= nt->sv;
                  nt = (TCB_PTR)nt->fl;
               }
            } while (nt != (TCB_PTR)eq);

            /* insert ct into queue ahead of nt */
            nt->bl->fl = (CB_PTR)ct;
            ct->bl = nt->bl;
            nt->bl = (CB_PTR)ct;
            ct->fl = (CB_PTR)nt;
         }
         else /* event queue is empty */
         {
            ct->fl = (ct->bl = (CB_PTR)eq);
            eq->fl = (eq->bl = ct);
         }
         ct->sv = count;
         ct->flags.in_eq = 1;
         smx_TimeoutSet(ct, timeout);
         pass = false;
      }
   }
   return pass;
}

/*
*  smx_EventQueueSignal_F()
*
*  Resume the first task in eq and any subsequent tasks which also have zero
*  diff counts. Shared between smx_EventQueueSignal() and smx_KeepTimeLSR()
*/
void smx_EventQueueSignal_F(EQCB_PTR eq)
{
   TCB_PTR  t = (TCB_PTR)eq->fl;
   smx_DO_CTTEST();
   do
   {
      eq->fl = (TCB_PTR)t->fl;
      t->fl->bl = (CB_PTR)eq;
      t->flags.in_eq = 0;
      t->rv = true;
      smx_PUT_RV_IN_EXR0(t)
      smx_NQRQTask(t);  /* q is set by macro */
      smx_timeout[t->indx] = SMX_TMO_INF;
      t = (TCB_PTR)eq->fl;
      if (t == (TCB_PTR)eq)
      {  /* event queue is now empty */
         eq->fl = NULL;
         break;   /* exit while loop */
      }
   } while (t->sv == 0);
}
