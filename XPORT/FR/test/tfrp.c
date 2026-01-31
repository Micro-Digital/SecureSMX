/*
* tfrp.c                                                    Version 6.0.0
*
* test FRPort
*
* Copyright (c) 2021-2026 Micro Digital Inc.
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#define  TP1 1
#define  TP2 2

void test_mutexes(void);
void test_queues(void);
void test_semaphores(void);
void test_tasks(void);
void test_timers(void);
void test_event_groups(void);
void tfail(void);
void tfrp_main(u32);
void t2a_main(void* pp);
void t2b_main(void* pp);

extern vbool   tick_cben;
extern ISR_PTR tick_cbptr;

bool           ts_pass;
TaskHandle_t   t2a;
TaskHandle_t   t2b;
TCB*           t2ax;
TCB*           t2bx;
TCB*           tfrp;
volatile bool  wait;

void frp_test(void)
{
   sb_ConWriteString(0,0,SB_CLR_WHITE,SB_CLR_BLACK,!SB_CON_BLINK,"FRPort Test  (Built " __DATE__ " " __TIME__ ")");
   sb_MsgDisplay();
   tfrp = smx_TaskCreate(tfrp_main, TP1, 400, 0, "tfrp");
   smx_TaskStart(tfrp);
}

void tfrp_main(u32)
{
   test_tasks();
   test_mutexes();
   test_semaphores();
   test_queues();
   test_timers();
   test_event_groups();
   sb_MsgOut(SB_MSG_INFO, "TESTS DONE");
   sb_MsgDisplay();
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

/* tt00 Test task create with parameter, task status functions, and task delete */
void tt00_t2a(void*);
void tt00_t2b(void*);

void tt00(void)
{
   /* create test tasks */
   if (xTaskCreate(tt00_t2a, "t2a", 100, (void*)0x37, TP1, &t2a) == pdFALSE)
      tfail();
   t2b = xTaskCreateStatic(tt00_t2b, "t2b", 100, (void*)0x66, TP1, NULL, NULL);
   if (t2b == NULL)
      tfail();

   /* start test tasks */
   vTaskPrioritySet(t2a, 2);
   vTaskPrioritySet(t2b, 2);

   /* cleanup (t2a already deleted) */
   vTaskDelete(t2b);
   t2a = t2b = NULL;
}

void tt00_t2a(void* pp)
{
   /* test parameter pass */
   if ((u32)pp != 0x37)
      tfail();

   /* test simple task functions */
   if (uxTaskGetTaskNumber(t2a) != ((TCB*)t2a)->indx ||
      uxTaskPriorityGet(t2a) != 2 || 
      xTaskGetCurrentTaskHandle() != t2a ||
      xTaskGetHandle("t2a") != t2a || 
      ulTaskGetIdleRunTimeCounter() != smx_cpd.idle ||
      xTaskGetIdleTaskHandle() != (TaskHandle_t)smx_Idle || 
      strcmp(pcTaskGetName(t2a), "t2a") != 0 ||
      uxTaskGetNumberOfTasks() != smx_tcbs.num_used || 
      xTaskGetSchedulerState() != taskSCHEDULER_RUNNING ||
      uxTaskGetStackHighWaterMark(t2a) != ((u32)((TCB*)t2a)->ssz - (u32)((TCB*)t2a)->shwm) ||
      uxTaskGetStackHighWaterMark2(t2a) != ((u32)((TCB*)t2a)->ssz - (u32)((TCB*)t2a)->shwm) ||
      eTaskGetState(t2a) != eRunning)
         tfail();
}

void tt00_t2b(void* pp)
{
   if ((u32)pp != 0x66)
      tfail();
   vTaskDelete(t2a);
   if (((TCB_PTR)t2a)->cbtype != SMX_CB_NULL)
      tfail();
}

/* ttS01 Test task delay, suspends, resumes, switch context */
void tt01_t2a(void*);
void tt01_t2b(void*);

void tt01(void)
{
   TickType_t time, init_time;

   /* create test tasks */
   if (xTaskCreate(tt01_t2a, "t2a", 100, (void*)0x37, TP1, &t2a) == pdFALSE)
      tfail();
   t2b = xTaskCreateStatic(tt01_t2b, "t2b", 100, (void*)0x66, TP1, NULL, NULL);
   if (t2b == NULL)
      tfail();

   /* start t2a and abort t2a self-delay */
   vTaskPrioritySet(t2a, 2);
   xTaskAbortDelay(t2a);

   /* start t2b */
   vTaskPrioritySet(t2b, 2);

   /* test TaskDelayUntil */
   if ((time = xTaskGetTickCount()) != (TickType_t)smx_etime)
      tfail();
   init_time = time;
   xTaskDelayUntil(&time, 5);
   if (time != init_time + 5)
      tfail();

   /* test suspend and resume scheduler */
   vTaskSuspendAll();
   if (smx_lockctr == 0)
      tfail();
   if (xTaskResumeAll() || smx_lockctr != 0)
      tfail();

   /* cleanup */
   vTaskDelete(t2a);
   vTaskDelete(t2b);
   t2a = t2b = NULL;
}

