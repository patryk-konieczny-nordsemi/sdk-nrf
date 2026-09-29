/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <provisioner/provisioner.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
	int err;

	printk("FP_PROV_TEST: provisioner image started\n");

	err = provisioner_run();

	if (err != 0) {
		printk("FP_PROV_TEST: provisioner image done : error %d\n", err);
	} else {
		printk("FP_PROV_TEST: provisioner image done : success\n");
	}

	return err;
}
