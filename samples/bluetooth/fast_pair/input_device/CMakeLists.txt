#
# Copyright (c) 2022-2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(input_device)

if(CONFIG_PROVISIONER)
  add_subdirectory(provisioner)
else()
  get_target_property(TFM_BINARY_DIR tfm TFM_BINARY_DIR)

  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/example_secure_partition/tfm_example_secure_partition.yaml.in
    ${CMAKE_CURRENT_BINARY_DIR}/example_secure_partition/tfm_example_secure_partition.yaml
  )

  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/example_secure_partition/tfm_manifest_list.yaml.in
    ${CMAKE_CURRENT_BINARY_DIR}/example_secure_partition/tfm_manifest_list.yaml
  )

  set_property(TARGET zephyr_property_target
    APPEND PROPERTY TFM_CMAKE_OPTIONS
    -DTFM_EXTRA_MANIFEST_LIST_FILES=${CMAKE_CURRENT_BINARY_DIR}/example_secure_partition/tfm_manifest_list.yaml
    -DTFM_EXTRA_PARTITION_PATHS=${CMAKE_CURRENT_LIST_DIR}/example_secure_partition
    -DZEPHYR_NRFXLIB_MODULE_DIR=${ZEPHYR_NRFXLIB_MODULE_DIR}
  )

  target_include_directories(app PRIVATE
    $<TARGET_PROPERTY:tfm,TFM_BINARY_DIR>/api_ns/interface/include
  ) 

  add_subdirectory(src)
  target_include_directories(app PRIVATE include)
endif()
