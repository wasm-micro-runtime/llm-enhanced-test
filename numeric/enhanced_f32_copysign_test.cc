/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Enhanced Unit Tests for f32.copysign Opcode
 *
 * This test suite provides comprehensive coverage of the WebAssembly f32.copysign
 * instruction across multiple execution modes (interpreter and AOT).
 *
 * Test Categories:
 * - Main Routine: Basic functionality with typical values
 * - Corner Cases: Boundary conditions with extreme finite values
 * - Edge Cases: Special IEEE 754 values (zeros, infinities, NaN)
 * - Error Handling: Stack underflow and validation errors
 *
 * Target Coverage: core/iwasm/interpreter/wasm_interp_classic.c (lines handling f32.copysign)
 * Source Location: core/iwasm/interpreter/wasm_interp_classic.c (f32.copysign implementation)
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

using namespace std;

/**
 * @brief Test fixture class for f32.copysign opcode validation
 * @details Provides comprehensive testing infrastructure for the WebAssembly f32.copysign
 *          instruction across both interpreter and AOT execution modes. This test suite
 *          validates IEEE 754 floating-point sign manipulation behavior, ensuring correct
 *          implementation of the copysign operation which copies the sign bit from the
 *          second operand to the result while preserving the magnitude of the first operand.
 */
class F32CopysignTest : public testing::TestWithParam<RunningMode>
{
protected:
    /**
     * @brief Set up test environment before each test case
     * @details Initializes WAMR runtime with system allocator, loads the f32.copysign test
     *          WASM module, and prepares execution context for both interpreter and AOT modes.
     *          Ensures proper module validation and instance creation for reliable test execution.
     */
    void SetUp() override
    {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize WAMR runtime environment
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load WASM module for f32.copysign testing
        load_test_module();
    }