void tt01_t2a(void* pp)
{
   /* test xTaskAbortDelay() in tt01() */
   TickType_t time = xTaskGetTickCount();
   vTaskDelay(10);
   if (xTaskGetTickCount() > time + 1)
      tfail();
}

void tt01_t2b(void* pp)
{
   /* test suspend tfrp then test resume tfrp */
   vTaskSuspend((TaskHandle_t)tfrp);
   if (smx_rq[1].fl == tfrp)
      tfail();
   vTaskResume((TaskHandle_t)tfrp);
   if (smx_rq[1].fl != tfrp)
      tfail();
   vTaskSwitchContext();
   tfail(); /* should not reach here */
}

/* tt02 Test task tag with callback function and TLS pointer */
extern TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask );
extern void vTaskSetApplicationTaskTag( TaskHandle_t xTask,
                                 TaskHookFunction_t pxHookFunction );
void tt02_t2a(void*);
void tt02_t2b(void*);
void tt02_fun(void*);
bool tt02_var;

void tt02(void)
{
   /* create test task with callback function */
   xTaskCreate(tt02_t2a, "t2a", 100, (void*)0x37, TP1, &t2a);
   vTaskSetApplicationTaskTag(t2a, (TaskHookFunction_t)tt02_fun);
   tt02_var = false;

   /* start t2a */
   vTaskPrioritySet(t2a, 2);

   /* test TLS pointer */
   vTaskSetThreadLocalStoragePointer((TaskHandle_t)tfrp, 0, (void*)0x76);
   if ((u32)pvTaskGetThreadLocalStoragePointer((TaskHandle_t)tfrp, 0) != (u32)(tfrp->sbp + SMX_RSA_SIZE))
      tfail();

   /* cleanup */
   vTaskDelete(t2a);
   t2a = NULL;
}

void tt02_t2a(void*)
{
   if (xTaskGetApplicationTaskTag(NULL) != (TaskHookFunction_t)tt02_fun)
      tfail();
   xTaskCallApplicationTaskHook(t2a, 0);
   if (tt02_var == false)
      tfail();
}

void tt02_fun(void* par)
{
   if (par == 0)
      tt02_var = true;
}

/* tt03 Test FromISR functions from LSR */
extern TaskHookFunction_t xTaskGetApplicationTaskTagFromISR( TaskHandle_t xTask );
void tt03_t2a(void*);
void tt03_ISR(void);
void tt03_LSR_main(u32 a);
LCB_PTR tt03_LSR;

void tt03(void)
{
   TickType_t time;

   /* create test task and set application task tag */
   xTaskCreate(tt03_t2a, "t2a", 100, (void*)0x37, TP1, &t2a);
   vTaskSetApplicationTaskTag(t2a, (long(*)(void*))0x18);

   /* create LSR */
   tt03_LSR = smx_LSRCreate((FUN_PTR)tt03_LSR_main, SMX_FL_TRUST, "tt03_LSR");

   /* start t2a */
   vTaskPrioritySet(t2a, 2);

   /* wait to be resumed */
   time = xTaskGetTickCount();
   vTaskDelay(10);
   if (xTaskGetTickCount() > time + 1)
      tfail();

   /* cleanup */
   smx_LSRDelete(&tt03_LSR);
   vTaskDelete(t2a);
   t2a = NULL;
}

void tt03_t2a(void*)
{
   /* enable interrupt to invoke LSR */
   tick_cbptr = tt03_ISR;
   tick_cben = true;
}

void tt03_ISR(void)
{
   smx_LSR_INVOKE(tt03_LSR, 0);
}

void tt03_LSR_main(u32 par)
{
   if (xTaskGetApplicationTaskTagFromISR(t2a) != (long(*)(void*))0x18 ||
      xTaskGetTickCountFromISR() != (TickType_t)smx_etime)
         tfail();
   xTaskResumeFromISR((TaskHandle_t)tfrp);
}


void tfail(void)
{
   sb_DEBUGTRAP();
   ts_pass = false;
}

/******************************************************************************
*                                                                             *
*                            MUTEX FUNCTION TESTS                             *
*                                                                             *
******************************************************************************/

void tm00(void);
void tm01(void);
void tm02(void);
void tm03(void);
SemaphoreHandle_t mtx1;
SemaphoreHandle_t mtx2;

void test_mutexes(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   tm00();
   tm01();
   tm02();
//   tm03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "MUTEXES PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "MUTEXES FAILED");
   sb_MsgDisplay();
}

/* tm00 Test mutex create, take, give, and delete */
void tm00_t2a(void*);

