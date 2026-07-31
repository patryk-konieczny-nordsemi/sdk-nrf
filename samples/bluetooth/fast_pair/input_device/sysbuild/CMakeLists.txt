#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

# Anchor the sysbuild configuration to the application directory so that
# sysbuild.conf / sysbuild_<suffix>.conf and the MCUboot signing key resolve
# relative to it (see sysbuild_boot.conf: SB_CONFIG_BOOT_SIGNATURE_KEY_FILE uses
# ${SB_APPLICATION_CONFIG_DIR}). This is the app directory itself (not a
# per-board subdirectory), so the plain standard and /ns builds are unaffected.
get_filename_component(app_dir "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(SB_APPLICATION_CONFIG_DIR "${app_dir}")

find_package(Sysbuild REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(sysbuild LANGUAGES)
