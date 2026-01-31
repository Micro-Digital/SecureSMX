/*
* tx_api.h                                                  Version 6.0.0
*
* TXPort Application Interface for TXPort.
*
* Author: Ralph Moore, Micro Digital Inc.
*
* For ThreadX/Azure Kernel V6.1
*
*****************************************************************************/

#ifndef TX_API_H
#define TX_API_H

#include "smx.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "tx_port.h"

/* redefinition of control blocks to handles */
#define TX_THREAD                TCB_PTR
#define TX_BLOCK_POOL            PCB
#define TX_BYTE_POOL             EHV
#define TX_EVENT_FLAGS_GROUP     EGCB_PTR
#define TX_MUTEX                 MUCB_PTR
#define TX_QUEUE                 PICB_PTR
#define TX_SEMAPHORE             SCB_PTR
#define TX_TIMER                 TMRCB_PTR

/* API return values.  */
#if 1
#define TX_SUCCESS                      ((UINT) 0x00)
#define TX_DELETED                      ((UINT) 0x01)
#define TX_POOL_ERROR                   ((UINT) 0x02)
#define TX_PTR_ERROR                    ((UINT) 0x03)
#define TX_WAIT_ERROR                   ((UINT) 0x04)
#define TX_SIZE_ERROR                   ((UINT) 0x05)
#define TX_GROUP_ERROR                  ((UINT) 0x06)
#define TX_NO_EVENTS                    ((UINT) 0x07)
#define TX_OPTION_ERROR                 ((UINT) 0x08)
#define TX_QUEUE_ERROR                  ((UINT) 0x09)
#define TX_QUEUE_EMPTY                  ((UINT) 0x0A)
#define TX_QUEUE_FULL                   ((UINT) 0x0B)
#define TX_SEMAPHORE_ERROR              ((UINT) 0x0C)
#define TX_NO_INSTANCE                  ((UINT) 0x0D)
#define TX_THREAD_ERROR                 ((UINT) 0x0E)
#define TX_PRIORITY_ERROR               ((UINT) 0x0F)
#define TX_NO_MEMORY                    ((UINT) 0x10)
#define TX_START_ERROR                  ((UINT) 0x10)
#define TX_DELETE_ERROR                 ((UINT) 0x11)
#define TX_RESUME_ERROR                 ((UINT) 0x12)
#define TX_CALLER_ERROR                 ((UINT) 0x13)
#define TX_SUSPEND_ERROR                ((UINT) 0x14)
#define TX_TIMER_ERROR                  ((UINT) 0x15)
#define TX_TICK_ERROR                   ((UINT) 0x16)
#define TX_ACTIVATE_ERROR               ((UINT) 0x17)
#define TX_THRESH_ERROR                 ((UINT) 0x18)
#define TX_SUSPEND_LIFTED               ((UINT) 0x19)
#define TX_WAIT_ABORTED                 ((UINT) 0x1A)
#define TX_WAIT_ABORT_ERROR             ((UINT) 0x1B)
#define TX_MUTEX_ERROR                  ((UINT) 0x1C)
#define TX_NOT_AVAILABLE                ((UINT) 0x1D)
#define TX_NOT_OWNED                    ((UINT) 0x1E)
#define TX_INHERIT_ERROR                ((UINT) 0x1F)
#define TX_NOT_DONE                     ((UINT) 0x20)
#define TX_CEILING_EXCEEDED             ((UINT) 0x21)
#define TX_INVALID_CEILING              ((UINT) 0x22)
#define TX_FEATURE_NOT_ENABLED          ((UINT) 0xFF)
#else
/* trial error type matchup. (TX numbers need to be changed.) */
#define TX_SUCCESS                      SMXE_OK
#define TX_DELETED                      ((UINT) 0x01)
#define TX_POOL_ERROR                   SMXE_WRONG_POOL
#define TX_PTR_ERROR                    SMXE_NULL_PTR_REF
#define TX_WAIT_ERROR                   SMXE_TMO
#define TX_SIZE_ERROR                   SMXE_UNKNOWN_SIZE
#define TX_GROUP_ERROR                  (SMXE_INV_EGCB
#define TX_NO_EVENTS                    ((UINT) 0x07)
#define TX_OPTION_ERROR                 ((UINT) 0x08)
#define TX_QUEUE_ERROR                  SMXE_INV_PICB
#define TX_QUEUE_EMPTY                  ((UINT) 0x0A)
#define TX_QUEUE_FULL                   ((UINT) 0x0B)
#define TX_SEMAPHORE_ERROR              SMXE_INV_SCB
#define TX_NO_INSTANCE                  ((UINT) 0x0D)
#define TX_THREAD_ERROR                 SMXE_INV_TCB
#define TX_PRIORITY_ERROR               SMXE_INV_PRI
#define TX_NO_MEMORY                    SMXE_INSUFF_HEAP || SMXE_HEAP_ERROR
#define TX_START_ERROR                  ((UINT) 0x10)
#define TX_DELETE_ERROR                 ((UINT) 0x11)
#define TX_RESUME_ERROR                 ((UINT) 0x12)
#define TX_CALLER_ERROR                 ((UINT) 0x13)
#define TX_SUSPEND_ERROR                ((UINT) 0x14)
#define TX_TIMER_ERROR                  SMXE_INV_TMRCB
#define TX_TICK_ERROR                   SMXE_INV_TIME
#define TX_ACTIVATE_ERROR               ((UINT) 0x17)
#define TX_THRESH_ERROR                 ((UINT) 0x18)
#define TX_SUSPEND_LIFTED               ((UINT) 0x19)
#define TX_WAIT_ABORTED                 ((UINT) 0x1A)
#define TX_WAIT_ABORT_ERROR             ((UINT) 0x1B)
#define TX_MUTEX_ERROR                  SMXE_INV_MUCB || SMXE_MTX_ALRDY_FREE
#define TX_NOT_AVAILABLE                SMXE_INV_FUNC || SMXE_OP_NOT_ALLOWED
#define TX_NOT_OWNED                    SMXE_MTX_NON_ONR_REL
#define TX_INHERIT_ERROR                ((UINT) 0x1F)
#define TX_NOT_DONE                     ((UINT) 0x20)
#define TX_CEILING_EXCEEDED             ((UINT) 0x21)
#define TX_INVALID_CEILING              ((UINT) 0x22)
#define TX_FEATURE_NOT_ENABLED          SMXE_OP_NOT_ALLOWED
#endif

