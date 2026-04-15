/*
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

/*
 * Fast Interpreter Coverage Test
 *
 * Loads and executes WASM modules through the fast interpreter to exercise
 * wasm_interp_fast.c opcode handlers. Each module runs in a fully isolated
 * runtime instance to prevent heap corruption across modules.
 */

#include <gtest/gtest.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "test_helper.h"
#include "wasm_runtime_common.h"
#include "bh_read_file.h"

static std::vector<std::string>
find_wasm_files(const std::string &dir)
{
    std::vector<std::string> result;
    DIR *d = opendir(dir.c_str());
    if (d == nullptr)
        return result;
    struct dirent *entry;
    while ((entry = readdir(d)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;
        std::string full = dir;
        full += "/";
        full += name;
        struct stat st;
        if (stat(full.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            auto sub = find_wasm_files(full);
            result.insert(result.end(), sub.begin(), sub.end());
        }
        else if (name.size() > 5
                 && name.substr(name.size() - 5) == ".wasm") {
            if (name.find("invalid") == std::string::npos
                && name.find("error") == std::string::npos
                && name.find("malformed") == std::string::npos
                && name.find("underflow") == std::string::npos) {
                result.push_back(full);
            }
        }
    }
    closedir(d);
    return result;
}

static void
call_exported_functions(wasm_module_t module, wasm_module_inst_t inst,
                        wasm_exec_env_t env)
{
    int32_t count = wasm_runtime_get_export_count(module);
    if (count <= 0)
        return;

    for (int32_t i = 0; i < count; i++) {
        wasm_export_t exp;
        wasm_runtime_get_export_type(module, i, &exp);

        if (exp.kind != WASM_IMPORT_EXPORT_KIND_FUNC)
            continue;

        wasm_function_inst_t func =
            wasm_runtime_lookup_function(inst, exp.name);
        if (func == nullptr)
            continue;

        wasm_func_type_t type = exp.u.func_type;
        if (type == nullptr)
            continue;

        uint32_t param_count = wasm_func_type_get_param_count(type);
        uint32_t result_count = wasm_func_type_get_result_count(type);

        uint32_t param_cells = 0;
        for (uint32_t p = 0; p < param_count; p++) {
            wasm_valkind_t kind = wasm_func_type_get_param_valkind(type, p);
            param_cells += (kind == WASM_I64 || kind == WASM_F64) ? 2 : 1;
        }
        uint32_t result_cells = 0;
        for (uint32_t r = 0; r < result_count; r++) {
            wasm_valkind_t kind = wasm_func_type_get_result_valkind(type, r);
            result_cells += (kind == WASM_I64 || kind == WASM_F64) ? 2 : 1;
        }

        uint32_t argv_size =
            (param_cells > result_cells ? param_cells : result_cells) + 4;
        std::vector<uint32_t> argv(argv_size, 0);

        uint32_t offset = 0;
        for (uint32_t p = 0; p < param_count; p++) {
            wasm_valkind_t kind = wasm_func_type_get_param_valkind(type, p);
            switch (kind) {
                case WASM_I32:
                    argv[offset++] = 1;
                    break;
                case WASM_I64:
                    argv[offset++] = 1;
                    argv[offset++] = 0;
                    break;
                case WASM_F32:
                {
                    float v = 1.0f;
                    memcpy(&argv[offset], &v, sizeof(float));
                    offset++;
                    break;
                }
                case WASM_F64:
                {
                    double v = 1.0;
                    memcpy(&argv[offset], &v, sizeof(double));
                    offset += 2;
                    break;
                }
                default:
                    argv[offset++] = 0;
                    break;
            }
        }

        wasm_runtime_call_wasm(env, func, offset, argv.data());
        wasm_runtime_clear_exception(inst);
    }
}

static constexpr uint32_t STACK_SIZE = 16384;
static constexpr uint32_t HEAP_SIZE = 16384;

static void
run_wasm_in_isolation(const std::string &path)
{
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(init_args));
    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = (void *)malloc;
    init_args.mem_alloc_option.allocator.realloc_func = (void *)realloc;
    init_args.mem_alloc_option.allocator.free_func = (void *)free;

    if (!wasm_runtime_full_init(&init_args))
        return;

    uint32_t buf_size = 0;
    auto *buf =
        (uint8_t *)bh_read_file_to_buffer(path.c_str(), &buf_size);
    if (buf == nullptr) {
        wasm_runtime_destroy();
        return;
    }

    char error_buf[256] = { 0 };
    wasm_module_t module =
        wasm_runtime_load(buf, buf_size, error_buf, sizeof(error_buf));
    if (module == nullptr) {
        BH_FREE(buf);
        wasm_runtime_destroy();
        return;
    }

    wasm_module_inst_t inst = wasm_runtime_instantiate(
        module, STACK_SIZE, HEAP_SIZE, error_buf, sizeof(error_buf));
    if (inst == nullptr) {
        wasm_runtime_unload(module);
        BH_FREE(buf);
        wasm_runtime_destroy();
        return;
    }

    wasm_runtime_set_running_mode(inst, Mode_Interp);

    wasm_exec_env_t env = wasm_runtime_create_exec_env(inst, STACK_SIZE);
    if (env != nullptr) {
        call_exported_functions(module, inst, env);
        wasm_runtime_destroy_exec_env(env);
    }

    wasm_runtime_deinstantiate(inst);
    wasm_runtime_unload(module);
    BH_FREE(buf);
    wasm_runtime_destroy();
}

TEST(FastInterpCoverage, NumericOpcodes_ExecuteWithoutCrash)
{
    auto files = find_wasm_files("wasm-apps/numeric");
    ASSERT_GT(files.size(), 0u) << "No numeric WASM files found";
    for (const auto &f : files) {
        run_wasm_in_isolation(f);
    }
}

TEST(FastInterpCoverage, MemoryOpcodes_ExecuteWithoutCrash)
{
    auto files = find_wasm_files("wasm-apps/memory");
    ASSERT_GT(files.size(), 0u) << "No memory WASM files found";
    for (const auto &f : files) {
        run_wasm_in_isolation(f);
    }
}

TEST(FastInterpCoverage, ControlFlowOpcodes_ExecuteWithoutCrash)
{
    std::vector<std::string> files = {
        "wasm-apps/control-flow/select_test.wasm",
    };
    ASSERT_GT(files.size(), 0u) << "No control-flow WASM files found";
    for (const auto &f : files) {
        run_wasm_in_isolation(f);
    }
}

TEST(FastInterpCoverage, VariableOpcodes_ExecuteWithoutCrash)
{
    auto files = find_wasm_files("wasm-apps/variable");
    ASSERT_GT(files.size(), 0u) << "No variable WASM files found";
    for (const auto &f : files) {
        run_wasm_in_isolation(f);
    }
}

TEST(FastInterpCoverage, TableOpcodes_ExecuteWithoutCrash)
{
    auto files = find_wasm_files("wasm-apps/table");
    ASSERT_GT(files.size(), 0u) << "No table WASM files found";
    for (const auto &f : files) {
        run_wasm_in_isolation(f);
    }
}

TEST(FastInterpCoverage, ConstantOpcodes_ExecuteWithoutCrash)
{
    auto files = find_wasm_files("wasm-apps/constants");
    ASSERT_GT(files.size(), 0u) << "No constant WASM files found";
    for (const auto &f : files) {
        run_wasm_in_isolation(f);
    }
}
