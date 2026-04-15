/*
 * Copyright (C) 2024 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "platform_api_extension.h"
#include "wasm_export.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>

/**
 * POSIX Coverage Improvement Step 2: Directory Operations
 * 
 * Target Functions (8 functions, ~175 lines):
 * 1. os_mkdirat() - Directory creation
 * 2. os_readdir() - Reading directory entries
 * 3. os_rewinddir() - Directory stream reset
 * 4. os_seekdir() - Directory stream positioning
 * 5. os_open_preopendir() - Preopen directory opening
 * 6. os_linkat() - Hard link creation
 * 7. os_unlinkat() - File/directory removal
 * 8. os_renameat() - File/directory renaming
 */

class PosixDirectoryOperationsTest : public testing::Test {
protected:
    void SetUp() override {
        // Initialize WAMR runtime for POSIX API testing
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
        
        // Create test directory structure
        test_dir_base = "/tmp/wamr_posix_dir_test";
        cleanup_test_directories();
        create_test_directories();
    }
    
    void TearDown() override {
        cleanup_test_directories();
        wasm_runtime_destroy();
    }
    
    void create_test_directories() {
        mkdir(test_dir_base.c_str(), 0755);
        mkdir((test_dir_base + "/subdir1").c_str(), 0755);
        mkdir((test_dir_base + "/subdir2").c_str(), 0755);
        
        // Create test files
        int fd1 = open((test_dir_base + "/test_file1.txt").c_str(), O_CREAT | O_WRONLY, 0644);
        if (fd1 >= 0) {
            write(fd1, "test content 1", 14);
            close(fd1);
        }
        
        int fd2 = open((test_dir_base + "/test_file2.txt").c_str(), O_CREAT | O_WRONLY, 0644);
        if (fd2 >= 0) {
            write(fd2, "test content 2", 14);
            close(fd2);
        }
    }
    
    static void remove_dir_recursive(const std::string &path) {
        DIR *d = opendir(path.c_str());
        if (!d)
            return;
        struct dirent *entry;
        while ((entry = readdir(d)) != nullptr) {
            if (strcmp(entry->d_name, ".") == 0
                || strcmp(entry->d_name, "..") == 0)
                continue;
            std::string child = path + "/" + entry->d_name;
            struct stat st;
            if (lstat(child.c_str(), &st) == 0) {
                if (S_ISDIR(st.st_mode))
                    remove_dir_recursive(child);
                else
                    unlink(child.c_str());
            }
        }
        closedir(d);
        rmdir(path.c_str());
    }

    void cleanup_test_directories() {
        remove_dir_recursive(test_dir_base);
    }
    
    std::string test_dir_base;
};

// Test 1: os_mkdirat() - Directory creation
TEST_F(PosixDirectoryOperationsTest, os_mkdirat_create_directory) {
    os_file_handle base_handle;
    __wasi_errno_t result = os_open_preopendir(test_dir_base.c_str(), &base_handle);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    
    // Test successful directory creation
    result = os_mkdirat(base_handle, "new_directory");
    ASSERT_EQ(__WASI_ESUCCESS, result);
    
    // Verify directory was created
    struct stat st;
    std::string new_dir_path = test_dir_base + "/new_directory";
    ASSERT_EQ(0, stat(new_dir_path.c_str(), &st));
    ASSERT_TRUE(S_ISDIR(st.st_mode));
    
    os_close(base_handle, false);
}

TEST_F(PosixDirectoryOperationsTest, os_mkdirat_invalid_path) {
    os_file_handle base_handle;
    __wasi_errno_t result = os_open_preopendir(test_dir_base.c_str(), &base_handle);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    
    // Test directory creation with invalid path (already exists)
    result = os_mkdirat(base_handle, "subdir1");
    ASSERT_NE(__WASI_ESUCCESS, result);
    
    // Test directory creation with invalid nested path
    result = os_mkdirat(base_handle, "nonexistent/nested/path");
    ASSERT_NE(__WASI_ESUCCESS, result);
    
    os_close(base_handle, false);
}

// Test 3: os_rewinddir() - Directory stream reset
TEST_F(PosixDirectoryOperationsTest, os_rewinddir_reset) {
    os_file_handle dir_handle;
    __wasi_errno_t result = os_open_preopendir(test_dir_base.c_str(), &dir_handle);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    
    os_dir_stream dir_stream;
    result = os_fdopendir(dir_handle, &dir_stream);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    
    // Read first entry
    __wasi_dirent_t entry1, entry2;
    const char *first_name = nullptr, *second_name = nullptr;
    
    result = os_readdir(dir_stream, &entry1, &first_name);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    ASSERT_NE(nullptr, first_name);

    // Read second entry
    result = os_readdir(dir_stream, &entry2, &second_name);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Rewind directory stream
    result = os_rewinddir(dir_stream);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Read first entry again after rewind
    const char *rewound_name = nullptr;
    __wasi_dirent_t rewound_entry;
    result = os_readdir(dir_stream, &rewound_entry, &rewound_name);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    ASSERT_NE(nullptr, rewound_name);

    // Verify we're back at the beginning
    ASSERT_STREQ(first_name, rewound_name);
    
    os_closedir(dir_stream);
}

TEST_F(PosixDirectoryOperationsTest, os_linkat_invalid_path) {
    os_file_handle base_handle;
    __wasi_errno_t result = os_open_preopendir(test_dir_base.c_str(), &base_handle);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    
    // Try to create link to nonexistent file
    result = os_linkat(base_handle, "nonexistent.txt", base_handle, "link_to_nothing.txt", 0);
    ASSERT_NE(__WASI_ESUCCESS, result);
    
    os_close(base_handle, false);
}

TEST_F(PosixDirectoryOperationsTest, os_unlinkat_remove_directory) {
    os_file_handle base_handle;
    __wasi_errno_t result = os_open_preopendir(test_dir_base.c_str(), &base_handle);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    
    // Create empty directory to remove
    std::string temp_dir = test_dir_base + "/temp_dir";
    ASSERT_EQ(0, mkdir(temp_dir.c_str(), 0755));
    
    // Verify directory exists
    struct stat st;
    ASSERT_EQ(0, stat(temp_dir.c_str(), &st));
    ASSERT_TRUE(S_ISDIR(st.st_mode));
    
    // Remove directory
    result = os_unlinkat(base_handle, "temp_dir", true);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    
    // Verify directory is gone
    ASSERT_NE(0, stat(temp_dir.c_str(), &st));
    
    os_close(base_handle, false);
}
