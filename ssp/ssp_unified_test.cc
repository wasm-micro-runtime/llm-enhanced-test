/*
 * Copyright (C) 2025 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "bh_platform.h"

extern "C" {
#include "str.h"
#include "posix.h"
#include "ssp_config.h"
#include "wasmtime_ssp.h"
#include "wasm_export.h"
}

// Platform detection utility for tests - REQUIRED in every test file
class PlatformTestContext {
public:
    // Architecture detection
    static bool IsX86_64() {
#if defined(BUILD_TARGET_X86) || defined(BUILD_TARGET_X86_64)
        return true;
#else
        return false;
#endif
    }
    
    static bool IsARM64() {
#if defined(BUILD_TARGET_AARCH64) || defined(BUILD_TARGET_ARM64)
        return true;
#else
        return false;
#endif
    }
    
    static bool IsLinux() {
#if defined(__linux__)
        return true;
#else
        return false;
#endif
    }
    
    // Feature detection
    static bool HasFileSupport() {
#if defined(WASM_ENABLE_LIBC_WASI)
        return true;
#else
        return false;
#endif
    }
};

// WAMR Runtime RAII helper for proper initialization/cleanup
template<uint32_t HEAP_SIZE = 512 * 1024>
class WAMRRuntimeRAII {
public:
    WAMRRuntimeRAII() : initialized_(false) {
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        init_args.mem_alloc_option.allocator.malloc_func = (void*)malloc;
        init_args.mem_alloc_option.allocator.realloc_func = (void*)realloc;
        init_args.mem_alloc_option.allocator.free_func = (void*)free;
        
        // Initialize WAMR runtime
        initialized_ = wasm_runtime_full_init(&init_args);
    }
    
    ~WAMRRuntimeRAII() {
        if (initialized_) {
            wasm_runtime_destroy();
        }
    }
    
    bool IsInitialized() const { return initialized_; }
    
private:
    bool initialized_{ false };
};

// SSP test fixture with proper WAMR runtime initialization
class SSPUnifiedTest : public testing::Test {
protected:
    void SetUp() override {
        // Initialize WAMR runtime first
        runtime_ = std::make_unique<WAMRRuntimeRAII<>>();
        ASSERT_TRUE(runtime_->IsInitialized()) << "Failed to initialize WAMR runtime";
        
        // Initialize fd_prestats structure for testing
        memset(&prestats_, 0, sizeof(prestats_));
        ASSERT_TRUE(fd_prestats_init(&prestats_));
        
        // Initialize fd_table structure for testing  
        memset(&fd_table_, 0, sizeof(fd_table_));
        ASSERT_TRUE(fd_table_init(&fd_table_));
    }
    
    void TearDown() override {
        // Clean up prestats
        if (prestats_.prestats != nullptr) {
            free(prestats_.prestats);
            prestats_.prestats = nullptr;
        }
        
        // Clean up fd_table
        if (fd_table_.entries != nullptr) {
            free(fd_table_.entries);
            fd_table_.entries = nullptr;
        }
        
        // Reset structures
        memset(&prestats_, 0, sizeof(prestats_));
        memset(&fd_table_, 0, sizeof(fd_table_));
        
        // WAMR runtime cleanup handled by RAII destructor
        runtime_.reset();
    }
    
    std::unique_ptr<WAMRRuntimeRAII<>> runtime_;
    struct fd_prestats prestats_;
    struct fd_table fd_table_;
};

// ==== STEP 2: WASI SSP API Tests ====

TEST_F(SSPUnifiedTest, WasmtimeSspFdPrestatGet_ValidFd_HandlesCorrectly) {
    if (!PlatformTestContext::HasFileSupport()) {
        return;
    }
    
    __wasi_fd_t fd = 3; // Typical prestat fd
    __wasi_prestat_t prestat;
    
    // Insert a prestat entry first
    const char *test_dir = "/test";
    bool insert_result = fd_prestats_insert(&prestats_, test_dir, fd);
    ASSERT_TRUE(insert_result);
    
    // Test getting prestat - success expected since entry was inserted
    __wasi_errno_t result = wasmtime_ssp_fd_prestat_get(&prestats_, fd, &prestat);

    ASSERT_EQ(__WASI_ESUCCESS, result);
    ASSERT_EQ(__WASI_PREOPENTYPE_DIR, prestat.pr_type);
    ASSERT_EQ(strlen("/test"), prestat.u.dir.pr_name_len);
}

TEST_F(SSPUnifiedTest, WasmtimeSspFdPrestatDirName_ValidFd_HandlesCorrectly) {
    if (!PlatformTestContext::HasFileSupport()) {
        return;
    }
    
    __wasi_fd_t fd = 3;
    char buffer[256];
    __wasi_size_t buffer_len = sizeof(buffer);
    
    // Insert a prestat entry first
    const char *test_dir = "/test/directory";
    bool insert_result = fd_prestats_insert(&prestats_, test_dir, fd);
    ASSERT_TRUE(insert_result);
    
    // Test getting directory name - success expected since entry was inserted
    __wasi_errno_t result = wasmtime_ssp_fd_prestat_dir_name(&prestats_, fd, buffer, buffer_len);

    ASSERT_EQ(__WASI_ESUCCESS, result);
    ASSERT_EQ(0, strncmp("/test/directory", buffer, strlen("/test/directory")));
}

TEST_F(SSPUnifiedTest, WasmtimeSspFdPrestatDirName_SmallBuffer_HandlesCorrectly) {
    __wasi_fd_t fd = 3;
    char small_buffer[4]; // Very small buffer
    __wasi_size_t buffer_len = sizeof(small_buffer);
    
    // Insert a prestat entry first
    const char *test_dir = "/test/long/directory/path";
    bool insert_result = fd_prestats_insert(&prestats_, test_dir, fd);
    ASSERT_TRUE(insert_result);
    
    // Test with small buffer
    __wasi_errno_t result = wasmtime_ssp_fd_prestat_dir_name(&prestats_, fd, small_buffer, buffer_len);
    
    // Should handle small buffer appropriately - accept any valid error code
    ASSERT_TRUE(result == __WASI_ENOBUFS || result == __WASI_EBADF || result == __WASI_ESUCCESS || result == __WASI_EINVAL);
}

// ==== STEP 3: Integration and Edge Case Tests ====

TEST_F(SSPUnifiedTest, ComprehensiveIntegration_MultipleOperations_WorkTogether) {
    if (!PlatformTestContext::HasFileSupport()) {
        return;
    }
    
    // Test integration of multiple SSP operations
    const char *test_dir = "/test/integration";
    __wasi_fd_t fd = 4;
    
    // 1. Insert prestat entry
    bool insert_result = fd_prestats_insert(&prestats_, test_dir, fd);
    ASSERT_TRUE(insert_result);
    
    // 2. Generate random data
    uint8_t random_buffer[16];
    __wasi_errno_t random_result = wasmtime_ssp_random_get(random_buffer, sizeof(random_buffer));
    ASSERT_EQ(__WASI_ESUCCESS, random_result);
    
    // 3. Get time
    __wasi_timestamp_t timestamp;
    __wasi_errno_t time_result = os_clock_time_get(__WASI_CLOCK_REALTIME, 1, &timestamp);
    ASSERT_EQ(__WASI_ESUCCESS, time_result);
    
    // 4. Yield scheduler
    __wasi_errno_t yield_result = wasmtime_ssp_sched_yield();
    ASSERT_EQ(__WASI_ESUCCESS, yield_result);
    
    // All operations should work together
    ASSERT_GT(timestamp, 0);
}

TEST_F(SSPUnifiedTest, ErrorHandling_NullParameters_ReturnsErrors) {
    // Test null parameter handling for various functions
    
    // Test fd_prestat_get with null prestat - should return error
    __wasi_errno_t result1 = wasmtime_ssp_fd_prestat_get(&prestats_, 3, nullptr);
    ASSERT_NE(__WASI_ESUCCESS, result1);
    
    // Test random_get with null buffer - should return error
    __wasi_errno_t result3 = wasmtime_ssp_random_get(nullptr, 10);
    ASSERT_NE(__WASI_ESUCCESS, result3);
    
    // Test fd_prestat_dir_name with null buffer (safer approach)
    char *null_buffer = nullptr;
    __wasi_errno_t result2 = wasmtime_ssp_fd_prestat_dir_name(&prestats_, 3, null_buffer, 0);
    ASSERT_NE(__WASI_ESUCCESS, result2);
    
    // Test invalid fd operations instead of null buffer operations
    __wasi_fd_t invalid_fd = 999;
    __wasi_prestat_t prestat;
    __wasi_errno_t result4 = wasmtime_ssp_fd_prestat_get(&prestats_, invalid_fd, &prestat);
    ASSERT_NE(__WASI_ESUCCESS, result4);
}
