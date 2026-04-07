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
    bool initialized_{false};
};

// SSP Step 1 test fixture - Core File Descriptor Operations
class SSPStep1Test : public testing::Test {
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

TEST_F(SSPStep1Test, WasmtimeSspFdPread_InvalidParams_ReturnsError) {
    if (!PlatformTestContext::HasWASISupport()) {
        return; // Skip if WASI not supported
    }
    
    __wasi_fd_t invalid_fd = 9999;
    __wasi_iovec_t iov;
    char buffer[256];
    size_t nread = 0;
    __wasi_filesize_t offset = 0;
    
    iov.buf = (uint8_t*)buffer;
    iov.buf_len = sizeof(buffer);
    
    // Test with invalid fd
    __wasi_errno_t result = wasmtime_ssp_fd_pread(exec_env_, &fd_table_, invalid_fd, &iov, 1, offset, &nread);
    
    // Should return error for invalid parameters
    ASSERT_NE(__WASI_ESUCCESS, result);
}

TEST_F(SSPStep1Test, WasmtimeSspFdPwrite_InvalidParams_ReturnsError) {
    if (!PlatformTestContext::HasWASISupport()) {
        return; // Skip if WASI not supported
    }
    
    __wasi_fd_t invalid_fd = 9999;
    __wasi_ciovec_t iov;
    const char test_data[] = "test";
    size_t nwritten = 0;
    __wasi_filesize_t offset = 0;
    
    iov.buf = (const uint8_t*)test_data;
    iov.buf_len = strlen(test_data);
    
    // Test with invalid fd
    __wasi_errno_t result = wasmtime_ssp_fd_pwrite(exec_env_, &fd_table_, invalid_fd, &iov, 1, offset, &nwritten);
    
    // Should return error for invalid parameters
    ASSERT_NE(__WASI_ESUCCESS, result);
}

// Additional comprehensive tests for Step 1
TEST_F(SSPStep1Test, Step1Integration_InvalidFd_BothReturnEBADF) {
    if (!PlatformTestContext::HasWASISupport()) {
        return; // Skip if WASI not supported
    }

    // fd=1 (stdout) is not registered in the empty fd_table, so both calls return EBADF
    __wasi_fd_t test_fd = 1; // stdout - not registered in fd_table

    // 1. Test write operation - fd not in table, expect EBADF
    __wasi_ciovec_t write_iov;
    const char test_data[] = "Integration test\n";
    size_t nwritten = 0;

    write_iov.buf = (const uint8_t*)test_data;
    write_iov.buf_len = strlen(test_data);

    __wasi_errno_t write_result = wasmtime_ssp_fd_write(exec_env_, &fd_table_, test_fd, &write_iov, 1, &nwritten);

    // 2. Test seek operation - fd not in table, expect EBADF
    __wasi_filesize_t newoffset = 0;
    __wasi_errno_t seek_result = wasmtime_ssp_fd_seek(exec_env_, &fd_table_, test_fd, 0, __WASI_WHENCE_CUR, &newoffset);

    // Both operations fail with EBADF because fd=1 is not registered in fd_table
    ASSERT_EQ(__WASI_EBADF, write_result);
    ASSERT_EQ(__WASI_EBADF, seek_result);
}

TEST_F(SSPStep1Test, Step1ErrorHandling_InvalidFd_BothReturnEBADF) {
    if (!PlatformTestContext::HasWASISupport()) {
        return; // Skip if WASI not supported
    }

    // fd=1 is not registered in the empty fd_table; fd_object_get fails with EBADF
    // before the null iovec is ever dereferenced
    __wasi_fd_t test_fd = 1;

    // Test fd_write with null iovec and unregistered fd - EBADF from fd lookup
    size_t nwritten = 0;
    __wasi_errno_t result1 = wasmtime_ssp_fd_write(exec_env_, &fd_table_, test_fd, nullptr, 0, &nwritten);

    // Test fd_read with null iovec and unregistered fd - EBADF from fd lookup
    size_t nread = 0;
    __wasi_errno_t result2 = wasmtime_ssp_fd_read(exec_env_, &fd_table_, test_fd, nullptr, 0, &nread);

    // Both fail at fd_object_get before null iov is examined, so EBADF is the specific result
    ASSERT_EQ(__WASI_EBADF, result1);
    ASSERT_EQ(__WASI_EBADF, result2);
}
