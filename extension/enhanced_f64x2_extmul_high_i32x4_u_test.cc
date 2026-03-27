/*
 * Copyright (C) 2024 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <cstring>
#include <gtest/gtest.h>
#include <climits>
#include <cfloat>
#include <memory>

#include "wasm_runtime.h"
#include "wasm_runtime_common.h"
#include "wasm_export.h"
#include "test_helper.h"

class F64x2ExtmulHighI32x4UTest : public testing::Test {
protected:
    std::unique_ptr<WAMRRuntimeRAII<>> runtime_raii;
    std::unique_ptr<DummyExecEnv> dummy_env;

    /**
     * @brief Set up the test fixture with WAMR runtime initialization and module loading
     * @details Initializes WAMR with SIMD support, loads the test WASM module, and creates execution environment
     */
    void SetUp() override {
        // Initialize WAMR runtime using RAII helper
        runtime_raii = std::make_unique<WAMRRuntimeRAII<>>();

        // Load the f64x2.extmul_high_i32x4_u test module using DummyExecEnv
        dummy_env = std::make_unique<DummyExecEnv>("wasm-apps/f64x2_extmul_high_i32x4_u_test.wasm");
        ASSERT_NE(nullptr, dummy_env->get())
            << "Failed to create execution environment for f64x2.extmul_high_i32x4_u tests";
    }

    /**
     * @brief Clean up test resources and destroy WAMR runtime components
     * @details Properly deallocates execution environment and WAMR runtime using RAII pattern
     */
    void TearDown() override {
        dummy_env.reset();
        runtime_raii.reset();
    }

    /**
     * @brief Execute f64x2.extmul_high_i32x4_u operation with two v128 inputs
     * @param func_name WASM function name to call
     * @param a0,a1,a2,a3 u32 lanes for first v128 vector (lanes 0,1,2,3)
     * @param b0,b1,b2,b3 u32 lanes for second v128 vector (lanes 0,1,2,3)
     * @param result_0,result_1 Output f64 values from lanes 2,3 multiplication
     * @details Calls WASM function and extracts the two f64 results from extended multiplication of high lanes
     */
    void call_f64x2_extmul_high_i32x4_u(const char* func_name,
                                        uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3,
                                        uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3,
                                        double* result_0, double* result_1) {
        wasm_module_inst_t module_inst = wasm_runtime_get_module_inst(dummy_env->get());
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name);
        ASSERT_NE(nullptr, func) << "Failed to lookup function: " << func_name;

        uint32_t argv[8] = {a0, a1, a2, a3, b0, b1, b2, b3};

        bool ret = wasm_runtime_call_wasm(dummy_env->get(), func, 8, argv);
        ASSERT_TRUE(ret) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        // Extract f64 results from v128 return value stored in argv[0] and argv[1]
        // The WASM function returns f64x2 as two consecutive f64 values
        uint64_t result_bits_0 = (static_cast<uint64_t>(argv[1]) << 32) | argv[0];
        uint64_t result_bits_1 = (static_cast<uint64_t>(argv[3]) << 32) | argv[2];

        memcpy(result_0, &result_bits_0, sizeof(double));
        memcpy(result_1, &result_bits_1, sizeof(double));
    }
};

/**
 * @test ModuleValidation_HandlesMalformedInput
 * @brief Validates proper error handling for malformed WASM modules and invalid SIMD instructions
 * @details Tests WAMR's module validation capabilities when encountering invalid SIMD opcodes
 *          or modules without proper SIMD feature declarations.
 * @test_category Error - Module validation and error handling testing
 * @coverage_target core/iwasm/common/wasm_loader.c:module_validation
 * @input_conditions Invalid WASM module structures, malformed SIMD instructions, missing features
 * @expected_behavior Proper module loading failure and error reporting
 * @validation_method ASSERT_EQ for expected failure conditions and error message validation
 */
TEST_F(F64x2ExtmulHighI32x4UTest, ModuleValidation_HandlesMalformedInput) {
    // Test module with invalid SIMD instruction encoding
    // This test validates WAMR's ability to detect malformed SIMD opcodes during module loading

    // Create minimal invalid WASM bytecode with malformed f64x2.extmul_high_i32x4_u instruction
    uint8_t invalid_wasm[] = {
        0x00, 0x61, 0x73, 0x6d,  // WASM magic number
        0x01, 0x00, 0x00, 0x00,  // Version 1
        0x01, 0x04, 0x01, 0x60,  // Type section: function type
        0x00, 0x00,              // No parameters, no results
        0x03, 0x02, 0x01, 0x00,  // Function section: one function of type 0
        0x0a, 0x06, 0x01, 0x04,  // Code section: one function body
        0x00, 0xfd, 0xff, 0x01   // Invalid SIMD opcode sequence
    };

    char error_buf[128];
    wasm_module_t invalid_module = wasm_runtime_load(invalid_wasm, sizeof(invalid_wasm),
                                                    error_buf, sizeof(error_buf));

    ASSERT_EQ(nullptr, invalid_module)
        << "Expected module load to fail for invalid SIMD bytecode, but got valid module";

    // Verify that error message indicates WASM validation issue (any validation error is acceptable)
    ASSERT_TRUE(strlen(error_buf) > 0)
        << "Expected validation error message, but got empty error buffer";
    ASSERT_TRUE(strstr(error_buf, "WASM module load failed") != nullptr ||
                strstr(error_buf, "invalid") != nullptr ||
                strstr(error_buf, "section") != nullptr ||
                strstr(error_buf, "opcode") != nullptr)
        << "Expected WASM validation error message, got: " << error_buf;
}
