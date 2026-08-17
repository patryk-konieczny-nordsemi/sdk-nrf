#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library_named(provisioner)
zephyr_library_sources(src/provisioner.c)
zephyr_library_include_directories(${ZEPHYR_NRF_MODULE_DIR}/include)

zephyr_linker_sources(SECTIONS provisioner_entry.ld)

add_subdirectory_ifdef(CONFIG_FP_PROVISION_DATA
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fp_provision
  ${CMAKE_CURRENT_BINARY_DIR}/fp_provision)