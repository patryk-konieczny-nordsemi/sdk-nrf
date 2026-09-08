#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(tfm_app_rot_partition_eik STATIC)

set(EIK_OPERATIONS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(FP_CRYPTO_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fp_crypto)

target_sources(tfm_app_rot_partition_eik
    PRIVATE
        tfm_eik_secure_partition.c
        ${EIK_OPERATIONS_DIR}/eik_operations_core.c
        ${CMAKE_BINARY_DIR}/eik_secure_partition/auto_generated/intermedia_tfm_eik_secure_partition.c

        ${FP_CRYPTO_DIR}/fp_crypto_oberon.c
)

target_include_directories(tfm_app_rot_partition_eik
    PRIVATE
        ${EIK_OPERATIONS_DIR}/include_priv
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fhn/include_priv
        ${FP_CRYPTO_DIR}/include
        ${FP_CRYPTO_DIR}/common
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/include/common
        ${CMAKE_BINARY_DIR}/eik_secure_partition

        ${ZEPHYR_NRFXLIB_MODULE_DIR}/crypto/nrf_oberon/include
)

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/eik_secure_partition
)

target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/eik_secure_partition/auto_generated/load_info_tfm_eik_secure_partition.c
)

add_dependencies(tfm_app_rot_partition_eik manifest_tool)

target_compile_options(tfm_app_rot_partition_eik PRIVATE
    "SHELL: -imacros ${ZEPHYR_AUTOCONF}"
)

target_compile_definitions(tfm_app_rot_partition_eik PRIVATE FP_CRYPTO_SPE=1)

target_link_libraries(tfm_app_rot_partition_eik
    PRIVATE
        platform_s
        tfm_sprt
        psa_crypto_config
        mbedcrypto_oberon_imported
)

target_link_libraries(tfm_spm PRIVATE tfm_app_rot_partition_eik)
target_link_libraries(tfm_partitions INTERFACE tfm_app_rot_partition_eik)
