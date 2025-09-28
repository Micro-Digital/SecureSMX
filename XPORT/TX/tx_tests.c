/*
* tx_tests.c                                                Version 5.4.0
*
* TXPort Tests
*
* Copyright (c) 1994-2025 Micro Digital Inc.
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
* For ThreadX/Azure Kernel V6.1
*
*****************************************************************************/

#include "xsmx.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tx_api.h"

/* ThreadX priorities for tests */
#define TP1 1  /* higher priority */
#define TP2 2  /* mid priority */
#define TP3 3  /* lower priority */

#define TSSZ 300

void test_block_pools(void);
void test_byte_pools(void);
void test_event_flags(void);
void test_mutexes(void);
void test_queues(void);
void test_semaphores(void);
void test_tasks(void);
void test_timers(void);
void tfail(void);
void ttxp_main(u32);
void t2a_main(void* pp);
void t2b_main(void* pp);
int  tx_main(void);

UINT           active;
ULONG          available_storage;
ULONG          count;
ULONG          current_value;
ULONG          enqueued; 
TX_THREAD      *first_suspended;
CHAR           *name;
TX_THREAD      *owner;
ULONG          remaining_ticks;
ULONG          reschedule_ticks;
void*          sp1;              /* stack pointer 1 */
void*          sp2;              /* stack pointer 2 */
ULONG          suspended_count;
TX_THREAD      t2a;
TX_THREAD      t2b;
UINT           ts_pass;
TCB_PTR        ttxp;
UINT           wait = 0;

void txp_test(void)
{
   sb_ConWriteString(0,0,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,"TXPort Test  (Built " __DATE__ " " __TIME__ ")");
   ttxp = smx_TaskCreate(ttxp_main, TP1, 400, 0, "ttxp");
   smx_TaskStart(ttxp);
}

void ttxp_main(u32)
{
   sp1 = smx_HeapMalloc(TSSZ, 3);
   sp2 = smx_HeapMalloc(TSSZ, 3);
   test_tasks();
   test_semaphores();
   test_mutexes();
   test_event_flags();
   test_block_pools();
   test_byte_pools();
   test_queues();
   test_timers();

   sb_MsgOut(SB_MSG_INFO, "TESTS DONE");
   sb_MsgDisplay();
   sb_DelayMsec(100); /* delay for last msg to display before halting */
   tx_main();
   sb_HALTEXEC()
}

void tfail(void)
{
   sb_DEBUGTRAP();
   ts_pass = false;
}

/******************************************************************************
*                                                                             *
*                             TASK FUNCTION TESTS                             *
*                                                                             *
******************************************************************************/

void tt00(void);
void tt01(void);
void tt02(void);
void tt03(void);

void test_tasks(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   tt00();
   tt01();
   tt02();
   tt03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "TASKS PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "TASKS FAILED");
   sb_MsgDisplay();
}

void tt00_t2a(ULONG par);
void tt00_t2b(ULONG par);

