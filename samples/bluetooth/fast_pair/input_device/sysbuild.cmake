#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

# The provisioner is only relevant for the nRF54L15 /ns (TF-M) target, where the
# Fast Pair registration data is sourced from hardware-protected storage.
if(SB_CONFIG_PROVISIONER_IMAGE AND "${BOARD}" MATCHES "nrf54l15")
  # The provisioner application is provided by the Fast Pair subsystem: the user
  # only maintains the main application. Enabling the provisioner image in
  # sysbuild pulls in the generated provisioner from the library.
  ExternalZephyrProject_Add(
    APPLICATION provisioner
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/provision
    BUILD_ONLY TRUE
  )

  # Apply the *exact same* devicetree overlay as the input_device image so both
  # images see an identical flash partition layout. This guarantees TF-M's
  # ITS/PS/OTP secure-storage partitions live at the same addresses in both
  # images, so keys provisioned at runtime by the provisioner are found (and can
  # be cleared) by the input_device application.
  add_overlay_dts(provisioner ${APP_DIR}/boards/nrf54l15dk_nrf54l15_cpuapp_ns.overlay)

  # Feed the Fast Pair credentials and storage locations (single source of truth
  # in Kconfig.sysbuild) into the provisioner image, and mirror the storage
  # locations into the application image so both agree on the KMU slot and the
  # Protected Storage uid.
  set_config_int(provisioner CONFIG_FP_PROVISION_MODEL_ID ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
  set_config_string(provisioner CONFIG_FP_PROVISION_ANTI_SPOOFING_KEY
    "${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}")
  set_config_int(provisioner CONFIG_FP_PROVISION_KMU_SLOT ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(provisioner CONFIG_FP_PROVISION_PS_ID ${SB_CONFIG_BT_FAST_PAIR_PS_ID})

  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_KMU_SLOT ${SB_CONFIG_BT_FAST_PAIR_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_PS_ID ${SB_CONFIG_BT_FAST_PAIR_PS_ID})
endif()
