/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <csignal>

extern "C" {
#include "bh_assert.h"
#include "bh_platform.h"
#include "bh_bitmap.h"
#include "bh_read_file.h"
#include "wasm_export.h"
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

// Test fixture for Assertion and File I/O Functions
class AssertionFileIOTest : public testing::Test {
protected:
    void SetUp() override {
        // Initialize WAMR runtime for memory allocation
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        bool init_success = wasm_runtime_full_init(&init_args);
        ASSERT_TRUE(init_success);
        
        // Create temporary test files
        test_file_path = "/tmp/wamr_test_file.txt";
        test_content = "Hello WAMR Test Content\nLine 2\nLine 3";
        
        // Create test file
        CreateTestFile(test_file_path, test_content);
        
        // Set up bitmap test data
        test_bitmap = nullptr;
    }
    
    void TearDown() override {
        // Clean up test files
        if (access(test_file_path.c_str(), F_OK) == 0) {
            unlink(test_file_path.c_str());
        }
        
        // Clean up bitmap
        if (test_bitmap != nullptr) {
            bh_bitmap_delete(test_bitmap);
            test_bitmap = nullptr;
        }
        
        // Destroy WAMR runtime
        wasm_runtime_destroy();
    }
    
    void CreateTestFile(const std::string& path, const std::string& content) {
        int fd = open(path.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd >= 0) {
            write(fd, content.c_str(), content.length());
            close(fd);
        }
    }
    
    std::string test_file_path;
    std::string test_content;
    bh_bitmap* test_bitmap;
};

TEST_F(AssertionFileIOTest, BhReadFile_WithNullFilename_ReturnsNull) {
    // Test error path: NULL filename
    uint32 file_size = 0;
    char* buffer = bh_read_file_to_buffer(nullptr, &file_size);
    
    ASSERT_TRUE(buffer == nullptr);
}

TEST_F(AssertionFileIOTest, BhBitmap_Operations_WithOffsetRange_WorkCorrectly) {
    // Test bitmap operations with non-zero begin_index
    test_bitmap = bh_bitmap_new(100, 32);
    ASSERT_TRUE(test_bitmap != nullptr);
    
    // Test range checking
    ASSERT_TRUE(bh_bitmap_is_in_range(test_bitmap, 100));
    ASSERT_TRUE(bh_bitmap_is_in_range(test_bitmap, 131));
    ASSERT_FALSE(bh_bitmap_is_in_range(test_bitmap, 99));
    ASSERT_FALSE(bh_bitmap_is_in_range(test_bitmap, 132));
    
    // Test bit operations with offset
    bh_bitmap_set_bit(test_bitmap, 105);
    bh_bitmap_set_bit(test_bitmap, 120);
    
    ASSERT_EQ(1, bh_bitmap_get_bit(test_bitmap, 105));
    ASSERT_EQ(1, bh_bitmap_get_bit(test_bitmap, 120));
    ASSERT_EQ(0, bh_bitmap_get_bit(test_bitmap, 100));
    ASSERT_EQ(0, bh_bitmap_get_bit(test_bitmap, 131));
    
    // Clear a bit
    bh_bitmap_clear_bit(test_bitmap, 105);
    ASSERT_EQ(0, bh_bitmap_get_bit(test_bitmap, 105));
    ASSERT_EQ(1, bh_bitmap_get_bit(test_bitmap, 120));
}

