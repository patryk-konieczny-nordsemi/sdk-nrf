/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>

ZTEST(fp_provisioner_prov, test_placeholder)
{
	printk("FP_PROV_TEST: provisioned app image started\n");
}

ZTEST_SUITE(fp_provisioner_prov, NULL, NULL, NULL, NULL, NULL);
