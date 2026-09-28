#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Fast Pair provisioner test")

if(CONFIG_PROVISIONER)
  target_sources(app PRIVATE provisioner/provisioner_main.c)
elseif(CONFIG_TEST_BT_FAST_PAIR_PROVISIONED)
  target_sources(app PRIVATE src/main_prov.c)
else()
  target_sources(app PRIVATE src/main_unprov.c)
endif()
