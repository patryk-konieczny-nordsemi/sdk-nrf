/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <limits.h>
#include <stdint.h>

#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>
#include <psa/crypto.h>
#include <psa/internal_trusted_storage.h>

#include <zephyr/sys/util.h>

#include <provisioner/provisioner.h>

#include "fp_registration_data.h"

/** Model ID in big-endian wire format (24 bits). */
static const uint8_t fp_model_id_data[FP_REG_DATA_MODEL_ID_LEN] = {
	((CONFIG_BT_FAST_PAIR_MODEL_ID >> 16) & 0xFF),
	((CONFIG_BT_FAST_PAIR_MODEL_ID >> 8) & 0xFF),
	(CONFIG_BT_FAST_PAIR_MODEL_ID & 0xFF),
};

/** Model ID provisioning payload */
static const struct provisioner_data fp_model_id_provision = {
	.data = fp_model_id_data,
	.payload_length = sizeof(fp_model_id_data),
	.format = PROVISIONER_DATA_FORMAT_RAW,
};

/** Model ID ITS settings */
static const struct provisioner_its_config fp_model_id_provision_conf = {
	.uid = CONFIG_BT_FAST_PAIR_MODEL_ID_ITS_ID,
	.create_flags = PSA_STORAGE_FLAG_WRITE_ONCE,
};

PROVISIONER_ENTRY_ITS_REGISTER(fp_model_id, fp_model_id_provision, fp_model_id_provision_conf);

/** Base64-encoded Anti-Spoofing key embedded from Kconfig. */
static const char fp_anti_spoofing_key_b64[] = CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY;

/** Anti-Spoofing Private Key provisioning payload - encoded in Base64 */
static const struct provisioner_data fp_anti_spoofing_key_provision = {
	.data = fp_anti_spoofing_key_b64,
	.payload_length = sizeof(fp_anti_spoofing_key_b64),
	.format = PROVISIONER_DATA_FORMAT_BASE64,
};

/** Anti-Spoofing Private Key KMU settings */
static const struct provisioner_kmu_config fp_anti_spoofing_key_provision_conf = {
	.key_bits = (FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN * CHAR_BIT),
	.id = PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_RAW,
		CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT),
	.type = PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1),
	.lifetime = PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
		CRACEN_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN_KMU),
	.usage_flags = PSA_KEY_USAGE_DERIVE,
	.alg = PSA_ALG_ECDH,
};

PROVISIONER_ENTRY_KMU_REGISTER(
	fp_anti_spoofing_key, fp_anti_spoofing_key_provision, fp_anti_spoofing_key_provision_conf);
