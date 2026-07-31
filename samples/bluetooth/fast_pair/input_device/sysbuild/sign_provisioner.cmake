#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Sign the provisioner's TF-M merged image for MCUboot, deriving the layout-
# dependent imgtool parameters from the provisioner's own devicetree so nothing
# is hardcoded and everything follows the partition overlay automatically.
#
# Invoked with:
#   -DIMGTOOL=<imgtool.py> -DPYTHON=<python> -DDTS=<zephyr.dts>
#   -DCONFIG_FILE=<.config> -DKEY=<pem>
#   -DINPUT=<tfm_merged.hex> -DOUTPUT=<zephyr.signed.hex>

# MCUboot header size for the TF-M combined image (CONFIG_TFM_MCUBOOT_HEADER_SIZE).
file(READ "${CONFIG_FILE}" _cfg)
if(NOT _cfg MATCHES "CONFIG_TFM_MCUBOOT_HEADER_SIZE=(0x[0-9a-fA-F]+)")
  message(FATAL_ERROR "sign_provisioner: CONFIG_TFM_MCUBOOT_HEADER_SIZE not found in ${CONFIG_FILE}")
endif()
set(HEADER_SIZE "${CMAKE_MATCH_1}")

file(READ "${DTS}" _dts)

# slot0_partition: reg = < <base> <size> >  (the primary MCUboot slot). The
# [^{}]* stops before any child node's brace, so we capture slot0's own reg and
# not a child partition's (e.g. slot0_s).
if(NOT _dts MATCHES "slot0_partition:[^{]*{[^{}]*reg[ \t]*=[ \t]*<[ \t]*(0x[0-9a-fA-F]+)[ \t]+(0x[0-9a-fA-F]+)")
  message(FATAL_ERROR "sign_provisioner: slot0_partition reg not found in ${DTS}")
endif()
set(_slot0_base "${CMAKE_MATCH_1}")
set(_slot_size "${CMAKE_MATCH_2}")

# slot0_ns_partition: reg = < <offset-within-slot0> <size> >. The MCUboot header
# / ih_load_addr must point at the absolute Non-Secure image address.
if(NOT _dts MATCHES "slot0_ns_partition:[^{]*{[^{}]*reg[ \t]*=[ \t]*<[ \t]*(0x[0-9a-fA-F]+)")
  message(FATAL_ERROR "sign_provisioner: slot0_ns_partition reg not found in ${DTS}")
endif()
set(_slot0_ns_off "${CMAKE_MATCH_1}")

math(EXPR _rom_fixed "${_slot0_base} + ${_slot0_ns_off}" OUTPUT_FORMAT HEXADECIMAL)

execute_process(
  COMMAND ${PYTHON} ${IMGTOOL} sign
    --version 0.0.0+0
    --slot-size ${_slot_size}
    --header-size ${HEADER_SIZE} --pad-header
    --overwrite-only --align 1
    --rom-fixed ${_rom_fixed}
    --sha 512
    -k ${KEY}
    ${INPUT} ${OUTPUT}
  RESULT_VARIABLE _rc
)
if(NOT _rc EQUAL 0)
  message(FATAL_ERROR "sign_provisioner: imgtool sign failed (${_rc})")
endif()
