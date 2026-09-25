#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Fast Pair EIK operations unit test")

set(NCS_FAST_PAIR_BASE ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair)
set(EIK_OPS_DIR ${NCS_FAST_PAIR_BASE}/fhn/fp_fhn_eik_operations)
set(EIK_SPE_DIR ${EIK_OPS_DIR}/eik_secure_partition)

target_sources(app PRIVATE src/main.c)

add_subdirectory(${NCS_FAST_PAIR_BASE}/fp_crypto fp_crypto)
target_link_libraries(app PRIVATE fp_crypto)

add_subdirectory(${NCS_FAST_PAIR_BASE}/fp_storage fp_storage)
target_link_libraries(app PRIVATE fp_storage)

target_include_directories(app PRIVATE
  ${NCS_FAST_PAIR_BASE}/fhn/include_priv
  ${EIK_OPS_DIR}/include
  ${EIK_OPS_DIR}/include_priv
)

if(CONFIG_BT_FAST_PAIR_FHN_EIK)
  if(CONFIG_BT_FAST_PAIR_FHN_EIK_SPE)
    target_sources(app PRIVATE
      ${EIK_OPS_DIR}/fp_fhn_eik_operations_spe_client.c
      ${EIK_OPS_DIR}/fp_fhn_eik_spe_storage.c
    )
    target_include_directories(app PRIVATE
      ${NCS_FAST_PAIR_BASE}/fp_storage/include_priv
    )

    set(EIK_SECURE_PARTITION_DIR
    ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fhn/fp_fhn_eik_operations/eik_secure_partition)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/eik_secure_partition)

    set(EIK_SECURE_PARTITION_YAML
      ${CMAKE_CURRENT_BINARY_DIR}/eik_secure_partition/tfm_eik_secure_partition.yaml)

    set(EIK_SECURE_PARTITION_TFM_OUT
      ${CMAKE_BINARY_DIR}/tfm/eik_secure_partition)

    configure_file(
      ${EIK_SECURE_PARTITION_DIR}/tfm_eik_secure_partition.yaml.in
      ${EIK_SECURE_PARTITION_YAML}
    )

    configure_file(
      ${EIK_SECURE_PARTITION_DIR}/tfm_manifest_list.yaml.in
      ${CMAKE_CURRENT_BINARY_DIR}/eik_secure_partition/tfm_manifest_list.yaml
    )

    set_property(TARGET zephyr_property_target
      APPEND PROPERTY TFM_CMAKE_OPTIONS
      -DTFM_EXTRA_MANIFEST_LIST_FILES=${CMAKE_CURRENT_BINARY_DIR}/eik_secure_partition/tfm_manifest_list.yaml
      -DTFM_EXTRA_PARTITION_PATHS=${EIK_SECURE_PARTITION_DIR}
      -DZEPHYR_NRFXLIB_MODULE_DIR=${ZEPHYR_NRFXLIB_MODULE_DIR}
      -DZEPHYR_AUTOCONF=${AUTOCONF_H}
      -DCONFIG_BT_FAST_PAIR_CRYPTO_OBERON=${CONFIG_BT_FAST_PAIR_CRYPTO_OBERON}
      -DCONFIG_BT_FAST_PAIR_CRYPTO_PSA=${CONFIG_BT_FAST_PAIR_CRYPTO_PSA}
      -DCONFIG_BT_FAST_PAIR_CRYPTO_CUSTOM=${CONFIG_BT_FAST_PAIR_CRYPTO_CUSTOM}
      -DCONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_OFF=${CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_OFF}
      -DCONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_ERR=${CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_ERR}
      -DCONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_WRN=${CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_WRN}
      -DCONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_INF=${CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_INF}
      -DCONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_DBG=${CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_DBG}
      -DCONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_DEF=${CONFIG_BT_FAST_PAIR_FHN_EIK_SPE_LOG_LEVEL_DEF}
    )
  else()
    target_sources(app PRIVATE
      ${EIK_OPS_DIR}/fp_fhn_eik_operations_common.c
      ${EIK_OPS_DIR}/fp_fhn_eik_operations.c
    )
  endif()
endif()
