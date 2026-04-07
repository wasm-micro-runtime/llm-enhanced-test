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
    static bool HasSocketSupport() {
#if defined(WASM_ENABLE_LIBC_WASI) && defined(WASM_ENABLE_WASI_NN)
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
    
    static bool HasNetworkSupport() {
#if defined(__linux__) && (WASM_ENABLE_LIBC_WASI != 0)
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

// SSP Step 3 Test Class - Socket Operations & Network Functions
class SSPStep3Test : public testing::Test {
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
        if (exec_env_ != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env_);
        }
        runtime_.reset();
    }
    
    std::unique_ptr<WAMRRuntimeRAII<>> runtime_;
    wasm_exec_env_t exec_env_;
    fd_table fd_table_;
    fd_prestats prestats_;
};

// Function 4: wasi_ssp_sock_accept() tests
TEST_F(SSPStep3Test, WasiSspSockAccept_InvalidFd_ReturnsError) {
    if (!PlatformTestContext::HasNetworkSupport()) {
        return; // Skip if network not supported
    }

    __wasi_fd_t listen_fd = 1; // fd=1 is not in the fd_table, so fd_object_get fails
    __wasi_fd_t conn_fd;
    __wasi_fdflags_t flags = 0;

    // Test socket accept operation with invalid fd
    __wasi_errno_t result = wasi_ssp_sock_accept(exec_env_, &fd_table_, listen_fd, flags, &conn_fd);

    // fd=1 is not in the table, so fd_object_get returns an error immediately
    ASSERT_EQ(__WASI_EBADF, result);
}

TEST_F(SSPStep3Test, WasiSspSockBufferOps_InvalidFd_ReturnsBadf) {
    if (!PlatformTestContext::HasNetworkSupport()) {
        return; // Skip if network not supported
    }

    __wasi_fd_t test_fd = 3; // fd=3 is not in the fd_table, so fd_object_get fails

    // Test buffer size operations together
    __wasi_size_t current_size = 0;
    __wasi_errno_t get_result = wasi_ssp_sock_get_recv_buf_size(exec_env_, &fd_table_, test_fd, &current_size);

    __wasi_size_t new_size = 16384; // 16KB
    __wasi_errno_t set_result = wasi_ssp_sock_set_recv_buf_size(exec_env_, &fd_table_, test_fd, new_size);

    // fd=3 is not in the table, so fd_object_get returns an error on both calls
    ASSERT_EQ(__WASI_EBADF, get_result);
    ASSERT_EQ(__WASI_EBADF, set_result);
}

TEST_F(SSPStep3Test, Step3ErrorHandling_InvalidParameters_HandledGracefully) {
    if (!PlatformTestContext::HasNetworkSupport()) {
        return; // Skip if network not supported
    }
    
    // Test with invalid file descriptors
    __wasi_fd_t invalid_fd = 9999;
    
    // Test sock_bind with invalid fd
    __wasi_addr_t addr;
    memset(&addr, 0, sizeof(addr));
    addr_pool pool;
    memset(&pool, 0, sizeof(pool));
    
    __wasi_errno_t bind_result = wasi_ssp_sock_bind(exec_env_, &fd_table_, &pool, invalid_fd, &addr);
    ASSERT_NE(__WASI_ESUCCESS, bind_result);
    
    // Test sock_listen with invalid fd
    __wasi_errno_t listen_result = wasi_ssp_sock_listen(exec_env_, &fd_table_, invalid_fd, 10);
    ASSERT_NE(__WASI_ESUCCESS, listen_result);
    
    // Test addr operations with invalid fd
    __wasi_addr_t test_addr;
    __wasi_errno_t local_result = wasi_ssp_sock_addr_local(exec_env_, &fd_table_, invalid_fd, &test_addr);
    __wasi_errno_t remote_result = wasi_ssp_sock_addr_remote(exec_env_, &fd_table_, invalid_fd, &test_addr);
    
    ASSERT_NE(__WASI_ESUCCESS, local_result);
    ASSERT_NE(__WASI_ESUCCESS, remote_result);
}
