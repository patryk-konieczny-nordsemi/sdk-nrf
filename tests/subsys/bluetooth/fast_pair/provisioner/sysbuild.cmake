#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set_config_int(${DEFAULT_IMAGE} CONFIG_TEST_BT_FAST_PAIR_EXPECTED_MODEL_ID
  ${SB_CONFIG_BT_FAST_PAIR_MODEL_ID})
  
set_config_string(${DEFAULT_IMAGE} CONFIG_TEST_BT_FAST_PAIR_EXPECTED_PUB_KEY
  "${SB_CONFIG_TEST_BT_FAST_PAIR_EXPECTED_PUB_KEY}")
