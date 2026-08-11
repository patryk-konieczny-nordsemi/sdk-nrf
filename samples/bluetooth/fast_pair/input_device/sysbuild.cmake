#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Sysbuild glue for the input_device sample. Build variants use FILE_SUFFIX;
# Kconfig/DTS fragments are discovered via the suffix mechanism wherever
# possible (including mcuboot child via sysbuild/mcuboot.overlay and
# sysbuild/mcuboot.conf). Manual steps are limited to:
#   - subsys/provisioner/prj.conf (external canonical provisioner config)
#   - sysbuild Kconfig bridge (SB_CONFIG_BT_FAST_PAIR_* -> app/provisioner)

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

set(_input_device_app_dir ${INPUT_DEVICE_APP_DIR})
if(NOT _input_device_app_dir)
  set(_input_device_app_dir ${APP_DIR})
endif()

set(_prov_prj "${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/prj.conf")

string(REGEX MATCH "^mcuboot" _is_mcuboot_family "${FILE_SUFFIX}")
string(REGEX MATCH "provisioner$" _is_provisioner "${FILE_SUFFIX}")

# ----------------------------------------------------------------------------
# Validation
# ----------------------------------------------------------------------------
if(_is_provisioner AND NOT SB_CONFIG_BOARD_IS_NON_SECURE)
  message(FATAL_ERROR
    "FILE_SUFFIX=${FILE_SUFFIX} requires a non-secure TF-M board target "
    "(e.g. nrf54l15dk/nrf54l15/cpuapp/ns). Current board: ${BOARD}")
endif()

if(_is_mcuboot_family AND NOT SB_CONFIG_BOOTLOADER_MCUBOOT)
  message(FATAL_ERROR
    "FILE_SUFFIX=${FILE_SUFFIX} requires MCUboot sysbuild configuration "
    "(sysbuild_mcuboot_base.conf via sysbuild/CMakeLists.txt).")
endif()

# ----------------------------------------------------------------------------
# /ns main application (not provisioner)
# KMU/ITS slot values: Kconfig.sysbuild (SB_CONFIG_BT_FAST_PAIR_*).
# ----------------------------------------------------------------------------
if(NOT _is_provisioner AND SB_CONFIG_BOARD_IS_NON_SECURE AND "${BOARD}" MATCHES "nrf54l15")
  if(_is_mcuboot_family)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_BOOTLOADER_MCUBOOT y)
  endif()

  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE y)
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_KMU_SLOT
    ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ITS_ID
    ${SB_CONFIG_BT_FAST_PAIR_ITS_ID})
endif()

# ----------------------------------------------------------------------------
# Provisioner
# ----------------------------------------------------------------------------
if(_is_provisioner)
  add_overlay_config(${DEFAULT_IMAGE} ${_prov_prj})

  set_config_bool(${DEFAULT_IMAGE} CONFIG_PROVISIONER y)

  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_MODEL_ID
    ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
  set_config_string(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY
    "${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}")
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_KMU_SLOT
    ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ITS_ID
    ${SB_CONFIG_BT_FAST_PAIR_ITS_ID})
endif()