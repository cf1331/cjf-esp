/**
 * @file freertos.h
 * @brief Convenience header including all FreeRTOS RAII wrappers
 *
 * This header provides statically-allocated RAII wrappers for FreeRTOS primitives:
 * - task<StackSize> - Tasks with embedded stack storage
 * - task_with_controller<StackSize> - Tasks with lifecycle management
 * - queue<ItemType, QueueLength> - Type-safe queues with embedded storage
 * - binary_semaphore - Binary semaphores for signaling
 * - mutex - Mutexes for resource protection
 * - lock_guard - RAII lock guard for mutexes
 * - event_group - Event groups for multi-bit synchronization
 * - timer - Software timers for delayed/periodic execution
 *
 * All wrappers use static allocation (*CreateStatic APIs) to eliminate heap usage
 * and provide move-only semantics for RAII resource management.
 */

#ifndef FB667086_22F0_43C4_9192_DE00F238FBCC
#define FB667086_22F0_43C4_9192_DE00F238FBCC

#include <cjf/freertos/controllable_task.h>
#include <cjf/freertos/event_group.h>
#include <cjf/freertos/queue.h>
#include <cjf/freertos/semaphore.h>
#include <cjf/freertos/task.h>
#include <cjf/freertos/timer.h>

#endif /* FB667086_22F0_43C4_9192_DE00F238FBCC */
