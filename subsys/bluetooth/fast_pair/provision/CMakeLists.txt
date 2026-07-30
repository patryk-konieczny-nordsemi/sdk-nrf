#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(fp_provision)

# Decode the base64 Anti-Spoofing key at build time into a C header, so no
# base64 decoding is done on the device during provisioning.
set(fp_prov_gen_dir ${CMAKE_CURRENT_BINARY_DIR}/gen)
set(fp_prov_key_header ${fp_prov_gen_dir}/fp_provision_key.h)

add_custom_command(
  OUTPUT ${fp_prov_key_header}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${fp_prov_gen_dir}
  COMMAND ${PYTHON_EXECUTABLE}
          ${CMAKE_CURRENT_SOURCE_DIR}/fp_provision_key_gen.py
          --key "${CONFIG_FP_PROVISION_ANTI_SPOOFING_KEY}"
          --output ${fp_prov_key_header}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fp_provision_key_gen.py
  COMMENT "Fast Pair: decoding Anti-Spoofing key into ${fp_prov_key_header}"
  VERBATIM
)
add_custom_target(fp_provision_key_gen DEPENDS ${fp_prov_key_header})

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE ${fp_prov_gen_dir})
add_dependencies(app fp_provision_key_gen)