void tm00(void)
{
   /* create test task */
   t2a = xTaskCreateStatic(tm00_t2a, "t2a", 100, (void*)0x66, TP1, NULL, NULL);

   /* create mutexes and test */
   mtx1 = xSemaphoreCreateMutex();
   mtx2 = xSemaphoreCreateMutexStatic(NULL);
   if (mtx1 == NULL || mtx2 == NULL)
      tfail();

   /* take mtx1 and test that tfrp owns it */
   if (!xSemaphoreTake(mtx1, 0) || xSemaphoreGetMutexHolder(mtx1) != (TaskHandle_t)tfrp)
      tfail();

   /* start t2a */
   vTaskPrioritySet(t2a, 2);

   /* give mtx1 and test that tfrp does not own it */
   if (!xSemaphoreGive(mtx1) || xSemaphoreGetMutexHolder(mtx1) == (TaskHandle_t)tfrp)
      tfail();

   /* attempt to give mtx2 and fail */
   if (xSemaphoreGive(mtx2) || xSemaphoreGetMutexHolder(mtx2) != (TaskHandle_t)t2a)
      tfail();

   /* cleanup */
   vSemaphoreDelete(mtx1);
   vSemaphoreDelete(mtx2);
   mtx1 = mtx2 = NULL;
   vTaskDelete(t2a);
   t2a = NULL;
}

void tm00_t2a(void*)
{
   /* attempt to take mtx1 when owned by tfrp */
   if (xSemaphoreTake(mtx1, 0))
      tfail();

   /* take mtx1 after give by tfrp */
   if (!xSemaphoreTake(mtx1, 10))
      tfail();

   /* take mtx2 */
   if (!xSemaphoreTake(mtx2, 0) || xSemaphoreGetMutexHolder(mtx2) != (TaskHandle_t)t2a)
      tfail();
}
void tm00_t2a(void*);

/* tm01 Test recursive mutex create, take, give, and delete */
void tm01_t2a(void*);

void tm01(void)
{
   /* create test task */
   t2a = xTaskCreateStatic(tm01_t2a, "t2a", 100, (void*)0x66, TP1, NULL, NULL);

   /* create mutexes and test */
   mtx1 = xSemaphoreCreateRecursiveMutex();
   mtx2 = xSemaphoreCreateRecursiveMutexStatic(NULL);
   if (mtx1 == NULL || mtx2 == NULL)
      tfail();

   /* take mtx1 and test that tfrp owns it */
   if (!xSemaphoreTakeRecursive(mtx1, 0) || xSemaphoreGetMutexHolder(mtx1) != (TaskHandle_t)tfrp)
      tfail();

   /* start t2a */
   vTaskPrioritySet(t2a, 2);

   /* give mtx1 and test that tfrp does not own it */
   if (!xSemaphoreGiveRecursive(mtx1) || xSemaphoreGetMutexHolder(mtx1) == (TaskHandle_t)tfrp)
      tfail();

   /* attempt to give mtx2 and fail */
   if (xSemaphoreGiveRecursive(mtx2) || xSemaphoreGetMutexHolder(mtx2) != (TaskHandle_t)t2a)
      tfail();

   /* cleanup */
   vSemaphoreDelete(mtx1);
   vSemaphoreDelete(mtx2);
   mtx1 = mtx2 = NULL;
   vTaskDelete(t2a);
   t2a = NULL;
}

void tm01_t2a(void*)
{
   /* attempt to take mtx1 when owned by tfrp */
   if (xSemaphoreTakeRecursive(mtx1, 0))
      tfail();

   /* take mtx1 after give by tfrp */
   if (!xSemaphoreTakeRecursive(mtx1, 10))
      tfail();

   /* take mtx2 */
   if (!xSemaphoreTakeRecursive(mtx2, 0) || xSemaphoreGetMutexHolder(mtx2) != (TaskHandle_t)t2a)
      tfail();
}

/* tm02: Test get mutex holder from LSR */
void tm02_ISR(void);
void tm02_LSR_main(u32 a);
LCB_PTR tm02_LSR;

void tm02(void)
{
   /* create mutex and take it */
   mtx1 = xSemaphoreCreateMutex();
   xSemaphoreTake(mtx1, 0);

   /* create LSR */
   tm02_LSR = smx_LSRCreate((FUN_PTR)tm02_LSR_main, SMX_FL_TRUST, "tm02_LSR");

   /* enable interrupt and wait */
   tick_cbptr = tm02_ISR;
   tick_cben = true;
   vTaskDelay(10);

   /* cleanup */
   smx_LSRDelete(&tm02_LSR);
   vSemaphoreDelete(mtx1);
   mtx1 = NULL;
}

void tm02_ISR(void)
{
   smx_LSR_INVOKE(tm02_LSR, 0);
}

void tm02_LSR_main(u32 par)
{
   if (xSemaphoreGetMutexHolderFromISR(mtx1) != (TaskHandle_t)tfrp)
      tfail();
}


/******************************************************************************
*                                                                             *
*                          SEMAPHORE FUNCTION TESTS                           *
*                                                                             *
******************************************************************************/

void ts00(void);
void ts01(void);
void ts02(void);
void ts03(void);
SemaphoreHandle_t sem1;
SemaphoreHandle_t sem2;
SemaphoreHandle_t sem3;

