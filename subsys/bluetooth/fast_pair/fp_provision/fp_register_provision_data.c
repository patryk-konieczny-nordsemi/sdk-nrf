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

#include "provision_entry.h"

/** Fast Pair Model ID length (24 bits). */
#define FP_MODEL_ID_LEN 3U

/** Anti-Spoofing private key length (256 bits). */
#define FP_ANTI_SPOOFING_KEY_LEN 32U

/** Anti-Spoofing key size in bits. */
#define FP_ANTI_SPOOFING_KEY_BITS (FP_ANTI_SPOOFING_KEY_LEN * CHAR_BIT)

/** RRAM write granularity on nRF54L: 128-bit (16-byte) wordline. */
#define FP_RRAM_WRITE_BLOCK 16U

/** RRAM storage for the base64 secret (padded to whole write blocks). */
#define FP_ANTI_SPOOFING_KEY_B64_STORAGE_LEN 64U

BUILD_ASSERT(FP_ANTI_SPOOFING_KEY_B64_STORAGE_LEN % FP_RRAM_WRITE_BLOCK == 0,
	     "Anti-Spoofing key storage must cover whole RRAM write blocks");

/** KMU key id for the Anti-Spoofing private key (RAW usage scheme). */
#define FP_KMU_KEY_ID                                                                                \
	PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, CONFIG_FP_PROVISION_KMU_SLOT)

/** ITS uid for the Fast Pair Model ID. */
#define FP_ITS_MODEL_ID_UID ((psa_storage_uid_t)CONFIG_FP_PROVISION_ITS_ID)

/** KMU lifetime for a read-only CRACEN key. */
#define FP_KMU_KEY_LIFETIME                                                                            \
	PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(CRACEN_KEY_PERSISTENCE_READ_ONLY,               \
						       PSA_KEY_LOCATION_CRACEN_KMU)

/** Model ID in big-endian wire format (24 bits). */
static const uint8_t fp_model_id_data[FP_MODEL_ID_LEN] = {
	((CONFIG_FP_PROVISION_MODEL_ID >> 16) & 0xFF),
	((CONFIG_FP_PROVISION_MODEL_ID >> 8) & 0xFF),
	(CONFIG_FP_PROVISION_MODEL_ID & 0xFF),
};

/** Base64-encoded Anti-Spoofing key embedded from Kconfig (purged from RRAM after import). */
static const char fp_anti_spoofing_key_b64[FP_ANTI_SPOOFING_KEY_B64_STORAGE_LEN]
	__aligned(FP_RRAM_WRITE_BLOCK) = CONFIG_FP_PROVISION_ANTI_SPOOFING_KEY;

REGISTER_PROVISION_KMU_ENTRY_SECRET(fp_anti_spoofing_key, fp_anti_spoofing_key_b64,
				    sizeof(CONFIG_FP_PROVISION_ANTI_SPOOFING_KEY),
				    sizeof(fp_anti_spoofing_key_b64),
				    FP_ANTI_SPOOFING_KEY_BITS, PROVISION_DATA_FORMAT_BASE64,
				    FP_KMU_KEY_ID,
				    PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1),
				    FP_KMU_KEY_LIFETIME, PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH);

REGISTER_PROVISION_ITS_ENTRY(fp_model_id, fp_model_id_data, sizeof(fp_model_id_data),
			     PROVISION_DATA_FORMAT_RAW, FP_ITS_MODEL_ID_UID,
			     PSA_STORAGE_FLAG_WRITE_ONCE);
