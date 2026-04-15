/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * WASI Coverage Test
 *
 * Exercises libc_wasi_wrapper.c functions by loading and running WASM modules
 * that import and call WASI preview1 APIs (fd_write, fd_read, path_open,
 * args_get, environ_get, clock_time_get, poll_oneoff, etc.)
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include "wasm_export.h"
#include "bh_read_file.h"

static const char *WASI_FD_OPS = "wasm-apps/wasi_fd_ops.wasm";
static const char *WASI_ENV_ARGS = "wasm-apps/wasi_env_args.wasm";
static const char *WASI_PATH_OPS = "wasm-apps/wasi_path_ops.wasm";
static const char *WASI_POLL = "wasm-apps/wasi_poll.wasm";

class WasiCoverageTest : public ::testing::Test
{
  protected:
    static constexpr uint32_t STACK_SIZE = 32768;
    static constexpr uint32_t HEAP_SIZE = 32768;
    char temp_dir[256] = { 0 };
    bool runtime_inited = false;

    void SetUp() override
    {
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(init_args));
        init_args.mem_alloc_type = Alloc_With_Allocator;
        init_args.mem_alloc_option.allocator.malloc_func = (void *)malloc;
        init_args.mem_alloc_option.allocator.realloc_func = (void *)realloc;
        init_args.mem_alloc_option.allocator.free_func = (void *)free;

        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
        runtime_inited = true;

        (void)snprintf(temp_dir, sizeof(temp_dir), "/tmp/wasi_test_XXXXXX");
        ASSERT_NE(mkdtemp(temp_dir), nullptr);
    }

    void TearDown() override
    {
        if (temp_dir[0] != '\0') {
            char cmd[512];
            int n = snprintf(cmd, sizeof(cmd), "rm -rf %s", temp_dir);
            if (n > 0 && static_cast<size_t>(n) < sizeof(cmd)) {
                (void)system(cmd); // NOLINT(cert-env33-c,concurrency-mt-unsafe)
            }
        }
        if (runtime_inited)
            wasm_runtime_destroy();
    }

    void run_wasi_module(const char *wasm_file, const char *func_name,
                         uint32_t *argv = nullptr, uint32_t argc = 0)
    {
        uint32_t buf_size = 0;
        auto *buf =
            (uint8_t *)bh_read_file_to_buffer(wasm_file, &buf_size);
        ASSERT_NE(buf, nullptr) << "Cannot read " << wasm_file;

        char error_buf[256] = { 0 };
        wasm_module_t module =
            wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Load failed: " << error_buf;

        const char *dir_list[] = { temp_dir, "." };
        const char *env_list[] = { "TEST_VAR=hello", "PATH=/usr/bin" };
        char *arg_list[] = { (char *)"test_prog", (char *)"--verbose" };
        wasm_runtime_set_wasi_args(module, dir_list, 2, nullptr, 0,
                                   env_list, 2, arg_list, 2);

        wasm_module_inst_t inst = wasm_runtime_instantiate(
            module, STACK_SIZE, HEAP_SIZE, error_buf, sizeof(error_buf));
        ASSERT_NE(inst, nullptr) << "Instantiate failed: " << error_buf;

        wasm_exec_env_t env =
            wasm_runtime_create_exec_env(inst, STACK_SIZE);
        ASSERT_NE(env, nullptr);

        wasm_function_inst_t func =
            wasm_runtime_lookup_function(inst, func_name);
        ASSERT_NE(func, nullptr) << "Function not found: " << func_name;

        uint32_t default_argv[4] = { 0 };
        if (argv == nullptr) {
            argv = default_argv;
            argc = 0;
        }
        bool ret = wasm_runtime_call_wasm(env, func, argc, argv);
        const char *exception = wasm_runtime_get_exception(inst);
        wasm_runtime_clear_exception(inst);
        ASSERT_TRUE(ret) << "Call failed: "
                         << (exception != nullptr ? exception : "unknown");

        wasm_runtime_destroy_exec_env(env);
        wasm_runtime_deinstantiate(inst);
        wasm_runtime_unload(module);
        BH_FREE(buf);
    }

    void run_all_exports(const char *wasm_file)
    {
        uint32_t buf_size = 0;
        auto *buf =
            (uint8_t *)bh_read_file_to_buffer(wasm_file, &buf_size);
        ASSERT_NE(buf, nullptr) << "Cannot read " << wasm_file;

        char error_buf[256] = { 0 };
        wasm_module_t module =
            wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
        ASSERT_NE(module, nullptr) << "Load failed: " << error_buf;

        const char *dir_list2[] = { temp_dir, "." };
        const char *env_list2[] = { "TEST_VAR=hello", "PATH=/usr/bin" };
        char *arg_list2[] = { (char *)"test_prog", (char *)"--verbose" };
        wasm_runtime_set_wasi_args(module, dir_list2, 2, nullptr, 0,
                                   env_list2, 2, arg_list2, 2);

        wasm_module_inst_t inst = wasm_runtime_instantiate(
            module, STACK_SIZE, HEAP_SIZE, error_buf, sizeof(error_buf));
        ASSERT_NE(inst, nullptr) << "Instantiate failed: " << error_buf;

        wasm_exec_env_t env =
            wasm_runtime_create_exec_env(inst, STACK_SIZE);
        ASSERT_NE(env, nullptr);

        int32_t export_count = wasm_runtime_get_export_count(module);
        for (int32_t i = 0; i < export_count; i++) {
            wasm_export_t exp_info;
            wasm_runtime_get_export_type(module, i, &exp_info);

            if (exp_info.kind != WASM_IMPORT_EXPORT_KIND_FUNC)
                continue;
            if (strcmp(exp_info.name, "memory") == 0)
                continue;

            wasm_function_inst_t func =
                wasm_runtime_lookup_function(inst, exp_info.name);
            if (func == nullptr)
                continue;

            uint32_t argv[4] = { 0 };
            bool ret = wasm_runtime_call_wasm(env, func, 0, argv);
            const char *exception = wasm_runtime_get_exception(inst);
            wasm_runtime_clear_exception(inst);
            ASSERT_TRUE(ret)
                << "Call to " << exp_info.name << " failed: "
                << (exception != nullptr ? exception : "unknown");
        }

        wasm_runtime_destroy_exec_env(env);
        wasm_runtime_deinstantiate(inst);
        wasm_runtime_unload(module);
        BH_FREE(buf);
    }
};

