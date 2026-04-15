/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <climits>
#include "gtest/gtest.h"
#include "wasm_export.h"
#include "bh_platform.h"
#include "../../common/test_helper.h"
#include <fstream>
#include <vector>

class AOTPlatformIntegrationTest : public testing::Test
{
protected:
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
    }

    void TearDown() override
    {
        wasm_runtime_destroy();
    }

    bool load_aot_file(const char *filename, uint8_t **buffer, uint32_t *size)
    {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return false;
        }

        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);

        *buffer = (uint8_t *)wasm_runtime_malloc(file_size);
        if (*buffer == nullptr) {
            return false;
        }

        if (file.read(reinterpret_cast<char*>(*buffer), file_size).fail()) {
            wasm_runtime_free(*buffer);
            *buffer = nullptr;
            return false;
        }

        *size = static_cast<uint32_t>(file_size);
        return true;
    }

    void cleanup_buffer(uint8_t *buffer)
    {
        if (buffer != nullptr) {
            wasm_runtime_free(buffer);
        }
    }

    RuntimeInitArgs init_args;
    static char global_heap_buf[512 * 1024];
};

char AOTPlatformIntegrationTest::global_heap_buf[512 * 1024];

TEST_F(AOTPlatformIntegrationTest, RelocationHandling_CorruptedAOTBytes_LoadFails)
{
    uint8_t *buffer = nullptr;
    uint32_t size = 0;

    if (!load_aot_file("wasm-apps/simple_function.aot", &buffer, &size)) {
        GTEST_SKIP() << "AOT file not available";
    }

    // Corrupt bytes in the AOT file to trigger load failure
    if (size > 100) {
        for (uint32_t i = 50; i < size - 10 && i < 200; i++) {
            if (buffer[i] == 0x01 || buffer[i] == 0x02) {
                buffer[i] = 0xFF;
                break;
            }
        }
    }

    char error_buf[128] = { 0 };
    wasm_module_t module =
        wasm_runtime_load(buffer, size, error_buf, sizeof(error_buf));

    // The corruption should cause a load or instantiation failure
    if (module != nullptr) {
        wasm_module_inst_t module_inst =
            wasm_runtime_instantiate(module, 65536, 0, error_buf,
                                     sizeof(error_buf));
        // At least one of load or instantiate should fail with corrupted data
        EXPECT_EQ(nullptr, module_inst)
            << "Corrupted AOT file should fail to instantiate";
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        wasm_runtime_unload(module);
    }

    cleanup_buffer(buffer);
}

TEST_F(AOTPlatformIntegrationTest, NativeFunctionBinding_ValidSignature_Success)
{
    // Test native function binding with correct signature
    static bool native_func_called = false;
    
    auto native_func = [](wasm_exec_env_t exec_env) -> int32_t {
        native_func_called = true;
        return 42;
    };

    NativeSymbol native_symbols[] = {
        {"test_native", reinterpret_cast<void*>(+native_func), "(*)i", nullptr}
    };

    uint8_t *buffer = nullptr;
    uint32_t size = 0;
    
    if (!load_aot_file("wasm-apps/simple_function.aot", &buffer, &size)) {
        GTEST_SKIP() << "AOT file not available";
    }

    // Register native functions
    ASSERT_TRUE(wasm_runtime_register_natives("env", native_symbols, 1))
        << "Native function registration should succeed";

    wasm_module_t module = wasm_runtime_load(buffer, size, nullptr, 0);
    ASSERT_NE(nullptr, module) << "Module should load successfully";

    wasm_module_inst_t module_inst = wasm_runtime_instantiate(module, 65536, 0, nullptr, 0);
    ASSERT_NE(nullptr, module_inst) << "Module with native bindings should instantiate";

    wasm_runtime_deinstantiate(module_inst);
    wasm_runtime_unload(module);
    cleanup_buffer(buffer);
}

TEST_F(AOTPlatformIntegrationTest, PlatformCallingConvention_FactorialCall_Success)
{
    uint8_t *buffer = nullptr;
    uint32_t size = 0;

    if (!load_aot_file("wasm-apps/multi_function.aot", &buffer, &size)) {
        GTEST_SKIP() << "AOT file not available";
    }

    wasm_module_t module = wasm_runtime_load(buffer, size, nullptr, 0);
    ASSERT_NE(nullptr, module) << "Module should load";

    wasm_module_inst_t module_inst =
        wasm_runtime_instantiate(module, 65536, 0, nullptr, 0);
    ASSERT_NE(nullptr, module_inst) << "Module should instantiate";

    wasm_exec_env_t exec_env =
        wasm_runtime_create_exec_env(module_inst, 32768);
    ASSERT_NE(nullptr, exec_env) << "Exec env should be created";

    wasm_function_inst_t func =
        wasm_runtime_lookup_function(module_inst, "factorial");
    ASSERT_NE(nullptr, func) << "Function 'factorial' should be found";

    uint32_t argv[1] = { 5 };
    ASSERT_TRUE(wasm_runtime_call_wasm(exec_env, func, 1, argv))
        << "Function call should succeed";
    ASSERT_EQ(120u, argv[0]) << "factorial(5) should return 120";

    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_deinstantiate(module_inst);
    wasm_runtime_unload(module);
    cleanup_buffer(buffer);
}

