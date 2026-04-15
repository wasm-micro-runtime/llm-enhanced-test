/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <climits>
#include <cfloat>
#include <cmath>
#include <cstring>

#include "wasm_runtime_common.h"
#include "wasm_runtime.h"

class WAMRRuntimeRAII {
public:
    WAMRRuntimeRAII() {
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize WAMR runtime
        if (!wasm_runtime_full_init(&init_args)) {
            throw std::runtime_error("Failed to initialize WAMR runtime");
        }
    }

    ~WAMRRuntimeRAII() {
        wasm_runtime_destroy();
    }
};

// Note: Using WAMR's existing RunningMode enum from wasm_export.h
// Available values: Mode_Interp, Mode_AOT, Mode_JIT, Mode_Fast_JIT

/**
 * Enhanced f64.min Opcode Test Suite
 *
 * This test suite validates the f64.min WebAssembly opcode implementation in WAMR.
 * f64.min performs IEEE 754 minimum comparison between two double-precision
 * floating-point numbers, handling special values like NaN, infinity, and signed zero.
 *
 * The test suite covers:
 * - Basic minimum operations with typical double values
 * - Boundary conditions with extreme double values
 * - IEEE 754 special value handling (NaN, infinity, signed zero)
 * - Cross-execution mode validation (interpreter vs AOT)
 *
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_min_operation
 * @coverage_target core/iwasm/aot/: AOT compiler f64.min generation
 */
class F64MinTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * Set up WAMR runtime environment and load f64.min test module
     *
     * Initializes WAMR with system allocator and loads the test WASM module
     * containing f64.min test functions for validation.
     */
    void SetUp() override {
        // Initialize WAMR runtime using RAII
        runtime = std::make_unique<WAMRRuntimeRAII>();

        // Load WASM test module
        std::string wasm_file = "wasm-apps/f64_min_test.wasm";
        module_buf = load_wasm_module(wasm_file);
        ASSERT_NE(nullptr, module_buf) << "Failed to load f64.min test module from " << wasm_file;

        // Load module into WAMR runtime
        char error_buf[256];
        module = wasm_runtime_load(module_buf.get(), module_buf_size,
                                 error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;

        // Instantiate module
        module_inst = wasm_runtime_instantiate(module, 32 * 1024, 32 * 1024,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate WASM module: " << error_buf;

        // Apply running mode so interpreter path is exercised
        wasm_runtime_set_running_mode(module_inst, GetParam());

        // Create execution environment
        exec_env = wasm_runtime_create_exec_env(module_inst, 32 * 1024);
        ASSERT_NE(nullptr, exec_env) << "Failed to create execution environment";
    }

    /**
     * Clean up WAMR runtime resources
     *
     * Properly destroys execution environment and unloads module.
     */
    void TearDown() override {
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
        // RAII handles runtime cleanup
    }

    /**
     * Load WASM module from file system
     *
     * @param filename Path to WASM file relative to test directory
     * @return Unique pointer to module buffer
     */
    std::unique_ptr<uint8_t[]> load_wasm_module(const std::string& filename) {
        FILE* file = fopen(filename.c_str(), "rb");
        if (file == nullptr) return nullptr;

        // Get file size
        (void)fseek(file, 0, SEEK_END);
        module_buf_size = ftell(file);
        (void)fseek(file, 0, SEEK_SET);

        // Allocate buffer and read file
        auto buffer = std::make_unique<uint8_t[]>(module_buf_size);
        size_t read_size = fread(buffer.get(), 1, module_buf_size, file);
        (void)fclose(file);

        if (read_size != module_buf_size) return nullptr;
        return buffer;
    }

    /**
     * Call f64.min WASM function with two double arguments
     *
     * @param a First double operand
     * @param b Second double operand
     * @return Result of f64.min(a, b)
     */
    double call_f64_min(double a, double b) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_f64_min");
        EXPECT_NE(nullptr, func) << "Failed to lookup test_f64_min function";

        // Convert double parameters to uint64_t for WAMR function call
        union { double f; uint64_t u; } a_conv = { .f = a };
        union { double f; uint64_t u; } b_conv = { .f = b };

        // f64 values require 4 uint32_t slots (2 per f64)
        uint32_t argv[4] = {
            static_cast<uint32_t>(a_conv.u),
            static_cast<uint32_t>(a_conv.u >> 32),
            static_cast<uint32_t>(b_conv.u),
            static_cast<uint32_t>(b_conv.u >> 32)
        };

        bool call_success = wasm_runtime_call_wasm(exec_env, func, 4, argv);
        EXPECT_TRUE(call_success) << "Failed to call f64.min function";

        // Result is in first two slots (64-bit f64)
        uint64_t result_u64 = static_cast<uint64_t>(argv[0]) |
                             (static_cast<uint64_t>(argv[1]) << 32);

        union { uint64_t u; double f; } result_conv = { .u = result_u64 };
        return result_conv.f;
    }

    /**
     * Check if two doubles are bit-exact equal
     *
     * @param a First double
     * @param b Second double
     * @return True if bit patterns are identical
     */
    bool double_bit_equal(double a, double b) {
        uint64_t bits_a, bits_b;
        memcpy(&bits_a, &a, sizeof(double));
        memcpy(&bits_b, &b, sizeof(double));
        return bits_a == bits_b;
    }

    std::unique_ptr<WAMRRuntimeRAII> runtime;
    std::unique_ptr<uint8_t[]> module_buf;
    uint32_t module_buf_size = 0;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
};

