/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>

#include <zephyr/sys/util.h>

#include "psa/client.h"
#include "psa_manifest/sid.h"

#include "eik_operations.h"
#include "eik_operations_core.h"
#include "eik_secure_ipc.h"
#include "fp_fhn_lengths.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fp_fhn_eik_operations, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

int eik_delete(void)
{
	psa_status_t status;

	struct eik_secure_req req = {
		.op = EIK_SECURE_OP_DELETE,
	};

	psa_invec in_vec[] = {
		{ &req, sizeof(req) },
	};

	status = psa_call(TFM_EIK_SECURE_HANDLE, PSA_IPC_CALL,
			  in_vec, ARRAY_SIZE(in_vec),
			  NULL, 0);

	if (status != PSA_SUCCESS) {
		LOG_ERR("EIK operations: delete failed [ERR: %d]", status);
		return -EIO;
	}

	return 0;
}

int eik_is_provisioned(void)
{
	int result = -EIO;
	psa_status_t status;

	struct eik_secure_req req = {
		.op = EIK_SECURE_OP_IS_PROVISIONED,
	};

	psa_invec in_vec[] = {
		{ &req, sizeof(req) },
	};

	psa_outvec out_vec[] = {
		{ &result, sizeof(result) },
	};

	status = psa_call(TFM_EIK_SECURE_HANDLE, PSA_IPC_CALL,
			  in_vec, ARRAY_SIZE(in_vec),
			  out_vec, ARRAY_SIZE(out_vec));

	if (status != PSA_SUCCESS) {
		LOG_ERR("EIK operations: provision check failed [ERR: %d]", status);
		return -EIO;
	}

	return result;
}

bool eik_hash_compare(const uint8_t *eik_hash, const uint8_t *random_nonce)
{
	bool result = false;
	psa_status_t status;

	struct eik_secure_req req = {
		.op = EIK_SECURE_OP_HASH_COMPARE,
	};

	psa_invec in_vec[] = {
		{ &req, sizeof(req) },
		{ eik_hash, EIK_HASH_COMPARE_LEN },
		{ random_nonce, EIK_RANDOM_NONCE_LEN },
	};

	psa_outvec out_vec[] = {
		{ &result, sizeof(result) },
	};

	status = psa_call(TFM_EIK_SECURE_HANDLE, PSA_IPC_CALL,
			  in_vec, ARRAY_SIZE(in_vec),
			  out_vec, ARRAY_SIZE(out_vec));

	if (status != PSA_SUCCESS) {
		LOG_ERR("EIK operations: hash compare failed [ERR: %d]", status);
		return false;
	}

	return result;
}

int eik_eid_encode(const uint8_t *eid_seed_buf_data, uint8_t *fhn_eid,
		   uint8_t *fhn_frame_hashed_flags_xor_operand)
{
	psa_status_t status;

	struct eik_secure_req req = {
		.op = EIK_SECURE_OP_EID_ENCODE,
	};

	psa_invec in_vec[] = {
		{ &req, sizeof(req) },
		{ eid_seed_buf_data, FP_FHN_EID_SEED_LEN },
	};

	psa_outvec out_vec[] = {
		{ fhn_eid, FP_FHN_STATE_EID_LEN },
		{ fhn_frame_hashed_flags_xor_operand, FP_FHN_FRAME_HASHED_FLAGS_XOR_LEN },
	};

	status = psa_call(TFM_EIK_SECURE_HANDLE, PSA_IPC_CALL,
			  in_vec, ARRAY_SIZE(in_vec),
			  out_vec, ARRAY_SIZE(out_vec));

	if (status != PSA_SUCCESS) {
		LOG_ERR("EIK operations: EID encode failed [ERR: %d]", status);
		return -EIO;
	}

	return 0;
}

int eik_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key)
{
	psa_status_t status;

	struct eik_secure_req req = {
		.op = EIK_SECURE_OP_PROVISION_ENCRYPTED,
	};

	psa_invec in_vec[] = {
		{ &req, sizeof(req) },
		{ encrypted_eik, FP_FHN_ENCRYPTED_EIK_LEN },
		{ account_key, FP_FHN_ACCOUNT_KEY_LEN },
	};

	status = psa_call(TFM_EIK_SECURE_HANDLE, PSA_IPC_CALL,
			  in_vec, ARRAY_SIZE(in_vec),
			  NULL, 0);

	if (status != PSA_SUCCESS) {
		LOG_ERR("EIK operations: EIK decryption and provisioning failed [ERR: %d]", status);
		return -EIO;
	}

	return 0;
}

int eik_get_encrypted(const uint8_t *owner_account_key, uint8_t *encrypted_eik)
{
	psa_status_t status;

	struct eik_secure_req req = {
		.op = EIK_SECURE_OP_GET_ENCRYPTED,
	};

	psa_invec in_vec[] = {
		{ &req, sizeof(req) },
		{ owner_account_key, FP_FHN_ACCOUNT_KEY_LEN },
	};

	psa_outvec out_vec[] = {
		{ encrypted_eik, FP_FHN_ENCRYPTED_EIK_LEN },
	};

	status = psa_call(TFM_EIK_SECURE_HANDLE, PSA_IPC_CALL,
			  in_vec, ARRAY_SIZE(in_vec),
			  out_vec, ARRAY_SIZE(out_vec));

	if (status != PSA_SUCCESS) {
		LOG_ERR("EIK operations: EIK encryption failed [ERR: %d]", status);
		return -EIO;
	}

	return 0;
}

int eik_derive_key(uint8_t seed_end_byte, uint8_t *eik_derived_key, size_t eik_derived_key_len)
{
	psa_status_t status;

	struct eik_secure_req req = {
		.op = EIK_SECURE_OP_DERIVE_KEY,
	};

	if (eik_derived_key_len == 0 || eik_derived_key_len > FP_FHN_EIK_DERIVED_KEY_MAX_LEN) {
		return -EINVAL;
	}

	psa_invec in_vec[] = {
		{ &req, sizeof(req) },
		{ &seed_end_byte, sizeof(seed_end_byte) },
	};

	psa_outvec out_vec[] = {
		{ eik_derived_key, eik_derived_key_len },
	};

	status = psa_call(TFM_EIK_SECURE_HANDLE, PSA_IPC_CALL,
		in_vec, ARRAY_SIZE(in_vec),
		out_vec, ARRAY_SIZE(out_vec));

	if (status != PSA_SUCCESS) {
		LOG_ERR("EIK operations: EIK key derive failed [ERR: %d]", status);
		return -EIO;
	}

	return 0;
}
