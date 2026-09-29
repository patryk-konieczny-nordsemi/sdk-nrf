/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/internal_trusted_storage.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>

#include <zephyr/ztest.h>

#define FP_KMU_KEY_ID										\
	PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW,		        \
					CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT)

#define FP_ITS_MODEL_ID_UID ((psa_storage_uid_t)CONFIG_BT_FAST_PAIR_MODEL_ID_ITS_ID)

ZTEST(fp_provisioner_pre_prov, test_ask_not_present)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;

	zassert_not_equal(psa_get_key_attributes(FP_KMU_KEY_ID, &attr), PSA_SUCCESS,
		"Anti-Spoofing key unexpectedly present in KMU");
}

ZTEST(fp_provisioner_pre_prov, test_mid_not_present)
{
	struct psa_storage_info_t info;

	zassert_not_equal(psa_its_get_info(FP_ITS_MODEL_ID_UID, &info), PSA_SUCCESS,
		"Model ID unexpectedly present in ITS");
}

ZTEST_SUITE(fp_provisioner_pre_prov, NULL, NULL, NULL, NULL, NULL);
