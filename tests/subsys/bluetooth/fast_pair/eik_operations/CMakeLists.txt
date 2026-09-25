#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Fast Pair EIK operations unit test")

set(NCS_FAST_PAIR_BASE ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair)
set(EIK_OPS_DIR ${NCS_FAST_PAIR_BASE}/fhn/fp_fhn_eik_operations)

target_sources(app PRIVATE src/main.c)

add_subdirectory(${NCS_FAST_PAIR_BASE}/fp_crypto fp_crypto)
target_link_libraries(app PRIVATE fp_crypto)

add_subdirectory(${NCS_FAST_PAIR_BASE}/fp_storage fp_storage)
target_link_libraries(app PRIVATE fp_storage)

target_include_directories(app PRIVATE
  ${NCS_FAST_PAIR_BASE}/fhn/include_priv
  ${EIK_OPS_DIR}/include
)

add_library(fhn OBJECT)
target_link_libraries(fhn PRIVATE zephyr_interface fp_crypto fp_storage)
target_include_directories(fhn PRIVATE
  ${NCS_FAST_PAIR_BASE}/fhn/include_priv
  ${NCS_FAST_PAIR_BASE}/include/common
)

add_subdirectory(${EIK_OPS_DIR} fp_fhn_eik_operations)

target_link_libraries(app PRIVATE fhn)
