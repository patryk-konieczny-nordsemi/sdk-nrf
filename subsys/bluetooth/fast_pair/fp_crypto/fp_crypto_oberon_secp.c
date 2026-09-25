/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>

#include <ocrypto_secp160r1.h>
#include <ocrypto_curve_p256.h>
#include <ocrypto_sc_p256.h>
#include "fp_crypto.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_crypto, CONFIG_FP_CRYPTO_LOG_LEVEL);

#define SECP160R1_DATA_LEN (32U)
#define SECP256R1_DATA_LEN (32U)

/* START: importing additional APIs from the Oberon runtime. */
typedef struct {
	uint32_t w[6];
} ocrypto_sc_p160;

void ocrypto_sc_p160_from32bytes_alt(ocrypto_sc_p160 *r, const uint8_t x[32]);
/* END: importing additional APIs from the Oberon runtime. */

int fp_crypto_ecc_secp160r1_calculate(uint8_t *out,
				      uint8_t *mod,
				      const uint8_t *in,
				      size_t datalen)
{
	/* SECP160R1 key length corresponds to one coordinate. */
	uint8_t public_key[FP_CRYPTO_ECC_SECP160R1_KEY_LEN * 2];
	ocrypto_sc_p160 mod_le;

	/* Verify the input length. */
	if (datalen != SECP160R1_DATA_LEN) {
		LOG_ERR("secp160r1 invalid input length");
		return -ENOTSUP;
	}

	/* Compute the intermediate modulo result.
	 * Store it in the output buffer after little-endian to big-endian conversion.
	 */
	ocrypto_sc_p160_from32bytes_alt(&mod_le, in);
	sys_memcpy_swap(mod, mod_le.w, FP_CRYPTO_ECC_SECP160R1_MOD_LEN);

	/* Calculate the public key and store the X coordinate in the output buffer. */
	ocrypto_p160_scalar_mult_alt(public_key, in);
	memcpy(out, public_key, FP_CRYPTO_ECC_SECP160R1_KEY_LEN);

	return 0;
}

int fp_crypto_ecc_secp256r1_calculate(uint8_t *out,
				      uint8_t *mod,
				      const uint8_t *in,
				      size_t datalen)
{
	ocrypto_cp_p256 public_key;
	ocrypto_sc_p256 mod_le;

	/* Verify the input length. */
	if (datalen != SECP256R1_DATA_LEN) {
		LOG_ERR("secp256r1 invalid input length");
		return -ENOTSUP;
	}

	/* Compute the intermediate modulo result.
	 * Store it in the output buffer after little-endian to big-endian conversion.
	 */
	(void) ocrypto_sc_p256_from32bytes(&mod_le, in);
	sys_memcpy_swap(mod, mod_le.w, FP_CRYPTO_ECC_SECP256R1_MOD_LEN);

	/* Calculate the public key and store the X coordinate in the output buffer. */
	(void) ocrypto_curve_p256_scalarmult_base(&public_key, &mod_le);
	ocrypto_curve_p256_to32bytes(out, &public_key);

	return 0;
}
