/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>

extern "C" {
#include "bh_common.h"
#include "bh_platform.h"
}

// Platform detection utility for tests - REQUIRED in every test file
class PlatformTestContext {
public:
    // Architecture detection
    static bool IsX86_64() {
#if defined(BUILD_TARGET_X86) || defined(BUILD_TARGET_X86_64)
        return true;
#else
        return false;
#endif
    }
    
    static bool IsARM64() {
#if defined(BUILD_TARGET_AARCH64) || defined(BUILD_TARGET_ARM64)
        return true;
#else
        return false;
#endif
    }
    
    static bool IsARM32() {
#if defined(BUILD_TARGET_ARM) && !defined(BUILD_TARGET_AARCH64)
        return true;
#else
        return false;
#endif
    }
    
    static bool IsRISCV() {
#if defined(BUILD_TARGET_RISCV) || defined(BUILD_TARGET_RISCV64) || defined(BUILD_TARGET_RISCV32)
        return true;
#else
        return false;
#endif
    }
    
    // Feature detection
    static bool HasSIMDSupport() {
#if WASM_ENABLE_SIMD != 0
        return true;
#else
        return false;
#endif
    }
    
    static bool HasAOTSupport() {
#if WASM_ENABLE_AOT != 0
        return true;
#else
        return false;
#endif
    }
    
    static bool HasJITSupport() {
#if WASM_ENABLE_JIT != 0
        return true;
#else
        return false;
#endif
    }
    
    static bool HasMemory64Support() {
#if WASM_ENABLE_MEMORY64 != 0
        return true;
#else
        return false;
#endif
    }
    
    // Platform capability helpers
    static bool Is64BitArchitecture() {
        return IsX86_64() || IsARM64() || (IsRISCV() && sizeof(void*) == 8);
    }
    
    static bool Is32BitArchitecture() {
        return IsARM32() || (IsRISCV() && sizeof(void*) == 4);
    }
};

// External function declarations for testing internal functions
// (declarations are already in bh_common.h, no redundant re-declaration needed)

// Helper function to replicate align_ptr logic for testing
// Since align_ptr is static, we replicate its functionality
static char *test_align_ptr(char *src, unsigned int b)
{
    auto v = reinterpret_cast<uintptr_t>(src);
    uintptr_t m = b - 1;
    return reinterpret_cast<char *>((v + m) & ~m);
}

// Test fixture for Memory Utility Functions
class MemoryUtilityTest : public testing::Test {
protected:
    void SetUp() override {
        // Set up test buffers
        memset(source_buffer, 0x55, sizeof(source_buffer));
        memset(dest_buffer, 0xAA, sizeof(dest_buffer));
        
        // Create test patterns
        for (int i = 0; i < TEST_BUFFER_SIZE; i++) {
            test_pattern[i] = (char)(i % 256);
        }
    }
    
    void TearDown() override {
        // Clean up any allocated memory
    }
    
    static const size_t TEST_BUFFER_SIZE = 1024;
    char source_buffer[TEST_BUFFER_SIZE];
    char dest_buffer[TEST_BUFFER_SIZE];
    char test_pattern[TEST_BUFFER_SIZE];
};

// ============================================================================
// Test Cases for b_memcpy_wa() function (Word-Aligned Memory Copy)
// Target: 30 uncovered lines in core/shared/utils/bh_common.c lines 18-47
// ============================================================================

TEST_F(MemoryUtilityTest, BMemcpyWA_WithZeroLength_ReturnsZero) {
    // Test zero-length copy
    int result = b_memcpy_wa(dest_buffer, sizeof(dest_buffer), source_buffer, 0);
    ASSERT_EQ(0, result);
    
    // Verify destination buffer unchanged
    for (size_t i = 0; i < 10; i++) {
        ASSERT_EQ((char)0xAA, dest_buffer[i]);
    }
}

TEST_F(MemoryUtilityTest, BMemcpyWA_WithComplexAlignment_HandlesAllPaths) {
    // Test scenario that exercises both leading and trailing word handling
    char complex_src[48];
    char complex_dest[48];
    
    // Fill with recognizable pattern
    for (int i = 0; i < 48; i++) {
        complex_src[i] = (char)(0x30 + (i % 16));
    }
    memset(complex_dest, 0, sizeof(complex_dest));
    
    // Test copy with complex alignment (starts at +2, copies 37 bytes)
    char *offset_src = complex_src + 2;
    int result = b_memcpy_wa(complex_dest, sizeof(complex_dest), offset_src, 37);
    ASSERT_EQ(0, result);
    
    // Verify complete data integrity
    ASSERT_EQ(0, memcmp(complex_dest, offset_src, 37));
}

// ============================================================================
// Test Cases for bh_system() function
// Target: 1 uncovered line in core/shared/utils/bh_common.c line 169
// Note: Only available when WASM_ENABLE_WAMR_COMPILER != 0 || WASM_ENABLE_JIT != 0
// ============================================================================

#if WASM_ENABLE_WAMR_COMPILER != 0 || WASM_ENABLE_JIT != 0

#else

#endif // WASM_ENABLE_WAMR_COMPILER != 0 || WASM_ENABLE_JIT != 0