void test_semaphores(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   ts00();
   ts01();
   ts02();
//   ts03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "SEMAPHORES PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "SEMAPHORES FAILED");
   sb_MsgDisplay();
}

/* ts00 Test binary semaphore create, take, give, and delete */
void ts00_t2a(void*);

void ts00(void)
{
   /* create test task */
   t2a = xTaskCreateStatic(ts00_t2a, "t2a", 100, (void*)0x66, TP1, NULL, NULL);

   /* create semaphores and test */
   vSemaphoreCreateBinary(sem1);
   sem2 = xSemaphoreCreateBinary();
   sem3 = xSemaphoreCreateBinaryStatic(NULL);
   if (sem1 == NULL || sem2 == NULL || sem3 == NULL)
      tfail();

   /* take sem1 */
   if (!xSemaphoreTake(sem1, 0))
      tfail();

   /* start t2a */
   vTaskPrioritySet(t2a, 2);

   /* give sem1, t2a preeempts */
   if (!xSemaphoreGive(sem1))
      tfail();

   /* give sem2 and sem3. ok since semaphores not owned as mutexes are. */
   if (!xSemaphoreGive(sem2) || !xSemaphoreGive(sem3))
      tfail();

   /* cleanup */
   vSemaphoreDelete(sem1);
   vSemaphoreDelete(sem2);
   vSemaphoreDelete(sem3);
   sem1 = sem2 = sem3 = NULL;
   vTaskDelete(t2a);
   t2a = NULL;
}

void ts00_t2a(void*)
{
   /* attempt to take sem1 after taken by tfrp */
   if (xSemaphoreTake(sem1, 0))
      tfail();

   /* take sem1 after given by tfrp */
   if (!xSemaphoreTake(sem1, 10))
      tfail();

   /* take sem2 */
   if (!xSemaphoreTake(sem2, 0))
      tfail();

   /* take sem3 */
   if (!xSemaphoreTake(sem3, 0))
      tfail();
}

/* ts01 Test counting semaphore create, take, give, and delete */
void ts01_t2a(void*);

void ts01(void)
{
   /* create test task */
   t2a = xTaskCreateStatic(ts01_t2a, "t2a", 100, (void*)0x66, TP1, NULL, NULL);

   /* create semaphores with counts == 2 and test */
   sem1 = xSemaphoreCreateCounting(2, 2);
   sem2 = xSemaphoreCreateCountingStatic(2, 2, NULL);
   if (sem1 == NULL || sem2 == NULL)
      tfail();
   if (uxSemaphoreGetCount(sem1) != 2 || uxSemaphoreGetCount(sem2) != 2)
      tfail();

   /* take sem1 twice and test count == 0 */
   if (!xSemaphoreTake(sem1, 0) || !xSemaphoreTake(sem1, 0))
      tfail();
   if (uxSemaphoreGetCount(sem1) != 0)
      tfail();

   /* start t2a */
   vTaskPrioritySet(t2a, 2);

   /* give sem1, t2a preeempts */
   if (!xSemaphoreGive(sem1))
      tfail();

   /* cleanup */
   vSemaphoreDelete(sem1);
   vSemaphoreDelete(sem2);
   sem1 = sem2 = NULL;
   vTaskDelete(t2a);
   t2a = NULL;
}

void ts01_t2a(void*)
{
   /* attempt to take sem1 after taken by tfrp */
   if (xSemaphoreTake(sem1, 0))
      tfail();

   /* take sem1 after one give by tfrp */
   if (!xSemaphoreTake(sem1, 10))
      tfail();

   /* attempt to take sem2 three times and fail */
   if (!xSemaphoreTake(sem2, 0) || !xSemaphoreTake(sem2, 0) || 
        xSemaphoreTake(sem2, 0))
      tfail();
}

/* ts02: Test semaphore give and take from LSR */
void ts02_ISR(void);
void ts02_LSR_main(u32 a);
LCB_PTR ts02_LSR;

void ts02(void)
{
   /* create semaphores and take them once each */
   sem1 = xSemaphoreCreateBinary();
   sem2 = xSemaphoreCreateCounting(2, 2);
   xSemaphoreTake(sem1, 0);
   xSemaphoreTake(sem2, 0);

   /* create LSR */
   ts02_LSR = smx_LSRCreate((FUN_PTR)ts02_LSR_main, SMX_FL_TRUST, "ts02_LSR");

   /* enable interrupt and wait */
   tick_cbptr = ts02_ISR;
   tick_cben = true;
   vTaskDelay(10);

   /* test semaphore counts after LSR */
   if (((SCB_PTR)sem1)->count !=1 || ((SCB_PTR)sem2)->count != 0)
      tfail();

   /* cleanup */
   smx_LSRDelete(&ts02_LSR);
   vSemaphoreDelete(sem1);
   vSemaphoreDelete(sem2);
   sem1 = sem2 = NULL;
}

void ts02_ISR(void)
{
   smx_LSR_INVOKE(ts02_LSR, 0);
}

