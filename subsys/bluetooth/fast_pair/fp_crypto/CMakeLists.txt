#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fp_crypto)
add_dependencies(fp_crypto zephyr_generated_headers)

if(CONFIG_BT_FAST_PAIR_CRYPTO)
  target_sources(fp_crypto PRIVATE
    fp_crypto_common.c
  )
endif()
if(CONFIG_BT_FAST_PAIR_CRYPTO_OBERON)
  target_sources(fp_crypto PRIVATE fp_crypto_oberon.c)
  target_link_libraries(fp_crypto PRIVATE nrfxlib_crypto)
endif()
if(CONFIG_BT_FAST_PAIR_CRYPTO_PSA)
  target_sources(fp_crypto PRIVATE fp_crypto_psa.c)

  if(CONFIG_BUILD_WITH_TFM)
    set(FP_CRYPTO_PARTITION_DIR
      ${ZEPHYR_NRF_MODULE_DIR}/subsys/bluetooth/fast_pair/fp_crypto/fp_crypto_partition)

    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fp_crypto_partition)

    set(FP_CRYPTO_PARTITION_YAML
      "${CMAKE_CURRENT_BINARY_DIR}/fp_crypto_partition/tfm_fp_crypto_partition.yaml")

    set(FP_CRYPTO_PARTITION_TFM_OUT
      "${CMAKE_BINARY_DIR}/tfm/fp_crypto_partition")

    configure_file(
      ${FP_CRYPTO_PARTITION_DIR}/tfm_fp_crypto_partition.yaml.in
      ${FP_CRYPTO_PARTITION_YAML}
    )

    configure_file(
      ${FP_CRYPTO_PARTITION_DIR}/tfm_manifest_list.yaml.in
      ${CMAKE_CURRENT_BINARY_DIR}/fp_crypto_partition/tfm_manifest_list.yaml
    )

    set_property(TARGET zephyr_property_target 
      APPEND PROPERTY TFM_CMAKE_OPTIONS
      -DTFM_EXTRA_MANIFEST_LIST_FILES=${CMAKE_CURRENT_BINARY_DIR}/fp_crypto_partition/tfm_manifest_list.yaml
      -DTFM_EXTRA_PARTITION_PATHS=${FP_CRYPTO_PARTITION_DIR}
      -DZEPHYR_NRFXLIB_MODULE_DIR=${ZEPHYR_NRFXLIB_MODULE_DIR}
    )

    target_sources(fp_crypto PRIVATE fp_crypto_spe_client.c)
  endif()
endif()

target_include_directories(fp_crypto PUBLIC include)
target_include_directories(fp_crypto PUBLIC common)
target_include_directories(fp_crypto PUBLIC ../include/common)
