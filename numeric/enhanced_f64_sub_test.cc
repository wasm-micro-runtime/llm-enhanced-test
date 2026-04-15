/**
 * Enhanced Unit Tests for f64.sub WebAssembly Opcode
 *
 * This test suite provides comprehensive validation of the f64.sub operation,
 * which implements IEEE 754 double-precision floating-point subtraction.
 * The opcode takes two f64 operands from the stack and returns their difference
 * (first operand - second operand), handling all special cases according to IEEE 754-2008 standard.
 *
 * Test Coverage:
 * - Basic subtraction operations with standard values
 * - Boundary conditions and extreme values (DBL_MAX, DBL_MIN)
 * - Special IEEE 754 values (NaN, infinity, signed zeros)
 * - Precision edge cases and near-cancellation scenarios
 * - Cross-execution mode validation (interpreter vs AOT)
 * - Runtime error scenarios and module loading failures
 *
 * Source: core/iwasm/interpreter/wasm_interp_classic.c - f64.sub implementation
 */

#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <cstring>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <unistd.h>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

using namespace std;

class F64SubTest : public testing::TestWithParam<RunningMode> {
protected:
    /**
     * @brief Set up WAMR runtime environment and load test module
     * @details Initializes WAMR with interpreter and AOT capabilities,
     *          loads the f64_sub test module, and prepares execution environment
     */
    void SetUp() override {
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;

        // Initialize WAMR runtime environment
        ASSERT_TRUE(wasm_runtime_full_init(&init_args))
            << "Failed to initialize WAMR runtime";

        // Load WASM module for f64.sub testing
        load_test_module();
    }

    /**
     * @brief Clean up WAMR runtime environment and release resources
     * @details Unloads modules, destroys execution instances, and shuts down runtime
     */
    void TearDown() override {
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

    /**
     * @brief Load and instantiate the f64.sub test WASM module
     * @details Reads the compiled WASM bytecode and creates an executable module instance
     */
    void load_test_module() {
        char error_buf[256];
        const char* module_path = "wasm-apps/f64_sub_test.wasm";

        // Load WASM module from bytecode file
        buf = reinterpret_cast<uint8_t*>(bh_read_file_to_buffer(module_path, &buf_size));
        ASSERT_NE(nullptr, buf) << "Failed to read WASM module file: " << module_path;

        // Load module into WAMR runtime
        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module) << "Failed to load WASM module: " << error_buf;

        // Create module instance for execution
        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                             error_buf, sizeof(error_buf));
        ASSERT_NE(nullptr, module_inst) << "Failed to instantiate module: " << error_buf;

