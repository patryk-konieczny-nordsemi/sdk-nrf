#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_BT_FAST_PAIR_FHN_STATE OR CONFIG_BT_FAST_PAIR_FMDN_STATE)
  target_sources(fhn PRIVATE
    eik_operations.c
    eik_operations_core.c
  )
endif()

target_include_directories(fhn PRIVATE include)
target_include_directories(fhn PRIVATE include_priv)
