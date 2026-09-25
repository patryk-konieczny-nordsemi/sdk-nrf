#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

add_library(tfm_app_rot_partition_eik STATIC)

set(EIK_OPERATIONS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(FP_CRYPTO_DIR ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fp_crypto)

set(EIK_SPE_FP_CRYPTO_SRCS
    tfm_eik_secure_partition.c
    ${EIK_OPERATIONS_DIR}/fp_fhn_eik_operations_common.c
    ${CMAKE_BINARY_DIR}/eik_secure_partition/auto_generated/intermedia_tfm_eik_secure_partition.c
)

if(CONFIG_BT_FAST_PAIR_CRYPTO_OBERON)
    list(APPEND EIK_SPE_FP_CRYPTO_SRCS
        ${FP_CRYPTO_DIR}/fp_crypto_oberon.c
        ${FP_CRYPTO_DIR}/fp_crypto_oberon_secp.c
    )
elseif(CONFIG_BT_FAST_PAIR_CRYPTO_PSA)
    list(APPEND EIK_SPE_FP_CRYPTO_SRCS
        ${FP_CRYPTO_DIR}/fp_crypto_psa.c
    )
elseif(CONFIG_BT_FAST_PAIR_CRYPTO_CUSTOM)
    list(APPEND EIK_SPE_FP_CRYPTO_SRCS
        ${FP_CRYPTO_DIR}/fp_crypto_psa.c
        ${FP_CRYPTO_DIR}/fp_crypto_oberon_secp.c
    )
else()
    message(FATAL_ERROR
        "EIK secure partition requires FP crypto backend")
endif()

target_sources(tfm_app_rot_partition_eik
    PRIVATE
        ${EIK_SPE_FP_CRYPTO_SRCS}
)

target_include_directories(tfm_app_rot_partition_eik
    PRIVATE
        ${EIK_OPERATIONS_DIR}/include_priv
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fhn/include_priv
        ${FP_CRYPTO_DIR}/include
        ${FP_CRYPTO_DIR}/common
        ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/include/common
        ${CMAKE_BINARY_DIR}/eik_secure_partition
)

if(CONFIG_BT_FAST_PAIR_CRYPTO_OBERON OR CONFIG_BT_FAST_PAIR_CRYPTO_CUSTOM)
    target_include_directories(tfm_app_rot_partition_eik
        PRIVATE
            ${ZEPHYR_NRFXLIB_MODULE_DIR}/crypto/nrf_oberon/include
    )
endif()

target_include_directories(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/eik_secure_partition
)

target_sources(tfm_partitions
    INTERFACE
        ${CMAKE_BINARY_DIR}/eik_secure_partition/auto_generated/load_info_tfm_eik_secure_partition.c
)

add_dependencies(tfm_app_rot_partition_eik manifest_tool)

if(CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_DBG)
    set(EIK_SPE_LOG_LEVEL LOG_LEVEL_VERBOSE)
elseif(CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_INF)
    set(EIK_SPE_LOG_LEVEL LOG_LEVEL_INFO)
elseif(CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_WRN)
    set(EIK_SPE_LOG_LEVEL LOG_LEVEL_WARNING)
elseif(CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_ERR)
    set(EIK_SPE_LOG_LEVEL LOG_LEVEL_ERROR)
elseif(CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_OFF)
    set(EIK_SPE_LOG_LEVEL LOG_LEVEL_NONE)
elseif(CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_DEF)
    set(EIK_SPE_LOG_LEVEL ${TFM_PARTITION_LOG_LEVEL})
else()
    message(FATAL_ERROR
        "EIK secure partition log level not forwarded to TF-M build")
endif()

target_compile_options(tfm_app_rot_partition_eik PRIVATE
    "SHELL: -imacros ${ZEPHYR_AUTOCONF}"
    -ULOG_LEVEL
    -DLOG_LEVEL=${EIK_SPE_LOG_LEVEL}
)

target_link_libraries(tfm_app_rot_partition_eik
    PRIVATE
        platform_s
        tfm_sprt
        psa_crypto_config
        mbedcrypto_oberon_imported
)

target_link_libraries(tfm_spm PRIVATE tfm_app_rot_partition_eik)
target_link_libraries(tfm_partitions INTERFACE tfm_app_rot_partition_eik)
