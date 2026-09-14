/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_api_vmcore.h"
#include "platform_api_extension.h"
#include "posix_test_helper.h"
#include <pthread.h>
#include <cerrno>
#include <unistd.h>
#include <csignal>
#include <sys/types.h>

class EnhancedPosixThreadTest : public testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}

  public:
    WAMRRuntimeRAII<512 * 1024> runtime;
};

/******
 * Test Case: os_recursive_mutex_init_AttributeValidation_Success
 * Source: core/shared/platform/common/posix/posix_thread.c:130-145
 * Target Lines: 137 (pthread_mutexattr_init), 141 (pthread_mutexattr_settype),
 *               142 (pthread_mutex_init), 143 (pthread_mutexattr_destroy)
 * Functional Purpose: Validates the complete attribute initialization sequence:
 *                     pthread_mutexattr_init -> pthread_mutexattr_settype ->
 *                     pthread_mutex_init -> pthread_mutexattr_destroy
 * Call Path: os_recursive_mutex_init() [PUBLIC API - DIRECT CALL]
 * Coverage Goal: Exercise all pthread attribute manipulation lines
 ******/
TEST_F(EnhancedPosixThreadTest, os_recursive_mutex_init_AttributeValidation_Success)
{
    korp_mutex mutex;

    // Test the complete attribute sequence by initializing
    int result = os_recursive_mutex_init(&mutex);
    ASSERT_EQ(BHT_OK, result);

    // Validate the mutex was created with recursive attributes
    // by attempting nested locks from the same thread
    int first_lock = os_mutex_lock(&mutex);
    ASSERT_EQ(BHT_OK, first_lock);

    int second_lock = os_mutex_lock(&mutex);
    ASSERT_EQ(BHT_OK, second_lock);

    int third_lock = os_mutex_lock(&mutex);
    ASSERT_EQ(BHT_OK, third_lock);

    // Unlock in reverse order
    int third_unlock = os_mutex_unlock(&mutex);
    ASSERT_EQ(BHT_OK, third_unlock);

    int second_unlock = os_mutex_unlock(&mutex);
    ASSERT_EQ(BHT_OK, second_unlock);

    int first_unlock = os_mutex_unlock(&mutex);
    ASSERT_EQ(BHT_OK, first_unlock);

    // Clean up
    int destroy_result = os_mutex_destroy(&mutex);
    ASSERT_EQ(BHT_OK, destroy_result);
}

/******
 * Test Case: os_cond_wait_MultipleWaiters_Success
 * Source: core/shared/platform/common/posix/posix_thread.c:204-212
 * Target Lines: 206 (assert(cond)), 207 (assert(mutex)), 209 (pthread_cond_wait check), 212 (return BHT_OK)
 * Functional Purpose: Validates that os_cond_wait() works correctly with multiple threads
 *                     waiting on the same condition variable, ensuring the function's
 *                     parameter validation and pthread_cond_wait integration work properly.
 * Call Path: os_cond_wait() [PUBLIC API - DIRECT CALL]
 * Coverage Goal: Exercise condition wait in multi-threaded scenario
 ******/
TEST_F(EnhancedPosixThreadTest, os_cond_wait_MultipleWaiters_Success)
{
    korp_cond condition;
    korp_mutex mutex;

    // Initialize condition variable and mutex
    int cond_init_result = os_cond_init(&condition);
    ASSERT_EQ(BHT_OK, cond_init_result);

    int mutex_init_result = os_mutex_init(&mutex);
    ASSERT_EQ(BHT_OK, mutex_init_result);

    const int NUM_WAITERS = 2;
    pthread_t waiter_threads[NUM_WAITERS];
    volatile int wait_count = 0;

    struct WaiterData {
        korp_cond* cond;
        korp_mutex* mutex;
        volatile int* count;
    } waiter_data = { &condition, &mutex, &wait_count };

    auto waiter_func = [](void* arg) -> void* {
        auto* data = static_cast<WaiterData*>(arg);

        int lock_result = os_mutex_lock(data->mutex);
        if (lock_result == BHT_OK) {
            int wait_result = os_cond_wait(data->cond, data->mutex);
            if (wait_result == BHT_OK) {
                (*data->count)++;
            }
            os_mutex_unlock(data->mutex);
        }
        return nullptr;
    };

    // Create waiter threads
    for (auto& waiter_thread : waiter_threads) {
        int thread_create_result = pthread_create(&waiter_thread, nullptr, waiter_func, &waiter_data);
        ASSERT_EQ(0, thread_create_result);
    }

    // Give threads time to start waiting
    usleep(20000);

    // Signal all waiters
    int lock_result = os_mutex_lock(&mutex);
    ASSERT_EQ(BHT_OK, lock_result);

    int broadcast_result = os_cond_broadcast(&condition);
    ASSERT_EQ(BHT_OK, broadcast_result);

    int unlock_result = os_mutex_unlock(&mutex);
    ASSERT_EQ(BHT_OK, unlock_result);

    // Wait for all threads to complete
    for (auto& waiter_thread : waiter_threads) {
        void* thread_result;
        int join_result = pthread_join(waiter_thread, &thread_result);
        ASSERT_EQ(0, join_result);
    }

    // Verify all waiters were awakened
    ASSERT_EQ(NUM_WAITERS, wait_count);

    // Clean up
    int cond_destroy_result = os_cond_destroy(&condition);
    ASSERT_EQ(BHT_OK, cond_destroy_result);

    int mutex_destroy_result = os_mutex_destroy(&mutex);
    ASSERT_EQ(BHT_OK, mutex_destroy_result);
}
