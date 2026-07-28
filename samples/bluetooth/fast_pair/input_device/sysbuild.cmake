#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/extensions.cmake)

# The provisioner and its overlay are specific to the nRF54L15 /ns target.
if(SB_CONFIG_PROVISIONER_IMAGE AND "${BOARD}" MATCHES "nrf54l15")
  # Build the provisioner as its own domain, using the same board target
  # (including the /ns variant, so it gets its own TF-M).
  # BUILD_ONLY keeps it out of the default `west flash` sequence so it is
  # flashed separately, on its own, before the input_device application.
  ExternalZephyrProject_Add(
    APPLICATION provisioner
    SOURCE_DIR ${APP_DIR}/provisioner
    BUILD_ONLY TRUE
  )

  # Apply the *exact same* devicetree overlay as the input_device image so both
  # images see an identical flash partition layout. This guarantees TF-M's
  # ITS/PS/OTP secure-storage partitions live at the same addresses in both
  # images, so keys provisioned at runtime by the provisioner are found (and can
  # be cleared) by the input_device application.
  add_overlay_dts(provisioner ${APP_DIR}/boards/nrf54l15dk_nrf54l15_cpuapp_ns.overlay)

  # Feed the Fast Pair credentials (single source of truth in Kconfig.sysbuild)
  # into the provisioner image. On /ns builds these replace the legacy hex-based
  # provisioning: the provisioner consumes them at runtime via the PSA API.
  set_config_int(provisioner CONFIG_PROV_FP_MODEL_ID ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
  set_config_string(provisioner CONFIG_PROV_FP_ANTI_SPOOFING_KEY "${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}")
endif()
