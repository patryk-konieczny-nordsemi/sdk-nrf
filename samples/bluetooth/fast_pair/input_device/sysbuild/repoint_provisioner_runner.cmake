#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Point the provisioner image's `west flash` runner at the merged provisioning
# image (MCUboot + signed provisioner) instead of the unsigned tfm_merged.hex,
# so `west flash --domain provisioner` programs a single bootable image.
#
# Invoked at build time with -DRUNNERS=<runners.yaml> -DMERGED=<merged hex>.

file(READ "${RUNNERS}" _content)
get_filename_component(_merged_name "${MERGED}" NAME)
# Replace whatever hex the image build points at (e.g. zephyr.signed.hex) with the
# merged provisioning image, regardless of its current value.
string(REGEX REPLACE "hex_file:[ \t]*[^\n]*" "hex_file: ${_merged_name}" _content "${_content}")
file(WRITE "${RUNNERS}" "${_content}")