#ifndef TX_TIMER_TICKS_PER_SECOND
#define TX_TIMER_TICKS_PER_SECOND       ((ULONG) SMX_TICKS_PER_SEC)
#endif

#ifndef ALIGN_TYPE_DEFINED
#define ALIGN_TYPE                      ULONG
#endif

/* API input parameters and general constants.  */
#define TX_NO_WAIT                      ((ULONG)  0)
#define TX_WAIT_FOREVER                 ((ULONG)  0xFFFFFFFFUL)
#define TX_AND                          ((UINT)   2)
#define TX_AND_CLEAR                    ((UINT)   3)
#define TX_OR                           ((UINT)   0)
#define TX_OR_CLEAR                     ((UINT)   1)
#define TX_1_ULONG                      ((UINT)   1)
#define TX_2_ULONG                      ((UINT)   2)
#define TX_4_ULONG                      ((UINT)   4)
#define TX_8_ULONG                      ((UINT)   8)
#define TX_16_ULONG                     ((UINT)   16)
#define TX_NO_TIME_SLICE                ((ULONG)  0)
#define TX_AUTO_START                   ((UINT)   1)
#define TX_DONT_START                   ((UINT)   0)
#define TX_AUTO_ACTIVATE                ((UINT)   1)
#define TX_NO_ACTIVATE                  ((UINT)   0)
#define TX_TRUE                         ((UINT)   1)
#define TX_FALSE                        ((UINT)   0)
#define TX_NULL                         (         0)
#define TX_INHERIT                      ((UINT)   1)
#define TX_NO_INHERIT                   ((UINT)   0)
#define TX_THREAD_ENTRY                 ((UINT)   0)
#define TX_THREAD_EXIT                  ((UINT)   1)
#define TX_NO_SUSPENSIONS               ((UINT)   0)
#define TX_NO_MESSAGES                  ((UINT)   0)
#define TX_EMPTY                        ((ULONG)  0)
#define TX_CLEAR_ID                     ((ULONG)  0)
#define TX_STACK_FILL                   ((ULONG)  0xEFEFEFEFUL)

