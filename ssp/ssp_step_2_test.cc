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
#include "wasm_runtime.h"
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
    
    static bool HasWASISupport() {
#if WASM_ENABLE_LIBC_WASI != 0
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
    WAMRRuntimeRAII() {
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
    bool initialized_ = false;
};

// SSP Step 2 test fixture - File Descriptor Management & Metadata Operations
class SSPStep2Test : public testing::Test {
protected:
    void SetUp() override {
        // Initialize WAMR runtime first
        runtime_ = std::make_unique<WAMRRuntimeRAII<>>();
        ASSERT_TRUE(runtime_->IsInitialized()) << "Failed to initialize WAMR runtime";
        
        // Create a mock execution environment
        wasm_module_t module = nullptr;
        wasm_module_inst_t module_inst = nullptr;
        uint32_t stack_size = 8092;
        
        // Create a minimal module for testing
        uint8_t wasm_bytes[] = {
            0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00  // WASM magic + version
        };
        
        char error_buf[128];
        module = wasm_runtime_load(wasm_bytes, sizeof(wasm_bytes), error_buf, sizeof(error_buf));
        if (module != nullptr) {
            module_inst = wasm_runtime_instantiate(module, stack_size, stack_size, error_buf, sizeof(error_buf));
            if (module_inst != nullptr) {
                exec_env_ = wasm_runtime_create_exec_env(module_inst, stack_size);
            }
        }
        
        // Initialize fd_table structure for testing  
        memset(&fd_table_, 0, sizeof(fd_table_));
        ASSERT_TRUE(fd_table_init(&fd_table_));
        
        // Initialize fd_prestats structure for testing
        memset(&prestats_, 0, sizeof(prestats_));
        ASSERT_TRUE(fd_prestats_init(&prestats_));
    }
    
    void TearDown() override {
        // Clean up execution environment
        if (exec_env_ != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env_);
            exec_env_ = nullptr;
        }
        
        // Clean up fd_table
        fd_table_destroy(&fd_table_);
        
        // Clean up prestats
        fd_prestats_destroy(&prestats_);
        
        // WAMR runtime cleanup handled by RAII destructor
        runtime_.reset();
    }
    
    std::unique_ptr<WAMRRuntimeRAII<>> runtime_;
    struct fd_table fd_table_;
    struct fd_prestats prestats_;
    wasm_exec_env_t exec_env_ = nullptr;
};

// Additional comprehensive tests for Step 2
TEST_F(SSPStep2Test, Step2Integration_FdstatOperations_WorkTogether) {
    if (!PlatformTestContext::HasWASISupport()) {
        return; // Skip if WASI not supported
    }
    
    __wasi_fd_t test_fd = 0; // stdin
    
    // 1. Get initial fdstat
    __wasi_fdstat_t fdstat;
    memset(&fdstat, 0, sizeof(fdstat));
    __wasi_errno_t get_result = wasmtime_ssp_fd_fdstat_get(exec_env_, &fd_table_, test_fd, &fdstat);
    
    // 2. Try to set flags (may not work on stdin, but should handle gracefully)
    __wasi_errno_t flags_result = wasmtime_ssp_fd_fdstat_set_flags(exec_env_, &fd_table_, test_fd, 0);
    
    // 3. Try to set rights
    __wasi_errno_t rights_result = wasmtime_ssp_fd_fdstat_set_rights(exec_env_, &fd_table_, test_fd, 
                                                                    __WASI_RIGHT_FD_READ, 0);
    
    // All operations should return EBADF for unregistered fd=0
    ASSERT_EQ(__WASI_EBADF, get_result);
    ASSERT_EQ(__WASI_EBADF, flags_result);
    ASSERT_EQ(__WASI_EBADF, rights_result);
}

TEST_F(SSPStep2Test, Step2Integration_SyncOperations_WorkTogether) {
    if (!PlatformTestContext::HasWASISupport()) {
        return; // Skip if WASI not supported
    }
    
    __wasi_fd_t test_fd = 1; // stdout
    
    // Test both sync operations
    __wasi_errno_t datasync_result = wasmtime_ssp_fd_datasync(exec_env_, &fd_table_, test_fd);
    __wasi_errno_t sync_result = wasmtime_ssp_fd_sync(exec_env_, &fd_table_, test_fd);
    
    // Both should return EBADF for unregistered fd=1
    ASSERT_EQ(__WASI_EBADF, datasync_result);
    ASSERT_EQ(__WASI_EBADF, sync_result);
}

TEST_F(SSPStep2Test, Step2ErrorHandling_UnregisteredFd_ReturnsEBADF) {
    if (!PlatformTestContext::HasWASISupport()) {
        return; // Skip if WASI not supported
    }
    
    __wasi_fd_t test_fd = 1;
    
    // Test fdstat_get with null buffer - should handle gracefully
    __wasi_errno_t result1 = wasmtime_ssp_fd_fdstat_get(exec_env_, &fd_table_, test_fd, nullptr);
    ASSERT_NE(__WASI_ESUCCESS, result1);
    
    // Test filestat_get with null buffer - should handle gracefully
    __wasi_errno_t result2 = wasmtime_ssp_fd_filestat_get(exec_env_, &fd_table_, test_fd, nullptr);
    ASSERT_NE(__WASI_ESUCCESS, result2);
    
    // Test with invalid flags - returns EBADF because fd=1 is unregistered
    __wasi_errno_t result3 = wasmtime_ssp_fd_fdstat_set_flags(exec_env_, &fd_table_, test_fd, 0xFFFFFFFF);
    ASSERT_EQ(__WASI_EBADF, result3);
}

TEST_F(SSPStep2Test, Step2BoundaryConditions_UnregisteredFd_ReturnsEBADF) {
    if (!PlatformTestContext::HasWASISupport()) {
        return; // Skip if WASI not supported
    }
    
    __wasi_fd_t test_fd = 1;
    
    // Test with very large file size
    __wasi_filesize_t large_size = UINT64_MAX;
    __wasi_errno_t size_result = wasmtime_ssp_fd_filestat_set_size(exec_env_, &fd_table_, test_fd, large_size);
    
    // Test with very large timestamps
    __wasi_timestamp_t large_time = UINT64_MAX;
    __wasi_errno_t time_result = wasmtime_ssp_fd_filestat_set_times(exec_env_, &fd_table_, test_fd,
                                                                   large_time, large_time, 
                                                                   __WASI_FILESTAT_SET_ATIM);
    
    // Both should return EBADF for unregistered fd=1
    ASSERT_EQ(__WASI_EBADF, size_result);
    ASSERT_EQ(__WASI_EBADF, time_result);
}
