/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_CRYPTO_SIZES_H_
#define _FP_CRYPTO_SIZES_H_

/**
 * @defgroup fp_crypto Fast Pair crypto data sizes
 * @brief Internal API for Fast Pair data sizes
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Length of SHA256 hash result (256 bits = 32 bytes). */
#define FP_CRYPTO_SHA256_HASH_LEN			32U
/** Length of AES-128 block (128 bits = 16 bytes). */
#define FP_CRYPTO_AES128_BLOCK_LEN			16U
/** Length of AES-128 key (128 bits = 16 bytes). */
#define FP_CRYPTO_AES128_KEY_LEN			16U
/** Length of AES-256 block (256 bits = 32 bytes). */
#define FP_CRYPTO_AES256_BLOCK_LEN			32U
/** Length of AES-256 key (256 bits = 32 bytes). */
#define FP_CRYPTO_AES256_KEY_LEN			32U
/** Length of ECDH public key (512 bits = 64 bytes). */
#define FP_CRYPTO_ECDH_PUBLIC_KEY_LEN			64U
/** Length of ECDH shared key (256 bits = 32 bytes). */
#define FP_CRYPTO_ECDH_SHARED_KEY_LEN			32U
/** Length of SECP160R1 modulo normalization (160 bits = 20 bytes). */
#define FP_CRYPTO_ECC_SECP160R1_MOD_LEN			20U
/** Length of SECP160R1 elliptic curve (160 bits = 20 bytes). */
#define FP_CRYPTO_ECC_SECP160R1_KEY_LEN			20U
/** Length of SECP256R1 modulo normalization (256 bits = 32 bytes). */
#define FP_CRYPTO_ECC_SECP256R1_MOD_LEN			32U
/** Length of SECP256R1 elliptic curve (256 bits = 32 bytes). */
#define FP_CRYPTO_ECC_SECP256R1_KEY_LEN			32U
/** Length of nonce in Additional Data packet (64 bits = 8 bytes). */
#define FP_CRYPTO_ADDITIONAL_DATA_NONCE_LEN		8U
/** Length of Additional Data packet header (128 bits = 16 bytes). */
#define FP_CRYPTO_ADDITIONAL_DATA_HEADER_LEN		16U
/** Length of battery info (1-byte length and type field and 3-byte battery values field). */
#define FP_CRYPTO_BATTERY_INFO_LEN			4U

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_CRYPTO_SIZES_H_ */
