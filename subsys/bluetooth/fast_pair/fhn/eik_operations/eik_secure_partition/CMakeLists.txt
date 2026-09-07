#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(tfm_app_rot_partition_eik STATIC)

set(EIK_OPERATIONS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(FP_CRYPTO_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fp_crypto)
set(FP_COMMON_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/include/common)

# Shared crypto logic plus the Oberon (ocrypto) backend, reused from the
# non-secure sources with their Zephyr logging commented out.
target_sources(tfm_app_rot_partition_eik
    PRIVATE
        tfm_eik_secure_partition.c
        ${EIK_OPERATIONS_DIR}/eik_operations_core.c
        ${FP_CRYPTO_DIR}/fp_crypto_oberon.c
)

target_include_directories(tfm_app_rot_partition_eik
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${EIK_OPERATIONS_DIR}/include
        ${EIK_OPERATIONS_DIR}/include_priv
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fhn/include_priv
        ${FP_CRYPTO_DIR}/include
        ${FP_CRYPTO_DIR}/common
        ${FP_COMMON_DIR}
        ${ZEPHYR_BASE}/include
        ${ZEPHYR_NRFXLIB_MODULE_DIR}/crypto/nrf_oberon/include
        ${CMAKE_BINARY_DIR}/eik_secure_partition
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/eik_secure_partition
)

# The intermedia file defines the partition stack.
target_sources(tfm_app_rot_partition_eik
    PRIVATE
        ${CMAKE_BINARY_DIR}/eik_secure_partition/auto_generated/intermedia_tfm_eik_secure_partition.c
)

# The load info file includes the static data of the partition.
target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/eik_secure_partition/auto_generated/load_info_tfm_eik_secure_partition.c
)

add_dependencies(tfm_app_rot_partition_eik manifest_tool)

# Provide Kconfig symbols (CONFIG_BT_FAST_PAIR_FHN_*) to the shared sources.
target_compile_options(tfm_app_rot_partition_eik PRIVATE
    "SHELL: -imacros ${ZEPHYR_AUTOCONF}"
)

target_link_libraries(tfm_app_rot_partition_eik
    PRIVATE
        platform_s
        tfm_sprt
        psa_crypto_config
)

# ocrypto (SECP operations, and hash/AES for the Oberon backend).
if(NOT TARGET mbedcrypto_oberon_imported)
  message(FATAL_ERROR "Expected mbedcrypto_oberon_imported in TF-M build")
endif()
target_link_libraries(tfm_app_rot_partition_eik PRIVATE mbedcrypto_oberon_imported)

target_link_libraries(tfm_spm
    PRIVATE
        tfm_app_rot_partition_eik
)

target_link_libraries(tfm_partitions
    INTERFACE
        tfm_app_rot_partition_eik
)

target_compile_definitions(tfm_config
    INTERFACE
        TFM_PARTITION_EIK_SECURE_PARTITION
)
