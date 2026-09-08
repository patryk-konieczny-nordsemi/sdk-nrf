#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_BT_FAST_PAIR_FHN_STATE)
  target_sources(fhn PRIVATE
    eik_operations_core.c
  )

  if(CONFIG_TFM_IPC AND CONFIG_BUILD_WITH_TFM)
    target_sources(fhn PRIVATE
      eik_operations_spe_client.c
    )
  else()
    target_sources(fhn PRIVATE
      eik_operations.c
    )
  endif()
endif()

if(CONFIG_TFM_IPC AND CONFIG_BUILD_WITH_TFM AND CONFIG_BT_FAST_PAIR_FHN_STATE)
  set(EIK_SECURE_PARTITION_DIR
    ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fhn/eik_operations/eik_secure_partition)

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
  )
endif()

target_include_directories(fhn PRIVATE include)
target_include_directories(fhn PRIVATE include_priv)
