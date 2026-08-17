#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Sysbuild hook for the runtime provisioner subsystem. Included from
# nrf/sysbuild/CMakeLists.txt post_cmake. Generates sanitize_ns_slot.hex
# when CONFIG_PROVISIONER_SANITIZE_HEX is enabled in the application image.

include(${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/cmake/sanitize_hex.cmake)
provisioner_generate_sanitize_hex(${DEFAULT_IMAGE})
