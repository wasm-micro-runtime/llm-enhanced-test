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
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>


class PosixSocketCoreTest : public testing::Test {
protected:
    void SetUp() override {
        // Initialize WAMR runtime for socket operations
        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));
        init_args.mem_alloc_type = Alloc_With_System_Allocator;
        
        ASSERT_TRUE(wasm_runtime_full_init(&init_args));
        
        // Create test sockets for operations
        test_tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
        test_udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        
        // Set up test addresses
        memset(&test_addr_ipv4, 0, sizeof(test_addr_ipv4));
        test_addr_ipv4.sin_family = AF_INET;
        test_addr_ipv4.sin_addr.s_addr = inet_addr("127.0.0.1");
        test_addr_ipv4.sin_port = htons(12345);
        
        // Set up bh_sockaddr for testing
        memset(&bh_test_addr, 0, sizeof(bh_test_addr));
        bh_test_addr.is_ipv4 = true;
        bh_test_addr.port = 12345;
        bh_test_addr.addr_buffer.ipv4 = 0x7F000001; // 127.0.0.1 in network byte order
    }
    
    void TearDown() override {
        // Clean up test sockets
        if (test_tcp_socket >= 0) {
            close(test_tcp_socket);
        }
        if (test_udp_socket >= 0) {
            close(test_udp_socket);
        }
        if (server_socket >= 0) {
            close(server_socket);
        }
        
        wasm_runtime_destroy();
    }
    
    // Helper function to create a server socket for testing
    int create_test_server_socket(int port) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) return -1;
        
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock);
            return -1;
        }
        
        return sock;
    }
    
    int test_tcp_socket = -1;
    int test_udp_socket = -1;
    int server_socket = -1;
    struct sockaddr_in test_addr_ipv4;
    bh_sockaddr_t bh_test_addr;
};

// Test Function 1: os_socket_addr_resolve() - IPv6 address resolution  
TEST_F(PosixSocketCoreTest, SocketAddrResolve_IPv6Localhost_ResolvesCorrectly) {
    bh_addr_info_t addr_info[10];
    size_t max_info_size = 0;
    uint8_t hint_is_tcp = 1;
    uint8_t hint_is_ipv4 = 0;
    
    int result = os_socket_addr_resolve("::1", "443", &hint_is_tcp, &hint_is_ipv4,
                                       addr_info, 10, &max_info_size);
    
    // IPv6 may not be available on all systems
    if (result == BHT_OK) {
        ASSERT_GT(max_info_size, 0);
        ASSERT_FALSE(addr_info[0].sockaddr.is_ipv4);
        ASSERT_EQ(443, addr_info[0].sockaddr.port);
        ASSERT_TRUE(addr_info[0].is_tcp);
    }
}

// Test Function 8: os_socket_recv_from() - UDP loopback receiving with address population
TEST_F(PosixSocketCoreTest, SocketRecvFrom_UDPLoopback_ReceivesFromAddress) {
    // Create UDP receiver socket bound to loopback
    int recv_sock = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(recv_sock, 0);

    struct sockaddr_in recv_addr;
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    recv_addr.sin_port = htons(0); // let OS assign port

    ASSERT_EQ(0, bind(recv_sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)));

    // Get the assigned port
    socklen_t addr_len = sizeof(recv_addr);
    ASSERT_EQ(0, getsockname(recv_sock, (struct sockaddr *)&recv_addr, &addr_len));
    int assigned_port = ntohs(recv_addr.sin_port);

    // Create UDP sender socket
    int send_sock = socket(AF_INET, SOCK_DGRAM, 0);
    ASSERT_GE(send_sock, 0);

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest_addr.sin_port = htons(assigned_port);

    const char *test_data = "UDP Receive Test";
    ssize_t sent = sendto(send_sock, test_data, strlen(test_data), 0,
                          (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    ASSERT_GT(sent, 0);

    // Receive data using os_socket_recv_from - src_addr should be populated
    char recv_buffer[256];
    bh_sockaddr_t src_addr;
    memset(&src_addr, 0, sizeof(src_addr));

    int result = os_socket_recv_from(recv_sock, recv_buffer, sizeof(recv_buffer),
                                     0, &src_addr);

    ASSERT_GT(result, 0);
    ASSERT_EQ(strlen(test_data), (size_t)result);
    recv_buffer[result] = '\0';
    ASSERT_STREQ(test_data, recv_buffer);
    // src_addr should have been populated via sockaddr_to_bh_sockaddr
    ASSERT_TRUE(src_addr.is_ipv4);
    ASSERT_GT(src_addr.port, 0);

    close(recv_sock);
    close(send_sock);
}

// Test os_socket_listen with invalid socket
TEST_F(PosixSocketCoreTest, SocketListen_InvalidSocket_ReturnsError) {
    int result = os_socket_listen(-1, 5);
    ASSERT_EQ(BHT_ERROR, result);
}