void ts02_LSR_main(u32 par)
{
   xSemaphoreGiveFromISR(sem1, 0);
   xSemaphoreTakeFromISR(sem2, 0);
   xTaskResumeFromISR((TaskHandle_t)tfrp);
}

/******************************************************************************
*                                                                             *
*                            QUEUE FUNCTION TESTS                             *
*                                                                             *
******************************************************************************/

void tq00(void);
void tq01(void);
void tq02(void);
void tq03(void);
QueueHandle_t q1;
QueueHandle_t q2;
BaseType_t q2buf[6]; /* one item larger than max capacity */

void test_queues(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   tq00();
   tq01();
   tq02();
//   tq03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "QUEUES PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "QUEUES FAILED");
   sb_MsgDisplay();
}

/* tq00 Test queue create, send, receive, and delete */
void tq00_t2a(void*);
void tq00_t2b(void*);

void tq00(void)
{
   /* create test tasks */
   t2a = xTaskCreateStatic(tq00_t2a, "t2a", 100, (void*)0x66, TP1, NULL, NULL);
   t2b = xTaskCreateStatic(tq00_t2b, "t2b", 100, (void*)0x66, TP1, NULL, NULL);

   /* create queues */
   q1 = xQueueCreate(10, 1);
   q2 = xQueueCreateStatic(5, 4, (uint8_t*)q2buf, NULL );

   /* start test tasks */
   vTaskPrioritySet(t2a, 2);
   vTaskPrioritySet(t2b, 2);

   /* cleanup */
   vQueueDelete(q1);
   vQueueDelete(q2);
   q1 = q2 = NULL;
   vTaskDelete(t2a);
   vTaskDelete(t2b);
   t2a = t2b = NULL;
}

void tq00_t2a(void*)
{
   BaseType_t i;
   char buf1[11];
   BaseType_t buf2[6];

   /* load buffers */
   for (i = 0; i < 11; i++)
      buf1[i] = i;
   for (i = 0; i < 6; i++)
      buf2[i] = i*4;

   /* fill q1 and verify it will not overfill */
   for (i = 0; i < 10; i++)
      if (xQueueSend(q1, (const void*)&buf1[i], 0) != pdPASS)
         tfail();
   if (xQueueSend(q1, (const void*)&buf1[10], 0) != errQUEUE_FULL)
      tfail();

   /* fill q2 and verify it will not overfill */
   for (i = 0; i < 5; i++)
      if (xQueueSendToBack(q2, (const void*)&buf2[i], 0) != pdPASS)
         tfail();
   if (xQueueSendToBack(q2, (const void*)&buf2[5], 0) != errQUEUE_FULL)
      tfail();
}

void tq00_t2b(void*)
{
   BaseType_t i;
   char buf1[10];
   BaseType_t buf2[5];

   /* unload q1 and verify it will not over-read */
   for (i = 0; i < 10; i++)
      if (xQueueReceive(q1, (void*)&buf1[i], 0) != pdPASS)
         tfail();
   if (xQueueReceive(q1, (void*)&buf1[10], 0) != errQUEUE_EMPTY)
      tfail();

   /* unload q2 and verify it will not over-read */
   for (i = 0; i < 5; i++)
      if (xQueueReceive(q2, (void*)&buf2[i], 0) != pdPASS)
         tfail();
   if (xQueueReceive(q2, (void*)&buf2[5], 0) != errQUEUE_EMPTY)
      tfail();

   /* check buffers */
   for (i = 0; i < 10; i++)
      if (buf1[i] != i)
         tfail();
   for (i = 0; i < 5; i++)
      if (buf2[i] != i*4)
         tfail();
}

/* tq01 Test queue register, get name, items waiting, space available, and reset */
void tq01_t2a(void*);
void tq01_t2b(void*);

void tq01(void)
{
   /* create test tasks */
   t2a = xTaskCreateStatic(tq01_t2a, "t2a", 100, (void*)0x66, TP1, NULL, NULL);
   t2b = xTaskCreateStatic(tq01_t2b, "t2b", 100, (void*)0x66, TP1, NULL, NULL);

   /* create and register q2 */
   q2 = xQueueCreateStatic(5, 4, (uint8_t*)q2buf, NULL );
   vQueueAddToRegistry(q2, "q2");

   /* start test tasks */
   vTaskPrioritySet(t2a, 2);
   vTaskPrioritySet(t2b, 2);

   /* cleanup */
   vQueueDelete(q2);
   #if defined(SMX_DEBUG)
   smx_HTDelete(q2);
   #endif
   q2 = NULL;
   vTaskDelete(t2a);
   vTaskDelete(t2b);
   t2a = t2b = NULL;
}

void tq01_t2a(void*)
{
   BaseType_t i;
   BaseType_t buf2[5];

  #if defined(SMX_DEBUG)
   /* check q2 name */
   if (pcQueueGetName(q2) != "q2")
      tfail();
  #endif

   /* load buf2 */
   for (i = 0; i < 5; i++)
      buf2[i] = i*4;

   /* fill q2  */
   for (i = 0; i < 5; i++)
      if (xQueueSendToBack(q2, (const void*)&buf2[i], 0) != pdPASS)
         tfail();
}

