/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_api_vmcore.h"
#include <unistd.h>

class PosixTimeTest : public testing::Test
{
  protected:
    virtual void SetUp() {}
    virtual void TearDown() {}

  public:
    WAMRRuntimeRAII<512 * 1024> runtime;
};

TEST_F(PosixTimeTest, GetThreadCpuTime)
{
    // Test os_time_thread_cputime_us() function
    uint64 cpu_time1 = os_time_thread_cputime_us();

    // Perform some CPU work to advance CPU time
    volatile int sum = 0;
    for (int i = 0; i < 10000; i++) {
        sum += i * i;
    }

    uint64 cpu_time2 = os_time_thread_cputime_us();

    // CPU time should be non-negative
    EXPECT_GE(cpu_time1, 0);
    EXPECT_GE(cpu_time2, 0);

    // CPU time should generally increase or stay same (may be 0 if not
    // supported)
    if (cpu_time1 > 0 && cpu_time2 > 0) {
        EXPECT_GE(cpu_time2, cpu_time1);
    }
}