TEST_F(AOTPlatformIntegrationTest, ErrorHandling_CorruptedAOTBytes_LoadOrInstantiateFails)
{
    uint8_t *buffer = nullptr;
    uint32_t size = 0;

    if (!load_aot_file("wasm-apps/simple_function.aot", &buffer, &size)) {
        GTEST_SKIP() << "AOT file not available";
    }

    // Corrupt bytes in the instruction area of the file
    if (size > 200) {
        for (uint32_t i = 100; i < size - 50 && i < 300; i++) {
            if (buffer[i] < 0x80) {
                buffer[i] = 0xFF;
                break;
            }
        }
    }

    char error_buf[128] = { 0 };
    wasm_module_t module =
        wasm_runtime_load(buffer, size, error_buf, sizeof(error_buf));

    if (module != nullptr) {
        wasm_module_inst_t module_inst =
            wasm_runtime_instantiate(module, 65536, 0, error_buf,
                                     sizeof(error_buf));
        // Corrupted file should fail at load or instantiation
        EXPECT_EQ(nullptr, module_inst)
            << "Corrupted AOT file should fail to instantiate";
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        wasm_runtime_unload(module);
    }

    cleanup_buffer(buffer);
}

TEST_F(AOTPlatformIntegrationTest, ErrorHandling_MalformedAOTFile_FailsGracefully)
{
    // Create malformed AOT file data
    uint8_t malformed_data[] = {
        0x00, 0x61, 0x73, 0x6d, // Invalid magic number
        0x01, 0x00, 0x00, 0x00, // Version
        0xFF, 0xFF, 0xFF, 0xFF  // Corrupted data
    };

    wasm_module_t module = wasm_runtime_load(malformed_data, sizeof(malformed_data), nullptr, 0);
    ASSERT_EQ(nullptr, module) << "Malformed AOT file should fail to load";
}

TEST_F(AOTPlatformIntegrationTest, AOTFunctionCall_BasicAdd_ReturnsCorrectResult)
{
    uint8_t *buffer = nullptr;
    uint32_t size = 0;

    if (!load_aot_file("wasm-apps/simple_function.aot", &buffer, &size)) {
        GTEST_SKIP() << "AOT file not available";
    }

    wasm_module_t module = wasm_runtime_load(buffer, size, nullptr, 0);
    ASSERT_NE(nullptr, module) << "Module should load successfully";

    wasm_module_inst_t module_inst =
        wasm_runtime_instantiate(module, 65536, 0, nullptr, 0);
    ASSERT_NE(nullptr, module_inst) << "Module should instantiate";

    wasm_exec_env_t exec_env =
        wasm_runtime_create_exec_env(module_inst, 32768);
    ASSERT_NE(nullptr, exec_env) << "Exec env should be created";

    wasm_function_inst_t func =
        wasm_runtime_lookup_function(module_inst, "add");
    ASSERT_NE(nullptr, func) << "Function 'add' should be found";

    uint32_t argv[2] = { 1, 2 };
    ASSERT_TRUE(wasm_runtime_call_wasm(exec_env, func, 2, argv))
        << "Function call should succeed";
    ASSERT_EQ(3u, argv[0]) << "add(1,2) should return 3";

    wasm_runtime_destroy_exec_env(exec_env);
    wasm_runtime_deinstantiate(module_inst);
    wasm_runtime_unload(module);
    cleanup_buffer(buffer);
}

TEST_F(AOTPlatformIntegrationTest, ValidationSecurityChecks_RejectMaliciousContent_Success)
{
    // Create potentially malicious AOT content
    uint8_t malicious_data[1024];
    memset(malicious_data, 0, sizeof(malicious_data));
    
    // Set AOT magic number but with suspicious patterns
    malicious_data[0] = 0x00; malicious_data[1] = 0x61; 
    malicious_data[2] = 0x6f; malicious_data[3] = 0x74; // "aot" magic
    malicious_data[4] = 0x01; malicious_data[5] = 0x00; 
    malicious_data[6] = 0x00; malicious_data[7] = 0x00; // version
    
    // Fill with suspicious patterns that might trigger security checks
    for (int i = 8; i < 1020; i += 4) {
        malicious_data[i] = 0xFF;
        malicious_data[i+1] = 0xFF;
        malicious_data[i+2] = 0xFF;
        malicious_data[i+3] = 0xFF;
    }

    wasm_module_t module = wasm_runtime_load(malicious_data, sizeof(malicious_data), nullptr, 0);
    ASSERT_EQ(nullptr, module) << "Malicious AOT content should be rejected by security checks";
}