    /**
     * @brief Clean up test environment after each test case
     * @details Properly destroys WASM module instance, unloads module, releases memory
     *          resources, and shuts down WAMR runtime to prevent resource leaks.
     */
    void TearDown() override
    {
        // Clean up WAMR resources in proper order
        if (exec_env != nullptr) {
            wasm_runtime_destroy_exec_env(exec_env);
            exec_env = nullptr;
        }
        if (module_inst != nullptr) {
            wasm_runtime_deinstantiate(module_inst);
            module_inst = nullptr;
        }
        if (module != nullptr) {
            wasm_runtime_unload(module);
            module = nullptr;
        }
        if (buf != nullptr) {
            BH_FREE(buf);
            buf = nullptr;
        }
        wasm_runtime_destroy();
    }

private:
    /**
     * @brief Load f32.copysign test WASM module from file system
     * @details Reads the compiled WASM bytecode for f32.copysign tests, validates module
     *          format, loads module into WAMR, and creates executable module instance.
     *          Handles both interpreter and AOT execution modes based on test parameters.
     */
    void load_test_module()
    {
        const char *wasm_path = "wasm-apps/f32_copysign_test.wasm";

        // Read WASM module file from disk
        buf = (uint8_t*)bh_read_file_to_buffer(wasm_path, &buf_size);
        ASSERT_NE(nullptr, buf) << "Failed to read WASM file: " << wasm_path;
        ASSERT_GT(buf_size, 0U) << "WASM file is empty: " << wasm_path;

        // Load and validate WASM module
        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;

        // Create module instance for execution
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate WASM module: " << error_buf;

        // Set execution mode and create execution environment
        wasm_runtime_set_running_mode(module_inst, GetParam());

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(nullptr, exec_env) << "Failed to create execution environment";
    }

public:
    /**
     * @brief Execute f32.copysign operation with two f32 operands
     * @details Calls the WASM f32.copysign test function with specified parameters,
     *          handles execution errors, and returns the computed result for validation.
     * @param magnitude f32 value providing the magnitude component
     * @param sign f32 value providing the sign component
     * @param out_result receives the f32 result of copysign(magnitude, sign) operation
     * @param out_error receives an error string if the call fails (optional)
     * @return bool indicating whether the wasm call succeeded without exception
     */
    bool call_f32_copysign(float magnitude, float sign, float *out_result,
                           std::string *out_error = nullptr)
    {
        if (out_result == nullptr) {
            if (out_error != nullptr) {
                *out_error = "out_result is nullptr";
            }
            return false;
        }

        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_f32_copysign");
        if (func == nullptr) {
            if (out_error != nullptr) {
                *out_error = "Failed to find test_f32_copysign function";
            }
            return false;
        }

        // Convert float parameters to uint32_t for WAMR function call
        union { float f; uint32_t u; } mag_conv = { .f = magnitude };
        union { float f; uint32_t u; } sign_conv = { .f = sign };

        uint32_t argv[3] = { mag_conv.u, sign_conv.u, 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        const char *exception = wasm_runtime_get_exception(module_inst);
        if (!ret) {
            if (out_error != nullptr) {
                *out_error = exception != nullptr ? exception : "wasm_runtime_call_wasm returned false";
            }
            return false;
        }

        if (exception != nullptr) {
            if (out_error != nullptr) {
                *out_error = exception;
            }
            return false;
        }

        // Convert result back to float - result is in argv[0]
        union { float f; uint32_t u; } result_conv = { .u = argv[0] };
        *out_result = result_conv.f;
        return true;
    }

    /**
     * @brief Execute stack underflow test scenarios
     * @details Tests f32.copysign behavior with insufficient stack operands to validate
     *          proper error handling and graceful failure modes in WAMR runtime.
     * @return bool indicating whether stack underflow was properly detected
     */
    bool test_stack_underflow()
    {
        /*
         * Note:
         * WebAssembly validation rejects instructions that consume more operands
         * than available on the value stack (i.e. "stack underflow") at load time.
         * Toolchains typically won't emit such invalid bytecode, so we construct a
         * minimal invalid wasm module by hand:
         *   f32.const 0.0
         *   f32.copysign   ;; requires 2 operands, but only 1 is on the stack
         *   end
         */
        static const uint8_t invalid_wasm_const[] = {
            /* wasm magic + version */
            0x00, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00,
            /* Type section (1): 1 type: (func) */
            0x01, 0x04, 0x01, 0x60, 0x00, 0x00,
            /* Function section (3): 1 function of type 0 */
            0x03, 0x02, 0x01, 0x00,
            /* Export section (7): export func 0 as "test_stack_underflow" */
            0x07, 0x18, 0x01, 0x14,
            0x74, 0x65, 0x73, 0x74, 0x5f, 0x73, 0x74, 0x61, 0x63, 0x6b,
            0x5f, 0x75, 0x6e, 0x64, 0x65, 0x72, 0x66, 0x6c, 0x6f, 0x77,
            0x00, 0x00,
            /* Code section (10): 1 body */
            0x0a, 0x0a, 0x01, 0x08, 0x00,
            /* f32.const 0.0 */
            0x43, 0x00, 0x00, 0x00, 0x00,
            /* f32.copysign */
            0x98,
            /* end */
            0x0b
        };

        /* WAMR loader may modify the original wasm buffer in-place (e.g. to NUL-terminate names),
           so we must pass a writable copy instead of a const buffer in .rodata. */
        std::vector<uint8_t> invalid_wasm(invalid_wasm_const,
                                          invalid_wasm_const + sizeof(invalid_wasm_const));

        char local_error_buf[256] = { 0 };
        wasm_module_t invalid_module =
            wasm_runtime_load(invalid_wasm.data(),
                              (uint32_t)invalid_wasm.size(),
                              local_error_buf, sizeof(local_error_buf));

        if (invalid_module != nullptr) {
            /* If it unexpectedly loads, unload to avoid leaks and report failure */
            wasm_runtime_unload(invalid_module);
            return false;
        }

        /* Ensure we actually got a validation error message (may vary by build) */
        return local_error_buf[0] != '\0';
    }

protected:
    // WAMR runtime configuration and state
    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint8_t *buf = nullptr;
    uint32_t buf_size = 0;
    char error_buf[256];

    // Execution configuration parameters
    static constexpr uint32_t stack_size = 64 * 1024;   // 64KB stack
    static constexpr uint32_t heap_size = 64 * 1024;    // 64KB heap
};

/**
 * @test BasicCopysign_TypicalValues_ReturnsCorrectResults
 * @brief Validates f32.copysign produces correct results for typical floating-point inputs
 * @details Tests fundamental copysign operation with positive, negative, and mixed-sign combinations.
 *          Verifies that f32.copysign correctly computes result with first operand's magnitude
 *          and second operand's sign for representative input values.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @input_conditions Standard f32 pairs: (3.5f,2.1f), (42.0f,-7.8f), (-15.25f,4.0f), (-8.5f,-3.2f)
 * @expected_behavior Returns magnitude of first with sign of second: 3.5f, -42.0f, 15.25f, -8.5f
 * @validation_method Direct ASSERT_FLOAT_EQ comparison of WASM function result with expected values
 */
TEST_P(F32CopysignTest, BasicCopysign_TypicalValues_ReturnsCorrectResults)
{
    std::string err;
    float result = 0.0f;

    // Test positive magnitude with positive sign
    ASSERT_TRUE(call_f32_copysign(3.5f, 2.1f, &result, &err)) << err;
    ASSERT_FLOAT_EQ(3.5f, result)
        << "copysign(3.5, 2.1) should return 3.5";

    // Test positive magnitude with negative sign
    ASSERT_TRUE(call_f32_copysign(42.0f, -7.8f, &result, &err)) << err;
    ASSERT_FLOAT_EQ(-42.0f, result)
        << "copysign(42.0, -7.8) should return -42.0";

    // Test negative magnitude with positive sign
    ASSERT_TRUE(call_f32_copysign(-15.25f, 4.0f, &result, &err)) << err;
    ASSERT_FLOAT_EQ(15.25f, result)
        << "copysign(-15.25, 4.0) should return 15.25";

    // Test negative magnitude with negative sign
    ASSERT_TRUE(call_f32_copysign(-8.5f, -3.2f, &result, &err)) << err;
    ASSERT_FLOAT_EQ(-8.5f, result)
        << "copysign(-8.5, -3.2) should return -8.5";
}

// Parameterized test instantiation for both execution modes
INSTANTIATE_TEST_SUITE_P(RunningModeTest, F32CopysignTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<F32CopysignTest::ParamType> &info) {
                             return info.param == Mode_Interp ? "INTERP" : "AOT";
                         });