void tq01_t2b(void*)
{
   BaseType_t i;
   BaseType_t buf2[5];

   /* test for q2 full and no spaces available */
   if (uxQueueMessagesWaiting(q2) != 5 || uxQueueSpacesAvailable(q2) != 0)
      tfail();

   /* unload q2 */
   for (i = 0; i < 5; i++)
      if (xQueueReceive(q2, (void*)&buf2[i], 0) != pdPASS)
         tfail();

   /* test for q2 empty and 5 spaces available */
   if (uxQueueMessagesWaiting(q2) != 0 || uxQueueSpacesAvailable(q2) != 5)
      tfail();
}

/* tq02 Test queue send, receive, and other FromISR functions */
void tq02_t2a(void*);
void tq02_ISR(void);
void tq02_LSR_main(u32 a);
LCB_PTR tq02_LSR;

void tq02(void)
{
   /* create test tasks */
   t2a = xTaskCreateStatic(tq02_t2a, "t2a", 100, (void*)0x66, TP1, NULL, NULL);

   /* create LSR */
   tq02_LSR = smx_LSRCreate((FUN_PTR)tq02_LSR_main, SMX_FL_TRUST, "tq02_LSR");

   /* create queues and add to registry */
   q1 = xQueueCreate(10, 1);
   q2 = xQueueCreateStatic(5, 4, (uint8_t*)q2buf, NULL );
   vQueueAddToRegistry(q1, "q1");
   vQueueAddToRegistry(q2, "q2");

   /* start test tasks and wait for completion */
   vTaskPrioritySet(t2a, 2);
   wait = true;
   while (wait){}

   /* cleanup */
   vQueueDelete(q1);
   vQueueDelete(q2);
  #if defined(SMX_DEBUG)
   smx_HTDelete(q1);
   smx_HTDelete(q2);
  #endif
   q1 = q2 = NULL;
   smx_LSRDelete(&tq02_LSR);
   vTaskDelete(t2a);
   t2a = NULL;
}

void tq02_t2a(void*)
{
   BaseType_t i;
   char buf1[11];
   BaseType_t buf2[6];

   /* load buffers */
   for (i = 0; i < 11; i++)
      buf1[i] = i;
   for (i = 0; i < 6; i++)
      buf2[i] = i*4;

   /* fill q1 */
   for (i = 0; i < 10; i++)
      xQueueSend(q1, (const void*)&buf1[i], 0);

   /* fill q2 */
   for (i = 0; i < 5; i++)
      xQueueSendToBack(q2, (const void*)&buf2[i], 0);

   /* enable interrupt and wait */
   tick_cbptr = tq02_ISR;
   tick_cben = true;
   vTaskDelay(5);

   /* unload q1 */
   for (i = 0; i < 10; i++)
      if (xQueueReceive(q1, (void*)&buf1[i], 0) != pdPASS)
         tfail();

   /* unload q2 */
   for (i = 0; i < 5; i++)
      if (xQueueReceive(q2, (void*)&buf2[i], 0) != pdPASS)
         tfail();

   /* check buffers */
   for (i = 0; i < 10; i++)
      if (buf1[i] != 10 - i)
         tfail();
   for (i = 0; i < 5; i++)
      if (buf2[i] != 20 - i*4)
         tfail();

   /* enable tfrp to continue */
   wait = false;
}

void tq02_ISR(void)
{
   smx_LSR_INVOKE(tq02_LSR, 0);
}

void tq02_LSR_main(u32 par)
{
   BaseType_t i;
   char buf1[10];
   BaseType_t buf2[5];

   /* test for q1 and q2 full */
   if (uxQueueMessagesWaitingFromISR(q1) != 10 || uxQueueMessagesWaitingFromISR(q2) != 5)
      tfail();

   /* unload q1 */
   for (i = 0; i < 10; i++)
      if (xQueueReceiveFromISR(q1, (void*)&buf1[i], 0) != pdPASS)
         tfail();

   /* unload q2 */
   for (i = 0; i < 5; i++)
      if (xQueueReceiveFromISR(q2, (void*)&buf2[i], 0) != pdPASS)
         tfail();

   /* check buffers */
   for (i = 0; i < 10; i++)
      if (buf1[i] != i)
         tfail();
   for (i = 0; i < 5; i++)
      if (buf2[i] != i*4)
         tfail();

   if (!xQueueIsQueueEmptyFromISR(q2))
      tfail();

   /* load buffers */
   for (i = 0; i < 10; i++)
      buf1[i] = 10 - i;
   for (i = 0; i < 5; i++)
      buf2[i] = 20 - i*4;

   /* fill q1 */
   for (i = 0; i < 10; i++)
      if (xQueueSendFromISR(q1, (const void*)&buf1[i], 0) != pdPASS)
         tfail();

   /* fill q2 */
   for (i = 0; i < 5; i++)
      if (xQueueSendToBackFromISR(q2, (const void*)&buf2[i], 0) != pdPASS)
         tfail();

   if (!xQueueIsQueueFullFromISR(q2))
      tfail();
}

