#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(tfm_app_rot_partition_fpc STATIC)

target_include_directories(tfm_app_rot_partition_fpc
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../common
        ${CMAKE_BINARY_DIR}/fp_crypto_partition
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/fp_crypto_partition
)

target_sources(tfm_app_rot_partition_fpc
    PRIVATE
        tfm_fp_crypto_partition.c
)

# The intermedia file defines the partition stack.
target_sources(tfm_app_rot_partition_fpc
    PRIVATE
        ${CMAKE_BINARY_DIR}/fp_crypto_partition/auto_generated/intermedia_tfm_fp_crypto_partition.c
)

# The load info file includes the static data of the partition.
target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/fp_crypto_partition/auto_generated/load_info_tfm_fp_crypto_partition.c
)

add_dependencies(tfm_app_rot_partition_fpc manifest_tool)

target_link_libraries(tfm_app_rot_partition_fpc
    PRIVATE
        platform_s
        tfm_sprt
        psa_crypto_config
)

target_link_libraries(tfm_spm
    PRIVATE
        tfm_app_rot_partition_fpc
)

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_app_rot_partition_fpc
)

target_compile_definitions(tfm_config
    INTERFACE
        TFM_PARTITION_FP_CRYPTO_PARTITION
)

if(NOT TARGET mbedcrypto_oberon_imported)
  message(FATAL_ERROR "Expected mbedcrypto_oberon_imported in TF-M build")
endif()
target_link_libraries(tfm_app_rot_partition_fpc PRIVATE mbedcrypto_oberon_imported)
