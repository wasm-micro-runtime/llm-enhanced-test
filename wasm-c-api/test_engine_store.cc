/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <memory>

#include "bh_platform.h"
#include "wasm_c_api.h"
#include "wasm_c_api_internal.h"

#ifndef own
#define own
#endif

class EngineStoreTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        bh_log_set_verbose_level(5);
    }

    void TearDown() override
    {
        // Cleanup handled by individual tests
    }
};

TEST_F(EngineStoreTest, EngineConfig_WithLinuxPerf_CreatesSuccessfully)
{
    wasm_config_t* config = wasm_config_new();
    ASSERT_NE(nullptr, config);
    
    wasm_config_t* perf_config = wasm_config_set_linux_perf_opt(config, true);
    ASSERT_NE(nullptr, perf_config);
    
    wasm_engine_t* engine = wasm_engine_new_with_config(perf_config);
    ASSERT_NE(nullptr, engine);
    
    wasm_engine_delete(engine);
}

TEST_F(EngineStoreTest, EngineConfig_WithSegueFlags_CreatesSuccessfully)
{
    wasm_config_t* config = wasm_config_new();
    ASSERT_NE(nullptr, config);
    
    wasm_config_t* segue_config = wasm_config_set_segue_flags(config, 0x1F1F);
    ASSERT_NE(nullptr, segue_config);
    
    wasm_engine_t* engine = wasm_engine_new_with_config(segue_config);
    ASSERT_NE(nullptr, engine);
    
    wasm_engine_delete(engine);
}

TEST_F(EngineStoreTest, Store_WithNullEngine_FailsGracefully)
{
    wasm_store_t* store = wasm_store_new(nullptr);
    ASSERT_EQ(nullptr, store);
}

TEST_F(EngineStoreTest, Store_DeletedEngine_HandlesGracefully)
{
    wasm_engine_t* engine = wasm_engine_new();
    ASSERT_NE(nullptr, engine);
    
    wasm_store_t* store = wasm_store_new(engine);
    ASSERT_NE(nullptr, store);
    
    // Delete engine first, then store
    wasm_engine_delete(engine);
    wasm_store_delete(store);
}

TEST_F(EngineStoreTest, Store_ConcurrentCreation_ThreadSafe)
{
    wasm_engine_t* engine = wasm_engine_new();
    ASSERT_NE(nullptr, engine);
    
    const int num_threads = 4;
    std::vector<std::thread> threads;
    std::vector<wasm_store_t*> stores(num_threads);
    
    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&stores, engine, i]() {
            stores[i] = wasm_store_new(engine);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All stores should be created successfully and be different
    for (int i = 0; i < num_threads; ++i) {
        ASSERT_NE(nullptr, stores[i]);
        for (int j = i + 1; j < num_threads; ++j) {
            ASSERT_NE(stores[i], stores[j]);
        }
    }
    
    for (int i = 0; i < num_threads; ++i) {
        wasm_store_delete(stores[i]);
    }
    wasm_engine_delete(engine);
}

// Resource Management Tests
TEST_F(EngineStoreTest, Engine_DoubleDelete_HandlesGracefully)
{
    wasm_engine_t* engine = wasm_engine_new();
    ASSERT_NE(nullptr, engine);
    
    wasm_engine_delete(engine);
    // Second delete should not crash
    wasm_engine_delete(engine);
}

TEST_F(EngineStoreTest, Engine_DeleteNull_HandlesGracefully)
{
    // Should not crash
    wasm_engine_delete(nullptr);
}

TEST_F(EngineStoreTest, Store_DeleteNull_HandlesGracefully)
{
    // Should not crash
    wasm_store_delete(nullptr);
}

TEST_F(EngineStoreTest, Config_MultipleConfigurations_WorkCorrectly)
{
    // Test multiple different configurations
    std::vector<wasm_config_t*> configs;
    std::vector<wasm_engine_t*> engines;
    
    // Default config
    configs.push_back(wasm_config_new());
    
    // Pool allocator config
    wasm_config_t* pool_config = wasm_config_new();
    static uint8_t pool_heap_buf[512 * 1024];
    MemAllocOption pool_option = {};
    pool_option.pool.heap_buf = pool_heap_buf;
    pool_option.pool.heap_size = sizeof(pool_heap_buf);
    configs.push_back(wasm_config_set_mem_alloc_opt(
        pool_config, Alloc_With_Pool, &pool_option));
    
    // Performance config
    wasm_config_t* perf_config = wasm_config_new();
    configs.push_back(wasm_config_set_linux_perf_opt(perf_config, true));
    
    for (wasm_config_t* config : configs) {
        ASSERT_NE(nullptr, config);
        wasm_engine_t* engine = wasm_engine_new_with_config(config);
        ASSERT_NE(nullptr, engine);
        engines.push_back(engine);
    }
    
    // All engines should be the same (singleton behavior)
    for (size_t i = 1; i < engines.size(); ++i) {
        ASSERT_EQ(engines[0], engines[i]);
    }
    
    wasm_engine_delete(engines[0]);
}
