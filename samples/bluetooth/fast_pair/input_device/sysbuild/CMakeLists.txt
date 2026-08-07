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
# (see sysbuild_mcuboot_base.conf). Anchor it here so it resolves for both the
# main application and provisioner builds.
set(SB_APPLICATION_CONFIG_DIR "${input_device_app_dir}")

# Promote FILE_SUFFIX to a typed cache entry so sysbuild forwards it to images.
if(DEFINED FILE_SUFFIX)
  set(FILE_SUFFIX "${FILE_SUFFIX}" CACHE STRING
      "Sysbuild/application file suffix (e.g. mcuboot, provisioner)" FORCE)
endif()

# MCUboot sysbuild settings (shared by mcuboot and mcuboot_provisioner).
if(DEFINED FILE_SUFFIX AND FILE_SUFFIX MATCHES "^mcuboot")
  list(APPEND SB_EXTRA_CONF_FILE "${input_device_app_dir}/sysbuild_mcuboot_base.conf")
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
