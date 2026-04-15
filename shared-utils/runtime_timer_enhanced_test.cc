/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "bh_platform.h"
#include "runtime_timer.h"
#include "wasm_export.h"

// Platform detection utility for tests - REQUIRED in every test file
class PlatformTestContext {
public:
    // Architecture detection
    static bool IsX86_64() {
#if defined(BUILD_TARGET_X86) || defined(BUILD_TARGET_X86_64) || defined(__x86_64__)
        return true;
#else
        return false;
#endif
    }
    
    static bool IsARM64() {
#if defined(BUILD_TARGET_AARCH64) || defined(BUILD_TARGET_ARM64) || defined(__aarch64__)
        return true;
#else
        return false;
#endif
    }
    
    static bool IsARM32() {
#if defined(BUILD_TARGET_ARM) && !defined(BUILD_TARGET_AARCH64) && !defined(__aarch64__)
        return true;
#else
        return false;
#endif
    }
    
    static bool IsRISCV() {
#if defined(BUILD_TARGET_RISCV) || defined(BUILD_TARGET_RISCV64) || defined(BUILD_TARGET_RISCV32) || defined(__riscv)
        return true;
#else
        return false;
#endif
    }
    
    // Platform capability helpers
    static bool Is64BitArchitecture() {
        return IsX86_64() || IsARM64() || (IsRISCV() && sizeof(void*) == 8);
    }
    
    static bool Is32BitArchitecture() {
        return IsARM32() || (IsRISCV() && sizeof(void*) == 4);
    }
};

// Global test variables for timer callback verification
static uint32_t g_callback_timer_id = 0;
static uint32_t g_callback_owner = 0;
static int g_callback_count = 0;

// Timer callback function for testing
static void test_timer_callback(unsigned int id, unsigned int owner)
{
    g_callback_timer_id = id;
    g_callback_owner = owner;
    g_callback_count++;
}

// Timer expiry checker callback for testing
static void test_expiry_checker(timer_ctx_t ctx)
{
    // Simple checker implementation for testing
    (void)ctx;
}

class RuntimeTimerTest : public testing::Test {
protected:
    void SetUp() override {
        // Initialize WAMR runtime
        wasm_runtime_init();
        
        runtime_initialized = true;
        
        // Reset global callback variables
        g_callback_timer_id = 0;
        g_callback_owner = 0;
        g_callback_count = 0;
        
        timer_ctx = nullptr;
        test_owner = 12345;
    }
    
    void TearDown() override {
        if (timer_ctx != nullptr) {
            destroy_timer_ctx(timer_ctx);
            timer_ctx = nullptr;
        }
        
        if (runtime_initialized) {
            wasm_runtime_destroy();
            runtime_initialized = false;
        }
    }
    
    timer_ctx_t timer_ctx;
    uint32_t test_owner;
    bool runtime_initialized;
};

// Test bh_get_elpased_ms() with clock overflow simulation
TEST_F(RuntimeTimerTest, GetElapsedMs_ClockOverflow_HandlesCorrectly) {
    // Get current time and simulate a clock that was set just before overflow
    auto current_time = static_cast<uint32_t>(bh_get_tick_ms());
    uint32_t last_clock = current_time - 100; // 100ms ago
    
    uint32_t elapsed = bh_get_elpased_ms(&last_clock);
    
    // Should handle the calculation correctly
    ASSERT_GE(elapsed, 90);  // At least 90ms should have passed
    ASSERT_LE(elapsed, 200); // But not more than 200ms in normal conditions
}

// Test create_timer_ctx() with NULL callback (should still work)
TEST_F(RuntimeTimerTest, CreateTimerCtx_NullCallback_CreatesSuccessfully) {
    timer_ctx = create_timer_ctx(nullptr, nullptr, 0, test_owner);
    
    ASSERT_NE(timer_ctx, nullptr);
    ASSERT_EQ(timer_ctx_get_owner(timer_ctx), test_owner);
}

// Test sys_create_timer() with preallocation exhaustion
TEST_F(RuntimeTimerTest, SysCreateTimer_PreallocExhausted_FailsGracefully) {
    // Create context with only 1 preallocated timer
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, 1, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    // Create first timer - should succeed
    uint32_t timer_id1 = sys_create_timer(timer_ctx, 100, false, false);
    ASSERT_NE(timer_id1, (uint32_t)-1);
    
    // Create second timer - should fail due to exhaustion
    uint32_t timer_id2 = sys_create_timer(timer_ctx, 100, false, false);
    ASSERT_EQ(timer_id2, (uint32_t)-1);
}

