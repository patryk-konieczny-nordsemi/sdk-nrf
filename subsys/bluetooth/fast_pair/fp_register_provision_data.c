/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>

#include "provision_entry.h"

LOG_MODULE_DECLARE(fp_provision, LOG_LEVEL_INF);

static int fp_provision_register(void)
{
	LOG_INF("FP provision plugin");
	return 0;
}

GENERIC_PROVISION_CALLBACK_REGISTER(test_callback, fp_provision_register);
