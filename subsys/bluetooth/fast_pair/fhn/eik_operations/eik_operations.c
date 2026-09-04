/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "eik_operations.h"
#include "eik_operations_core.h"
#include "fp_storage_eik.h"
#include <stdbool.h>
#include <string.h>
 
int eik_delete()
{
	return fp_storage_eik_delete();
}


int eik_is_provisioned()
{
	return fp_storage_eik_is_provisioned();
}

bool eik_hash_compare(const uint8_t *eik_hash, const uint8_t *random_nonce){
	uint8_t eik[EPHEMERAL_IDENTITY_KEY_READ_RSP_EIK_LEN];
	int err;

	err = fp_storage_eik_is_provisioned();
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key hash compare:"
			" EIK not provisioned: %d", err);

		return false;
	}

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key hash compare:"
			" EIK read failed: %d", err);

		return false;
	}

	return eik_core_hash_compare(eik, eik_hash, random_nonce);
}

int eik_core_eid_encode(uint32_t fhn_clock, const uint8_t *eid_seed_buf_data, const uint8_t *fhn_eid, const uint8_t *fhn_frame_hashed_flags_xor_operand)
{
	uint8_t eik[EPHEMERAL_IDENTITY_KEY_READ_RSP_EIK_LEN];
	int err;

	err = fp_storage_eik_is_provisioned();
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key EID encode:"
			" EIK not provisioned: %d", err);

		return err;
	}

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity EID encode:"
			" EIK read failed: %d", err);

		return err;
	}

	err = eik_core_eid_encode(eik, fhn_clock, eid_seed_buf_data, fhn_eid, fhn_frame_hashed_flags_xor_operand);
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key EID encode:"
			" EID encode failed: %d", err);

		return err;
	}

	return 0;
}

int eik_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key)
{
	uint8_t eik[EPHEMERAL_IDENTITY_KEY_READ_RSP_EIK_LEN];
	int err;

	err = eik_core_provision_encrypted(encrypted_eik, account_key, eik);
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key Provision request:"
			" EIK provisioning failed: %d", err);
		return err;
	}

	err = fp_storage_eik_save(eik);
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key Provision request:"
			" EIK save failed: %d", err);
		return err;
	}

	return 0;
}

int eik_get_encrypted(const uint8_t *owner_account_key, const uint8_t *encrypted_eik){
	uint8_t eik[EPHEMERAL_IDENTITY_KEY_READ_RSP_EIK_LEN];
	int err;

	err = fp_storage_eik_is_provisioned();
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key get encrypted:"
			" EIK not provisioned: %d", err);

		return err;
	}

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key get encrypted:"
			" EIK read failed: %d", err);

		return err;
	}

	err = eik_core_get_encrypted(eik, owner_account_key, encrypted_eik);
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key get encrypted:"
			" EIK encryption failed: %d", err);

		return err;
	}

	return 0;
}

int eik_derive_key(uint8_t seed_end_byte, uint8_t *eik_derived_key, size_t eik_derived_key_len)
{
	uint8_t eik[EPHEMERAL_IDENTITY_KEY_READ_RSP_EIK_LEN];
	int err;

	err = fp_storage_eik_is_provisioned();
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key derive key:"
			" EIK not provisioned: %d", err);

		return err;
	}

	err = fp_storage_eik_get(eik);
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Key derive key:"
			" EIK read failed: %d", err);

		return err;
	}

	err = eik_core_derive_key(eik, seed_end_byte, eik_derived_key, eik_derived_key_len);
	if (err) {
		LOG_ERR("EIK operations: Ephemeral Identity Keyderive key:"
			" key generation failed: %d", err);

		return err;
	}

	return 0;
}