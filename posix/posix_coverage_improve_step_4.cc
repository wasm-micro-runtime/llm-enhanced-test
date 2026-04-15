/*
 * Copyright (C) 2024 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "gtest/gtest.h"
#include "test_helper.h"
#include "wasm_export.h"
#include "bh_platform.h"
#include "platform_api_extension.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>


class PosixSocketConfigTest : public testing::Test {
protected:
    void SetUp() override {
        // Initialize WAMR runtime for socket operations
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
        
        // Create test sockets for configuration operations
        test_tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
        test_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        
        ASSERT_NE(-1, test_tcp_socket) << "Failed to create TCP test socket";
        ASSERT_NE(-1, test_udp_socket) << "Failed to create UDP test socket";
    }
    
    void TearDown() override {
        // Clean up test sockets
        if (test_tcp_socket != -1) {
            close(test_tcp_socket);
        }
        if (test_udp_socket != -1) {
            close(test_udp_socket);
        }
        
        wasm_runtime_destroy();
    }
    
    bh_socket_t test_tcp_socket = -1;
    bh_socket_t test_udp_socket = -1;
};

TEST_F(PosixSocketConfigTest, os_socket_set_recv_buf_size_ValidSize_SetsCorrectly) {
    size_t new_size = 8192;
    size_t retrieved_size = 0;
    
    int set_result = os_socket_set_recv_buf_size(test_tcp_socket, new_size);
    ASSERT_EQ(BHT_OK, set_result) << "Should successfully set receive buffer size";
    
    int get_result = os_socket_get_recv_buf_size(test_tcp_socket, &retrieved_size);
    ASSERT_EQ(BHT_OK, get_result) << "Should successfully get receive buffer size";
    ASSERT_GE(retrieved_size, new_size) << "Retrieved size should be at least the set size";
}

// Error Handling Tests

TEST_F(PosixSocketConfigTest, os_socket_get_recv_buf_size_InvalidSocket_ReturnsError) {
    size_t buffer_size = 0;
    bh_socket_t invalid_socket = -1;
    
    int result = os_socket_get_recv_buf_size(invalid_socket, &buffer_size);
    
    ASSERT_EQ(BHT_ERROR, result) << "Should return error for invalid socket";
}

TEST_F(PosixSocketConfigTest, os_socket_get_recv_timeout_InvalidSocket_ReturnsError) {
    uint64 timeout_us = 0;
    bh_socket_t invalid_socket = -1;
    
    int result = os_socket_get_recv_timeout(invalid_socket, &timeout_us);
    
    ASSERT_EQ(BHT_ERROR, result) << "Should return error for invalid socket";
}
