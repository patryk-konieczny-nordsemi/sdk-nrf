#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library_named(fp_provisioner)

zephyr_library_sources(fp_register_provisioner_data.c)

target_include_directories(fp_provisioner PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/include/common)