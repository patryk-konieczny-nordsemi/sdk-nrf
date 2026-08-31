/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/util.h>

#include "fp_crypto_ipc.h"
#include "fp_crypto_sizes.h"
#include "fp_crypto_spe_client.h"

psa_status_t fp_crypto_spe_aes256_ecb_encrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	struct fp_crypto_req req = {
        .op = FP_CRYPTO_OP_AES256_ECB_ENCRYPT,
    };

	if (!out || !in || !k) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

    psa_invec in_vec[] = {
        { &req, sizeof(req) },
        { in,   FP_CRYPTO_AES256_BLOCK_LEN },
        { k,    FP_CRYPTO_AES256_KEY_LEN },
    };

    psa_outvec out_vec[] = {
        { out, FP_CRYPTO_AES256_BLOCK_LEN },
    };

    return psa_call(TFM_FP_CRYPTO_HANDLE, PSA_IPC_CALL,
                      in_vec, ARRAY_SIZE(in_vec),
                      out_vec, ARRAY_SIZE(out_vec));
}

psa_status_t fp_crypto_spe_aes256_ecb_decrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	struct fp_crypto_req req = {
        .op = FP_CRYPTO_OP_AES256_ECB_DECRYPT,
    };

	if (!out || !in || !k) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

    psa_invec in_vec[] = {
        { &req, sizeof(req) },
        { in,   FP_CRYPTO_AES256_BLOCK_LEN },
        { k,    FP_CRYPTO_AES256_KEY_LEN },
    };

    psa_outvec out_vec[] = {
        { out, FP_CRYPTO_AES256_BLOCK_LEN },
    };

    return psa_call(TFM_FP_CRYPTO_HANDLE, PSA_IPC_CALL,
                      in_vec, ARRAY_SIZE(in_vec),
                      out_vec, ARRAY_SIZE(out_vec));
}

#define FP_CRYPTO_ECC_SCALAR_INPUT_LEN 32U

psa_status_t fp_crypto_spe_ecc_secp160r1_calculate(uint8_t *out, uint8_t *mod,
						     const uint8_t *in, size_t datalen)
{
	struct fp_crypto_req req = {
		.op = FP_CRYPTO_OP_SECP160R1_CALCULATE,
	};

	if (!out || !mod || !in) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (datalen != FP_CRYPTO_ECC_SCALAR_INPUT_LEN) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	psa_invec in_vec[] = {
		{ &req, sizeof(req) },
		{ in, FP_CRYPTO_ECC_SCALAR_INPUT_LEN },
	};

	psa_outvec out_vec[] = {
		{ out, FP_CRYPTO_ECC_SECP160R1_KEY_LEN },
		{ mod, FP_CRYPTO_ECC_SECP160R1_MOD_LEN },
	};

	return psa_call(TFM_FP_CRYPTO_HANDLE, PSA_IPC_CALL,
			in_vec, ARRAY_SIZE(in_vec),
			out_vec, ARRAY_SIZE(out_vec));
}

psa_status_t fp_crypto_spe_ecc_secp256r1_calculate(uint8_t *out, uint8_t *mod,
						     const uint8_t *in, size_t datalen)
{
	struct fp_crypto_req req = {
		.op = FP_CRYPTO_OP_SECP256R1_CALCULATE,
	};

	if (!out || !mod || !in) {
		return PSA_ERROR_INVALID_ARGUMENT;
	}

	if (datalen != FP_CRYPTO_ECC_SCALAR_INPUT_LEN) {
		return PSA_ERROR_NOT_SUPPORTED;
	}

	psa_invec in_vec[] = {
		{ &req, sizeof(req) },
		{ in, FP_CRYPTO_ECC_SCALAR_INPUT_LEN },
	};

	psa_outvec out_vec[] = {
		{ out, FP_CRYPTO_ECC_SECP256R1_KEY_LEN },
		{ mod, FP_CRYPTO_ECC_SECP256R1_MOD_LEN },
	};

	return psa_call(TFM_FP_CRYPTO_HANDLE, PSA_IPC_CALL,
			in_vec, ARRAY_SIZE(in_vec),
			out_vec, ARRAY_SIZE(out_vec));
}
