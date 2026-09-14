#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_BT_FAST_PAIR_FHN_STATE)
  if(CONFIG_BT_FAST_PAIR_FHN_EIK_SPE)
    target_sources(fhn PRIVATE
      fp_fhn_eik_operations_spe_client.c
      fp_fhn_eik_spe_storage.c
    )
    target_include_directories(fhn PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/../../fp_storage/include_priv
    )
  else()
    target_sources(fhn PRIVATE
      fp_fhn_eik_operations.c
      fp_fhn_eik_operations_common.c
    )
  endif()
endif()

if(CONFIG_BT_FAST_PAIR_FHN_EIK_SPE)
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
endif()

target_include_directories(fhn PRIVATE include)
target_include_directories(fhn PRIVATE include_priv)
