/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "eik_operations_core.h"
#include "fp_crypto.h"
#include "fp_fhn_state.h"

#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#define EIK_HASH_COMPARE_LEN 8U
#define EIK_RANDOM_NONCE_LEN CONFIG_BT_FAST_PAIR_FHN_RANDOM_NONCE_LEN
#define SECP_MOD_RES_LEN     FP_FHN_STATE_EID_LEN

bool eik_core_hash_compare(const uint8_t *eik, const uint8_t *eik_hash, const uint8_t *random_nonce)
{
	int err;
	uint8_t eik_hash_local[FP_CRYPTO_SHA256_HASH_LEN];
	uint8_t hash_input[FP_FHN_STATE_EIK_LEN + EIK_RANDOM_NONCE_LEN];

	/* Calculate: (Ephemeral Identity Key || random_nonce) */
	memcpy(hash_input, eik, FP_FHN_STATE_EIK_LEN);
	memcpy(hash_input + FP_FHN_STATE_EIK_LEN, random_nonce, EIK_RANDOM_NONCE_LEN);

	err = fp_crypto_sha256(eik_hash_local, hash_input, sizeof(hash_input));
	if (err) {
		return false;
	}

	return !memcmp(eik_hash_local, eik_hash, EIK_HASH_COMPARE_LEN);
}

int eik_core_eid_encode(const uint8_t *eik, const uint8_t *eid_seed_buf_data,
			uint8_t *fhn_eid, uint8_t *fhn_frame_hashed_flags_xor_operand)
{
	int err;
	uint8_t encrypted_eid_seed[FP_CRYPTO_AES256_BLOCK_LEN];
	uint8_t secp_mod_res[SECP_MOD_RES_LEN];
	uint8_t mod_res_hash[FP_CRYPTO_SHA256_HASH_LEN];

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

int eik_core_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key,
				 uint8_t *eik)
{
	int err;

	err = fp_crypto_aes128_ecb_decrypt(eik, encrypted_eik, account_key);
	if (err) {
		return err;
	}

	err = fp_crypto_aes128_ecb_decrypt(eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   encrypted_eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   account_key);
	if (err) {
		return err;
	}

	return 0;
}

int eik_core_get_encrypted(const uint8_t *eik, const uint8_t *owner_account_key,
			   uint8_t *encrypted_eik)
{
	int err;

	err = fp_crypto_aes128_ecb_encrypt(encrypted_eik, eik, owner_account_key);
	if (err) {
		return err;
	}

	err = fp_crypto_aes128_ecb_encrypt(encrypted_eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   eik + FP_CRYPTO_AES128_BLOCK_LEN,
					   owner_account_key);
	if (err) {
		return err;
	}

	return 0;
}

int eik_core_derive_key(const uint8_t *eik, uint8_t seed_end_byte, uint8_t *eik_derived_key,
			size_t eik_derived_key_len)
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
