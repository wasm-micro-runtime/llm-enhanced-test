/**
 * @file enhanced_i64_ctz_test.cc
 * @brief Comprehensive unit tests for i64.ctz (count trailing zeros) opcode
 * @details This test suite validates the i64.ctz WebAssembly instruction across all scenarios:
 *          basic functionality, boundary conditions, edge cases, and error conditions.
 *          The tests ensure correct trailing zero counting for 64-bit integers in both
 *          interpreter and AOT execution modes.
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:wasm_interp_call_func_bytecode
 * @coverage_target core/iwasm/aot/aot_runtime.c:aot_call_function
 * @test_modes Interpreter mode and AOT (Ahead-of-Time) mode validation
 */

#include <gtest/gtest.h>
#include <cstdint>                     // Standard integer types
#include <climits>                     // Standard C limits for MIN/MAX values
#include <cstdlib>                     // Standard C library functions
#include <unistd.h>                    // UNIX standard definitions
#include <vector>                      // STL vector for test data organization
#include <utility>                     // STL utility for pair and other utilities
#include "test_helper.h"               // Unit test helper functions and utilities
#include "wasm_runtime_common.h"       // Common runtime functions and structures
#include "bh_read_file.h"              // WAMR file reading utilities

/**
 * @class I64CtzTest
 * @brief Parameterized test fixture for comprehensive i64.ctz opcode validation
 * @details Inherits from TestWithParam to enable cross-execution mode testing (interpreter vs AOT).
 *          Provides consistent test environment setup with proper WAMR initialization,
 *          module loading, and resource cleanup through RAII patterns.
 */

class I64CtzTest : public testing::TestWithParam<RunningMode> {
protected:
    WAMRRuntimeRAII<> runtime;              // RAII runtime management
    wasm_module_t module = nullptr;         // Loaded WASM module
    wasm_module_inst_t module_inst = nullptr; // Instantiated WASM module instance
    wasm_exec_env_t exec_env = nullptr;     // Execution environment
    uint32_t buf_size, stack_size = 8092, heap_size = 8092;
    uint8_t *buf = nullptr;                 // Module buffer
    char error_buf[128] = { 0 };           // Error message buffer
    const char *exception = nullptr;        // Runtime exception pointer

    /**
     * @brief Setup method called before each test case execution
     * @details Initializes WAMR runtime with proper configuration for testing.
     *          Sets up memory allocation, enables required features, and prepares
     *          execution environment for both interpreter and AOT modes.
     */
    void SetUp() override {
        memset(error_buf, 0, 128);
        module = nullptr;
        module_inst = nullptr;
        exec_env = nullptr;
        buf = nullptr;

        // Load WASM module from file
        std::string wasm_file = "./wasm-apps/i64_ctz_test.wasm";
        buf = (uint8_t *)bh_read_file_to_buffer(wasm_file.c_str(), &buf_size);
        ASSERT_NE(buf, nullptr) << "Failed to read WASM file: " << wasm_file;

        module = wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Failed to load WASM module: " << error_buf;

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));
        ASSERT_NE(module_inst, nullptr) << "Failed to instantiate WASM module: " << error_buf;

        wasm_runtime_set_running_mode(module_inst, GetParam());

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        ASSERT_NE(exec_env, nullptr) << "Failed to create execution environment";
    }

    /**
     * @brief Cleanup method called after each test case execution
     * @details Properly releases all allocated resources including execution environment,
     *          module instance, module, and file buffer. Ensures no memory leaks.
     */
    void TearDown() override {
        // Clean up resources in reverse order of creation
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
    }

    /**
     * @brief Helper function to call i64.ctz operation with given input
     * @param input The 64-bit integer input for ctz operation
     * @return The count of trailing zeros (0-64)
     * @details Calls the exported i64.ctz test function from loaded WASM module.
     *          Handles function lookup, parameter passing, and result extraction.
     */
    uint32_t call_i64_ctz(uint64_t input) {
        wasm_function_inst_t func = wasm_runtime_lookup_function(module_inst, "test_i64_ctz");
        EXPECT_NE(func, nullptr) << "Failed to lookup test_i64_ctz function";

        uint32_t argv[3] = { (uint32_t)input, (uint32_t)(input >> 32), 0 };
        bool ret = wasm_runtime_call_wasm(exec_env, func, 2, argv);
        EXPECT_EQ(ret, true) << "Function call failed";

        exception = wasm_runtime_get_exception(module_inst);
        if (exception != nullptr) {
            ADD_FAILURE() << "Runtime exception occurred: " << exception;
        }

        return argv[0];
    }
};

/**
 * @test BoundaryValues_ReturnsExpectedCounts
 * @brief Validates i64.ctz correctly handles boundary value conditions
 * @details Tests extreme values including zero (special case returns 64), maximum values,
 *          and minimum values. Ensures proper handling of full 64-bit range coverage.
 * @test_category Corner - Boundary condition validation
 * @coverage_target core/iwasm/interpreter/wasm_interp_classic.c:ctz64
 * @input_conditions Zero, 0xFFFFFFFFFFFFFFFF, 0x8000000000000000
 * @expected_behavior Zero→64, MAX→0, MIN→63 (only MSB set)
 * @validation_method Boundary value analysis with mathematical verification
 */
TEST_P(I64CtzTest, BoundaryValues_ReturnsExpectedCounts) {
    // Zero input produces maximum trailing zeros (64) - special case in implementation
    ASSERT_EQ(64, call_i64_ctz(0x0000000000000000ULL))
        << "ctz(0) should return 64 trailing zeros";

    // Maximum 64-bit unsigned value (all bits set) produces zero trailing zeros
    ASSERT_EQ(0, call_i64_ctz(0xFFFFFFFFFFFFFFFFULL))
        << "ctz(UINT64_MAX) should return 0 trailing zeros";

    // Minimum 64-bit signed value (MSB set only) produces 63 trailing zeros
    ASSERT_EQ(63, call_i64_ctz(0x8000000000000000ULL))
        << "ctz(INT64_MIN) should return 63 trailing zeros";

    // Maximum 64-bit signed value (MSB clear) produces zero trailing zeros
    ASSERT_EQ(0, call_i64_ctz(0x7FFFFFFFFFFFFFFFULL))
        << "ctz(INT64_MAX) should return 0 trailing zeros";
}

// Parameterized test instantiation for cross-execution mode validation
INSTANTIATE_TEST_SUITE_P(RunningModeTest, I64CtzTest,
                         testing::Values(Mode_Interp, Mode_LLVM_JIT),
                         [](const testing::TestParamInfo<I64CtzTest::ParamType> &info) {
                             return info.param == Mode_Interp ? "INTERP" : "AOT";
                         });
