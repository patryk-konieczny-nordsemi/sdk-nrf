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

	printk("FP_PROV_TEST: provisioner image done (err %d)\n", err);

	return err;
}