/******************************************************************************
*                                                                             *
*                            TIMER FUNCTION TESTS                             *
*                                                                             *
******************************************************************************/

void ti00(void);
void ti01(void);
void ti02(void);
void ti03(void);
TimerHandle_t  t1;
TimerHandle_t  t2;
TMRCB_PTR t1x;
TMRCB_PTR t2x;

void test_timers(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   ti00();
   ti01();
//   ti02();
//   ti03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "TIMERS PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "TIMERS FAILED");
   sb_MsgDisplay();
}

/* ti00 Test qtimer create, start, stop, and delete */
void ti00_LSR1(u32 par);
void ti00_LSR2(u32 par);
u32 start_time;
u32 cycle_count;

void ti00(void)
{
   /* create and start timers */
   if ((t1 = xTimerCreate("t1", 10, pdFALSE, (void*)1, (TimerCallbackFunction_t)ti00_LSR1)) == NULL)
      tfail();
   t1x = (TMRCB_PTR)t1;

   if ((t2 = xTimerCreateStatic("t2", 5, pdTRUE, (void*)2, (TimerCallbackFunction_t)ti00_LSR2, NULL)) == NULL)
      tfail();
   t2x = (TMRCB_PTR)t2;

   /* start timers and wait for timeouts */
   start_time = smx_etime;
   cycle_count = 0;
   xTimerStart(t1, 0);
   xTimerStart(t2, 0);

   /* test timer status functions */
   if (!xTimerIsTimerActive(t1) || xTimerGetExpiryTime(t1) != (10 + smx_etime) || 
        pcTimerGetName(t1) != "t1")
      tfail();

   /* test timer change period */
   if (xTimerGetExpiryTime(t2) != (5 + smx_etime))
      tfail();
   xTimerChangePeriod(t2, 6, 0);
   if (xTimerGetExpiryTime(t2) != (6 + smx_etime))
      tfail();

   /* test change timer id */
   if (pvTimerGetTimerID(t1) != (void*)1)
      tfail();
   vTimerSetTimerID(t1, (void*)7);
   if (pvTimerGetTimerID(t1) != (void*)7)
      tfail();

   /* wait for timeouts */
   wait = true;
   while (wait);

   /* cleanup */
   xTimerDelete(t1, 0);
   t1 = NULL;
   xTimerDelete(t2, 0);
   t2 = NULL;
}

void ti00_LSR1(u32 par)
{
   if (smx_etime != (start_time + 16) || par != 7)
      tfail();
   wait = false;
}

void ti00_LSR2(u32 par)
{
   switch (cycle_count++)
   {
      case 0:
         if (smx_etime != (start_time + 6) || par !=2)
            tfail();
         xTimerReset(t1, 0);
         break;
      case 1:
         if (smx_etime != start_time + 12)
            tfail();
         xTimerStop(t2, 0);
   }
}

/* ti01 Test timer from ISR functions */
void ti01_ISR(void);
void ti01_LSR_main(u32 par);
void ti01_LSR1_main(u32 par);
void ti01_LSR2_main(u32 par);
LCB_PTR ti01_LSR;

void ti01(void)
{
   /* create LSR */
   ti01_LSR = smx_LSRCreate((FUN_PTR)ti01_LSR_main, SMX_FL_TRUST, "ti01_LSR", NULL, 0, NULL);

   /* create timers */
   if ((t1 = xTimerCreate("t1", 10, pdFALSE, (void*)1, (TimerCallbackFunction_t)ti01_LSR1_main)) == NULL)
      tfail();
   t1x = (TMRCB_PTR)t1;

   if ((t2 = xTimerCreate("t2", 5, pdTRUE, (void*)2, (TimerCallbackFunction_t)ti01_LSR2_main)) == NULL)
      tfail();
   t2x = (TMRCB_PTR)t2;

   /* set test values and start t1 */
   start_time = smx_etime;
   cycle_count = 0;
   xTimerStart(t1, 0);

   /* enable interrupt to invoke LSR and wait for timeouts */
   tick_cbptr = ti01_ISR;
   tick_cben = true;

   /* wait for timeouts */
   wait = true;
   while (wait);

   /* cleanup */
   xTimerDelete(t1, 0);
   t1 = NULL;
   xTimerDelete(t2, 0);
   t2 = NULL;
   smx_LSRDelete(&ti01_LSR);
}

void ti01_ISR(void)
{
   smx_LSR_INVOKE(ti01_LSR, 0);
}

void ti01_LSR_main(u32 par)
{
   xTimerResetFromISR(t1, 0);
   xTimerStartFromISR(t2, 0);
   xTimerChangePeriodFromISR(t2, 6, 0);
   tick_cben = false;
}

void ti01_LSR1_main(u32 par)
{
   if (smx_etime != (start_time + 11) || par != 1)
      tfail();
   wait = false;
}

