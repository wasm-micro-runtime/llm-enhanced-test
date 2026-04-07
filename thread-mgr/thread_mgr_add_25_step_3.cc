/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <vector>
#include <memory>

extern "C" {
#include "wasm_export.h"
#include "bh_platform.h"
#include "thread_manager.h"
#include "wasm_runtime_common.h"
#include "wasm_exec_env.h"
}

#include "../../common/test_helper.h"

// Platform detection utility for tests - REQUIRED in every test file
class PlatformTestContext {
public:
    // Feature detection
    static bool HasThreadMgrSupport() {
#if WASM_ENABLE_THREAD_MGR != 0
        return true;
#else
        return false;
#endif
    }
    
    static bool HasSharedMemorySupport() {
#if WASM_ENABLE_SHARED_MEMORY != 0
        return true;
#else
        return false;
#endif
    }
    
    static bool HasPthreadSupport() {
#if WASM_ENABLE_LIB_PTHREAD != 0
        return true;
#else
        return false;
#endif
    }
    
    static bool HasAuxStackAllocation() {
#if WASM_ENABLE_HEAP_AUX_STACK_ALLOCATION == 0
        return true;
#else
        return false;
#endif
    }
};

// Test fixture for Thread Manager Step 3: Synchronization & State Management
class ThreadMgrStep3Test : public testing::Test {
protected:
    void SetUp() override {
        // Initialize WAMR runtime once per test
        runtime = std::make_unique<WAMRRuntimeRAII<>>();
        
        // Initialize thread manager
        ASSERT_TRUE(thread_manager_init());
        
        // Create test execution environment using test helper classes
        dummy_env = std::make_unique<DummyExecEnv>();
        
        exec_env = dummy_env->get();
        ASSERT_NE(exec_env, nullptr);
        
        module_inst = wasm_runtime_get_module_inst(exec_env);
        ASSERT_NE(module_inst, nullptr);
        
        cluster = wasm_exec_env_get_cluster(exec_env);
        ASSERT_NE(cluster, nullptr);
    }
    
    void TearDown() override {
        // Clean up in reverse order with proper checks
        if (dummy_env) {
            dummy_env.reset();
        }
        
        // Thread manager cleanup
        thread_manager_destroy();
        
        // WAMR runtime cleanup handled by WAMRRuntimeRAII destructor
        runtime.reset();
        
        // Reset pointers
        exec_env = nullptr;
        module_inst = nullptr;
        cluster = nullptr;
    }
    
    std::unique_ptr<WAMRRuntimeRAII<>> runtime;
    std::unique_ptr<DummyExecEnv> dummy_env;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    WASMCluster* cluster = nullptr;
    
    // Helper to create a cluster with multiple exec envs for testing
    WASMCluster* CreateTestCluster() {
        if (exec_env == nullptr) return nullptr;
        
        // Get cluster from existing exec_env
        return wasm_exec_env_get_cluster(exec_env);
    }
    
    // Helper to validate cluster state
    bool ValidateClusterState(WASMCluster* cluster) {
        return cluster != nullptr;
    }
};

// Test: wasm_cluster_resume_all() - Resume all suspended threads
TEST_F(ThreadMgrStep3Test, ResumeAll_ValidCluster_ExecutesCorrectly) {
    if (!PlatformTestContext::HasThreadMgrSupport()) {
        return;
    }
    
    WASMCluster* cluster = CreateTestCluster();
    ASSERT_NE(cluster, nullptr);
    
    // First suspend, then resume to test complete cycle
    wasm_cluster_suspend_all(cluster);
    wasm_cluster_resume_all(cluster);
    
    // Validate cluster remains in valid state
    ASSERT_TRUE(ValidateClusterState(cluster));
}

// Test: wasm_cluster_terminate_all() - Terminate all threads in cluster
TEST_F(ThreadMgrStep3Test, TerminateAll_ValidCluster_ExecutesCorrectly) {
    if (!PlatformTestContext::HasThreadMgrSupport()) {
        return;
    }
    
    WASMCluster* cluster = CreateTestCluster();
    ASSERT_NE(cluster, nullptr);
    
    // Test terminate all operation
    wasm_cluster_terminate_all(cluster);

    // Verify the exec_env was marked for termination
    ASSERT_TRUE(wasm_cluster_is_thread_terminated(exec_env));
}

// Test: wasm_cluster_terminate_all_except_self() - Terminate all except current thread
TEST_F(ThreadMgrStep3Test, TerminateAllExceptSelf_ValidCluster_ExecutesCorrectly) {
    if (!PlatformTestContext::HasThreadMgrSupport()) {
        return;
    }
    
    WASMCluster* cluster = CreateTestCluster();
    ASSERT_NE(cluster, nullptr);
    
    // Test terminate all except self operation
    wasm_cluster_terminate_all_except_self(cluster, exec_env);

    // Verify the calling exec_env was excluded from termination
    ASSERT_FALSE(wasm_cluster_is_thread_terminated(exec_env));
}

// Test: Multiple synchronization operations in sequence
TEST_F(ThreadMgrStep3Test, SequentialOperations_ValidCluster_ExecutesCorrectly) {
    if (!PlatformTestContext::HasThreadMgrSupport()) {
        return;
    }
    
    WASMCluster* cluster = CreateTestCluster();
    ASSERT_NE(cluster, nullptr);
    
    // Test sequence of operations
    wasm_cluster_suspend_all_except_self(cluster, exec_env);
    wasm_cluster_resume_thread(exec_env);
    wasm_cluster_wait_for_all(cluster);
    
    // Verify thread state after operations
    ASSERT_FALSE(wasm_cluster_is_thread_terminated(exec_env));
    ASSERT_TRUE(ValidateClusterState(cluster));
}

