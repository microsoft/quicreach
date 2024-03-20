# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

project(quicreach)
cmake_minimum_required(VERSION 3.16) # 3.20 for non-shared libs

# Use, i.e. don't skip the full RPATH for the build tree.
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# When building, don't use the install RPATH already (but later on when
# installing).
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# Add the automatically determined parts of the RPATH which point to directories
# outside the build tree to the install RPATH.
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Configure output paths.
set(REACH_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${QUIC_BUILD_DIR}/obj/${CMAKE_LIBRARY_ARCHITECTURE}$<IF:$<CONFIG:Debug>,chk,fre>)
set(REACH_OUTPUT_DIR ${REACH_BUILD_DIR}/bin/${CMAKE_LIBRARY_ARCHITECTURE}$<IF:$<CONFIG:Debug>,chk,fre> CACHE STRING "Output directory for build artifacts")

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${REACH_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${REACH_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${REACH_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${REACH_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${REACH_OUTPUT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${REACH_OUTPUT_DIR})

if (WIN32)
    # Statically link the OS included part of the runtime.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:libucrt.lib /DEFAULTLIB:ucrt.lib")
endif()

# Configure and build msquic dependency.
if (WIN32)
    set(QUIC_TLS "schannel" CACHE STRING "TLS Library to use")
else()
    set(QUIC_TLS "openssl" CACHE STRING "TLS Library to use")
endif()
set(QUIC_BUILD_SHARED ON CACHE BOOL "Builds MsQuic as a dynamic library")
set(QUIC_ENABLE_LOGGING ON CACHE BOOL "Enable MsQuic logging")
set(CMAKE_BUILD_TYPE "Release")
add_subdirectory(msquic)
target_compile_features(inc INTERFACE cxx_std_20)

# Build quicreach source.
add_subdirectory(src)
