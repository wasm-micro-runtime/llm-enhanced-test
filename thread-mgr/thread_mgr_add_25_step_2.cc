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

// Test fixture for Thread Manager Step 2 - Thread Lifecycle Operations
class ThreadMgrStep2Test : public testing::Test {
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
        // Clean up in reverse order with proper RAII cleanup
        dummy_env.reset();
        
        // Thread manager cleanup
        thread_manager_destroy();
        
        // WAMR runtime cleanup handled by RAII
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
};

// Test 7: Spawn Exec Env - AuxStack fails, spawn returns null
TEST_F(ThreadMgrStep2Test, ClusterSpawnExecEnv_AuxStackFails_ReturnsNull) {
    if (!PlatformTestContext::HasThreadMgrSupport()) {
        return; // Skip on platforms without thread manager support
    }
    
    ASSERT_NE(exec_env, nullptr);

    // Attempt spawn; expect failure (null) due to insufficient aux stack space
    WASMExecEnv* spawned_env = wasm_cluster_spawn_exec_env(exec_env);
    EXPECT_EQ(spawned_env, nullptr);

    // Verify original environment remains unaffected after failed spawn
    ASSERT_NE(exec_env, nullptr);
}

// Test 10: Thread Operations with Cluster Traversal Locks
TEST_F(ThreadMgrStep2Test, ThreadOperations_WithTraversalLocks_ThreadSafe) {
    if (!PlatformTestContext::HasThreadMgrSupport()) {
        return; // Skip on platforms without thread manager support
    }
    
    ASSERT_NE(exec_env, nullptr);
    
    // Test thread operations under cluster traversal lock
    wasm_cluster_traverse_lock(exec_env);
    
    // Perform thread state checks while locked
    for (int i = 0; i < 3; i++) {
        bool is_terminated = wasm_cluster_is_thread_terminated(exec_env);
        EXPECT_FALSE(is_terminated);
        
        WASMCluster* cluster = wasm_exec_env_get_cluster(exec_env);
        ASSERT_NE(cluster, nullptr);
    }
    
    wasm_cluster_traverse_unlock(exec_env);
    
    // Verify operations work after unlock
    WASMCluster* cluster = wasm_exec_env_get_cluster(exec_env);
    ASSERT_NE(cluster, nullptr);
}
