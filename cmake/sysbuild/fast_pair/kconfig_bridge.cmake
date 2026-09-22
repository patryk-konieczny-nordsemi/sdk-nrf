#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Bridge sysbuild Fast Pair Kconfig (SB_CONFIG_*) to app Kconfig (CONFIG_*)
# on the default image. Only applies when secure-storage provisioning is used.

if(NOT SB_CONFIG_BT_FAST_PAIR_CREDENTIALS_PRESENT AND SB_CONFIG_BT_FAST_PAIR_RUNTIME_PROVISIONER)
  message(FATAL_ERROR
    "SB_CONFIG_BT_FAST_PAIR_RUNTIME_PROVISIONER is enabled but Fast Pair "
    "credentials are missing. Set SB_CONFIG_BT_FAST_PAIR_MODEL_ID (not "
    "0x1000000) and SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY.")
endif()

set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE
  SB_CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE)

# ----------------------------------------------------------------------------
# Secure Storage Configuration
# KMU/ITS slot values: sysbuild/Kconfig.bt_fast_pair (SB_CONFIG_BT_FAST_PAIR_*).
# ----------------------------------------------------------------------------
if(SB_CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE)
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT
    ${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT})
  set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_MODEL_ID_ITS_ID
    ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID_ITS_ID})

  # Provisioner only configuration (Anti-Spoofing Key and Model ID values)
  if(SB_CONFIG_BT_FAST_PAIR_RUNTIME_PROVISIONER)
    set_config_bool(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_RUNTIME_PROVISIONER y)

    set_config_int(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_MODEL_ID
      ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
    set_config_string(${DEFAULT_IMAGE} CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY
      "${SB_CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY}")
  endif()
endif()
