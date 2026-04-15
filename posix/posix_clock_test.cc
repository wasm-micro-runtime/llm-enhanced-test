/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_api_extension.h"

class PosixClockTest : public testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}

  public:
    WAMRRuntimeRAII<512 * 1024> runtime;
};

TEST_F(PosixClockTest, ClockResolutionGet_ValidClocks)
{
    __wasi_timestamp_t resolution;

    // Test MONOTONIC clock
    EXPECT_EQ(__WASI_ESUCCESS,
              os_clock_res_get(__WASI_CLOCK_MONOTONIC, &resolution));
    EXPECT_GT(resolution, 0);

    // Test REALTIME clock
    EXPECT_EQ(__WASI_ESUCCESS,
              os_clock_res_get(__WASI_CLOCK_REALTIME, &resolution));
    EXPECT_GT(resolution, 0);

    // Test PROCESS_CPUTIME_ID clock if supported
    __wasi_errno_t result =
        os_clock_res_get(__WASI_CLOCK_PROCESS_CPUTIME_ID, &resolution);
    if (result == __WASI_ESUCCESS) {
        EXPECT_GT(resolution, 0);
    }
    else {
        EXPECT_EQ(__WASI_ENOTSUP, result);
    }

    // Test THREAD_CPUTIME_ID clock if supported
    result = os_clock_res_get(__WASI_CLOCK_THREAD_CPUTIME_ID, &resolution);
    if (result == __WASI_ESUCCESS) {
        EXPECT_GT(resolution, 0);
    }
    else {
        EXPECT_EQ(__WASI_ENOTSUP, result);
    }
}

TEST_F(PosixClockTest, ClockTimeGet_InvalidClock)
{
    __wasi_timestamp_t time;

    // Test invalid clock ID
    EXPECT_EQ(__WASI_EINVAL,
              os_clock_time_get((__wasi_clockid_t)999, 0, &time));
}

TEST_F(PosixClockTest, ClockResolutionGet_InvalidClock)
{
    __wasi_timestamp_t resolution;

    // Test invalid clock ID
    EXPECT_EQ(__WASI_EINVAL,
              os_clock_res_get((__wasi_clockid_t)999, &resolution));
}
