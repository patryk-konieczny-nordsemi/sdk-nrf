/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>

#include <psa/internal_trusted_storage.h>
#include <mbedtls/platform_util.h>
#include "psa/service.h"
#include "psa_manifest/tfm_eik_secure_partition.h"

#include "eik_operations_core.h"
#include "fp_fhn_lengths.h"
#include "eik_secure_ipc.h"

#include "tfm_log_unpriv.h"

#define EIK_ITS_UID ((psa_storage_uid_t)CONFIG_BT_FAST_PAIR_FHN_EIK_ITS_ID)

static psa_status_t tfm_eik_read_from_its(uint8_t *eik, size_t eik_len)
{
	size_t read_len;
	psa_status_t status;

	status = psa_its_get(EIK_ITS_UID, 0, eik_len, eik, &read_len);
	if (status != PSA_SUCCESS) {
		return status;
	}

	if (read_len != eik_len) {
		return PSA_ERROR_DATA_CORRUPT;
	}

	return PSA_SUCCESS;
}

static psa_status_t tfm_eik_delete(const psa_msg_t *msg)
{
	psa_status_t status;
	int result;

	if (msg->out_size[0] != sizeof(result)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	status = psa_its_remove(EIK_ITS_UID);
	switch (status) {
	case PSA_SUCCESS:
	case PSA_ERROR_DOES_NOT_EXIST:
		result = 0;
		status = PSA_SUCCESS;
		break;
	default:
		return status;
	}

	psa_write(msg->handle, 0, &result, sizeof(result));
	return PSA_SUCCESS;
}

static psa_status_t tfm_eik_is_provisioned(const psa_msg_t *msg)
{
	psa_status_t status;
	struct psa_storage_info_t info;
	int result;

	if (msg->out_size[0] != sizeof(result)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	status = psa_its_get_info(EIK_ITS_UID, &info);
	switch (status) {
	case PSA_SUCCESS:
		result = (info.size == FP_FHN_STATE_EIK_LEN) ? 1 : 0;
		break;
	case PSA_ERROR_DOES_NOT_EXIST:
		result = 0;
		status = PSA_SUCCESS;
		break;
	default:
		return status;
	}

	psa_write(msg->handle, 0, &result, sizeof(result));
	return status;
}

static psa_status_t tfm_eik_hash_compare(const psa_msg_t *msg)
{
	uint8_t eik[FP_FHN_STATE_EIK_LEN];
	uint8_t eik_hash[EIK_HASH_COMPARE_LEN];
	uint8_t random_nonce[EIK_RANDOM_NONCE_LEN];
	psa_status_t status;
	bool result;

	if (msg->in_size[1] != sizeof(eik_hash) ||
	    msg->in_size[2] != sizeof(random_nonce) ||
	    msg->out_size[0] != sizeof(result)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (psa_read(msg->handle, 1, eik_hash, sizeof(eik_hash)) != sizeof(eik_hash)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (psa_read(msg->handle, 2, random_nonce, sizeof(random_nonce)) != sizeof(random_nonce)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	status = tfm_eik_read_from_its(eik, sizeof(eik));
	if (status != PSA_SUCCESS) {
		return status;
	}

	result = eik_core_hash_compare(eik, eik_hash, random_nonce);
	mbedtls_platform_zeroize(eik, sizeof(eik));
	psa_write(msg->handle, 0, &result, sizeof(result));

	return PSA_SUCCESS;
}

static psa_status_t tfm_eik_eid_encode(const psa_msg_t *msg)
{
	uint8_t eik[FP_FHN_STATE_EIK_LEN];
	uint8_t eid_seed_buf_data[FP_FHN_EID_SEED_LEN];
	uint8_t fhn_eid[FP_FHN_STATE_EID_LEN];
	uint8_t fhn_frame_hashed_flags_xor_operand;
	psa_status_t status;
	int result;

	if (msg->in_size[1] != sizeof(eid_seed_buf_data) ||
	    msg->out_size[0] != sizeof(result) ||
	    msg->out_size[1] != sizeof(fhn_eid) ||
	    msg->out_size[2] != sizeof(fhn_frame_hashed_flags_xor_operand)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (psa_read(msg->handle, 1, eid_seed_buf_data, sizeof(eid_seed_buf_data)) !=
	    sizeof(eid_seed_buf_data)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	status = tfm_eik_read_from_its(eik, sizeof(eik));
	if (status != PSA_SUCCESS) {
		return status;
	}

	result = eik_core_eid_encode(eik, eid_seed_buf_data, fhn_eid,
				    &fhn_frame_hashed_flags_xor_operand);
	mbedtls_platform_zeroize(eik, sizeof(eik));
	if (result != 0) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	psa_write(msg->handle, 0, &result, sizeof(result));
	psa_write(msg->handle, 1, fhn_eid, sizeof(fhn_eid));
	psa_write(msg->handle, 2, &fhn_frame_hashed_flags_xor_operand,
		  sizeof(fhn_frame_hashed_flags_xor_operand));

	return PSA_SUCCESS;
}

static psa_status_t tfm_eik_provision_encrypted(const psa_msg_t *msg)
{
	uint8_t eik[FP_FHN_STATE_EIK_LEN];
	uint8_t encrypted_eik[FP_FHN_ENCRYPTED_EIK_LEN];
	uint8_t account_key[FP_FHN_ACCOUNT_KEY_LEN];
	psa_status_t status;
	int result;

	if (msg->in_size[1] != sizeof(encrypted_eik) ||
	    msg->in_size[2] != sizeof(account_key) ||
	    msg->out_size[0] != sizeof(result)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (psa_read(msg->handle, 1, encrypted_eik, sizeof(encrypted_eik)) !=
	    sizeof(encrypted_eik)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (psa_read(msg->handle, 2, account_key, sizeof(account_key)) !=
	    sizeof(account_key)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	result = eik_core_provision_encrypted(encrypted_eik, account_key, eik);
	if (result != 0) {
		mbedtls_platform_zeroize(eik, sizeof(eik));
		return PSA_ERROR_GENERIC_ERROR;
	}

	status = psa_its_set(EIK_ITS_UID, sizeof(eik), eik, PSA_STORAGE_FLAG_NONE);
	mbedtls_platform_zeroize(eik, sizeof(eik));
	if (status != PSA_SUCCESS) {
		return status;
	}

	result = 0;
	psa_write(msg->handle, 0, &result, sizeof(result));

	return PSA_SUCCESS;
}

static psa_status_t tfm_eik_get_encrypted(const psa_msg_t *msg)
{
	uint8_t eik[FP_FHN_STATE_EIK_LEN];
	uint8_t encrypted_eik[FP_FHN_ENCRYPTED_EIK_LEN];
	uint8_t owner_account_key[FP_FHN_ACCOUNT_KEY_LEN];
	psa_status_t status;
	int result;

	if (msg->in_size[1] != sizeof(owner_account_key) ||
	    msg->out_size[0] != sizeof(result) ||
	    msg->out_size[1] != sizeof(encrypted_eik)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (psa_read(msg->handle, 1, owner_account_key, sizeof(owner_account_key)) !=
	    sizeof(owner_account_key)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	status = tfm_eik_read_from_its(eik, sizeof(eik));
	if (status != PSA_SUCCESS) {
		return status;
	}

	result = eik_core_get_encrypted(eik, owner_account_key, encrypted_eik);
	mbedtls_platform_zeroize(eik, sizeof(eik));
	if (result != 0) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	psa_write(msg->handle, 0, &result, sizeof(result));
	psa_write(msg->handle, 1, encrypted_eik, sizeof(encrypted_eik));

	return PSA_SUCCESS;
}

static psa_status_t tfm_eik_derive_key(const psa_msg_t *msg)
{
	uint8_t eik[FP_FHN_STATE_EIK_LEN];
	uint8_t seed_end_byte;
	uint8_t eik_derived_key[EIK_DERIVED_KEY_MAX_LEN];
	size_t derive_len;
	psa_status_t status;
	int result;

	if (msg->in_size[1] != sizeof(seed_end_byte) ||
	    msg->out_size[0] != sizeof(result) ||
	    msg->out_size[1] == 0 ||
	    msg->out_size[1] > sizeof(eik_derived_key)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	derive_len = msg->out_size[1];

	if (psa_read(msg->handle, 1, &seed_end_byte, sizeof(seed_end_byte)) !=
	    sizeof(seed_end_byte)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	status = tfm_eik_read_from_its(eik, sizeof(eik));
	if (status != PSA_SUCCESS) {
		return status;
	}

	result = eik_core_derive_key(eik, seed_end_byte, eik_derived_key, derive_len);
	mbedtls_platform_zeroize(eik, sizeof(eik));
	if (result != 0) {
		return PSA_ERROR_GENERIC_ERROR;
	}

	psa_write(msg->handle, 0, &result, sizeof(result));
	psa_write(msg->handle, 1, eik_derived_key, derive_len);

	return PSA_SUCCESS;
}

static psa_status_t tfm_eik_secure_dispatch(const struct eik_secure_req *req, const psa_msg_t *msg)
{
	switch (req->op) {
	case EIK_SECURE_OP_DELETE:
		INFO_UNPRIV("[EIK SECURE] Delete\n\r");
		return tfm_eik_delete(msg);

	case EIK_SECURE_OP_IS_PROVISIONED:
		INFO_UNPRIV("[EIK SECURE] Is provisioned\n\r");
		return tfm_eik_is_provisioned(msg);

	case EIK_SECURE_OP_HASH_COMPARE:
		INFO_UNPRIV("[EIK SECURE] Hash compare\n\r");
		return tfm_eik_hash_compare(msg);

	case EIK_SECURE_OP_EID_ENCODE:
		INFO_UNPRIV("[EIK SECURE] EID encode\n\r");
		return tfm_eik_eid_encode(msg);

	case EIK_SECURE_OP_PROVISION_ENCRYPTED:
		INFO_UNPRIV("[EIK SECURE] Provision encrypted\n\r");
		return tfm_eik_provision_encrypted(msg);

	case EIK_SECURE_OP_GET_ENCRYPTED:
		INFO_UNPRIV("[EIK SECURE] Get encrypted\n\r");
		return tfm_eik_get_encrypted(msg);

	case EIK_SECURE_OP_DERIVE_KEY:
		INFO_UNPRIV("[EIK SECURE] Derive Key\n\r");
		return tfm_eik_derive_key(msg);

	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t tfm_eik_secure_sfn(const psa_msg_t *msg)
{
	struct eik_secure_req req;

	if (msg == NULL) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	switch (msg->type) {
	case PSA_IPC_CALL:
		if (msg->in_size[0] != sizeof(req)) {
			return PSA_ERROR_PROGRAMMER_ERROR;
		}

		if (psa_read(msg->handle, 0, &req, sizeof(req)) != sizeof(req)) {
			return PSA_ERROR_PROGRAMMER_ERROR;
		}

		return tfm_eik_secure_dispatch(&req, msg);
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t tfm_eik_secure_init(void)
{
	return PSA_SUCCESS;
}
