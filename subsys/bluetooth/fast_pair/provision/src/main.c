/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <psa/protected_storage.h>
#include <psa/storage_common.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>

/* Anti-Spoofing key bytes decoded from base64 at build time. Provides
 * fp_provision_anti_spoofing_key[] and FP_PROVISION_ANTI_SPOOFING_KEY_LEN.
 */
#include "fp_provision_key.h"

LOG_MODULE_REGISTER(fp_provision, LOG_LEVEL_INF);

/* Fast Pair Model ID length (24 bits = 3 bytes). */
#define FP_MODEL_ID_LEN		3U

/* Handle of the Anti-Spoofing private key in the KMU (RAW usage scheme). */
#define FP_KMU_KEY_ID \
	PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, \
					CONFIG_FP_PROVISION_KMU_SLOT)

/* Protected Storage uid holding the Model ID. */
#define FP_PS_MODEL_ID_UID	((psa_storage_uid_t)CONFIG_FP_PROVISION_PS_ID)

static int provision_init(void)
{
	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed (err %d)", status);
		return -EIO;
	}

	return 0;
}

static int provision_anti_spoofing_key(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, FP_PROVISION_ANTI_SPOOFING_KEY_LEN * __CHAR_BIT__);
	psa_set_key_lifetime(&attr,
		PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
			CRACEN_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN_KMU));
	psa_set_key_id(&attr, FP_KMU_KEY_ID);

	status = psa_import_key(&attr, fp_provision_anti_spoofing_key,
			       FP_PROVISION_ANTI_SPOOFING_KEY_LEN, &key_id);
	psa_reset_key_attributes(&attr);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Anti-Spoofing key provisioning failed (err %d)", status);
		return -EIO;
	}

	LOG_INF("Anti-Spoofing key provisioned to KMU slot %d", CONFIG_FP_PROVISION_KMU_SLOT);

	return 0;
}

static int provision_model_id(void)
{
	uint8_t model_id[FP_MODEL_ID_LEN];
	psa_status_t status;

	sys_put_be24(CONFIG_FP_PROVISION_MODEL_ID, model_id);

	status = psa_ps_set(FP_PS_MODEL_ID_UID, sizeof(model_id), model_id, PSA_STORAGE_FLAG_NONE);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Model ID provisioning failed (err %d)", status);
		return -EIO;
	}

	LOG_INF("Model ID 0x%06x provisioned to Protected Storage (uid %u)",
		CONFIG_FP_PROVISION_MODEL_ID, (unsigned int)FP_PS_MODEL_ID_UID);

	return 0;
}

int main(void)
{
	LOG_INF("Fast Pair provisioner started");

	if (provision_init()) {
		return 0;
	}

	if (provision_anti_spoofing_key()) {
		return 0;
	}

	if (provision_model_id()) {
		return 0;
	}

	LOG_INF("Fast Pair provisioning completed successfully");

	return 0;
}
