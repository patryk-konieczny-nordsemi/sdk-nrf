/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>
#include <zephyr/init.h>
#include <psa/crypto.h>
#include <zephyr/kernel.h>

#include "fp_crypto.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fp_crypto, CONFIG_FP_CRYPTO_LOG_LEVEL);

static psa_key_id_t import_volatile_raw_key(psa_key_usage_t usage,
					    psa_algorithm_t alg,
					    psa_key_type_t type,
					    size_t key_bits,
					    const uint8_t *data,
					    size_t data_len)
{
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;

	psa_set_key_usage_flags(&key_attr, usage);
	psa_set_key_lifetime(&key_attr, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attr, alg);
	psa_set_key_type(&key_attr, type);
	psa_set_key_bits(&key_attr, key_bits);

	status = psa_import_key(&key_attr, data, data_len, &key_id);
	psa_reset_key_attributes(&key_attr);

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_import_key failed (err: %d)", status);
		return PSA_KEY_ID_NULL;
	}

	return key_id;
}

int fp_crypto_sha256(uint8_t *out, const uint8_t *in, size_t data_len)
{
	size_t hash_len = 0;
	psa_status_t status = psa_hash_compute(PSA_ALG_SHA_256, in, data_len,
					       out, FP_CRYPTO_SHA256_HASH_LEN, &hash_len);

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_hash_compute failed (err: %d)", status);
		return -EIO;
	}

	if (hash_len != FP_CRYPTO_SHA256_HASH_LEN) {
		LOG_ERR("Invalid psa_hash_compute output len: %zu", hash_len);
		return -EIO;
	}

	return 0;
}

static int fp_crypto_psa_hmac_sha256(uint8_t *out, const uint8_t *in, size_t data_len,
				     psa_key_id_t key_id)
{
	size_t olen = 0;
	psa_status_t status = psa_mac_compute(key_id, PSA_ALG_HMAC(PSA_ALG_SHA_256), in, data_len,
					      out, FP_CRYPTO_SHA256_HASH_LEN, &olen);

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_mac_compute failed (err: %d)", status);
		return -EIO;
	}

	if (olen != FP_CRYPTO_SHA256_HASH_LEN) {
		LOG_ERR("Invalid psa_mac_compute output length: %zu", olen);
		return -EIO;
	}

	return 0;
}

int fp_crypto_hmac_sha256(uint8_t *out, const uint8_t *in, size_t data_len,
			  const uint8_t *hmac_key, size_t hmac_key_len)
{
	int err = 0;
	psa_key_id_t hmac_key_id;
	psa_status_t status;

	hmac_key_id = import_volatile_raw_key(PSA_KEY_USAGE_SIGN_HASH,
					      PSA_ALG_HMAC(PSA_ALG_SHA_256),
					      PSA_KEY_TYPE_HMAC,
					      hmac_key_len * CHAR_BIT,
					      hmac_key,
					      hmac_key_len);
	if (hmac_key_id == PSA_KEY_ID_NULL) {
		LOG_ERR("HMAC key import failed");
		return -EIO;
	}

	err = fp_crypto_psa_hmac_sha256(out, in, data_len, hmac_key_id);

	status = psa_destroy_key(hmac_key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_destroy_key failed (err: %d)", status);
		/* Overwrite error code to forward information about psa_destroy_key failure. */
		err = -ECANCELED;
	}

	return err;
}

static int fp_crypto_psa_aes_ecb_crypt(uint8_t *out, const uint8_t *in, psa_key_id_t key_id,
				       size_t block_len, bool encrypt)
{
	size_t olen = 0;
	psa_status_t status;

	if (encrypt) {
		status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING,
					    in, block_len,
					    out, block_len, &olen);
	} else {
		status = psa_cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING,
					    in, block_len,
					    out, block_len, &olen);
	}

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_cipher_%scrypt failed (err: %d)", encrypt ? "en" : "de", status);
		return -EIO;
	}

	if (olen != block_len) {
		LOG_ERR("Invalid psa_cipher_%scrypt output length: %zu",
			encrypt ? "en" : "de", olen);
		return -EIO;
	}

	return 0;
}

static int fp_crypto_aes_ecb_crypt(uint8_t *out, const uint8_t *in, const uint8_t *k,
				   size_t key_len, size_t block_len, bool encrypt)
{
	int err = 0;
	psa_key_id_t key_id;
	psa_status_t status;

	key_id = import_volatile_raw_key(PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT,
					 PSA_ALG_ECB_NO_PADDING,
					 PSA_KEY_TYPE_AES,
					 key_len * CHAR_BIT,
					 k,
					 key_len);
	if (key_id == PSA_KEY_ID_NULL) {
		LOG_ERR("AES key import failed");
		return -EIO;
	}

	err = fp_crypto_psa_aes_ecb_crypt(out, in, key_id, block_len, encrypt);

	status = psa_destroy_key(key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_destroy_key failed (err: %d)", status);
		/* Overwrite error code to forward information about psa_destroy_key failure. */
		err = -ECANCELED;
	}

	return err;
}