TEST_F(WasiCoverageTest, FdWriteStdout)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_write_stdout");
}

TEST_F(WasiCoverageTest, FdWriteStderr)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_write_stderr");
}

TEST_F(WasiCoverageTest, FdFdstatGet)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_fdstat_get");
}

TEST_F(WasiCoverageTest, FdPrestatGet)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_prestat_get");
}

TEST_F(WasiCoverageTest, FdPrestatDirName)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_prestat_dir_name");
}

TEST_F(WasiCoverageTest, FdFilestatGet)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_filestat_get");
}

TEST_F(WasiCoverageTest, FdSync)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_sync");
}

TEST_F(WasiCoverageTest, FdDatasync)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_datasync");
}

TEST_F(WasiCoverageTest, FdFdstatSetFlags)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_fdstat_set_flags");
}

TEST_F(WasiCoverageTest, FdSeek)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_seek");
}

TEST_F(WasiCoverageTest, FdTell)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_tell");
}

TEST_F(WasiCoverageTest, FdAdvise)
{
    run_wasi_module(WASI_FD_OPS, "test_fd_advise");
}

TEST_F(WasiCoverageTest, AllFdOps)
{
    run_all_exports(WASI_FD_OPS);
}

TEST_F(WasiCoverageTest, ArgsSizesGet)
{
    run_wasi_module(WASI_ENV_ARGS, "test_args_sizes_get");
}

TEST_F(WasiCoverageTest, ArgsGet)
{
    run_wasi_module(WASI_ENV_ARGS, "test_args_get");
}

TEST_F(WasiCoverageTest, EnvironSizesGet)
{
    run_wasi_module(WASI_ENV_ARGS, "test_environ_sizes_get");
}

TEST_F(WasiCoverageTest, EnvironGet)
{
    run_wasi_module(WASI_ENV_ARGS, "test_environ_get");
}

TEST_F(WasiCoverageTest, ClockResGet)
{
    run_wasi_module(WASI_ENV_ARGS, "test_clock_res_get_realtime");
}

TEST_F(WasiCoverageTest, ClockTimeGet)
{
    run_wasi_module(WASI_ENV_ARGS, "test_clock_time_get_realtime");
}

TEST_F(WasiCoverageTest, RandomGet)
{
    run_wasi_module(WASI_ENV_ARGS, "test_random_get");
}

TEST_F(WasiCoverageTest, SchedYield)
{
    run_wasi_module(WASI_ENV_ARGS, "test_sched_yield");
}

TEST_F(WasiCoverageTest, AllEnvArgs)
{
    run_all_exports(WASI_ENV_ARGS);
}

TEST_F(WasiCoverageTest, PathCreateDirectory)
{
    run_wasi_module(WASI_PATH_OPS, "test_path_create_directory");
}

TEST_F(WasiCoverageTest, PathOpenCreate)
{
    run_wasi_module(WASI_PATH_OPS, "test_path_open_create");
}

TEST_F(WasiCoverageTest, FileWriteRead)
{
    run_wasi_module(WASI_PATH_OPS, "test_file_write_read");
}

TEST_F(WasiCoverageTest, PathFilestatGet)
{
    run_wasi_module(WASI_PATH_OPS, "test_path_filestat_get");
}

TEST_F(WasiCoverageTest, FdReaddir)
{
    run_wasi_module(WASI_PATH_OPS, "test_fd_readdir");
}

TEST_F(WasiCoverageTest, PwritePread)
{
    run_wasi_module(WASI_PATH_OPS, "test_pwrite_pread");
}

TEST_F(WasiCoverageTest, PathUnlink)
{
    run_wasi_module(WASI_PATH_OPS, "test_path_unlink");
}

TEST_F(WasiCoverageTest, PathRename)
{
    run_wasi_module(WASI_PATH_OPS, "test_path_rename");
}

TEST_F(WasiCoverageTest, AllPathOps)
{
    run_all_exports(WASI_PATH_OPS);
}

TEST_F(WasiCoverageTest, PollClock)
{
    run_wasi_module(WASI_POLL, "test_poll_clock");
}

TEST_F(WasiCoverageTest, PollFdRead)
{
    run_wasi_module(WASI_POLL, "test_poll_fd_read");
}

TEST_F(WasiCoverageTest, AllPoll)
{
    run_all_exports(WASI_POLL);
}
