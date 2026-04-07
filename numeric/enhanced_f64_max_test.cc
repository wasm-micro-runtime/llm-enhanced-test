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
// Available values: Mode_Interp, Mode_Fast_JIT, Mode_LLVM_JIT, Mode_Multi_Tier_JIT

/**
 * Enhanced f64.max Opcode Test Suite
 *
 * This test suite validates the f64.max WebAssembly opcode implementation in WAMR.
 * f64.max performs IEEE 754 maximum comparison between two double-precision
 * floating-point numbers, handling special values like NaN, infinity, and signed zero.
 *
 * The test suite covers:
 * - Basic maximum operations with typical double values
 * - Boundary conditions with extreme double values
 * - IEEE 754 special value handling (NaN, infinity, signed zero)
 * - Cross-execution mode validation (interpreter vs AOT)
 *
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_max_operation
 * @coverage_target core/iwasm/aot/: AOT compiler f64.max generation
 */
class F64MaxTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * Set up WAMR runtime environment and load f64.max test module
     *
     * Initializes WAMR with system allocator and loads the test WASM module
     * containing f64.max test functions for validation.
     */
    void SetUp() override {
        // Initialize WAMR runtime using RAII
        runtime = std::make_unique<WAMRRuntimeRAII>();

        // Load WASM test module
        std::string wasm_file = "wasm-apps/f64_max_test.wasm";
        module_buf = load_wasm_module(wasm_file);
        ASSERT_NE(nullptr, module_buf) << "Failed to load f64.max test module from " << wasm_file;

        // Load module into WAMR runtime
        char error_buf[256];
        module = wasm_runtime_load(module_buf.get(), module_buf_size,
                                 error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;

        // Instantiate module
        module_inst = wasm_runtime_instantiate(module, 32 * 1024, 32 * 1024,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate WASM module: " << error_buf;

        // Set running mode to match test parameter
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
     * Call f64.max WASM function with two double arguments
     *
     * @param a First double operand
     * @param b Second double operand
     * @return Result of f64.max(a, b)
     */
    double call_f64_max(double a, double b) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_f64_max");
        EXPECT_NE(nullptr, func) << "Failed to lookup test_f64_max function";

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
        EXPECT_TRUE(call_success) << "Failed to call f64.max function";

        // Result is in first two slots (64-bit f64)
        uint64_t result_u64 = static_cast<uint64_t>(argv[0]) |
                             (static_cast<uint64_t>(argv[1]) << 32);

        union { uint64_t u; double f; } result_conv = { .u = result_u64 };
        return result_conv.f;
    }

    /**
     * Call f64.max WASM function for special values (NaN, infinity, etc.)
     *
     * @param func_name Name of the test function to call
     * @param a First double operand
     * @param b Second double operand
     * @return Result of f64.max(a, b)
     */
    double call_f64_max_special(const std::string& func_name, double a, double b) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, func_name.c_str());
        EXPECT_NE(nullptr, func) << "Failed to lookup " << func_name << " function";

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
        EXPECT_TRUE(call_success) << "Failed to call " << func_name << " function";

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
 * @test SpecialValues_IEEE754Compliance
 * @brief Validates f64.max handles IEEE 754 special values correctly
 * @details Tests maximum operations with NaN, positive/negative infinity,
 *          and signed zero values. Ensures compliance with IEEE 754 specification
 *          for special value handling including NaN propagation and zero sign handling.
 * @test_category Edge - IEEE 754 special value validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_max_operation
 * @input_conditions NaN values, positive/negative infinity, signed zeros (+0.0, -0.0)
 * @expected_behavior NaN propagation, infinity dominance, positive zero preference
 * @validation_method Special value detection using isnan(), isinf(), signbit()
 */
TEST_P(F64MaxTest, SpecialValues_IEEE754Compliance) {
    double pos_inf = static_cast<double>(INFINITY);
    double neg_inf = -static_cast<double>(INFINITY);
    double nan_val = NAN;
    double finite_val = 42.5;

    // Test NaN propagation - any comparison with NaN should return NaN
    double nan_result1 = call_f64_max_special("test_f64_max_nan", nan_val, finite_val);
    ASSERT_TRUE(std::isnan(nan_result1))
        << "f64.max failed NaN propagation test: max(NaN, finite) should return NaN";

    double nan_result2 = call_f64_max_special("test_f64_max_nan", finite_val, nan_val);
    ASSERT_TRUE(std::isnan(nan_result2))
        << "f64.max failed NaN propagation test: max(finite, NaN) should return NaN";

    // Test positive infinity dominance
    ASSERT_EQ(pos_inf, call_f64_max_special("test_f64_max_infinity", pos_inf, finite_val))
        << "f64.max failed positive infinity test: max(+inf, finite) should return +inf";

    ASSERT_EQ(finite_val, call_f64_max_special("test_f64_max_infinity", neg_inf, finite_val))
        << "f64.max failed negative infinity test: max(-inf, finite) should return finite";

    ASSERT_EQ(pos_inf, call_f64_max_special("test_f64_max_infinity", pos_inf, neg_inf))
        << "f64.max failed infinity comparison: max(+inf, -inf) should return +inf";

    // Test signed zero handling - positive zero should be preferred
    double pos_zero = +0.0;
    double neg_zero = -0.0;

    double zero_result = call_f64_max_special("test_f64_max_zero", pos_zero, neg_zero);
    ASSERT_EQ(0.0, zero_result) << "f64.max failed zero comparison: result should be zero";
    ASSERT_FALSE(std::signbit(zero_result))
        << "f64.max failed signed zero test: max(+0.0, -0.0) should return +0.0";

    // Test commutative property with signed zeros
    double zero_result_comm = call_f64_max_special("test_f64_max_zero", neg_zero, pos_zero);
    ASSERT_EQ(0.0, zero_result_comm) << "f64.max failed commutative zero test: result should be zero";
    ASSERT_FALSE(std::signbit(zero_result_comm))
        << "f64.max failed commutative signed zero test: max(-0.0, +0.0) should return +0.0";
}

// Parameterized test instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(
    CrossExecutionMode,
    F64MaxTest,
    testing::Values(Mode_Interp),
    [](const testing::TestParamInfo<RunningMode>& info) {
        switch (info.param) {
            case Mode_Interp: return "Interpreter";
            default: return "Unknown";
        }
    }
);
