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

# ----------------------------------------------------------------------------
# Validation
# ----------------------------------------------------------------------------
if(SB_CONFIG_APP_PROVISIONER AND NOT SB_CONFIG_BOARD_IS_NON_SECURE)
  message(FATAL_ERROR "Runtime provisioner app not supported in secure builds: non-secure TF-M required")
endif()

# ----------------------------------------------------------------------------
# /ns main application (not provisioner)
# KMU/ITS slot values: Kconfig.sysbuild (SB_CONFIG_BT_FAST_PAIR_*).
# ----------------------------------------------------------------------------
if(NOT SB_CONFIG_APP_PROVISIONER AND SB_CONFIG_BOARD_IS_NON_SECURE)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE y)

  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_KMU_SLOT
    ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ITS_ID
    ${SB_CONFIG_BT_FAST_PAIR_ITS_ID})
endif()

# ----------------------------------------------------------------------------
# Provisioner
# ----------------------------------------------------------------------------
if(SB_CONFIG_APP_PROVISIONER)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_PROVISIONER y)

  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_MODEL_ID
    ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
  set_config_string(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY
    "${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}")
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_KMU_SLOT
    ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ITS_ID
    ${SB_CONFIG_BT_FAST_PAIR_ITS_ID})
else()
  set_config_bool(${DEFAULT_IMAGE} CONFIG_PROVISIONER n)
endif()