#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
  main.c
  bt_adv_helper.c
  hids_helper.c
  bt_le_adv_prov_uuid16.c
  battery_module.c
  example_secure_partition_client.c
)
