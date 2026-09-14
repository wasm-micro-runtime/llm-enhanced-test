# Copyright (C) 2024 Intel Corporation. All rights reserved.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

# The POSIX platform cases are split by the build switches they actually need.
# The dependency comes from
#   core/shared/platform/common/posix/platform_api_posix.cmake
# which only compiles posix_file.c / posix_clock.c when WAMR_BUILD_LIBC_WASI=1
# (posix_file.c also needs the libc-util sources that switch pulls in), and
# posix_socket.c when WAMR_BUILD_LIBC_WASI=1 or WAMR_BUILD_DEBUG_INTERP=1.
# posix_time.c / posix_malloc.c / posix_thread.c are compiled unconditionally.
#
#   platform/   no WAMR_BUILD_* switch
#               posix_time, posix_malloc, enhanced_posix_thread
#   libc-wasi/  WAMR_BUILD_LIBC_WASI=1
#               posix_clock, posix_file, enhanced_posix_file,
#               posix_socket, posix_socket_options, posix_multicast_ttl,
#               coverage_improve_step_1..4
#
# No case here needs WAMR_BUILD_LIBC_BUILTIN or WAMR_BUILD_SHARED_MEMORY.
add_subdirectory(platform)
add_subdirectory(libc-wasi)
