/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_api_extension.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string>
#include <cstring>

// POLICY: All tests for functions in posix_file.c go in this file
// FILE-BASED GROUPING: All tests for functions in posix_file.c use this fixture
class EnhancedPosixFileTest : public testing::Test {
protected:
    void SetUp() override {
        // Setup runtime environment following existing patterns
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_Pool;
        init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
        init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

        ASSERT_TRUE(wasm_runtime_full_init(&init_args));

        // Create temporary test directory structure for directory testing
        test_dir = "/tmp/wamr_enhanced_posix_file_test";
        mkdir(test_dir.c_str(), 0755);

        // Create subdirectory for directory entry testing
        test_subdir = test_dir + "/subdir";
        mkdir(test_subdir.c_str(), 0755);

        // Create regular file for testing
        test_file = test_dir + "/regular_file.txt";
        int fd = open(test_file.c_str(), O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            write(fd, "test", 4);
            close(fd);
        }

        // Create symbolic link for testing
        test_symlink = test_dir + "/test_symlink";
        symlink(test_file.c_str(), test_symlink.c_str());

        dir_stream = nullptr;
    }

    void TearDown() override {
        if (dir_stream != nullptr) {
            closedir(dir_stream);
        }

        // Cleanup test files and directories
        unlink(test_symlink.c_str());
        unlink(test_file.c_str());
        rmdir(test_subdir.c_str());
        rmdir(test_dir.c_str());

        wasm_runtime_destroy();
    }

public:
    char global_heap_buf[512 * 1024];
    RuntimeInitArgs init_args;
    std::string test_dir;
    std::string test_subdir;
    std::string test_file;
    std::string test_symlink;
    DIR* dir_stream;
};

/******
 * Test Case: os_readdir_FifoEntry_MapsToWasiSocketStream
 * Source: core/shared/platform/common/posix/posix_file.c:975-977
 * Target Lines: 975 (case DT_FIFO), 976 (assignment), 977 (break)
 * Functional Purpose: Validates that os_readdir() correctly maps DT_FIFO entries
 *                     to __WASI_FILETYPE_SOCKET_STREAM (note: this is an intentional
 *                     mapping in WAMR for FIFO pipes).
 * Call Path: os_readdir() directly (public API)
 * Coverage Goal: Exercise DT_FIFO case in switch statement
 ******/
TEST_F(EnhancedPosixFileTest, os_readdir_FifoEntry_MapsToWasiSocketStream) {
    // Create a FIFO pipe in test directory
    std::string fifo_path = test_dir + "/test_fifo";
    int mkfifo_result = mkfifo(fifo_path.c_str(), 0644);
    ASSERT_EQ(0, mkfifo_result);

    // Open test directory for reading
    dir_stream = opendir(test_dir.c_str());
    ASSERT_NE(nullptr, dir_stream);

    __wasi_dirent_t entry;
    const char* d_name = nullptr;
    bool found_fifo = false;

    // Read directory entries until we find our FIFO
    __wasi_errno_t result;
    while ((result = os_readdir(dir_stream, &entry, &d_name)) == __WASI_ESUCCESS && d_name != nullptr) {
        if (strcmp(d_name, "test_fifo") == 0) {
            found_fifo = true;
            // Verify that FIFO entry is mapped to WASI socket stream type
            ASSERT_EQ(__WASI_FILETYPE_SOCKET_STREAM, entry.d_type);
            ASSERT_EQ(strlen("test_fifo"), entry.d_namlen);
            break;
        }
    }

    ASSERT_TRUE(found_fifo);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Cleanup FIFO
    unlink(fifo_path.c_str());
}