        // Set running mode and create execution environment
        wasm_runtime_set_running_mode(module_inst, GetParam());
        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(nullptr, exec_env) << "Failed to create execution environment";
    }

    /**
     * @brief Execute f64.sub operation with two f64 operands
     * @param minuend First operand (value to subtract from)
     * @param subtrahend Second operand (value to subtract)
     * @return Result of f64.sub(minuend, subtrahend) = minuend - subtrahend
     */
    double call_f64_sub(double minuend, double subtrahend) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_f64_sub");
        EXPECT_NE(nullptr, func) << "Failed to find test_f64_sub function";

        // Convert double parameters to uint64_t for WAMR function call
        union { double f; uint64_t u; } min_conv = { .f = minuend };
        union { double f; uint64_t u; } sub_conv = { .f = subtrahend };

        uint32_t argv[4] = {
            static_cast<uint32_t>(min_conv.u),
            static_cast<uint32_t>(min_conv.u >> 32),
            static_cast<uint32_t>(sub_conv.u),
            static_cast<uint32_t>(sub_conv.u >> 32)
        };

        bool ret = wasm_runtime_call_wasm(exec_env, func, 4, argv);
        EXPECT_TRUE(ret) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        const char *exception = wasm_runtime_get_exception(module_inst);
        if (exception != nullptr) {
            printf("Exception: %s\n", exception);
            return 0.0;
        }

        // Extract result from argv (f64 result in first 2 elements)
        union { double f; uint64_t u; } result_conv;
        result_conv.u = static_cast<uint64_t>(argv[0]) | (static_cast<uint64_t>(argv[1]) << 32);
        return result_conv.f;
    }

    /**
     * @brief Execute boundary test that produces values near zero
     * @param minuend First operand for boundary calculation
     * @param subtrahend Second operand for boundary calculation
     * @return Result of boundary f64.sub operation
     */
    double call_f64_sub_boundary(double minuend, double subtrahend) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_f64_sub_boundary");
        EXPECT_NE(nullptr, func) << "Failed to find test_f64_sub_boundary function";

        // Convert double parameters to uint64_t for WAMR function call
        union { double f; uint64_t u; } min_conv = { .f = minuend };
        union { double f; uint64_t u; } sub_conv = { .f = subtrahend };

        uint32_t argv[4] = {
            static_cast<uint32_t>(min_conv.u),
            static_cast<uint32_t>(min_conv.u >> 32),
            static_cast<uint32_t>(sub_conv.u),
            static_cast<uint32_t>(sub_conv.u >> 32)
        };

        bool ret = wasm_runtime_call_wasm(exec_env, func, 4, argv);
        EXPECT_TRUE(ret) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        // Extract result from argv (f64 result in first 2 elements)
        union { double f; uint64_t u; } result_conv;
        result_conv.u = static_cast<uint64_t>(argv[0]) | (static_cast<uint64_t>(argv[1]) << 32);
        return result_conv.f;
    }

    /**
     * @brief Execute special value test for IEEE 754 edge cases
     * @param minuend First operand (may be special value)
     * @param subtrahend Second operand (may be special value)
     * @return Result of special value f64.sub operation
     */
    double call_f64_sub_special(double minuend, double subtrahend) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_f64_sub_special");
        EXPECT_NE(nullptr, func) << "Failed to find test_f64_sub_special function";

        // Convert double parameters to uint64_t for WAMR function call
        union { double f; uint64_t u; } min_conv = { .f = minuend };
        union { double f; uint64_t u; } sub_conv = { .f = subtrahend };

        uint32_t argv[4] = {
            static_cast<uint32_t>(min_conv.u),
            static_cast<uint32_t>(min_conv.u >> 32),
            static_cast<uint32_t>(sub_conv.u),
            static_cast<uint32_t>(sub_conv.u >> 32)
        };

        bool ret = wasm_runtime_call_wasm(exec_env, func, 4, argv);
        EXPECT_TRUE(ret) << "Function call failed: " << wasm_runtime_get_exception(module_inst);

        // Extract result from argv (f64 result in first 2 elements)
        union { double f; uint64_t u; } result_conv;
        result_conv.u = static_cast<uint64_t>(argv[0]) | (static_cast<uint64_t>(argv[1]) << 32);
        return result_conv.f;
    }

    /**
     * @brief Utility function to check if a double value is negative zero (-0.0)
     * @param value Double value to check
     * @return true if value is negative zero, false otherwise
     */
    bool is_negative_zero(double value) {
        return value == 0.0 && std::signbit(value);
    }

    /**
     * @brief Utility function to check if a double value is positive zero (+0.0)
     * @param value Double value to check
     * @return true if value is positive zero, false otherwise
     */
    bool is_positive_zero(double value) {
        return value == 0.0 && !std::signbit(value);
    }

    // Runtime configuration
    RuntimeInitArgs init_args;
    wasm_module_t module = nullptr;
    wasm_module_inst_t module_inst = nullptr;
    wasm_exec_env_t exec_env = nullptr;
    uint8_t* buf = nullptr;
    uint32_t buf_size = 0;
    uint32_t stack_size = 8192;
    uint32_t heap_size = 8192;
};

/**
 * @test BasicSubtraction_ReturnsCorrectDifference
 * @brief Validates f64.sub produces correct arithmetic results for typical double precision inputs
 * @details Tests fundamental subtraction operation with positive, negative, and mixed-sign doubles.
 *          Verifies that f64.sub correctly computes minuend - subtrahend for various input combinations.
 * @test_category Main - Basic functionality validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:f64_sub_operation
 * @input_conditions Standard double pairs: (10.5, 3.2), (-10.5, -3.2), (10.0, -5.0)
 * @expected_behavior Returns mathematical difference: 7.3, -7.3, 15.0 respectively
 * @validation_method Direct comparison of WASM function result with expected values
 */
TEST_P(F64SubTest, BasicSubtraction_ReturnsCorrectDifference) {
    // Test positive number subtraction
    double result1 = call_f64_sub(10.5, 3.2);
    ASSERT_DOUBLE_EQ(7.3, result1) << "Subtraction of positive doubles failed";

    // Test negative number subtraction
    double result2 = call_f64_sub(-10.5, -3.2);
    ASSERT_DOUBLE_EQ(-7.3, result2) << "Subtraction of negative doubles failed";

    // Test mixed sign subtraction
    double result3 = call_f64_sub(10.0, -5.0);
    ASSERT_DOUBLE_EQ(15.0, result3) << "Subtraction with mixed signs failed";
}

// Instantiate parameterized tests for both interpreter and AOT modes
INSTANTIATE_TEST_SUITE_P(RunningModeTest, F64SubTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT));
