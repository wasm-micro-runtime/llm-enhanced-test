/*
 * Copyright (C) 2025 WAMR Community. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include "test_helper.h"
#include "gtest/gtest.h"
#include "platform_api_extension.h"
#include <string>
#include <cstring>
#include <csignal>

class PosixSocketTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
        // Initialize sockets for testing
        tcp_socket_ipv4 = -1;
        tcp_socket_ipv6 = -1;
        udp_socket_ipv4 = -1;
        server_socket = -1;

        // Ignore SIGPIPE to prevent test crashes when writing to broken sockets
        (void)signal(SIGPIPE, SIG_IGN);
    }

    virtual void TearDown()
    {
        // Cleanup all created sockets
        if (tcp_socket_ipv4 >= 0) {
            os_socket_close(tcp_socket_ipv4);
        }
        if (tcp_socket_ipv6 >= 0) {
            os_socket_close(tcp_socket_ipv6);
        }
        if (udp_socket_ipv4 >= 0) {
            os_socket_close(udp_socket_ipv4);
        }
        if (server_socket >= 0) {
            os_socket_close(server_socket);
        }
    }

  public:
    WAMRRuntimeRAII<512 * 1024> runtime;
    bh_socket_t tcp_socket_ipv4;
    bh_socket_t tcp_socket_ipv6;
    bh_socket_t udp_socket_ipv4;
    bh_socket_t server_socket;
};

TEST_F(PosixSocketTest, ErrorHandling)
{
    // Test operations on invalid socket
    bh_socket_t invalid_socket = -1;

    bh_sockaddr_t addr;
    int result = os_socket_addr_local(invalid_socket, &addr);
    EXPECT_NE(0, result); // Should fail

    // Test invalid parameters
    result = os_socket_create(nullptr, true, true);
    EXPECT_NE(0, result); // Should fail with null pointer
}

TEST_F(PosixSocketTest, SocketRemoteAddressOperations)
{
    // Create socket
    int result = os_socket_create(&tcp_socket_ipv4, true, true);
    ASSERT_EQ(0, result);

    // Test remote address on unconnected socket (should fail)
    bh_sockaddr_t remote_addr;
    result = os_socket_addr_remote(tcp_socket_ipv4, &remote_addr);
    EXPECT_NE(0, result); // Should fail - not connected

    // Test invalid socket
    result = os_socket_addr_remote(-1, &remote_addr);
    EXPECT_NE(0, result); // Should fail - invalid socket
}

TEST_F(PosixSocketTest, TextualAddrToSockaddrInvalidInput)
{
    int result = os_socket_create(&tcp_socket_ipv4, true, true);
    ASSERT_EQ(0, result);

    // Test invalid IPv4 addresses
    result = os_socket_connect(tcp_socket_ipv4, "256.256.256.256", 65436);
    EXPECT_NE(0, result); // Should fail due to invalid address

    result = os_socket_connect(tcp_socket_ipv4, "invalid.address", 65437);
    EXPECT_NE(0, result); // Should fail due to invalid address

    result = os_socket_connect(tcp_socket_ipv4, "", 65438);
    EXPECT_NE(0, result); // Should fail due to empty address
}

// Step 2: Socket Communication Functions - Testing recv, send, sendto,
// recvfrom, shutdown
TEST_F(PosixSocketTest, SocketCommRecvUnconnectedSocket_ReturnsError)
{
    int result = os_socket_create(&tcp_socket_ipv4, true, true);
    ASSERT_EQ(0, result);

    // Test recv on unconnected socket (should fail or timeout)
    char buffer[1024];
    result = os_socket_recv(tcp_socket_ipv4, buffer, sizeof(buffer));
    EXPECT_NE(0, result);

    // Test recv with different buffer sizes to exercise different code paths
    result = os_socket_recv(tcp_socket_ipv4, buffer, 1);
    EXPECT_NE(0, result);

    result = os_socket_recv(tcp_socket_ipv4, buffer, 512);
    EXPECT_NE(0, result);

    // Test recv with zero buffer size
    result = os_socket_recv(tcp_socket_ipv4, buffer, 0);
    EXPECT_NE(0, result);
}

TEST_F(PosixSocketTest, SocketCommShutdown_UnconnectedSocket_ReturnsError)
{
    int result = os_socket_create(&tcp_socket_ipv4, true, true);
    ASSERT_EQ(0, result);

    // Test shutdown (closes both read and write)
    result = os_socket_shutdown(tcp_socket_ipv4);
    // Shutdown on unconnected socket returns ECONNABORTED (53) or other error codes
    EXPECT_NE(0, result); // Should fail on unconnected socket

    os_socket_close(tcp_socket_ipv4);
    tcp_socket_ipv4 = -1;

    // Test shutdown on different socket types
    result = os_socket_create(&udp_socket_ipv4, true, false);
    ASSERT_EQ(0, result);

    result = os_socket_shutdown(udp_socket_ipv4);
    // UDP shutdown on unconnected socket returns error code
    EXPECT_NE(0, result); // Should fail on unconnected socket

    os_socket_close(udp_socket_ipv4);
    udp_socket_ipv4 = -1;

    // Test shutdown on invalid socket
    result = os_socket_shutdown(-1);
    EXPECT_NE(0, result);
}

TEST_F(PosixSocketTest, SocketCommSocketOptionOperations)
{
    int result = os_socket_create(&tcp_socket_ipv4, true, true);
    ASSERT_EQ(0, result);

    // Test socket timeout operations which exercise internal socket option code
    uint64 send_timeout = 5000000; // 5 seconds
    result = os_socket_set_send_timeout(tcp_socket_ipv4, send_timeout);
    EXPECT_EQ(0, result);

    uint64 retrieved_timeout;
    result = os_socket_get_send_timeout(tcp_socket_ipv4, &retrieved_timeout);
    EXPECT_EQ(0, result);
    // Note: exact timeout values may vary by system

    // Test recv timeout
    uint64 recv_timeout = 3000000; // 3 seconds
    result = os_socket_set_recv_timeout(tcp_socket_ipv4, recv_timeout);
    EXPECT_EQ(0, result);

    result = os_socket_get_recv_timeout(tcp_socket_ipv4, &retrieved_timeout);
    EXPECT_EQ(0, result);

    // Test timeout operations on invalid socket
    result = os_socket_set_send_timeout(-1, send_timeout);
    EXPECT_NE(0, result);

    result = os_socket_get_send_timeout(-1, &retrieved_timeout);
    EXPECT_NE(0, result);
}

// Step 1 Enhanced: Target specific uncovered lines for 40 additional lines
// coverage
TEST_F(PosixSocketTest, SocketCoreIPv6SockaddrConversion)
{
    // Target IPv6 sockaddr_to_bh_sockaddr conversion (lines 62-78)
    int result = os_socket_create(&tcp_socket_ipv6, false, true);
    if (result != 0) {
        GTEST_SKIP() << "IPv6 not available on this system";
        return;
    }

    // Bind IPv6 socket to trigger sockaddr_to_bh_sockaddr with AF_INET6
    int port = 0;
    result = os_socket_bind(tcp_socket_ipv6, "::1", &port);
    if (result == 0) {
        EXPECT_GT(port, 0);

        // Get local address to trigger IPv6 sockaddr conversion
        bh_sockaddr_t local_addr;
        result = os_socket_addr_local(tcp_socket_ipv6, &local_addr);
        EXPECT_EQ(0, result);
        EXPECT_FALSE(local_addr.is_ipv4); // Should be IPv6
        EXPECT_EQ(port, local_addr.port);
    }
}

TEST_F(PosixSocketTest, SocketCoreBindInvalidAddr_HitsFailLabel)
{
    // Target socket creation error paths (line 134 - socket failure)
    // Create many sockets to potentially trigger failure
    std::vector<bh_socket_t> sockets;
    for (int i = 0; i < 1000; i++) {
        bh_socket_t temp_socket;
        int result = os_socket_create(&temp_socket, true, true);
        if (result != 0) {
            // Hit the error path where socket() returns -1
            break;
        }
        sockets.push_back(temp_socket);
    }

    // Cleanup sockets
    for (auto sock : sockets) {
        os_socket_close(sock);
    }

    // Test bind error paths with invalid addresses to hit fcntl/setsockopt/bind
    // failures
    int result = os_socket_create(&tcp_socket_ipv4, true, true);
    ASSERT_EQ(0, result);

    // Test bind with invalid address format to trigger textual_addr_to_sockaddr
    // failure
    int port = 8080;
    result = os_socket_bind(tcp_socket_ipv4, "999.999.999.999", &port);
    EXPECT_NE(0, result); // Should hit fail: label (line 189)
}

TEST_F(PosixSocketTest, SocketCoreSettimeoutFunction)
{
    // Target os_socket_settimeout function (lines 194-206) - completely
    // uncovered
    int result = os_socket_create(&tcp_socket_ipv4, true, true);
    ASSERT_EQ(0, result);

    // Call os_socket_settimeout to cover lines 194-206
    result = os_socket_settimeout(tcp_socket_ipv4, 5000000); // 5 seconds
    EXPECT_EQ(0, result); // Should succeed and cover lines 197-206

    // Test with zero timeout
    result = os_socket_settimeout(tcp_socket_ipv4, 0);
    EXPECT_EQ(0, result);

    // Test with large timeout
    result = os_socket_settimeout(tcp_socket_ipv4, 30000000); // 30 seconds
    EXPECT_EQ(0, result);
}

TEST_F(PosixSocketTest, SocketCoreAcceptNullAddr)
{
    // Target os_socket_accept NULL address path (lines 223-224)
    int result = os_socket_create(&server_socket, true, true);
    ASSERT_EQ(0, result);

    int port = 0;
    result = os_socket_bind(server_socket, "127.0.0.1", &port);
    ASSERT_EQ(0, result);

    result = os_socket_listen(server_socket, 1);
    ASSERT_EQ(0, result);

    // Set short timeout to avoid blocking
    result = os_socket_set_recv_timeout(server_socket, 10000); // 10ms
    EXPECT_EQ(0, result);

    // Test accept with NULL address to cover line 224
    bh_socket_t client_socket;
    result = os_socket_accept(server_socket, &client_socket, nullptr, nullptr);
    EXPECT_NE(0, result); // Should timeout, but covers NULL address path
}