void ti01_LSR2_main(u32 par)
{
   switch (cycle_count++)
   {
      case 0:
         if (smx_etime != (start_time + 7) || par != 2)
            tfail();
         xTimerStopFromISR(t2, 0);
         break;
      case 1:
         tfail();
   }
}


/******************************************************************************
*                                                                             *
*                         EVENT GROUP FUNCTION TESTS                          *
*                                                                             *
******************************************************************************/

void tg00(void);
void tg01(void);
void tg02(void);
void tg03(void);
EventGroupHandle_t g1;
EventGroupHandle_t g2;
EGCB* g1x;
EGCB* g2x;

void test_event_groups(void)
{
   ts_pass   = true;
   smx_errno = SMXE_OK;
   tg00();
   tg01();
//   tg02();
//   tg03();

   if(ts_pass)
      sb_MsgOut(SB_MSG_INFO, "EVENT GROUPS PASSED");
   else
      sb_MsgOut(SB_MSG_WARN, "EVENT GROUPS FAILED");
   sb_MsgDisplay();
}

/* tg00 Test queue create, send, receive, and delete */
void tg00_t2a(void*);
void tg00_t2b(void*);

void tg00(void)
{
   /* create test tasks */
   t2a = xTaskCreateStatic(tg00_t2a, "t2a", 100, (void*)0x66, TP1, NULL, NULL);
   t2ax = (TCB*)t2a;
   t2b = xTaskCreateStatic(tg00_t2b, "t2b", 100, (void*)0x66, TP1, NULL, NULL);
   t2bx = (TCB*)t2b;

   /* create event groups */
   g1 = xEventGroupCreate();
   g1x = (EGCB*)g1;
   g2 = xEventGroupCreateStatic(NULL);
   g2x = (EGCB*)g2;

   /* start test tasks */
   vTaskPrioritySet(t2a, 2);
   vTaskPrioritySet(t2b, 2);

   /* wait for timeouts */
   wait = true;
   while (wait);

   /* cleanup */
   vEventGroupDelete(g1);
   vEventGroupDelete(g2);
   g1 = g2 = NULL;
   vTaskDelete(t2a);
   vTaskDelete(t2b);
   t2a = t2b = NULL;
}

void tg00_t2a(void*)
{
   /* set g1 flags = 0x1010 */
   if (xEventGroupSetBits(g1, 0x1010) != 0x1010)
      tfail();

   /* change g1 flags = 0x1000 */
   if (xEventGroupClearBits(g1, 0x10) != 0x1010)
      tfail();
   if (((EGCB_PTR)g1)->flags != 0x1000)
      tfail();

   /* wait on g2 AND 0x0011 -- pass and not clear */
   if ((xEventGroupWaitBits(g2, 0x0011, pdFALSE, pdTRUE, 5) != 0x0011))
      tfail();
   if (((EGCB_PTR)g2)->flags != 0x0011)
      tfail();
}

void tg00_t2b(void*)
{
   /* set g2 flags = 0x0011 */
   if (xEventGroupSetBits(g2, 0x0011) != 0x0011)
      tfail();

   /* wait on g1 flags AND 0x1010 -- fail and timeout */
   if (xEventGroupGetBits(g1) != 0x1000)
      tfail();
   if (xEventGroupWaitBits(g1, 0x1010, pdTRUE, pdTRUE, 5) != 0)
      tfail();

   /* wait on g1 flags OR 0x1010 -- pass and clear */
   if (xEventGroupWaitBits(g1, 0x1010, pdTRUE, pdFALSE, 5) != 0x1000)
      tfail();
   if (((EGCB_PTR)g1)->flags != 0)
      tfail();

   /* allow tfrp to continue */
   wait = false;
}

/* tg01 Test sync and ISR functions */
void tg01_ISR(void);
void tg01_LSR_main(u32 par);
LCB_PTR tg01_LSR;

void tg01(void)
{
   /* create event group */
   g1 = xEventGroupCreate();
   g1x = (EGCB*)g1;

   /* create LSR */
   tg01_LSR = smx_LSRCreate((FUN_PTR)tg01_LSR_main, SMX_FL_TRUST, "tg01_LSR", NULL, 0, NULL);

   /* enable interrupt to invoke LSR */
   tick_cbptr = tg01_ISR;
   tick_cben = true;

   /* wait for LSR */
   wait = true;
   while (wait);

   /* synchronize with LSR */
   if (xEventGroupSync(g1, 0x01, 0x11, 5) != 0x11)
      tfail();

   /* cleanup */
   smx_LSRDelete(&tg01_LSR);
   vEventGroupDelete(g1);
   g1 = NULL;
}

void tg01_ISR(void)
{
   smx_LSR_INVOKE(tg01_LSR, 0);
}

void tg01_LSR_main(u32 par)
{
   if (xEventGroupGetBitsFromISR(g1) != 0)
      tfail();
   xEventGroupSetBitsFromISR(g1, 0x10, 0);
   wait = false;
}