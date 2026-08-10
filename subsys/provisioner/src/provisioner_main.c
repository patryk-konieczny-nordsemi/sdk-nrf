/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <provisioner/provisioner.h>

#include <zephyr/kernel.h>

int main(void)
{
	return provision_data();
}
