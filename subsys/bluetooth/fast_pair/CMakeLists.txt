#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fast_pair)

zephyr_library_include_directories(include)
zephyr_library_include_directories(include/common)
# This directory is included to make a workaround of Android issue with sending old RPA address
# during Key-based Pairing write.
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_ADVERTISING fp_advertising.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_AUTH fp_auth.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_GATT_SERVICE fp_gatt_service.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_KEYS fp_keys.c)
if(CONFIG_BT_FAST_PAIR_REGISTRATION_DATA)
  if(CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE)
    zephyr_library_sources(fp_registration_data_secure_storage.c)
  elseif(CONFIG_BT_FAST_PAIR_PROVISION_PARTITION)
    zephyr_library_sources(fp_registration_data_partition.c)
  endif()
endif()
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_BATTERY fp_battery.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_STORAGE_INTEGRATION fp_storage_integration.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_BOND_MANAGER fp_bond_manager.c)

zephyr_linker_sources_ifdef(CONFIG_BT_FAST_PAIR_ACTIVATION SECTIONS fp_activation.ld)
if(CONFIG_BT_FAST_PAIR_ACTIVATION)
  zephyr_iterable_section(NAME fp_activation_module KVMA RAM_REGION GROUP RODATA_REGION)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_ACTIVATION fp_activation.c)

if(CONFIG_BT_FAST_PAIR_CRYPTO)
  add_subdirectory(fp_crypto)
  target_link_libraries(fast_pair PRIVATE fp_crypto)
endif()

if(CONFIG_BT_FAST_PAIR_STORAGE)
  add_subdirectory(fp_storage)
  target_link_libraries(fast_pair PRIVATE fp_storage)
endif()

if(CONFIG_BT_FAST_PAIR_FHN OR CONFIG_BT_FAST_PAIR_FMDN)
  add_subdirectory(fhn)
  target_link_libraries(fast_pair PRIVATE fhn)
endif()

# Emit a single combined -D per TF-M extra-partition option. TF-M treats
# TFM_EXTRA_MANIFEST_LIST_FILES/TFM_EXTRA_PARTITION_PATHS as lists, but repeated
# -D tokens on the command line override each other. The entries are joined with an
# escaped separator (\;) so the value survives the COMMAND_EXPAND_LISTS expansion of
# TFM_CMAKE_OPTIONS as a single argument and is parsed as a list by the TF-M CMake run.
if(CONFIG_BUILD_WITH_TFM)
  get_property(nrf_tfm_manifest_lists GLOBAL PROPERTY NRF_TFM_EXTRA_MANIFEST_LIST_FILES)
  get_property(nrf_tfm_partition_paths GLOBAL PROPERTY NRF_TFM_EXTRA_PARTITION_PATHS)

  if(nrf_tfm_manifest_lists)
    list(JOIN nrf_tfm_manifest_lists "\\;" nrf_tfm_manifest_lists_arg)
    list(JOIN nrf_tfm_partition_paths "\\;" nrf_tfm_partition_paths_arg)

    set_property(TARGET zephyr_property_target
      APPEND PROPERTY TFM_CMAKE_OPTIONS
      -DTFM_EXTRA_MANIFEST_LIST_FILES=${nrf_tfm_manifest_lists_arg}
      -DTFM_EXTRA_PARTITION_PATHS=${nrf_tfm_partition_paths_arg}
    )
  endif()
endif()

if(CONFIG_BT_FAST_PAIR_FMDN)
  zephyr_library_compile_options(
    -include ${CMAKE_CURRENT_SOURCE_DIR}/fhn/include_priv/fp_fhn_kconfig_compat.h)
endif()

add_subdirectory_ifdef(CONFIG_BT_FAST_PAIR_ADV_MANAGER adv_manager)

zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_RUNTIME_PROVISIONER
  fp_provisioner/fp_provisioner_register_data.c)
