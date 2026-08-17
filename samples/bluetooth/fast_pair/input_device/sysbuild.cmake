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