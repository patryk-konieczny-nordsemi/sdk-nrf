#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

get_filename_component(input_device_app_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(INPUT_DEVICE_APP_DIR "${input_device_app_dir}" CACHE PATH
    "Input device application directory (shared board overlays and sysbuild assets)")

# The MCUboot signing key is referenced through ${SB_APPLICATION_CONFIG_DIR}
# (see sysbuild_mcuboot.conf). Anchor it here so it resolves for both the
# input_device and the provisioner main image. This only affects sysbuild's own
# configuration lookup, not the per-image prj.conf.
set(SB_APPLICATION_CONFIG_DIR "${input_device_app_dir}")

# Enable MCUboot only for the dedicated `build_ns_mcuboot` directory, so the
# standard `build` and `build_ns` configurations remain completely untouched.
# This is a single project: both applications (input_device and the Fast Pair
# provisioner) share this MCUboot sysbuild configuration, board overlay and
# signing key. The application source is selected with FILE_SUFFIX in the
# application CMakeLists.txt (unset -> input_device, provisioner -> provisioner).
if(DEFINED APPLICATION_BINARY_DIR)
  get_filename_component(_build_dir_name "${APPLICATION_BINARY_DIR}" NAME)
else()
  get_filename_component(_build_dir_name "${CMAKE_BINARY_DIR}" NAME)
endif()

if(_build_dir_name STREQUAL "build_ns_mcuboot")
  set(SB_CONF_FILE "${input_device_app_dir}/sysbuild_mcuboot.conf" CACHE STRING
      "Sysbuild configuration for the MCUboot-enabled /ns build" FORCE)
endif()

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)

if(SB_CONFIG_MCUBOOT_SIGNATURE_USING_KMU)
  message(WARNING "
          ------------------------------------------------------------------------------
          --- WARNING: MCUboot signature verification uses KMU-stored keys. You must ---
          --- use the `west flash` command with either the `--erase` or `--recover`  ---
          --- option to ensure the bootloader provisioning operation is included in  ---
          --- the flashing procedure. The application will fail to boot if MCUboot   ---
          --- is not properly provisioned. For detailed instructions, refer to the   ---
          --- `Building and running` section in the Fast Pair Input Device Readme.   ---
          ------------------------------------------------------------------------------
          ")
endif()