/* Thread execution state values.  */
#define TX_READY                        ((UINT) 0)
#define TX_COMPLETED                    ((UINT) 1)
#define TX_TERMINATED                   ((UINT) 2)
#define TX_SUSPENDED                    ((UINT) 3)
#define TX_SLEEP                        ((UINT) 4)
#define TX_QUEUE_SUSP                   ((UINT) 5)
#define TX_SEMAPHORE_SUSP               ((UINT) 6)
#define TX_EVENT_FLAG                   ((UINT) 7)
#define TX_BLOCK_MEMORY                 ((UINT) 8)
#define TX_BYTE_MEMORY                  ((UINT) 9)
#define TX_IO_DRIVER                    ((UINT) 10)
#define TX_FILE                         ((UINT) 11)
#define TX_TCP_IP                       ((UINT) 12)
#define TX_MUTEX_SUSP                   ((UINT) 13)
#define TX_PRIORITY_CHANGE              ((UINT) 14)

VOID        tx_application_define(VOID *first_unused_memory);

/* block memory pool management function prototypes.  */
UINT        tx_block_allocate(TX_BLOCK_POOL *pool_ptr, VOID **block_ptr, ULONG wait_option);
UINT        tx_block_pool_create(TX_BLOCK_POOL *pool_ptr, CHAR *name_ptr, ULONG block_size,
                    VOID *pool_start, ULONG pool_size);
UINT        tx_block_pool_delete(TX_BLOCK_POOL *pool_ptr);
UINT        tx_block_pool_info_get(TX_BLOCK_POOL *pool_ptr, CHAR **name, ULONG *available_blocks, 
                    ULONG *total_blocks, TX_THREAD **first_suspended, 
                    ULONG *suspended_count, TX_BLOCK_POOL **next_pool);
UINT        tx_block_pool_performance_info_get(TX_BLOCK_POOL *pool_ptr, ULONG *allocates, ULONG *releases,
                    ULONG *suspensions, ULONG *timeouts);
UINT        tx_block_pool_performance_system_info_get(ULONG *allocates, ULONG *releases,
                    ULONG *suspensions, ULONG *timeouts);
UINT        tx_block_pool_prioritize(TX_BLOCK_POOL *pool_ptr);
UINT        tx_block_release(VOID *block_ptr);

/* byte memory pool management function prototypes.  */
UINT        tx_byte_allocate(TX_BYTE_POOL *pool_ptr, VOID **memory_ptr, ULONG memory_size,
                    ULONG wait_option);
UINT        tx_byte_pool_create(TX_BYTE_POOL *pool_ptr, CHAR *name_ptr, VOID *pool_start,
                    ULONG pool_size);
UINT        tx_byte_pool_delete(TX_BYTE_POOL *pool_ptr);
UINT        tx_byte_pool_info_get(TX_BYTE_POOL *pool_ptr, CHAR **name, ULONG *available_bytes, 
                    ULONG *fragments, TX_THREAD **first_suspended, 
                    ULONG *suspended_count, TX_BYTE_POOL **next_pool);