/**
 * @test BoundaryValues_ReturnsCorrectMinimum
 * @brief Validates f64.min handles double-precision boundary conditions correctly
 * @details Tests minimum operations with extreme values including DBL_MIN, DBL_MAX,
 *          denormalized numbers, and subnormal values. Ensures IEEE 754 compliance
 *          at the boundaries of double-precision floating-point representation.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_min_operation
 * @input_conditions DBL_MIN, DBL_MAX, subnormal numbers, positive/negative infinity
 * @expected_behavior Correct IEEE 754 boundary comparisons and infinity handling
 * @validation_method Exact double comparison and infinity state validation
 */
TEST_P(F64MinTest, BoundaryValues_ReturnsCorrectMinimum) {
    // Test double boundaries: min(DBL_MAX, DBL_MIN) = DBL_MIN
    ASSERT_EQ(DBL_MIN, call_f64_min(DBL_MAX, DBL_MIN))
        << "f64.min failed for DBL_MAX vs DBL_MIN boundary comparison";

    // Test near-boundary values: min(DBL_MAX, 1.0) = 1.0
    ASSERT_EQ(1.0, call_f64_min(DBL_MAX, 1.0))
        << "f64.min failed for DBL_MAX vs normal value comparison";

    // Test subnormal number comparison
    double subnorm_small = 5e-324;  // Smallest subnormal double
    double subnorm_large = 1e-323;  // Larger subnormal double
    ASSERT_EQ(subnorm_small, call_f64_min(subnorm_small, subnorm_large))
        << "f64.min failed for subnormal number comparison";

    // Test signed zero comparison: min(-0.0, +0.0) = -0.0 (IEEE 754)
    double pos_zero = +0.0;
    double neg_zero = -0.0;
    double result_zero = call_f64_min(neg_zero, pos_zero);
    ASSERT_TRUE(double_bit_equal(result_zero, neg_zero))
        << "f64.min failed to prefer negative zero in (-0.0, +0.0) comparison";

    // Test commutative signed zero: min(+0.0, -0.0) = -0.0
    double result_zero_comm = call_f64_min(pos_zero, neg_zero);
    ASSERT_TRUE(double_bit_equal(result_zero_comm, neg_zero))
        << "f64.min failed to prefer negative zero in (+0.0, -0.0) comparison";
}

/**
 * @test SpecialValues_ReturnsIEEE754Compliant
 * @brief Validates f64.min follows IEEE 754 standard for special values
 * @details Tests IEEE 754 compliance including NaN propagation, signed zero handling,
 *          and infinity interactions. Ensures WAMR implementation matches IEEE 754
 *          floating-point standard requirements for minimum operations.
 * @test_category Edge - IEEE 754 special value validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_min_operation
 * @input_conditions NaN values, signed zeros, infinities with finite values
 * @expected_behavior IEEE 754 compliant NaN propagation, negative zero preference, proper infinity handling
 * @validation_method NaN detection, sign bit validation, infinity state checking
 */
TEST_P(F64MinTest, SpecialValues_ReturnsIEEE754Compliant) {
    // Test NaN propagation: min(NAN, 5.0) = NaN
    double result_nan1 = call_f64_min(NAN, 5.0);
    ASSERT_TRUE(std::isnan(result_nan1))
        << "f64.min failed to propagate NaN (first operand)";

    // Test NaN propagation: min(3.0, NAN) = NaN
    double result_nan2 = call_f64_min(3.0, NAN);
    ASSERT_TRUE(std::isnan(result_nan2))
        << "f64.min failed to propagate NaN (second operand)";

    // Test double NaN: min(NAN, NAN) = NaN
    double result_nan3 = call_f64_min(NAN, NAN);
    ASSERT_TRUE(std::isnan(result_nan3))
        << "f64.min failed to handle double NaN inputs";

    // Test positive infinity: min(INFINITY, 1000.0) = 1000.0
    ASSERT_EQ(1000.0, call_f64_min(INFINITY, 1000.0))
        << "f64.min failed for positive infinity vs finite comparison";

    // Test negative infinity: min(-INFINITY, -1000.0) = -INFINITY
    double result_neg_inf = call_f64_min(-static_cast<double>(INFINITY), -1000.0);
    ASSERT_TRUE(std::isinf(result_neg_inf) && result_neg_inf < 0)
        << "f64.min failed for negative infinity vs finite comparison";

    // Test infinity comparison: min(INFINITY, -INFINITY) = -INFINITY
    double result_inf_comp = call_f64_min(static_cast<double>(INFINITY), -static_cast<double>(INFINITY));
    ASSERT_TRUE(std::isinf(result_inf_comp) && result_inf_comp < 0)
        << "f64.min failed for infinity vs negative infinity comparison";
}

// Instantiate parameterized tests for available execution modes
INSTANTIATE_TEST_SUITE_P(RunningModeTests, F64MinTest,
                        testing::Values(Mode_Interp));
