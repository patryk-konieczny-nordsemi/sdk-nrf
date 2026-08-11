#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_PROVISIONER)
    return()
    message(WARNING "PROVISIONER DISABLED")
endif()

target_include_directories(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/include)

target_sources(app PRIVATE
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/src/provisioner.c)
  
zephyr_linker_sources(SECTIONS
  ${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/provision_entry.ld)

if(CONFIG_FP_PROVISION_DATA)
  add_subdirectory(${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fp_provision fp_provision)
endif()