UINT        tx_byte_pool_performance_info_get(TX_BYTE_POOL *pool_ptr, ULONG *allocates, ULONG *releases,
                    ULONG *fragments_searched, ULONG *merges, ULONG *splits, ULONG *suspensions, ULONG *timeouts);
UINT        tx_byte_pool_performance_system_info_get(ULONG *allocates, ULONG *releases,
                    ULONG *fragments_searched, ULONG *merges, ULONG *splits, ULONG *suspensions, ULONG *timeouts);
UINT        tx_byte_pool_prioritize(TX_BYTE_POOL *pool_ptr);
UINT        tx_byte_release(VOID *memory_ptr);

/* event flags management function prototypes.  */
UINT        tx_event_flags_create(TX_EVENT_FLAGS_GROUP *group_ptr, CHAR *name_ptr);
UINT        tx_event_flags_delete(TX_EVENT_FLAGS_GROUP *group_ptr);
UINT        tx_event_flags_get(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG requested_flags,
                    UINT get_option, ULONG *actual_flags_ptr, ULONG wait_option);
UINT        tx_event_flags_info_get(TX_EVENT_FLAGS_GROUP *group_ptr, CHAR **name, ULONG *current_flags, 
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_EVENT_FLAGS_GROUP **next_group);
UINT        tx_event_flags_performance_info_get(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG *sets, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts);
UINT        tx_event_flags_performance_system_info_get(ULONG *sets, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts);
UINT        tx_event_flags_set(TX_EVENT_FLAGS_GROUP *group_ptr, ULONG flags_to_set, 
                    UINT set_option);
UINT        tx_event_flags_set_notify(TX_EVENT_FLAGS_GROUP *group_ptr, VOID (*events_set_notify)(TX_EVENT_FLAGS_GROUP *notify_group_ptr));

/* initialization function prototypes.  */
VOID        tx_kernel_enter(VOID);

/* mutex management function prototypes.  */
UINT        tx_mutex_create(TX_MUTEX *mutex_ptr, CHAR *name_ptr, UINT inherit);
UINT        tx_mutex_delete(TX_MUTEX *mutex_ptr);
UINT        tx_mutex_get(TX_MUTEX *mutex_ptr, ULONG wait_option);
UINT        tx_mutex_info_get(TX_MUTEX *mutex_ptr, CHAR **name, ULONG *count, TX_THREAD **owner, 
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_MUTEX **next_mutex);
UINT        tx_mutex_performance_info_get(TX_MUTEX *mutex_ptr, ULONG *puts, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts, ULONG *inversions, ULONG *inheritances);
UINT        tx_mutex_performance_system_info_get(ULONG *puts, ULONG *gets, ULONG *suspensions, ULONG *timeouts,
                    ULONG *inversions, ULONG *inheritances);
UINT        tx_mutex_prioritize(TX_MUTEX *mutex_ptr);
UINT        tx_mutex_put(TX_MUTEX *mutex_ptr);

/* queue management function prototypes.  */
UINT        tx_queue_create(TX_QUEUE *queue_ptr, CHAR *name_ptr, UINT message_size, 
                        VOID *queue_start, ULONG queue_size);
UINT        tx_queue_delete(TX_QUEUE *queue_ptr);
UINT        tx_queue_flush(TX_QUEUE *queue_ptr);
UINT        tx_queue_info_get(TX_QUEUE *queue_ptr, CHAR **name, ULONG *enqueued, ULONG *available_storage,
                    TX_THREAD **first_suspended, ULONG *suspended_count, TX_QUEUE **next_queue);
UINT        tx_queue_performance_info_get(TX_QUEUE *queue_ptr, ULONG *messages_sent, ULONG *messages_received,
                    ULONG *empty_suspensions, ULONG *full_suspensions, ULONG *full_errors, ULONG *timeouts);
