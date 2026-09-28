/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/iterable_sections.h>
#include "fp_storage_manager.h"
#include "fp_fhn_eik_operations.h"

static int fp_fhn_eik_spe_reset_perform(void)
{
	return fp_fhn_eik_delete();
}

FP_STORAGE_MANAGER_MODULE_REGISTER(fp_fhn_eik_spe_storage_manager,
				   fp_fhn_eik_spe_reset_perform,
				   NULL, NULL);