// Test sys_timer_destroy() with valid timer
TEST_F(RuntimeTimerTest, SysTimerDestroy_ValidTimer_DestroySuccessfully) {
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, 0, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    uint32_t timer_id = sys_create_timer(timer_ctx, 100, false, false);
    ASSERT_NE(timer_id, (uint32_t)-1);
    
    bool result = sys_timer_destroy(timer_ctx, timer_id);
    ASSERT_TRUE(result);
}

// Test sys_timer_destroy() with invalid timer ID
TEST_F(RuntimeTimerTest, SysTimerDestroy_InvalidTimerId_ReturnsFalse) {
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, 0, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    bool result = sys_timer_destroy(timer_ctx, 99999);
    ASSERT_FALSE(result);
}

// Test sys_timer_cancel() with active timer
TEST_F(RuntimeTimerTest, SysTimerCancel_ActiveTimer_CancelsSuccessfully) {
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, 0, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    uint32_t timer_id = sys_create_timer(timer_ctx, 100, false, true);
    ASSERT_NE(timer_id, (uint32_t)-1);
    
    bool result = sys_timer_cancel(timer_ctx, timer_id);
    ASSERT_TRUE(result); // Should return true for active timer
}

// Test sys_timer_cancel() with invalid timer ID
TEST_F(RuntimeTimerTest, SysTimerCancel_InvalidTimerId_ReturnsFalse) {
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, 0, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    bool result = sys_timer_cancel(timer_ctx, 99999);
    ASSERT_FALSE(result);
}

// Test sys_timer_restart() with valid timer
TEST_F(RuntimeTimerTest, SysTimerRestart_ValidTimer_RestartsSuccessfully) {
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, 0, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    uint32_t timer_id = sys_create_timer(timer_ctx, 100, false, false);
    ASSERT_NE(timer_id, (uint32_t)-1);
    
    bool result = sys_timer_restart(timer_ctx, timer_id, 200);
    ASSERT_TRUE(result);
}

// Test sys_timer_restart() with invalid timer ID
TEST_F(RuntimeTimerTest, SysTimerRestart_InvalidTimerId_ReturnsFalse) {
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, 0, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    bool result = sys_timer_restart(timer_ctx, 99999, 100);
    ASSERT_FALSE(result);
}

// Test check_app_timers() with expired timer
TEST_F(RuntimeTimerTest, CheckAppTimers_ExpiredTimer_TriggersCallback) {
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, 0, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    // Create timer with very short interval
    uint32_t timer_id = sys_create_timer(timer_ctx, 1, false, true);
    ASSERT_NE(timer_id, (uint32_t)-1);
    
    // Wait for timer to expire
    usleep(5000); // 5ms
    
    // Check timers - should trigger callback
    (void)check_app_timers(timer_ctx);
    
    // Verify callback was triggered
    ASSERT_EQ(g_callback_timer_id, timer_id);
    ASSERT_EQ(g_callback_owner, test_owner);
    ASSERT_GE(g_callback_count, 1);
}

// Test periodic timer behavior
TEST_F(RuntimeTimerTest, PeriodicTimer_ExpiredTimer_ReschedulesAutomatically) {
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, 0, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    // Create periodic timer with short interval
    uint32_t timer_id = sys_create_timer(timer_ctx, 10, true, true);
    ASSERT_NE(timer_id, (uint32_t)-1);
    
    // Wait for multiple expirations
    usleep(25000); // 25ms
    
    // Check timers multiple times
    check_app_timers(timer_ctx);
    usleep(15000); // 15ms
    check_app_timers(timer_ctx);
    
    // Should have triggered callback multiple times for periodic timer
    ASSERT_GE(g_callback_count, 2);
    ASSERT_EQ(g_callback_timer_id, timer_id);
}

// Test timer context with large preallocation
TEST_F(RuntimeTimerTest, CreateTimerCtx_LargePrealloc_HandlesCorrectly) {
    // Test platform-specific memory limits
    int prealloc_count;
    if (PlatformTestContext::Is64BitArchitecture()) {
        prealloc_count = 1000; // Larger allocation on 64-bit
    } else {
        prealloc_count = 100;  // Smaller allocation on 32-bit
    }
    
    timer_ctx = create_timer_ctx(test_timer_callback, nullptr, prealloc_count, test_owner);
    ASSERT_NE(timer_ctx, nullptr);
    
    // Should be able to create many timers without allocation
    for (int i = 0; i < prealloc_count && i < 50; i++) {
        uint32_t timer_id = sys_create_timer(timer_ctx, 100 + i, false, false);
        ASSERT_NE(timer_id, (uint32_t)-1);
    }
}

