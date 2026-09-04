/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "eik_operations.h"
#include "eik_operations_core.h"
#include "fp_storage_eik.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fhn_eik_operations, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

int eik_delete(void)
{
	return fp_storage_eik_delete();
}

int eik_is_provisioned(void)
{
	return fp_storage_eik_is_provisioned();
}

bool eik_hash_compare(const uint8_t *eik_hash, const uint8_t *random_nonce)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: hash compare: EIK read failed: %d", err);
		return false;
	}

	return eik_core_hash_compare(eik, eik_hash, random_nonce);
}

int eik_eid_encode(const uint8_t *eid_seed_buf_data, uint8_t *fhn_eid,
		   uint8_t *fhn_frame_hashed_flags_xor_operand)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: EID encode: EIK read failed: %d", err);
		return err;
	}

	err = eik_core_eid_encode(eik, eid_seed_buf_data, fhn_eid,
				  fhn_frame_hashed_flags_xor_operand);
	if (err) {
		LOG_ERR("EIK operations: EID encode failed: %d", err);
		return err;
	}

	return 0;
}

int eik_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;

	err = eik_core_provision_encrypted(encrypted_eik, account_key, eik);
	if (err) {
		LOG_ERR("EIK operations: provisioning failed: %d", err);
		return err;
	}

	err = fp_storage_eik_save(eik);
	if (err) {
		LOG_ERR("EIK operations: EIK save failed: %d", err);
		return err;
	}

	return 0;
}

int eik_get_encrypted(const uint8_t *owner_account_key, uint8_t *encrypted_eik)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: get encrypted: EIK read failed: %d", err);
		return err;
	}

	err = eik_core_get_encrypted(eik, owner_account_key, encrypted_eik);
	if (err) {
		LOG_ERR("EIK operations: get encrypted: EIK encryption failed: %d", err);
		return err;
	}

	return 0;
}

int eik_derive_key(uint8_t seed_end_byte, uint8_t *eik_derived_key, size_t eik_derived_key_len)
{
	uint8_t eik[FP_STORAGE_EIK_LEN];
	int err;

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: derive key: EIK read failed: %d", err);
		return err;
	}

	err = eik_core_derive_key(eik, seed_end_byte, eik_derived_key, eik_derived_key_len);
	if (err) {
		LOG_ERR("EIK operations: derive key: key generation failed: %d", err);
		return err;
	}

	return 0;
}