UINT        tx_queue_performance_system_info_get(ULONG *messages_sent, ULONG *messages_received,
                    ULONG *empty_suspensions, ULONG *full_suspensions, ULONG *full_errors, ULONG *timeouts);
UINT        tx_queue_prioritize(TX_QUEUE *queue_ptr);
UINT        tx_queue_receive(TX_QUEUE *queue_ptr, VOID *destination_ptr, ULONG wait_option);
UINT        tx_queue_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option);
UINT        tx_queue_send_notify(TX_QUEUE *queue_ptr, VOID (*queue_send_notify)(TX_QUEUE *notify_queue_ptr));
UINT        tx_queue_front_send(TX_QUEUE *queue_ptr, VOID *source_ptr, ULONG wait_option);

/* semaphore management function prototypes.  */
UINT        tx_semaphore_ceiling_put(TX_SEMAPHORE *semaphore_ptr, ULONG ceiling);
UINT        tx_semaphore_create(TX_SEMAPHORE *semaphore_ptr, CHAR *name_ptr, ULONG initial_count);
UINT        tx_semaphore_delete(TX_SEMAPHORE *semaphore_ptr);
UINT        tx_semaphore_get(TX_SEMAPHORE *semaphore_ptr, ULONG wait_option);
UINT        tx_semaphore_info_get(TX_SEMAPHORE *semaphore_ptr, CHAR **name, ULONG *current_value, 
                    TX_THREAD **first_suspended, ULONG *suspended_count, 
                    TX_SEMAPHORE **next_semaphore);
UINT        tx_semaphore_performance_info_get(TX_SEMAPHORE *semaphore_ptr, ULONG *puts, ULONG *gets,
                    ULONG *suspensions, ULONG *timeouts);
UINT        tx_semaphore_performance_system_info_get(ULONG *puts, ULONG *gets, ULONG *suspensions, ULONG *timeouts);
UINT        tx_semaphore_prioritize(TX_SEMAPHORE *semaphore_ptr);
UINT        tx_semaphore_put(TX_SEMAPHORE *semaphore_ptr);
UINT        tx_semaphore_put_notify(TX_SEMAPHORE *semaphore_ptr, VOID (*semaphore_put_notify)(TX_SEMAPHORE *notify_semaphore_ptr));

/* thread control function prototypes.  */
VOID        tx_thread_context_save(VOID);
VOID        tx_thread_context_restore(VOID);
UINT        tx_thread_create(TX_THREAD *thread_ptr, CHAR *name_ptr, 
                VOID (*entry_function)(ULONG entry_input), ULONG entry_input,
                VOID *stack_start, ULONG stack_size, 
                UINT priority, UINT preempt_threshold, 
                ULONG time_slice, UINT auto_start);
UINT        tx_thread_delete(TX_THREAD *thread_ptr);
UINT        tx_thread_entry_exit_notify(TX_THREAD *thread_ptr, VOID (*thread_entry_exit_notify)(TX_THREAD *notify_thread_ptr, UINT type));
TX_THREAD  *tx_thread_identify(VOID);
UINT        tx_thread_info_get(TX_THREAD *thread_ptr, CHAR **name, UINT *state, ULONG *run_count, 
                UINT *priority, UINT *preemption_threshold, ULONG *time_slice, 
                TX_THREAD **next_thread, TX_THREAD **next_suspended_thread);
UINT        tx_thread_interrupt_control(UINT new_posture);
UINT        tx_thread_performance_info_get(TX_THREAD *thread_ptr, ULONG *resumptions, ULONG *suspensions, 
                ULONG *solicited_preemptions, ULONG *interrupt_preemptions, ULONG *priority_inversions,
                ULONG *time_slices, ULONG *relinquishes, ULONG *timeouts, ULONG *wait_aborts, TX_THREAD **last_preempted_by);
