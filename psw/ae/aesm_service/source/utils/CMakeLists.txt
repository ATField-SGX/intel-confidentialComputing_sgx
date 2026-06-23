#
# Copyright(c) 2011-2026 Intel Corporation
#
# SPDX-License-Identifier: BSD-3-Clause
#

aux_source_directory(. _srcs)
add_library(utils SHARED ${_srcs}
    # [SDK-X-REPO-INCLUDE] Cross-repo dep: these sources are compiled straight from the SDK submodule (encapsulation smell, to be removed).
    "../../../../../sdk/common/src/sgx_read_rand.cpp"
    "../../../../../sdk/common/src/sgx_memset_s.cpp"
    "../../../../../sdk/common/src/crypto_cmac_128.cpp")

target_include_directories(utils PRIVATE
    ${PROJECT_SOURCE_DIR}/common
    ${PROJECT_SOURCE_DIR}/common/oal
    ${PROJECT_SOURCE_DIR}/../../inc
    ${PROJECT_SOURCE_DIR}/../../inc/internal
    ${SGX_HEADER_DIR}
    ${PROJECT_SOURCE_DIR}/../../../../common/inc/internal
    ${PROJECT_SOURCE_DIR}/../../../../external/rdrand
    ${PROJECT_SOURCE_DIR}/../../data/constants/linux
    ${PROJECT_SOURCE_DIR}/../../../../prebuilt/dcap/openssl/inc
    # [SDK-X-REPO-INCLUDE] Cross-repo dep on SDK submodule internal headers (encapsulation smell, to be removed).
    # Actually used by this target: crypto_wrapper.h, se_memcpy.h, se_wrapper.h, se_time.h, sgx_memset_s.h, sgx_read_rand.h.
    ${PROJECT_SOURCE_DIR}/../../../../sdk/common/inc/internal
)

target_compile_definitions(utils PRIVATE
  $<$<CONFIG:Debug>:DBG_LOG>
)

set_property(TARGET utils APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-z,defs")

target_link_libraries(utils
    ${PROJECT_SOURCE_DIR}/../../../../prebuilt/dcap/openssl/lib/linux64/libcrypto.a
    oal
    ${CMAKE_SOURCE_DIR}/../../../../external/rdrand/src/librdrand.a
)

