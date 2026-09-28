/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

ZTEST(fp_provisioner_unprov, test_placeholder)
{
	printk("FP_PROV_TEST: unprovisioned app image started\n");
}

ZTEST_SUITE(fp_provisioner_unprov, NULL, NULL, NULL, NULL, NULL);
