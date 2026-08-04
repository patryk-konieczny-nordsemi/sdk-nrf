#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Shared Fast Pair sysbuild logic for the input_device sample (a single project).
# The MCUboot configuration, flash layout, signing key and Fast Pair credentials
# are identical no matter which application source FILE_SUFFIX selects.
#

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

set(_input_device_app_dir ${INPUT_DEVICE_APP_DIR})
if(NOT _input_device_app_dir)
  set(_input_device_app_dir ${APP_DIR})
endif()

# ----------------------------------------------------------------------------
# MCUboot-enabled /ns build (the `build_ns_mcuboot` directory).
#
# The single application image (input_device or, with FILE_SUFFIX=provisioner,
# the provisioner) shares the same MCUboot overwrite-only flash layout.
# ----------------------------------------------------------------------------
if(SB_CONFIG_BOOTLOADER_MCUBOOT AND "${BOARD}" MATCHES "nrf54l15")
  # MCUboot reuses the application flash map (single source of truth) and only
  # re-points its own code partition to the boot partition; it also disables the
  # unused external SPI-NOR flash (both slots live in internal RRAM).
  add_overlay_dts(mcuboot
    ${_input_device_app_dir}/sysbuild/mcuboot/boards/nrf54l15dk_nrf54l15_cpuapp_ns_mcuboot.overlay)
  add_overlay_config(mcuboot
    ${_input_device_app_dir}/sysbuild/mcuboot/prj.conf)

  # Give the application image the MCUboot flash layout and build it as an
  # MCUboot image booting from the devicetree code partition. NCS signs this
  # (primary) image automatically - there is no manual signing step.
  add_overlay_dts(${DEFAULT_IMAGE}
    ${_input_device_app_dir}/boards/nrf54l15dk_nrf54l15_cpuapp_ns_mcuboot.overlay)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BOOTLOADER_MCUBOOT y)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_USE_DT_CODE_PARTITION y)

  # Feed the Fast Pair credentials/storage locations (single source of truth in
  # Kconfig.sysbuild) to the selected application source.
  if("${FILE_SUFFIX}" STREQUAL "provisioner")
    set_config_int(${DEFAULT_IMAGE} CONFIG_FP_PROVISION_MODEL_ID
      ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
    set_config_string(${DEFAULT_IMAGE} CONFIG_FP_PROVISION_ANTI_SPOOFING_KEY
      "${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}")
    set_config_int(${DEFAULT_IMAGE} CONFIG_FP_PROVISION_KMU_SLOT
      ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
    set_config_int(${DEFAULT_IMAGE} CONFIG_FP_PROVISION_PS_ID
      ${SB_CONFIG_BT_FAST_PAIR_PS_ID})
  else()
    set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_KMU_SLOT
      ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
    set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_PS_ID
      ${SB_CONFIG_BT_FAST_PAIR_PS_ID})
  endif()
endif()

# ----------------------------------------------------------------------------
# Plain /ns (TF-M) build without MCUboot (the standard `build_ns` directory).
#
# The Fast Pair registration data is provisioned by a dedicated BUILD_ONLY
# provisioner domain, flashed once before the application. Preserved unchanged.
# ----------------------------------------------------------------------------
if(SB_CONFIG_PROVISIONER_IMAGE AND NOT SB_CONFIG_BOOTLOADER_MCUBOOT
    AND "${BOARD}" MATCHES "nrf54l15")
  ExternalZephyrProject_Add(
    APPLICATION provisioner
    SOURCE_DIR ${_input_device_app_dir}/../provisioner
    BUILD_ONLY TRUE
  )

  add_overlay_dts(provisioner
    ${_input_device_app_dir}/boards/nrf54l15dk_nrf54l15_cpuapp_ns.overlay)

  set_config_int(provisioner CONFIG_FP_PROVISION_MODEL_ID
    ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
  set_config_string(provisioner CONFIG_FP_PROVISION_ANTI_SPOOFING_KEY
    "${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}")
  set_config_int(provisioner CONFIG_FP_PROVISION_KMU_SLOT
    ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(provisioner CONFIG_FP_PROVISION_PS_ID
    ${SB_CONFIG_BT_FAST_PAIR_PS_ID})

  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_KMU_SLOT
    ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_PS_ID
    ${SB_CONFIG_BT_FAST_PAIR_PS_ID})
endif()