UINT        tx_thread_performance_system_info_get(ULONG *resumptions, ULONG *suspensions,
                ULONG *solicited_preemptions, ULONG *interrupt_preemptions, ULONG *priority_inversions,
                ULONG *time_slices, ULONG *relinquishes, ULONG *timeouts, ULONG *wait_aborts,
                ULONG *non_idle_returns, ULONG *idle_returns);
UINT        tx_thread_preemption_change(TX_THREAD *thread_ptr, UINT new_threshold,
                        UINT *old_threshold);
UINT        tx_thread_priority_change(TX_THREAD *thread_ptr, UINT new_priority,
                        UINT *old_priority);
VOID        tx_thread_relinquish(VOID);
UINT        tx_thread_reset(TX_THREAD *thread_ptr);
UINT        tx_thread_resume(TX_THREAD *thread_ptr);
UINT        tx_thread_sleep(ULONG timer_ticks);
UINT        tx_thread_stack_error_notify(VOID (*stack_error_handler)(TX_THREAD *thread_ptr));
UINT        tx_thread_suspend(TX_THREAD *thread_ptr);
UINT        tx_thread_terminate(TX_THREAD *thread_ptr);
UINT        tx_thread_time_slice_change(TX_THREAD *thread_ptr, ULONG new_time_slice, ULONG *old_time_slice);
UINT        tx_thread_wait_abort(TX_THREAD *thread_ptr);

/* timer management function prototypes.  */
UINT        tx_timer_activate(TX_TIMER *timer_ptr);
UINT        tx_timer_change(TX_TIMER *timer_ptr, ULONG initial_ticks, ULONG reschedule_ticks);
UINT        tx_timer_create(TX_TIMER *timer_ptr, CHAR *name_ptr, 
                VOID (*expiration_function)(ULONG input), ULONG expiration_input,
                ULONG initial_ticks, ULONG reschedule_ticks, UINT auto_activate);
UINT        tx_timer_deactivate(TX_TIMER *timer_ptr);
UINT        tx_timer_delete(TX_TIMER *timer_ptr);
UINT        tx_timer_info_get(TX_TIMER *timer_ptr, CHAR **name, UINT *active, ULONG *remaining_ticks, 
                ULONG *reschedule_ticks, TX_TIMER **next_timer);
UINT        tx_timer_performance_info_get(TX_TIMER *timer_ptr, ULONG *activates, ULONG *reactivates,
                ULONG *deactivates, ULONG *expirations, ULONG *expiration_adjusts);
UINT        tx_timer_performance_system_info_get(ULONG *activates, ULONG *reactivates,
                ULONG *deactivates, ULONG *expirations, ULONG *expiration_adjusts);
ULONG       tx_time_get(VOID);
VOID        tx_time_set(ULONG new_time);

/* trace API function prototypes.  */
UINT        tx_trace_enable(VOID *trace_buffer_start, ULONG trace_buffer_size, ULONG registry_entries);
UINT        tx_trace_event_filter(ULONG event_filter_bits);
UINT        tx_trace_event_unfilter(ULONG event_unfilter_bits);
UINT        tx_trace_disable(VOID);
VOID        tx_trace_isr_enter_insert(ULONG isr_id);
VOID        tx_trace_isr_exit_insert(ULONG isr_id);
UINT        tx_trace_buffer_full_notify(VOID (*full_buffer_callback)(VOID *buffer));
UINT        tx_trace_user_event_insert(ULONG event_id, ULONG info_field_1, ULONG info_field_2, ULONG info_field_3, ULONG info_field_4);
UINT        tx_trace_interrupt_control(UINT new_posture);

/* middleware-related definitions */

#ifndef FX_NO_LOCAL_PATH
#ifndef FX_LOCAL_PATH_SETUP
#ifndef FX_SINGLE_THREAD
#define FX_LOCAL_PATH_SETUP
#endif
#endif
#endif
#define _tx_thread_current_ptr   smx_ct
#define tx_thread_filex_ptr      fsp

#ifdef __cplusplus
}
#endif
#endif