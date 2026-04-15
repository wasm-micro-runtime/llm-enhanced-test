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
#include <string>
#include <cstring>

class PosixFileTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
        // Create temporary test directory
        test_dir = "/tmp/wamr_file_test";
        mkdir(test_dir.c_str(), 0755);

        test_file = test_dir + "/test_file.txt";
        test_fd = -1;
    }

    virtual void TearDown()
    {
        if (test_fd >= 0) {
            os_close(test_fd, false);
        }
        // Cleanup test files and directory
        unlink(test_file.c_str());
        rmdir(test_dir.c_str());
    }

  public:
    WAMRRuntimeRAII<512 * 1024> runtime;
    std::string test_dir;
    std::string test_file;
    os_file_handle test_fd;
};

TEST_F(PosixFileTest, FileManipulationOperations)
{
    // Create test file
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test os_fallocate (allocate 1KB)
    __wasi_filesize_t offset = 0;
    __wasi_filesize_t len = 1024;
    result = os_fallocate(test_fd, offset, len);
    // Accept success or ENOSYS (not supported on all filesystems)
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOSYS);

    // Test os_ftruncate
    __wasi_filesize_t size = 512;
    result = os_ftruncate(test_fd, size);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Verify truncation worked
    __wasi_filestat_t filestat;
    result = os_fstat(test_fd, &filestat);
    ASSERT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(size, filestat.st_size);
}

TEST_F(PosixFileTest, ErrorHandling)
{
    // Test operations on invalid file descriptor
    os_file_handle invalid_fd = 999999;

    __wasi_filestat_t filestat;
    __wasi_errno_t result = os_fstat(invalid_fd, &filestat);
    EXPECT_EQ(__WASI_EBADF, result);

    result = os_fsync(invalid_fd);
    EXPECT_EQ(__WASI_EBADF, result);

    result = os_ftruncate(invalid_fd, 100);
    EXPECT_EQ(__WASI_EBADF, result);

    // Test opening non-existent file without create flag
    os_file_handle bad_fd;
    result = os_openat(AT_FDCWD, "/non/existent/path", 0, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_ONLY, &bad_fd);
    EXPECT_EQ(__WASI_ENOENT, result);
}

TEST_F(PosixFileTest, FileSeekFilePositioning)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write test data
    const char *data = "ABCDEFGHIJ";
    __wasi_ciovec_t iov = { (const uint8_t *)data, strlen(data) };
    size_t nwritten;
    result = os_writev(test_fd, &iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test different seek operations
    __wasi_filesize_t new_offset;

    // Seek from beginning
    result = os_lseek(test_fd, 3, __WASI_WHENCE_SET, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(3, new_offset);

    // Seek from current position
    result = os_lseek(test_fd, 2, __WASI_WHENCE_CUR, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(5, new_offset);

    // Seek from end
    result = os_lseek(test_fd, -2, __WASI_WHENCE_END, &new_offset);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(data) - 2, new_offset);

    // Test invalid seek
    result = os_lseek(test_fd, -1000, __WASI_WHENCE_SET, &new_offset);
    EXPECT_NE(__WASI_ESUCCESS, result);
}

TEST_F(PosixFileTest, FileSyncAndFdatasyncOperations)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Write data to sync
    const char *data = "Data to be synchronized";
    __wasi_ciovec_t iov = { (const uint8_t *)data, strlen(data) };
    size_t nwritten;
    result = os_writev(test_fd, &iov, 1, &nwritten);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test fsync
    result = os_fsync(test_fd);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Test fdatasync (platform dependent)
    result = os_fdatasync(test_fd);
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOSYS);

    // Test sync on invalid fd
    result = os_fsync(999999);
    EXPECT_EQ(__WASI_EBADF, result);

    result = os_fdatasync(999999);
    EXPECT_TRUE(result == __WASI_EBADF || result == __WASI_ENOSYS);
}

TEST_F(PosixFileTest, FileIsattyTerminalDetection)
{
    // Test isatty on regular file (should fail)
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT, 0, 0,
                  WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    result = os_isatty(test_fd);
    EXPECT_NE(__WASI_ESUCCESS, result); // Regular file is not TTY

    // Test isatty on standard streams
    result = os_isatty(0); // stdin
    // Result depends on test environment
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOTTY);

    result = os_isatty(1); // stdout
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOTTY);

    result = os_isatty(2); // stderr
    EXPECT_TRUE(result == __WASI_ESUCCESS || result == __WASI_ENOTTY);
}

TEST_F(PosixFileTest, FileOpendirAndReaddirOperations)
{
    // Test directory operations properly
    __wasi_errno_t result;

    // First, open the test directory as a file descriptor
    os_file_handle dir_fd;
    result = os_openat(AT_FDCWD, test_dir.c_str(), 0, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_ONLY, &dir_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test fdopendir with valid directory fd
    os_dir_stream dir_stream;
    result = os_fdopendir(dir_fd, &dir_stream);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    if (result == __WASI_ESUCCESS) {
        // Test closedir
        result = os_closedir(dir_stream);
        EXPECT_EQ(__WASI_ESUCCESS, result);
    }

    // Close the directory fd
    os_close(dir_fd, false);

    // Test fdopendir on non-existent/invalid fd
    result = os_fdopendir(999999, &dir_stream);
    EXPECT_NE(__WASI_ESUCCESS, result); // Should fail with invalid fd
}

TEST_F(PosixFileTest, FileAdvancedIoErrorRecovery)
{
    __wasi_errno_t result =
        os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT | __WASI_O_TRUNC,
                  0, 0, WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    ASSERT_EQ(__WASI_ESUCCESS, result);

    // Test recovery from various error conditions

    // 1. Test operations on closed file
    os_file_handle closed_fd = test_fd;
    os_close(closed_fd, false);

    char buffer[100];
    size_t nread;
    __wasi_iovec_t read_iov = { (uint8_t *)buffer, 10 };
    result = os_preadv(closed_fd, &read_iov, 1, 0, &nread);
    EXPECT_NE(__WASI_ESUCCESS, result);

    size_t nwritten;
    __wasi_ciovec_t write_iov = { (const uint8_t *)"data", 4 };
    result = os_pwritev(closed_fd, &write_iov, 1, 0, &nwritten);
    EXPECT_NE(__WASI_ESUCCESS, result);

    // 2. Reopen and verify normal operation resumes
    result = os_openat(AT_FDCWD, test_file.c_str(), __WASI_O_CREAT, 0, 0,
                       WASI_LIBC_ACCESS_MODE_READ_WRITE, &test_fd);
    EXPECT_EQ(__WASI_ESUCCESS, result);

    // Normal operation should work
    const char *test_data = "Recovery test";
    write_iov.buf = (const uint8_t *)test_data;
    write_iov.buf_len = strlen(test_data);
    result = os_pwritev(test_fd, &write_iov, 1, 0, &nwritten);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(test_data), nwritten);

    // Verify data was written
    read_iov.buf_len = strlen(test_data);
    result = os_preadv(test_fd, &read_iov, 1, 0, &nread);
    EXPECT_EQ(__WASI_ESUCCESS, result);
    EXPECT_EQ(strlen(test_data), nread);
    EXPECT_EQ(0, strncmp(buffer, test_data, strlen(test_data)));
}
