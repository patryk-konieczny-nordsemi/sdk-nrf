/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <mbedtls/platform_util.h>

#include "fp_fhn_eik_operations.h"
#include "fp_fhn_eik_operations_common.h"
#include "fp_fhn_lengths.h"
#include "fp_storage_eik.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fhn_eik_operations, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

/* Verify if the length of the EIK is consistent with the storage module. */
BUILD_ASSERT(FP_FHN_STATE_EIK_LEN == FP_STORAGE_EIK_LEN);

int fp_fhn_eik_delete(void)
{
	return fp_storage_eik_delete();
}

int fp_fhn_eik_is_provisioned(void)
{
	return fp_storage_eik_is_provisioned();
}

bool fp_fhn_eik_hash_compare(const uint8_t *eik_hash, const uint8_t *random_nonce)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;
	bool res;

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: hash compare: EIK read failed: %d", err);
		return false;
	}

	res = fp_fhn_eik_common_hash_compare(eik, eik_hash, random_nonce);

	mbedtls_platform_zeroize(eik, sizeof(eik));
	return res;
}

int fp_fhn_eik_eid_encode(const uint8_t *eid_seed_buf_data, uint8_t *fhn_eid,
			  uint8_t *fhn_frame_hashed_flags_xor_operand)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: EID encode: EIK read failed: %d", err);
		return err;
	}

	err = fp_fhn_eik_common_eid_encode(eik, eid_seed_buf_data, fhn_eid,
					   fhn_frame_hashed_flags_xor_operand);
	mbedtls_platform_zeroize(eik, sizeof(eik));
	if (err) {
		LOG_ERR("EIK operations: EID encode failed: %d", err);
		return err;
	}

	return 0;
}

int fp_fhn_eik_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;

	err = fp_fhn_eik_common_decrypt(encrypted_eik, account_key, eik);
	if (err) {
		LOG_ERR("EIK operations: provisioning failed: %d", err);
		return err;
	}

	err = fp_storage_eik_save(eik);
	mbedtls_platform_zeroize(eik, sizeof(eik));
	if (err) {
		LOG_ERR("EIK operations: EIK save failed: %d", err);
		return err;
	}

	return 0;
}

int fp_fhn_eik_get_encrypted(const uint8_t *owner_account_key, uint8_t *encrypted_eik)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: get encrypted: EIK read failed: %d", err);
		return err;
	}

	err = fp_fhn_eik_common_encrypt(eik, owner_account_key, encrypted_eik);
	mbedtls_platform_zeroize(eik, sizeof(eik));
	if (err) {
		LOG_ERR("EIK operations: get encrypted: EIK encryption failed: %d", err);
		return err;
	}

	return 0;
}

int fp_fhn_eik_derive_key(uint8_t seed_end_byte, uint8_t *eik_derived_key,
			  size_t eik_derived_key_len)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;

	if (eik_derived_key_len == 0 || eik_derived_key_len > FP_FHN_EIK_DERIVED_KEY_MAX_LEN) {
		return -EINVAL;
	}

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: derive key: EIK read failed: %d", err);
		return err;
	}

	err = fp_fhn_eik_common_derive_key(eik, seed_end_byte, eik_derived_key,
					   eik_derived_key_len);
	mbedtls_platform_zeroize(eik, sizeof(eik));
	if (err) {
		LOG_ERR("EIK operations: derive key: key generation failed: %d", err);
		return err;
	}

	return 0;
}
