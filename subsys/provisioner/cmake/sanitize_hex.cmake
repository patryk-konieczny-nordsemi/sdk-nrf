# subsys/provisioner/cmake/sanitize_hex.cmake
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/bootloader_dts_utils.cmake)

function(provisioner_generate_sanitize_hex application)
  ExternalProject_Get_Property(${application} BINARY_DIR)
  import_kconfig(CONFIG_ ${BINARY_DIR}/zephyr/.config)

  if(NOT CONFIG_PROVISIONER OR NOT CONFIG_PROVISIONER_SANITIZE_HEX)
    return()
  endif()

  set(SANITIZE_HEX ${CMAKE_BINARY_DIR}/sanitize_ns_slot.hex)
  set(REGIONS "")
  set(DT_ARGS TARGET ${application} ABSOLUTE REQUIRED)

  if(CONFIG_PROVISIONER_SANITIZE_SLOT0_NS)
    dt_partition_addr(_a LABEL slot0_ns_partition ${DT_ARGS})
    dt_partition_size(_s LABEL slot0_ns_partition TARGET ${application} REQUIRED)
    list(APPEND REGIONS "${_a}:${_s}")
  endif()

  if(CONFIG_PROVISIONER_SANITIZE_SLOT0_FULL)
    dt_partition_addr(_a LABEL slot0_partition ${DT_ARGS})
    dt_partition_size(_s LABEL slot0_partition TARGET ${application} REQUIRED)
    list(APPEND REGIONS "${_a}:${_s}")
  endif()

  if(CONFIG_PROVISIONER_SANITIZE_SLOT1)
    dt_partition_addr(_a LABEL slot1_partition ${DT_ARGS})
    dt_partition_size(_s LABEL slot1_partition TARGET ${application} REQUIRED)
    list(APPEND REGIONS "${_a}:${_s}")
  endif()

  if(REGIONS STREQUAL "")
    message(FATAL_ERROR "provisioner sanitize: no regions selected")
  endif()
  string(JOIN "," REGIONS_ARG ${REGIONS})

  add_custom_command(
    OUTPUT ${SANITIZE_HEX}
    COMMAND ${PYTHON_EXECUTABLE}
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/provisioner/scripts/generate_sanitize_hex.py
      --regions ${REGIONS_ARG}
      --fill 0xFF
      --output ${SANITIZE_HEX}
    DEPENDS ${application}
    COMMENT "Generating sanitize_ns_slot.hex (${REGIONS_ARG})"
    VERBATIM
  )

  add_custom_target(provisioner_sanitize_hex ALL DEPENDS ${SANITIZE_HEX})
  set_property(GLOBAL APPEND PROPERTY extra_post_build_files ${SANITIZE_HEX})
endfunction()