#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library_named(provisioner)
zephyr_library_sources(src/provisioner.c)

zephyr_linker_sources(SECTIONS provisioner_entry.ld)
