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

# Promote command-line variables to typed cache entries so sysbuild forwards
# them to image builds (UNINITIALIZED -D values are not propagated otherwise).
if(DEFINED APP_SOURCE)
  set(APP_SOURCE "${APP_SOURCE}" CACHE STRING
      "Application source variant (provisioner or default)" FORCE)
endif()
if(DEFINED FILE_SUFFIX)
  set(FILE_SUFFIX "${FILE_SUFFIX}" CACHE STRING
      "Sysbuild/application file suffix (e.g. mcuboot)" FORCE)
endif()

# MCUboot sysbuild settings live in sysbuild_mcuboot.conf and are selected with
# FILE_SUFFIX=mcuboot (standard Zephyr sysbuild file-suffix mechanism). Both
# applications (input_device and the Fast Pair provisioner) share that MCUboot
# configuration, board overlay and signing key. The application source is
# selected with APP_SOURCE in the application CMakeLists.txt (unset ->
# input_device, provisioner -> provisioner).

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
