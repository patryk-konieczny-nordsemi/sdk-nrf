/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "eik_operations_core.h"
#include "fp_crypto.h"
#include <string.h>

bool eik_core_hash_compare(const uint8_t *eik, const uint8_t *eik_hash, const uint8_t *random_nonce)
{
	int err;
	uint8_t eik_hash_local[FP_CRYPTO_SHA256_HASH_LEN];
	uint8_t hash_input[FP_FHN_STATE_EIK_LEN + BEACON_ACTIONS_RANDOM_NONCE_LEN];

	/* Calculate: (Ephemeral Identity Key || random_nonce) */
	memcpy(hash_input,
		eik,
		FP_FHN_STATE_EIK_LEN);

	memcpy(hash_input + FP_FHN_STATE_EIK_LEN,
	       random_nonce,
	       BEACON_ACTIONS_RANDOM_NONCE_LEN);

	/* Generate local version of EIK Hash. */
	err = fp_crypto_sha256(eik_hash_local, hash_input, sizeof(hash_input));
	if (err) {
		return false;
	}

	return !memcmp(eik_hash_local, eik_hash, EPHEMERAL_IDENTITY_KEY_REQ_EIK_HASH_LEN);
}

int eik_core_eid_encode(const uint8_t *eik, uint32_t fhn_clock, const uint8_t *eid_seed_buf_data, const uint8_t *fhn_eid, const uint8_t *fhn_frame_hashed_flags_xor_operand)
{
	int err;
	uint8_t encrypted_eid_seed[FP_CRYPTO_AES256_BLOCK_LEN];
	const uint8_t uninitialized_eid[FP_FHN_STATE_EID_LEN] = {};
	uint8_t secp_mod_res[SECP_MOD_RES_LEN];
	uint8_t mod_res_hash[FP_CRYPTO_SHA256_HASH_LEN];

	/* Clear the K lowest bits in the clock value. */
	fhn_clock &= ~BIT_MASK(FHN_EID_SEED_ROT_PERIOD_EXP);

	/* Check if the EID seed or EIK has changed since the last call. */
	if (memcmp(fhn_eid, uninitialized_eid, sizeof(uninitialized_eid)) != 0) {
		if (fhn_clock == fhn_eid_clock_checkpoint) {
			return 0;
		}
	}
	fhn_eid_clock_checkpoint = fhn_clock;

	/* Encrypt the EID seed data with the Ephemeral Identity Key
	 * using the AES-ECB-256 scheme.
	 */
	err = fp_crypto_aes256_ecb_encrypt(encrypted_eid_seed, eid_seed_buf_data, eik);
	if (err) {
		return err;
	}

	/* Calculate the EID as the x coordinate of a point on the elliptic curve. */
	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_ECC_SECP160R1)) {
		err = fp_crypto_ecc_secp160r1_calculate(fhn_eid,
							secp_mod_res,
							encrypted_eid_seed,
							sizeof(encrypted_eid_seed));
		if (err) {
			return err;
		}
	} else if (IS_ENABLED(CONFIG_BT_FAST_PAIR_FHN_ECC_SECP256R1)) {
		err = fp_crypto_ecc_secp256r1_calculate(fhn_eid,
							secp_mod_res,
							encrypted_eid_seed,
							sizeof(encrypted_eid_seed));
		if (err) {
			return err;
		}
	} else {
		__ASSERT(0, "ECC selection not supported");
	}

	/* Calculate the XOR operand for the Hashed Flags bitmask. */
	err = fp_crypto_sha256(mod_res_hash, secp_mod_res, sizeof(secp_mod_res));
	if (err) {
		return err;
	}

	*fhn_frame_hashed_flags_xor_operand = mod_res_hash[sizeof(mod_res_hash) - 1];

	return 0;
}

int eik_core_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key, const uint8_t *eik)
{
	err = fp_crypto_aes128_ecb_decrypt(eik, encrypted_eik, account_key);
	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = fp_crypto_aes128_ecb_decrypt(eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   encrypted_eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   aaccount_key);
	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

int eik_core_get_encrypted(const uint8_t *eik, const uint8_t *owner_account_key, const uint8_t *encrypted_eik)
{
	err = fp_crypto_aes128_ecb_encrypt(encrypted_eik, eik, owner_account_key);
	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	err = fp_crypto_aes128_ecb_encrypt(encrypted_eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   owner_account_key);
	if (err) {
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	return 0;
}

int eik_core_derive_key(const uint8_t *eik, uint8_t seed_end_byte, uint8_t *eik_derived_key, size_t eik_derived_key_len)
{
	int err;
	uint8_t hash_input[FP_FHN_STATE_EIK_LEN + 1];
	uint8_t eik_derived_key_full[FP_CRYPTO_SHA256_HASH_LEN];

	memcpy(hash_input, eik, FP_FHN_STATE_EIK_LEN);

	hash_input[FP_FHN_STATE_EIK_LEN] = seed_end_byte;

	err = fp_crypto_sha256(eik_derived_key_full, hash_input, sizeof(hash_input));
	if (err) {
		return err;
	}
	
	memcpy(eik_derived_key, eik_derived_key_full, eik_derived_key_len);
	return 0;
}