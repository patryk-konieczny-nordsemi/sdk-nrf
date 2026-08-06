#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(input_device)

# APP_SOURCE selects which application is built in the single-project MCUboot flow:
#   -DAPP_SOURCE=provisioner -> Fast Pair provisioner (../provisioner/src/main.c)
#   unset (default)          -> input_device application
#
# FILE_SUFFIX is reserved for sysbuild configuration variants (e.g. mcuboot).
zephyr_get(APP_SOURCE SYSBUILD GLOBAL)

if("${APP_SOURCE}" STREQUAL "provisioner")
  target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../provisioner/src/main.c)
else()
  # NORDIC SDK APP START
  target_sources(app PRIVATE
    src/main.c
    src/bt_adv_helper.c
    src/hids_helper.c
    src/bt_le_adv_prov_uuid16.c
    src/battery_module.c
  )

  target_include_directories(app PRIVATE include)
  # NORDIC SDK APP END
endif()