int fp_crypto_aes128_ecb_encrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	return fp_crypto_aes_ecb_crypt(out, in, k, FP_CRYPTO_AES128_KEY_LEN,
				       FP_CRYPTO_AES128_BLOCK_LEN, true);
}

int fp_crypto_aes128_ecb_decrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	return fp_crypto_aes_ecb_crypt(out, in, k, FP_CRYPTO_AES128_KEY_LEN,
				       FP_CRYPTO_AES128_BLOCK_LEN, false);
}

static psa_key_id_t import_ecdh_priv_key(const uint8_t *data)
{
	/* SECP-R1 256-bit private key (256 bits = 32 bytes). */
	return import_volatile_raw_key(PSA_KEY_USAGE_DERIVE,
				       PSA_ALG_ECDH,
				       PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1),
				       FP_CRYPTO_ECDH_SHARED_KEY_LEN * CHAR_BIT,
				       data,
				       FP_CRYPTO_ECDH_SHARED_KEY_LEN);
}

static int fp_crypto_psa_ecdh_shared_secret(uint8_t *secret_key,
					    const uint8_t *public_key,
					    psa_key_id_t priv_key_id)
{
	/* Marker of the uncompressed binary format for a point on an elliptic curve. */
	static const uint8_t uncompressed_format_marker = 0x04;

	uint8_t public_key_uncompressed[sizeof(uncompressed_format_marker) +
					FP_CRYPTO_ECDH_PUBLIC_KEY_LEN];
	size_t olen = 0;
	psa_status_t status;

	/* Use the uncompressed binary format [0x04 X Y] for the public key point. */
	public_key_uncompressed[0] = uncompressed_format_marker;
	memcpy(&public_key_uncompressed[1], public_key, FP_CRYPTO_ECDH_PUBLIC_KEY_LEN);

	status = psa_raw_key_agreement(PSA_ALG_ECDH, priv_key_id,
				       public_key_uncompressed, sizeof(public_key_uncompressed),
				       secret_key, FP_CRYPTO_ECDH_SHARED_KEY_LEN, &olen);
	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_raw_key_agreement failed (err: %d)", status);
		return -EIO;
	}

	if (olen != FP_CRYPTO_ECDH_SHARED_KEY_LEN) {
		LOG_ERR("Invalid psa_raw_key_agreement output len: %zu", olen);
		return -EIO;
	}

	return 0;
}

int fp_crypto_ecdh_shared_secret(uint8_t *secret_key, const uint8_t *public_key,
				 const void *private_key)
{
	int err = 0;
	psa_key_id_t priv_key_id;
	psa_status_t status;

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE)) {
		priv_key_id = *((psa_key_id_t *)private_key);
	} else {
		priv_key_id = import_ecdh_priv_key((const uint8_t *)private_key);
	}

	if (priv_key_id == PSA_KEY_ID_NULL) {
		LOG_ERR("ECDH private key setup failed");
		return -EIO;
	}

	err = fp_crypto_psa_ecdh_shared_secret(secret_key, public_key, priv_key_id);

	if (IS_ENABLED(CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE)) {
		/* The KMU key is persistent - only drop its volatile copy, do not destroy it. */
		status = psa_purge_key(priv_key_id);
	} else {
		status = psa_destroy_key(priv_key_id);
	}

	/* Overwrite error code to forward information about psa destroy/purge key failure. */
	if (status != PSA_SUCCESS) {
		if (IS_ENABLED(CONFIG_BT_FAST_PAIR_PROVISION_SECURE_STORAGE)) {
			LOG_ERR("psa_purge_key failed (err: %d)", status);
		} else {
			LOG_ERR("psa_destroy_key failed (err: %d)", status);
		}
		err = -ECANCELED;
	}

	return err;
}

int fp_crypto_aes256_ecb_encrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	return fp_crypto_aes_ecb_crypt(out, in, k, FP_CRYPTO_AES256_KEY_LEN,
				       FP_CRYPTO_AES256_BLOCK_LEN, true);
}

int fp_crypto_aes256_ecb_decrypt(uint8_t *out, const uint8_t *in, const uint8_t *k)
{
	return fp_crypto_aes_ecb_crypt(out, in, k, FP_CRYPTO_AES256_KEY_LEN,
				       FP_CRYPTO_AES256_BLOCK_LEN, false);
}

static int fp_crypto_psa_init(void)
{
	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed (err: %d)", status);
		k_panic();
		return -EIO;
	}

	return 0;
}

SYS_INIT(fp_crypto_psa_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
