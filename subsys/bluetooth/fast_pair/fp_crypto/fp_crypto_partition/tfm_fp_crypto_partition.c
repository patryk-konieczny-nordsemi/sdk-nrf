/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdint.h>
#include <string.h>
#include <psa/crypto.h>
#include "psa/service.h"
#include "psa_manifest/tfm_fp_crypto_partition.h"

#include "fp_crypto_ipc.h"
#include "fp_crypto_sizes.h"

#include <ocrypto_secp160r1.h>
#include <ocrypto_aes_ecb.h>
#include <ocrypto_curve_p256.h>
#include <ocrypto_sc_p256.h>

#include "tfm_log_unpriv.h"

#define SECP160R1_DATA_LEN (32U)
#define SECP256R1_DATA_LEN (32U)

/* START: importing additional APIs from the Oberon runtime. */
typedef struct {
	uint32_t w[6];
} ocrypto_sc_p160;

void ocrypto_sc_p160_from32bytes_alt(ocrypto_sc_p160 *r, const uint8_t x[32]);
/* END: importing additional APIs from the Oberon runtime. */

static void fp_crypto_memcpy_swap(void *dst, const void *src, size_t len)
{
	const uint8_t *s = src;
	uint8_t *d = dst;

	for (size_t i = 0; i < len; i++) {
		d[i] = s[len - 1U - i];
	}
}

static psa_status_t tfm_fp_crypto_ecc_secp160r1_calculate(const psa_msg_t *msg)
{
	uint8_t in[SECP160R1_DATA_LEN];
	uint8_t out[FP_CRYPTO_ECC_SECP160R1_KEY_LEN];
	uint8_t mod[FP_CRYPTO_ECC_SECP160R1_MOD_LEN];
	uint8_t public_key[FP_CRYPTO_ECC_SECP160R1_KEY_LEN * 2U];
	ocrypto_sc_p160 mod_le;

	if (msg->in_size[1] != SECP160R1_DATA_LEN ||
	    msg->out_size[0] != FP_CRYPTO_ECC_SECP160R1_KEY_LEN ||
	    msg->out_size[1] != FP_CRYPTO_ECC_SECP160R1_MOD_LEN) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (psa_read(msg->handle, 1, in, sizeof(in)) != sizeof(in)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	ocrypto_sc_p160_from32bytes_alt(&mod_le, in);
	fp_crypto_memcpy_swap(mod, mod_le.w, FP_CRYPTO_ECC_SECP160R1_MOD_LEN);

	ocrypto_p160_scalar_mult_alt(public_key, in);
	memcpy(out, public_key, FP_CRYPTO_ECC_SECP160R1_KEY_LEN);

	psa_write(msg->handle, 0, out, sizeof(out));
	psa_write(msg->handle, 1, mod, sizeof(mod));

	return PSA_SUCCESS;
}

static psa_status_t tfm_fp_crypto_secp256r1_calculate(const psa_msg_t *msg)
{
	uint8_t in[SECP256R1_DATA_LEN];
	uint8_t out[FP_CRYPTO_ECC_SECP256R1_KEY_LEN];
	uint8_t mod[FP_CRYPTO_ECC_SECP256R1_MOD_LEN];
	ocrypto_cp_p256 public_key;
	ocrypto_sc_p256 mod_le;

	if (msg->in_size[1] != SECP256R1_DATA_LEN ||
	    msg->out_size[0] != FP_CRYPTO_ECC_SECP256R1_KEY_LEN ||
	    msg->out_size[1] != FP_CRYPTO_ECC_SECP256R1_MOD_LEN) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	if (psa_read(msg->handle, 1, in, sizeof(in)) != sizeof(in)) {
		return PSA_ERROR_PROGRAMMER_ERROR;
	}

	(void)ocrypto_sc_p256_from32bytes(&mod_le, in);
	fp_crypto_memcpy_swap(mod, mod_le.w, FP_CRYPTO_ECC_SECP256R1_MOD_LEN);

	(void)ocrypto_curve_p256_scalarmult_base(&public_key, &mod_le);
	ocrypto_curve_p256_to32bytes(out, &public_key);

	psa_write(msg->handle, 0, out, sizeof(out));
	psa_write(msg->handle, 1, mod, sizeof(mod));

	return PSA_SUCCESS;
}

static psa_status_t tfm_fp_crypto_dispatch(const struct fp_crypto_req *req, const psa_msg_t *msg)
{
	INFO_UNPRIV("Called custom spe crypto: ");
	switch (req->op) {
	case FP_CRYPTO_OP_SECP160R1_CALCULATE:
		INFO_UNPRIV("secp160r1_calculate\n\r");
		return tfm_fp_crypto_ecc_secp160r1_calculate(msg);
	case FP_CRYPTO_OP_SECP256R1_CALCULATE:
		INFO_UNPRIV("secp256r1_calculate\n\r");
		return tfm_fp_crypto_secp256r1_calculate(msg);
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t tfm_fp_crypto_sfn(const psa_msg_t *msg)
{
	struct fp_crypto_req req;

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

		return tfm_fp_crypto_dispatch(&req, msg);
	default:
		return PSA_ERROR_NOT_SUPPORTED;
	}
}

psa_status_t tfm_fp_crypto_init(void)
{
	return PSA_SUCCESS;
}