void tt00(void)
{
   smx_TaskLock();
   tx_thread_create(&t2a, (CHAR*)"t2a", tt00_t2a, 0x37, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_thread_create(&t2b, (CHAR*)"t2b", tt00_t2b, 0x47, sp2, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_DONT_START);
   smx_TaskUnlock();

   while (wait) {}

   /* cleanup */
   tx_thread_delete(&t2a);
   if (t2a != NULL)
      tfail();
   tx_thread_delete(&t2b);
}

void tt00_t2a(ULONG par)
{
   CHAR *name;
   UINT state;
   UINT priority;
   UINT old_priority;
   UINT time;

   /* test parameter pass */
   if (par != 0x37)
      tfail();
   /* test thread identity */
   if (t2a != *tx_thread_identify())
      tfail();
   /* test thread info get */
   if (tx_thread_info_get(&t2a, &name, &state, NULL, &priority, NULL, NULL, NULL, NULL) != TX_SUCCESS ||
                          name != "t2a" || state != TX_READY || priority != TP2)
      tfail();
   /* test thread priority change */
   smx_TaskLock();
   if (tx_thread_priority_change(&t2a, TP1, &old_priority) != TX_SUCCESS ||
                                 t2a->pri != TP1 || old_priority != TP2)
      tfail();
   /* test t2b delayed start */
   if (tx_thread_resume(&t2b) != TX_SUCCESS || t2b->state != SMX_TASK_READY)
      tfail();
   /* test t2a relinquish */
   t2a->pri = t2a->prinorm = TP2;
   count = 0;
   tx_thread_relinquish();
   smx_TaskUnlock();

   /* resume and verify that t2b ran */
   if (count != 1)
      tfail();
   /* test t2b terminate */
   if (tx_thread_terminate(&t2b) != TX_SUCCESS || t2b->state != SMX_TASK_WAIT || t2b->sp != NULL)
      tfail();
   /* test thread reset */
   if (tx_thread_reset(&t2b) != TX_SUCCESS || t2b->state != SMX_TASK_WAIT)
      tfail();
   /* test t2b resume */
   if (tx_thread_resume(&t2b) != TX_SUCCESS || t2b->state != SMX_TASK_READY)
      tfail();
   t2b->rv = 3;
   tx_thread_relinquish();

   /* resume and verify that t2b ran */
   if (count != 2)
      tfail();
   /* test thread sleep */
   time = smx_etime;
   wait = 1;
   if (tx_thread_sleep(5) != TX_SUCCESS || smx_etime != time + 5)
      tfail();
   wait = 0;
   /* test thread suspend */
   tx_thread_resume(&t2b);
   if (tx_thread_suspend(&t2b) != TX_SUCCESS || t2b->state != SMX_TASK_WAIT)
      tfail();
   /* test thread terminate */
   tx_thread_resume(&t2b);
   if (tx_thread_terminate(&t2b) != TX_SUCCESS || t2b->state != SMX_TASK_WAIT || t2b->sp != NULL)
      tfail();
   /* test thread wait abort */
   if (tx_thread_wait_abort(&t2b) != TX_SUCCESS || t2b->state != SMX_TASK_READY)
      tfail();
}

void tt00_t2b(ULONG par)
{
   count++;
   /* test that par is passed correctly for delayed start */
   if (count == 1)
      if (par != 0x47)
         tfail();
   /* test that par is changed correctly */
   if (count == 2)
      if (par != 3)
         tfail();
   /* resume t2a */
   tx_thread_relinquish();
}

/* tt01: test thread enter/exit notify */
void tt01_t2a(ULONG par);
void tt01_F(TCB_PTR *s, u32 p);

void tt01(void)
{
   smx_TaskLock();
   count = 0;
   tx_thread_create(&t2a, (CHAR*)"t2a", tt01_t2a, 0x37, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   if (tx_thread_entry_exit_notify(&t2a, tt01_F) != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* verify that callback function ran twice due t2a start and exit */
   if (count != 2)
      tfail();
   tx_thread_resume(&t2a);

   /* cleanup */
   tx_thread_delete(&t2a);
}

void tt01_t2a(ULONG par)
{
   tx_thread_suspend(&t2a);

   /* verify that callback function ran due t2a enter */
   if (count != 3)
      tfail();
}

void tt01_F(TCB_PTR *s, u32 p)
{
   count++;
}

/* tt02: test thread stack error notify. Note: smx_stk_ovfl(t) has been
   removed from smx_EM(), so count test disabled. */
void tt02_t2a(ULONG par);
void tt02_F(TCB_PTR *s);

void tt02(void)
{
   smx_TaskLock();
   count = 0;
   tx_thread_create(&t2a, (CHAR*)"t2a", tt02_t2a, 0x37, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   if (tx_thread_stack_error_notify(tt02_F) != TX_FEATURE_NOT_ENABLED)
      tfail();
   smx_TaskUnlock();

   /* verify that callback function ran due t2a stack error */
//   if (count != 1)
//      tfail();

   /* cleanup */
   tx_thread_delete(&t2a);
}

void tt02_t2a(ULONG par)
{
   t2a->shwm = t2a->ssz + 1;
}

void tt02_F(TCB_PTR *s)
{
   count++;
}

void tt03(void){}

/******************************************************************************
*                                                                             *
*                          SEMAPHORE FUNCTION TESTS                           *
*                                                                             *
******************************************************************************/

TX_SEMAPHORE   sema;

void ts00(void);
void ts01(void);
void ts02(void);
void ts03(void);

void test_semaphores(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   ts00();
   ts01();
   ts02();
   ts03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "SEMAPHORES PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "SEMAPHORES FAILED");
   sb_MsgDisplay();
}

/* ts00: test main semaphore services */
void ts00_t2a(ULONG par);
void ts00_t2b(ULONG par);

void ts00(void)
{
   smx_TaskLock();
   tx_thread_create(&t2a, (CHAR*)"t2a", ts00_t2a, 0x37, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_thread_create(&t2b, (CHAR*)"t2b", ts00_t2b, 0x37, sp2, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   if (tx_semaphore_create(&sema, (CHAR*)"sema", 1) != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* test that t2b resumed by semaphore put */
   if (tx_semaphore_put(&sema))
      tfail();
   /* test semaphore delete */
   if (tx_semaphore_delete(&sema) != TX_SUCCESS)
      tfail();

   /* cleanup */
   tx_thread_delete(&t2a);
   tx_thread_delete(&t2b);
}

void ts00_t2a(ULONG par)
{
   /* test semaphore get instance success */
   if (tx_semaphore_get(&sema, TX_NO_WAIT) != TX_SUCCESS)
      tfail();
   /* test t2a suspension on semaphore due to no instance available */
   if (tx_semaphore_get(&sema, TX_WAIT_FOREVER) != TX_SUCCESS)
      tfail();
}

void ts00_t2b(ULONG par)
{
   /* test semaphore info get for one task waiting */
   if (tx_semaphore_info_get(&sema, &name, &current_value, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "sema" || 
       current_value != 0 || first_suspended != (TX_THREAD*)t2a || suspended_count != 1)
      tfail();
   /* test ceiling put if ceiling is not exceeded */
   if (tx_semaphore_ceiling_put(&sema, 1) != TX_SUCCESS)
      tfail();
   /* test semaphore info get for no tasks waiting */
   if (tx_semaphore_info_get(&sema, &name, &current_value, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "sema" || 
       current_value != 0 || first_suspended != NULL || suspended_count != 0)
      tfail();
   /* test ceiling put if ceiling is exceeded */
   tx_semaphore_ceiling_put(&sema, 1);
   if (tx_semaphore_ceiling_put(&sema, 1) != TX_CEILING_EXCEEDED)
      tfail();
   /* test that t2b is resumed by semaphore put from ts00() */
   sema->count = 0;
   if (tx_semaphore_get(&sema, TX_WAIT_FOREVER) != TX_SUCCESS)
      tfail();
}

/* ts01: test semaphore put notify */
void ts01_t2a(ULONG par);
void ts01_F(SCB_PTR *s);

void ts01(void)
{
   smx_TaskLock();
   count = 0;
   tx_thread_create(&t2a, (CHAR*)"t2a", ts01_t2a, 0x37, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_semaphore_create(&sema, (CHAR*)"sema", 1);
   if (tx_semaphore_put_notify(&sema, ts01_F) != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* test that callback function ran due to put */
   if (count != 1)
      tfail();

   /* cleanup */
   tx_thread_delete(&t2a);
   tx_semaphore_delete(&sema);
}

void ts01_t2a(ULONG par)
{
   tx_semaphore_put(&sema);
}

void ts01_F(SCB_PTR *s)
{
   count = 1;
}

void ts02(void){}
void ts03(void){}

/******************************************************************************
*                                                                             *
*                            MUTEX FUNCTION TESTS                             *
*                                                                             *
******************************************************************************/

TX_MUTEX    mtxa;

void tm00(void);
void tm01(void);
void tm02(void);
void tm03(void);

void test_mutexes(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   tm00();
   tm01();
   tm02();
   tm03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "MUTEXES PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "MUTEXES FAILED");
   sb_MsgDisplay();
}

void tm00_t2a(ULONG par);
void tm00_t2b(ULONG par);

void tm00(void)
{
   /* create t2a, t2b, and mtxa */
   smx_TaskLock();
   tx_thread_create(&t2a, (CHAR*)"t2a", tm00_t2a, 0x37, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_thread_create(&t2b, (CHAR*)"t2b", tm00_t2b, 0x37, sp2, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   if (tx_mutex_create(&mtxa, (CHAR*)"mtxa", TX_INHERIT) != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* resume and test mutex delete */
   if (tx_mutex_delete(&mtxa) != TX_SUCCESS)
      tfail();

   /* cleanup */
   tx_thread_delete(&t2a);
   tx_thread_delete(&t2b);
}

void tm00_t2a(ULONG par)
{
   /* test mutex get success */
   if (tx_mutex_get(&mtxa, TX_NO_WAIT) != TX_SUCCESS)
      tfail();
   /* let t2b run */
   smx_TaskBump(t2a, SMX_PRI_NOCHG);

   /* resume and test mutex info get for t2b waiting */
   if (tx_mutex_info_get(&mtxa, &name, &count, &owner, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "mtxa" || count != 1 ||
       owner != (TX_THREAD*)t2a, first_suspended != (TX_THREAD*)t2b || suspended_count != 1)
      tfail();
   /* test release mtxa */
   if (tx_mutex_put(&mtxa) != TX_SUCCESS)
      tfail();
}

void tm00_t2b(ULONG par)
{
   /* verify that t2b cannot release mtxa */
   if (tx_mutex_put(&mtxa) != TX_NOT_OWNED)
      tfail();
   /* test t2b suspension on mtxa due to being owned by t2a */
   if (tx_mutex_get(&mtxa, TX_WAIT_FOREVER) != TX_SUCCESS)
      tfail();

   /* verify that t2b cannot release mtxa */

   /* resume and test mutex info get for no tasks waiting */
   if (tx_mutex_info_get(&mtxa, &name, &count, &owner, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "mtxa" || count != 0 || 
       owner != (TX_THREAD*)t2b, first_suspended != NULL || suspended_count != 0)
      tfail();
}

void tm01(void){}
void tm02(void){}
void tm03(void){}

/******************************************************************************
*                                                                             *
*                         EVENT FLAG FUNCTION TESTS                           *
*                                                                             *
******************************************************************************/

ULONG    current_flags;
TX_EVENT_FLAGS_GROUP efga;

void tef00(void);
void tef01(void);
void tef02(void);
void tef03(void);

void test_event_flags(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   tef00();
   tef01();
   tef02();
   tef03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "EVENT FLAGS PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "EVENT FLAGS FAILED");
   sb_MsgDisplay();
}

void  tef00_t2a(ULONG par);
void  tef00_t2b(ULONG par);
ULONG flags;

void tef00(void)
{
   /* create t2a, t2b, and efga */
   smx_TaskLock();
   tx_thread_create(&t2a, (CHAR*)"t2a", tef00_t2a, 0, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_thread_create(&t2b, (CHAR*)"t2b", tef00_t2b, 0, sp2, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   if (tx_event_flags_create(&efga, (CHAR*)"efga") != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* resume, delete efga, and verify t2b autostopped */
   if (tx_event_flags_delete(&efga) != TX_SUCCESS || t2b->sp != 0)
      tfail();

   /* cleanup */
   tx_thread_delete(&t2a);
   tx_thread_delete(&t2b);
}

void tef00_t2a(ULONG par)
{
   /* test setting flags and let t2b run */
   if (tx_event_flags_set(&efga, 0x1010, TX_OR) != TX_SUCCESS)
      tfail();
   smx_TaskBump(t2a, SMX_PRI_NOCHG);

   /* resume and test efga info get and let t2b run */
   if (tx_event_flags_info_get(&efga, &name, &current_flags, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "efga" || current_flags != 0 ||
       first_suspended != (TX_THREAD*)t2b || suspended_count != 1)
      tfail();
   smx_TaskBump(t2a, SMX_PRI_NOCHG);

   /* test setting lower flag to cause t2b to run */
   if (tx_event_flags_set(&efga, 0x1010, TX_OR) != TX_SUCCESS)
      tfail();
}

void tef00_t2b(ULONG par)
{
   /* test AND of flags in efga */
   if (tx_event_flags_get(&efga, 0x1010, TX_AND, &flags, TX_NO_WAIT) != TX_SUCCESS ||
       efga->flags != 0x1010)
      tfail();
   /*  test efga info get */
   if (tx_event_flags_info_get(&efga, &name, &current_flags, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "efga" || current_flags != 0x1010 ||
       first_suspended != NULL || suspended_count != 0)
      tfail();
   /* test clearing upper flag */
   if (tx_event_flags_set(&efga, 0x0010, TX_AND) != TX_SUCCESS || efga->flags != 0x0010)
      tfail();
   /* test clearing lower flag by get */
   if (tx_event_flags_get(&efga, 0x0010, TX_AND_CLEAR, &flags, TX_NO_WAIT) != TX_SUCCESS ||
       efga->flags != 0x0000)
      tfail();

   /* test wait for lower flag to be set by t2a and for correct flags */
   if (tx_event_flags_get(&efga, 0x0010, TX_AND, &flags, TX_WAIT_FOREVER) != TX_SUCCESS ||
       flags != 0x1010)
      tfail();
   /* test suspend on efga and fail due to efga deleted */
   if (tx_event_flags_get(&efga, 0x1011, TX_AND, &flags, TX_WAIT_FOREVER) != TX_NO_EVENTS)
      tfail();
}

/* tef01: test event flags set notify */
void tef01_t2a(ULONG par);
void tef01_F(EGCB_PTR *s);

void tef01(void)
{
   smx_TaskLock();
   count = 0;
   tx_thread_create(&t2a, (CHAR*)"t2a", tef01_t2a, 0x37, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_event_flags_create(&efga, (CHAR*)"efga");
   if (tx_event_flags_set_notify(&efga, tef01_F) != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* test that callback function ran due to put */
   if (count != 1)
      tfail();

   /* cleanup */
   tx_thread_delete(&t2a);
   tx_event_flags_delete(&efga);
}

void tef01_t2a(ULONG par)
{
   tx_event_flags_set(&efga, 0x05, TX_OR);
}

void tef01_F(EGCB_PTR *s)
{
   count = 1;
}

void tef02(void){}
void tef03(void){}


/******************************************************************************
*                                                                             *
*                         BLOCK POOL FUNCTION TESTS                           *
*                                                                             *
******************************************************************************/

TX_BLOCK_POOL  poola;

ULONG    available_blocks;
void*    bp1;
void*    bp2;
void*    bp3;
void*    poolap;
ULONG    total_blocks;

void tbp00(void);
void tbp01(void);
void tbp02(void);
void tbp03(void);

void test_block_pools(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   tbp00();
   tbp01();
   tbp02();
   tbp03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "BLOCK POOLS PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "BLOCK POOLS FAILED");
   sb_MsgDisplay();
}

void tbp00_t2a(ULONG par);
void tbp00_t2b(ULONG par);

void tbp00(void)
{
   /* create t2a, t2b, and poola */
   smx_TaskLock();
   tx_thread_create(&t2a, (CHAR*)"t2a", tbp00_t2a, 0, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_thread_create(&t2b, (CHAR*)"t2b", tbp00_t2b, 0, sp2, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   poolap = smx_HeapMalloc(200, 3, 0);
   if (tx_block_pool_create(&poola, (CHAR*)"poola", 80, poolap, 200) != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* resume and delete block pool */
   if (tx_block_pool_delete(&poola) != TX_SUCCESS)
      tfail();

   /* cleanup */
   smx_HeapFree(poolap);
   tx_thread_delete(&t2a);
   tx_thread_delete(&t2b);
}

void tbp00_t2a(ULONG par)
{
   /* get two blocks */
   if (tx_block_allocate(&poola, &bp1, TX_NO_WAIT) != TX_SUCCESS || bp1 == NULL)
      tfail();
   if (tx_block_allocate(&poola, &bp2, TX_NO_WAIT) != TX_SUCCESS || bp2 == NULL)
      tfail();
   /* let t2b run */
   smx_TaskBump(t2a, SMX_PRI_NOCHG);

   /* resume and test poola info get */
   if (tx_block_pool_info_get(&poola, &name, &available_blocks, &total_blocks, 
       &first_suspended, &suspended_count, NULL) != TX_SUCCESS || name != "poola" || 
       available_blocks!= 0 || total_blocks != 2 || first_suspended != (TX_THREAD*)t2b || 
       suspended_count != 1)
      tfail();
   /* release one block */
   if (tx_block_release(bp2) != TX_SUCCESS)
      tfail();
   /* let t2b run */
   smx_TaskBump(t2a, SMX_PRI_NOCHG);
}

void tbp00_t2b(ULONG par)
{
   /* suspend on poola waiting for a block and get block */
   if (tx_block_allocate(&poola, &bp3, 10) != TX_SUCCESS || bp3 == NULL)
      tfail();
}

void tbp01(void){}
void tbp02(void){}
void tbp03(void){}


/******************************************************************************
*                                                                             *
*                          BYTE POOL FUNCTION TESTS                           *
*                                                                             *
******************************************************************************/

u32 bsza1[] =                 /* heap1 bin size array <1> */
/*bin  0       end */  
      {24, 0xFFFFFFFF};

u32 bsza2[] =                 /* heap2 bin size array */
/*bin  0       end */  
      {24, 0xFFFFFFFF};

ULONG    available_bytes;
HBCB     bina1[(sizeof(bsza1)/4)-1];  /* heap1 bin array */
HBCB     bina2[(sizeof(bsza2)/4)-1];  /* heap2 bin array */
u32*     hbin[3][2] = {(u32*)NULL,   (u32*)NULL, 
                       (u32*)&bsza1, (u32*)&bina1,
                       (u32*)&bsza2, (u32*)&bina2};
ULONG    fragments;
SCB_PTR  hsem[EH_NUM_HEAPS];
u8       heap1[2000];
ULONG    total_bytes;

TX_BYTE_POOL   hv1;
TX_BYTE_POOL   hv2;

void tbyp00(void);
void tbyp01(void);
void tbyp02(void);
void tbyp03(void);

void test_byte_pools(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   tbyp00();
   tbyp01();
   tbyp02();
   tbyp03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "BYTE POOLS PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "BYTE POOLS FAILED");
   sb_MsgDisplay();
}

void tbyp00_t2a(ULONG par);
void tbyp00_t2b(ULONG par);

void tbyp00(void)
{
   /* create t2a and t2b */
   smx_TaskLock();
   tx_thread_create(&t2a, (CHAR*)"t2a", tbyp00_t2a, 0, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_thread_create(&t2b, (CHAR*)"t2b", tbyp00_t2b, 0, sp2, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   /* create byte pool */
   eh_hvp[1] = &hv1;
   if (tx_byte_pool_create(eh_hvp[1], (CHAR*)"heap1", &heap1, sizeof(heap1)) != TX_SUCCESS)
      tfail();
   /* delete byte pool */
   if (tx_byte_pool_delete(eh_hvp[1]) != TX_SUCCESS)
      tfail();
   /* verify byte pool can be re-created */
   if (tx_byte_pool_create(eh_hvp[1], (CHAR*)"heap1", &heap1, sizeof(heap1)) != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* delete byte pool */
   if (tx_byte_pool_delete(eh_hvp[1]) != TX_SUCCESS)
      tfail();
   /* verify byte pool can be re-created */
   if (tx_byte_pool_create(eh_hvp[1], (CHAR*)"heap1", &heap1, sizeof(heap1)) != TX_SUCCESS)
      tfail();

   /* resume and cleanup */
   tx_byte_pool_delete(eh_hvp[1]);
   tx_thread_delete(&t2a);
   tx_thread_delete(&t2b);
}

void tbyp00_t2a(ULONG par)
{
   /* get 200 and 100 byte blocks */
   if (tx_byte_allocate(eh_hvp[1], &bp1, 200, TX_NO_WAIT) != TX_SUCCESS || bp1 == NULL)
      tfail();
   if (tx_byte_allocate(eh_hvp[1], &bp2, 100, TX_NO_WAIT) != TX_SUCCESS || bp2 == NULL)
      tfail();
   /* let t2b run */
   smx_TaskBump(t2a, SMX_PRI_NOCHG);

   /* resume and test heap1 info get */
   if (tx_byte_pool_info_get(eh_hvp[1], &name, &available_bytes, &fragments, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "heap1" ||
       !(available_bytes == 1600 || available_bytes == 1608) || 
       fragments != 3 || first_suspended != (TX_THREAD*)t2b || suspended_count != 1)
      tfail(); 
   /* release 100 byte block */
   if (tx_byte_release(bp2) != TX_SUCCESS)
      tfail();
   /* let t2b run */
   smx_TaskBump(t2a, SMX_PRI_NOCHG);
}

void tbyp00_t2b(ULONG par)
{
   /* test heap1 info get */
   if (tx_byte_pool_info_get(eh_hvp[1], &name, &available_bytes, &fragments, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "heap1" || 
       !(available_bytes == 1600 || available_bytes == 1608) ||
       fragments != 3 || first_suspended != NULL || suspended_count != 0)
      tfail();
   /* attempt to get a 1700 byte block and wait. Then smx_INSUFF_HEAP */
   if (tx_byte_allocate(eh_hvp[1], &bp3, 1700, TX_WAIT_FOREVER) != TX_SUCCESS || bp3 == NULL)
      tfail();
}

void tbyp01(void){}
void tbyp02(void){}
void tbyp03(void){}

/******************************************************************************
*                                                                             *
*                       MESSAGE QUEUE FUNCTION TESTS                          *
*                                                                             *
******************************************************************************/

TX_QUEUE mqa;
void*    mqap;

void tmq00(void);
void tmq01(void);
void tmq02(void);
void tmq03(void);

void test_queues(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   tmq00();
   tmq01();
   tmq02();
   tmq03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "MESSAGE QUEUES PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "MESSAGE QUEUES FAILED");
   sb_MsgDisplay();
}

/* test tx_queue functions */
void tmq00_t2a(ULONG par);
void tmq00_t2b(ULONG par);
u32  msgs[] = {1, 2, 3, 4, 5};
u32  msgr[6];

void tmq00(void)
{
   /* create t2a, t2b, and mqa */
   smx_TaskLock();
   tx_thread_create(&t2a, (CHAR*)"t2a", tmq00_t2a, 0, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_thread_create(&t2b, (CHAR*)"t2b", tmq00_t2b, 0, sp2, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   /* create a word-wide queue with length of 4 words */
   mqap = smx_HeapMalloc(16, 3, 0);
   if (tx_queue_create(&mqa, (CHAR*)"mqa", 1, mqap, 16) != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* resume and delete mqa */
   if (tx_queue_delete(&mqa) != TX_SUCCESS || t2b->sp != 0)
      tfail();

   /* cleanup */
   smx_HeapFree(mqap);
   tx_thread_delete(&t2a);
   tx_thread_delete(&t2b);
}

void tmq00_t2a(ULONG par)
{
   u32 i;
   /* test sending messages and suspend on mqa */
   for (i = 0; i < 5; i++)
      if (tx_queue_send(&mqa, &msgs[i], TX_WAIT_FOREVER) != TX_SUCCESS)
         tfail();

   /*  resume and test mqa info get */
   if (tx_queue_info_get(&mqa, &name, &enqueued, &available_storage, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "mqa" || enqueued != 0 ||
       available_storage != 4 || first_suspended != (TX_THREAD*)t2b || suspended_count != 1)
      tfail();
   /* test resuming t2b waiting for msg */
   if (tx_queue_send(&mqa, &msgs[3], TX_NO_WAIT) != TX_SUCCESS || mqa->fl != NULL)
      tfail();
   smx_TaskBump(t2a, SMX_PRI_NOCHG);

   /* resume, fill queue, and suspend on mqa */
   for (i = 0; i < 5; i++)
      if (tx_queue_send(&mqa, &msgs[i], TX_WAIT_FOREVER) != TX_SUCCESS)
         tfail();
}

void tmq00_t2b(ULONG par)
{
   u32 i;
   /*  test mqa info get */
   if (tx_queue_info_get(&mqa, &name, &enqueued, &available_storage, &first_suspended, 
       &suspended_count, NULL) != TX_SUCCESS || name != "mqa" || enqueued != 4 ||
       available_storage != 0 || first_suspended != (TX_THREAD*)t2a || suspended_count != 1)
      tfail();
   /* test receiving messages and suspend on mqa */
   for (i = 0; i < 6; i++)
      if (tx_queue_receive(&mqa, &msgr[i], TX_WAIT_FOREVER) != TX_SUCCESS)
         tfail();
   if (msgr[4] != 5)
      tfail();
   smx_TaskBump(t2b, SMX_PRI_NOCHG);

   /* resume and test flushing mqa */
   if (tx_queue_flush(&mqa) != TX_SUCCESS || mqa->fl != NULL)
      tfail();
}

/* test tx_queue_front_send() to partially filled queue */
void tmq01_t2a(ULONG par);
void tmq01_t2b(ULONG par);

void tmq01(void)
{
   /* create t2a, t2b, and mqa */
   smx_TaskLock();
   tx_thread_create(&t2a, (CHAR*)"t2a", tmq01_t2a, 0, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_thread_create(&t2b, (CHAR*)"t2b", tmq01_t2b, 0, sp2, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   /* create a word-wide queue with length of 4 words */
   mqap = smx_HeapMalloc(16, 3, 0);
   tx_queue_create(&mqa, (CHAR*)"mqa", 1, mqap, 16);
   smx_TaskUnlock();

   /* cleanup */
   tx_thread_delete(&t2a);
   tx_thread_delete(&t2b);
   tx_queue_delete(&mqa);
   smx_HeapFree(mqap);
}

void tmq01_t2a(ULONG par)
{
   u32 i;
   /* partially fill mqa from back */
   for (i = 0; i < 3; i++)
      tx_queue_send(&mqa, &msgs[i], TX_WAIT_FOREVER);

   /* send msg to front of mqa */
   if (tx_queue_front_send(&mqa, &msgs[3], TX_NO_WAIT) != TX_SUCCESS)
      tfail();
   smx_TaskBump(t2a, SMX_PRI_NOCHG);
}

void tmq01_t2b(ULONG par)
{
   /* receive first message in mqa and verify it is the message sent to front */
   if (tx_queue_receive(&mqa, &msgr[0], TX_WAIT_FOREVER) != TX_SUCCESS || msgr[0] != msgs[3])
      tfail();
}

/* tmq02: test message queue send notify */
void tmq02_t2a(ULONG par);
void tmq02_F(PICB_PTR *s);

void tmq02(void)
{
   smx_TaskLock();
   count = 0;
   tx_thread_create(&t2a, (CHAR*)"t2a", tmq02_t2a, 0x37, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   mqap = smx_HeapMalloc(16, 3, 0);
   tx_queue_create(&mqa, (CHAR*)"mqa", 1, mqap, 16);
   if (tx_queue_send_notify(&mqa, tmq02_F) != TX_SUCCESS)
      tfail();
   smx_TaskUnlock();

   /* test that callback function ran due to put */
   if (count != 2)
      tfail();

   /* cleanup */
   tx_thread_delete(&t2a);
   tx_queue_delete(&mqa);
   smx_HeapFree(mqap);
}

void tmq02_t2a(ULONG par)
{
   tx_queue_send(&mqa, &msgs[0], TX_NO_WAIT);
   tx_queue_front_send(&mqa, &msgs[3], TX_NO_WAIT);
}

void tmq02_F(PICB_PTR *s)
{
   count++;
}

void tmq03(void){}


/******************************************************************************
*                                                                             *
*                            TIMER FUNCTION TESTS                             *
*                                                                             *
******************************************************************************/

TX_TIMER    tmra;

void ttmr00(void);
void ttmr01(void);
void ttmr02(void);
void ttmr03(void);

void test_timers(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   ttmr00();
   ttmr01();
   ttmr02();
   ttmr03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "TIMERS PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "TIMERS FAILED");
   sb_MsgDisplay();
}

void ttmr00_t2a(ULONG par);
void ttmr00_t2b(ULONG par);
void lsra(ULONG par);
u32  timea;

void ttmr00(void)
{
   /* create t2a, t2b, and tmra */
   smx_TaskLock();
   tx_thread_create(&t2a, (CHAR*)"t2a", ttmr00_t2a, 0, sp1, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   tx_thread_create(&t2b, (CHAR*)"t2b", ttmr00_t2b, 0, sp2, TSSZ, TP2, TP2, 
                    TX_NO_TIME_SLICE, TX_AUTO_START);
   if (tx_timer_create(&tmra, (CHAR*)"tmra", lsra, 1, 5, 10, TX_AUTO_ACTIVATE) != TX_SUCCESS)
      tfail();
   timea = smx_etime;
   smx_TaskUnlock();

   /* resume and test timer delete */
   if (tx_timer_delete(&tmra) != TX_TIMER_ERROR) /* already deleted */
      tfail();
   tx_timer_create(&tmra, (CHAR*)"tmra", lsra, 1, 5, 10, TX_AUTO_ACTIVATE);
   if (tx_timer_delete(&tmra) != TX_SUCCESS) /* not already stopped */
      tfail();

   /* cleanup */
   tx_thread_delete(&t2a);
   tx_thread_delete(&t2b);
}

void lsra(ULONG par)
{
   switch (par)
   {
      case 1:
         if (smx_etime != timea + 5) /* initial timeout */
            tfail();
         else
            tmra->par = 2;
         break;
      case 2:
         if (smx_etime != timea + 15) /* resched timeout */
            tfail();
         else
            tmra->par = 3;
         break;
      case 3:
         if (smx_etime != timea + 17) /* changed initial timeout */
            tfail();
         else
            tmra->par = 4;
   }
}

void ttmr00_t2a(ULONG par)
{
   /* test timer change */
   while (tmra->par != 3) {}
   if (tx_timer_change(&tmra, 2, 0) != TX_SUCCESS)
      tfail();
   /* let t2b run */
   smx_TaskBump(t2a, SMX_PRI_NOCHG);
}

void ttmr00_t2b(ULONG par)
{
   /* test timer info get */
   if (tx_timer_info_get(&tmra, &name, &active, &remaining_ticks, &reschedule_ticks, NULL) != TX_SUCCESS 
      || name != "tmra" || active != TX_TRUE || remaining_ticks != 2 || reschedule_ticks != 0)
      tfail();
   while (tmra->par != 4) {}

   /* test that timer has stopped */
   if (tx_timer_info_get(&tmra, &name, &active, &remaining_ticks, &reschedule_ticks, NULL) != TX_TIMER_ERROR)
      tfail();
}

void ttmr01(void){}
void ttmr02(void){}
void ttmr03(void){}


/******************************************************************************
*                                                                             *
*                                THREADX DEMOS                                *
*                                                                             *
******************************************************************************/

/* See also demos in AZURE\Examples. Link together after this demo. */

/* This demo includes examples of eight threads of different priorities, using 
   a message queue, semaphore, mutex, event flags group, byte pool, and block 
   pool.  */

#define DEMO_STACK_SIZE         1024
#define DEMO_BYTE_POOL_SIZE     9120
#define DEMO_BLOCK_POOL_SIZE    100
#define DEMO_QUEUE_SIZE         100


/* Define the ThreadX object control blocks...  */
TX_THREAD               thread_0;
TX_THREAD               thread_1;
TX_THREAD               thread_2;
TX_THREAD               thread_3;
TX_THREAD               thread_4;
TX_THREAD               thread_5;
TX_THREAD               thread_6;
TX_THREAD               thread_7;
TX_QUEUE                queue_0;
TX_SEMAPHORE            semaphore_0;
TX_MUTEX                mutex_0;
TX_EVENT_FLAGS_GROUP    event_flags_0;
TX_BYTE_POOL            byte_pool_0;
TX_BLOCK_POOL           block_pool_0;
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];

/* Define the counters used in the demo application...  */
ULONG                   thread_0_counter;
ULONG                   thread_1_counter;
ULONG                   thread_1_messages_sent;
ULONG                   thread_2_counter;
ULONG                   thread_2_messages_received;
ULONG                   thread_3_counter;
ULONG                   thread_4_counter;
ULONG                   thread_5_counter;
ULONG                   thread_6_counter;
ULONG                   thread_7_counter;

/* Define thread prototypes.  */
void    thread_0_entry(ULONG thread_input);
void    thread_1_entry(ULONG thread_input);
void    thread_2_entry(ULONG thread_input);
void    thread_3_and_4_entry(ULONG thread_input);
void    thread_5_entry(ULONG thread_input);
void    thread_6_and_7_entry(ULONG thread_input);

void    tx_kernel_enter(void);
void    tx_application_define(void *first_unused_memory);

/* Define main entry point. */
int tx_main(void)
{
    /* Enter the ThreadX kernel.  */
   tx_kernel_enter();
   return true;
}

void tx_kernel_enter(void)
{
   smx_TaskBump(ttxp, PRI_SYS);
   sb_MsgOut(SB_MSG_INFO, "DEMO INITIALIZATION");
   sb_MsgDisplay();
   sb_DelayMsec(50); /* delay for msg to display */
   tx_application_define(0);
   sb_MsgOut(SB_MSG_INFO, "DEMO TASKS RUNNING");
   sb_MsgDisplay();
   sb_DelayMsec(50); /* delay for msg to display */
   smx_TaskBump(ttxp, PRI_MIN);
}

/* Define what the initial system looks like.  */

enum TX_PRIORITIES {P16, P8, P4, P1};

void    tx_application_define(void *first_unused_memory)
{

CHAR    *pointer = NULL;

    /* Create a byte memory pool from which to allocate the thread stacks.  */
    eh_hvp[2] = &byte_pool_0;
    tx_byte_pool_create(&byte_pool_0, (CHAR*)"byte pool 0", memory_area, DEMO_BYTE_POOL_SIZE);

    /* Put system definition stuff in here, e.g. thread creates and other assorted
       create information.  */

    /* Allocate the stack for thread 0.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create the main thread.  */
    tx_thread_create(&thread_0, (CHAR*)"thread 0", thread_0_entry, 0,  
            pointer, DEMO_STACK_SIZE, 
            P1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);


    /* Allocate the stack for thread 1.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create threads 1 and 2. These threads pass information through a ThreadX 
       message queue.  It is also interesting to note that these threads have a time
       slice.  */
    tx_thread_create(&thread_1, (CHAR*)"thread 1", thread_1_entry, 1,  
            pointer, DEMO_STACK_SIZE, 
            P16, 16, 4, TX_AUTO_START);

    /* Allocate the stack for thread 2.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_2, (CHAR*)"thread 2", thread_2_entry, 2,  
            pointer, DEMO_STACK_SIZE, 
            P16, 16, 4, TX_AUTO_START);

    /* Allocate the stack for thread 3.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create threads 3 and 4.  These threads compete for a ThreadX counting semaphore.  
       An interesting thing here is that both threads share the same instruction area.  */
    tx_thread_create(&thread_3, (CHAR*)"thread 3", thread_3_and_4_entry, 3,  
            pointer, DEMO_STACK_SIZE, 
            P8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 4.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_4, (CHAR*)"thread 4", thread_3_and_4_entry, 4,  
            pointer, DEMO_STACK_SIZE, 
            P8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 5.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create thread 5.  This thread simply pends on an event flag which will be set
       by thread_0.  */
    tx_thread_create(&thread_5, (CHAR*)"thread 5", thread_5_entry, 5,  
            pointer, DEMO_STACK_SIZE, 
            P4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 6.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create threads 6 and 7.  These threads compete for a ThreadX mutex.  */
    tx_thread_create(&thread_6, (CHAR*)"thread 6", thread_6_and_7_entry, 6,  
            pointer, DEMO_STACK_SIZE, 
            P8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 7.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    tx_thread_create(&thread_7, (CHAR*)"thread 7", thread_6_and_7_entry, 7,  
            pointer, DEMO_STACK_SIZE, 
            P8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the message queue.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(ULONG), TX_NO_WAIT);

    /* Create the message queue shared by threads 1 and 2.  */
    tx_queue_create(&queue_0, (CHAR*)"queue 0", TX_1_ULONG, pointer, DEMO_QUEUE_SIZE*sizeof(ULONG));

    /* Create the semaphore used by threads 3 and 4.  */
    tx_semaphore_create(&semaphore_0, (CHAR*)"semaphore 0", 1);

    /* Create the event flags group used by threads 1 and 5.  */
    tx_event_flags_create(&event_flags_0, (CHAR*)"event flags 0");

    /* Create the mutex used by thread 6 and 7 without priority inheritance.  */
    tx_mutex_create(&mutex_0, (CHAR*)"mutex 0", TX_NO_INHERIT);

    /* Allocate the memory for a small block pool.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_BLOCK_POOL_SIZE, TX_NO_WAIT);

    /* Create a block memory pool to allocate a message buffer from.  */
    tx_block_pool_create(&block_pool_0, (CHAR*)"block pool 0", sizeof(ULONG), pointer, DEMO_BLOCK_POOL_SIZE);

    /* Allocate a block and release the block memory.  */
    tx_block_allocate(&block_pool_0, (VOID **) &pointer, TX_NO_WAIT);

    /* Release the block back to the pool.  */
    tx_block_release(pointer);
}



/* Define the test threads.  */

void    thread_0_entry(ULONG thread_input)
{

UINT    status;


    /* This thread simply sits in while-forever-sleep loop.  */
    while(1)
    {

        /* Increment the thread counter.  */
        thread_0_counter++;

        /* Sleep for 10 ticks.  */
        tx_thread_sleep(10);

        /* Set event flag 0 to wakeup thread 5.  */
        status =  tx_event_flags_set(&event_flags_0, 0x1, TX_OR);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;
    }
}


void    thread_1_entry(ULONG thread_input)
{

UINT    status;


    /* This thread simply sends messages to a queue shared by thread 2.  */
    while(1)
    {

        /* Increment the thread counter.  */
        thread_1_counter++;

        /* Send message to queue 0.  */
        status =  tx_queue_send(&queue_0, &thread_1_messages_sent, TX_WAIT_FOREVER);

        /* Check completion status.  */
        if (status != TX_SUCCESS)
            break;

        /* Increment the message sent.  */
        thread_1_messages_sent++;
    }
}


void    thread_2_entry(ULONG thread_input)
{

ULONG   received_message;
UINT    status;

    /* This thread retrieves messages placed on the queue by thread 1.  */
    while(1)
    {

        /* Increment the thread counter.  */
        thread_2_counter++;

        /* Retrieve a message from the queue.  */
        status = tx_queue_receive(&queue_0, &received_message, TX_WAIT_FOREVER);

        /* Check completion status and make sure the message is what we 
           expected.  */
        if ((status != TX_SUCCESS) || (received_message != thread_2_messages_received))
            break;
        
        /* Otherwise, all is okay.  Increment the received message count.  */
        thread_2_messages_received++;
    }
}


void    thread_3_and_4_entry(ULONG thread_input)
{

UINT    status;


    /* This function is executed from thread 3 and thread 4.  As the loop
       below shows, these function compete for ownership of semaphore_0.  */
    while(1)
    {

        /* Increment the thread counter.  */
        if (thread_input == 3)
            thread_3_counter++;
        else
            thread_4_counter++;

        /* Get the semaphore with suspension.  */
        status =  tx_semaphore_get(&semaphore_0, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

        /* Sleep for 2 ticks to hold the semaphore.  */
        tx_thread_sleep(2);

        /* Release the semaphore.  */
        status =  tx_semaphore_put(&semaphore_0);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;
    }
}


void    thread_5_entry(ULONG thread_input)
{

UINT    status;
ULONG   actual_flags;


    /* This thread simply waits for an event in a forever loop.  */
    while(1)
    {

        /* Increment the thread counter.  */
        thread_5_counter++;

        /* Wait for event flag 0.  */
        status =  tx_event_flags_get(&event_flags_0, 0x1, TX_OR_CLEAR, 
                                                &actual_flags, TX_WAIT_FOREVER);

        /* Check status.  */
        if ((status != TX_SUCCESS) || (actual_flags != 0x1))
            break;
    }
}


void    thread_6_and_7_entry(ULONG thread_input)
{

UINT    status;


    /* This function is executed from thread 6 and thread 7.  As the loop
       below shows, these function compete for ownership of mutex_0.  */
    while(1)
    {

        /* Increment the thread counter.  */
        if (thread_input == 6)
            thread_6_counter++;
        else
            thread_7_counter++;

        /* Get the mutex with suspension.  */
        status =  tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

        /* Get the mutex again with suspension.  This shows
           that an owning thread may retrieve the mutex it
           owns multiple times.  */
        status =  tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

        /* Sleep for 2 ticks to hold the mutex.  */
        tx_thread_sleep(2);

        /* Release the mutex.  */
        status =  tx_mutex_put(&mutex_0);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;

        /* Release the mutex again.  This will actually 
           release ownership since it was obtained twice.  */
        status =  tx_mutex_put(&mutex_0);

        /* Check status.  */
        if (status != TX_SUCCESS)
            break;
    }
}


/* Notes:
   1. mheap is defined in main().
*/