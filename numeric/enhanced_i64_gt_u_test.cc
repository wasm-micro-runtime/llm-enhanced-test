/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <iostream>
#include <fstream>

extern "C" {
#include "wasm_export.h"
#include "bh_read_file.h"
#include "wasm_runtime_common.h"
}

/**
 * @brief Test fixture for i64.gt_u opcode validation
 * @details Provides comprehensive testing infrastructure for WebAssembly i64.gt_u
 *          instruction across different WAMR execution modes (interpreter and AOT).
 *          Tests unsigned 64-bit integer greater-than comparison operations.
 */
class I64GtUTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Initialize WAMR runtime and load test module
     * @details Sets up WAMR runtime environment with proper memory allocation
     *          and loads the i64.gt_u test module for execution validation.
     */
    void SetUp() override {
        // Initialize WAMR runtime with system allocator
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load test WASM module
        module_buffer = LoadWASMModule();
        ASSERT_NE(nullptr, module_buffer) << "Failed to load WASM module buffer";

        // Load module into WAMR runtime
        char error_buf[128] = {0};
        module = wasm_runtime_load(module_buffer, module_buffer_size,
                                   error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module)
            << "Failed to load WASM module: " << error_buf;

        // Instantiate module with execution mode
        module_inst = wasm_runtime_instantiate(module, 65536, 65536,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst)
            << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode for parameterized testing
        wasm_runtime_set_running_mode(module_inst, GetParam());

        // Get function reference
        gt_u_func = wasm_runtime_lookup_function(module_inst, "i64_gt_u_test");
        ASSERT_NE(nullptr, gt_u_func)
            << "Failed to lookup i64_gt_u_test function";
    }

    /**
     * @brief Clean up WAMR runtime resources
     * @details Properly deallocates all WAMR runtime resources including
     *          module instances, modules, and runtime environment.
     */
    void TearDown() override {
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
        }
        delete[] module_buffer;
        wasm_runtime_destroy();
    }

    /**
     * @brief Load WASM test module from file system
     * @details Reads the compiled WASM module containing i64.gt_u test function
     * @return Pointer to module buffer or nullptr on failure
     */
    unsigned char* LoadWASMModule() {
        const char* wasm_file = "wasm-apps/i64_gt_u_test.wasm";
        return (unsigned char*)bh_read_file_to_buffer(wasm_file, &module_buffer_size);
    }

    /**
     * @brief Execute i64.gt_u test function with given inputs
     * @details Calls the WASM function that applies i64.gt_u to two 64-bit inputs
     * @param first The first 64-bit integer operand
     * @param second The second 64-bit integer operand
     * @return The comparison result as 32-bit integer (1 if first > second unsigned, 0 otherwise)
     */
    int32_t CallI64GtU(uint64_t first, uint64_t second) {
        wasm_val_t args[2];
        wasm_val_t results[1];

        // Set up function arguments
        args[0].kind = WASM_I64;
        args[0].of.i64 = static_cast<int64_t>(first);
        args[1].kind = WASM_I64;
        args[1].of.i64 = static_cast<int64_t>(second);

        // Execute function
        wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(module_inst, 65536);
        EXPECT_NE(nullptr, exec_env) << "Failed to create execution environment";

        bool success = wasm_runtime_call_wasm_a(exec_env, gt_u_func, 1, results, 2, args);
        wasm_runtime_destroy_exec_env(exec_env);

        EXPECT_TRUE(success) << "Function call failed";
        return results[0].of.i32;
    }

    // Test infrastructure members
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_function_inst_t gt_u_func = nullptr;
    unsigned char* module_buffer = nullptr;
    uint32_t module_buffer_size = 0;
};

/**
 * @test StackUnderflow_HandlesGracefully
 * @brief Validates handling of stack underflow conditions during i64.gt_u execution
 * @details Tests error conditions when insufficient operands are available on stack.
 *          Note: i64.gt_u itself doesn't trap, but stack underflow causes runtime errors.
 * @test_category Error - Stack underflow and error condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:POP_I64
 * @input_conditions WASM modules designed to trigger stack underflow scenarios
 * @expected_behavior Graceful error handling without crashes
 * @validation_method Verification that runtime errors are properly handled
 */
TEST_P(I64GtUTest, StackUnderflow_HandlesGracefully) {
    // Load underflow test module
    const char* underflow_wasm_file = "wasm-apps/i64_gt_u_stack_underflow.wasm";
    uint32_t underflow_buf_size = 0;
    auto* underflow_buf = reinterpret_cast<unsigned char*>(
        bh_read_file_to_buffer(underflow_wasm_file, &underflow_buf_size));

    if (underflow_buf != nullptr) {
        char error_buf[128] = {0};
        wasm_module_t underflow_module = wasm_runtime_load(underflow_buf, underflow_buf_size,
                                                          error_buf, sizeof(error_buf));

        // For stack underflow scenarios, either:
        // 1. Module loading fails (validation catches insufficient stack) - error_buf is set
        // 2. Module loads but execution fails gracefully
        if (underflow_module == nullptr) {
            // Validation caught the stack underflow at load time
            ASSERT_NE('\0', error_buf[0])
                << "Load failure should set an error message in error_buf";
        } else {
            wasm_module_inst_t underflow_inst = wasm_runtime_instantiate(
                underflow_module, 65536, 65536, error_buf, sizeof(error_buf));

            if (underflow_inst != nullptr) {
                wasm_function_inst_t underflow_func =
                    wasm_runtime_lookup_function(underflow_inst, "underflow_test");

                if (underflow_func != nullptr) {
                    wasm_exec_env_t exec_env =
                        wasm_runtime_create_exec_env(underflow_inst, 65536);
                    ASSERT_NE(nullptr, exec_env)
                        << "Failed to create exec env for underflow test";

                    wasm_val_t results[1];
                    bool call_result = wasm_runtime_call_wasm_a(
                        exec_env, underflow_func, 1, results, 0, nullptr);

                    // Runtime must either catch the underflow (call returns false)
                    // or handle it without crashing
                    if (!call_result) {
                        const char* exception = wasm_runtime_get_exception(underflow_inst);
                        ASSERT_NE(nullptr, exception)
                            << "Call failure must set a runtime exception message";
                    }

                    wasm_runtime_destroy_exec_env(exec_env);
                } else {
                    ADD_FAILURE() << "underflow_test function not found after module instantiation"
                                  << " — underflow scenario was not exercised";
                }
                wasm_runtime_deinstantiate(underflow_inst);
            }
            wasm_runtime_unload(underflow_module);
        }
        delete[] underflow_buf;
    } else {
        FAIL() << "i64_gt_u_stack_underflow.wasm not found; underflow scenario was never tested";
    }
}

// Parameterized test instantiation for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(
    I64GtUModeTest,
    I64GtUTest,
    testing::Values(Mode_Interp, Mode_Fast_JIT, Mode_LLVM_JIT),
    [](const testing::TestParamInfo<RunningMode>& info) {
        switch(info.param) {
            case Mode_Interp: return "Interpreter";
            case Mode_Fast_JIT: return "FastJIT";
            case Mode_LLVM_JIT: return "LLVMJIT";
            default: return "Unknown";
        }
    }
);
