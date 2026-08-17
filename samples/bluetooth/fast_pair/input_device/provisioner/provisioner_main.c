/*
 * Copyright (c) 2022-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <provisioner/provisioner.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(provisioner_app, LOG_LEVEL_INF);

int main(void)
{
	int err;

	LOG_INF("Provisioner application starting");
	err = provisioner_run();
	if (err != 0) {
		LOG_ERR("Provisioning failed (err %d)", err);
	}

	return err;
}
