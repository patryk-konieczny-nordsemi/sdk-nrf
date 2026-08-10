#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(input_device)

# FILE_SUFFIX selects the build variant (e.g. provisioner, mcuboot,
# mcuboot_provisioner). When the suffix matches *provisioner*, build the user
# project's provisioner sub-application: thin main in provisioner/ calls the
# shared provision_data() API from subsys/provisioner.
zephyr_get(FILE_SUFFIX SYSBUILD GLOBAL)
if(FILE_SUFFIX MATCHES "provisioner")
  include(${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/cmake/provisioner_app.cmake)
  target_sources(app PRIVATE provisioner/provisioner_main.c)
else()
  target_sources(app PRIVATE
    src/main.c
    src/bt_adv_helper.c
    src/hids_helper.c
    src/bt_le_adv_prov_uuid16.c
    src/battery_module.c
  )

  target_include_directories(app PRIVATE include)
endif()
