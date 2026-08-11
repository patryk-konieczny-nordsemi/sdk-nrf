#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_PROVISIONER)
  message(WARNING "PROVISIONER DISABLED")
	return()
endif()

target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/src/provisioner.c)

zephyr_linker_sources(SECTIONS
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/provision_entry.ld)

if(CONFIG_FP_PROVISION_DATA)
  add_subdirectory(${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fp_provision fp_provision)
endif()
