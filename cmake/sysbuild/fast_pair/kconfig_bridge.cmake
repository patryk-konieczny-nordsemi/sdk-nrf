#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Bridge sysbuild Fast Pair Kconfig (SB_CONFIG_*) to app Kconfig (CONFIG_*)
# on the default image. Only applies when secure-storage provisioning is used.

# ----------------------------------------------------------------------------
# main application (not provisioner)
# KMU/ITS slot values: Kconfig.sysbuild (SB_CONFIG_BT_FAST_PAIR_*).
# ----------------------------------------------------------------------------
if(NOT SB_CONFIG_BT_FAST_PAIR_RUNTIME_PROVISIONER AND SB_CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE y)

  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT
    ${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_MODEL_ID_ITS_ID
    ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID_ITS_ID})
endif()

# ----------------------------------------------------------------------------
# Provisioner
# ----------------------------------------------------------------------------
if(SB_CONFIG_BT_FAST_PAIR_RUNTIME_PROVISIONER)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_RUNTIME_PROVISIONER y)

  # configs required to access fp_register_provisioner_data.c to register data
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT y)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR y)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_CRYPTO_PSA y)

  # explicit selection of secure storage to disable "no fast pair partition" error
  set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE y)

  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_MODEL_ID
    ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
  set_config_string(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY
    "${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}")
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT
    ${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_MODEL_ID_ITS_ID
    ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID_ITS_ID})
endif()
