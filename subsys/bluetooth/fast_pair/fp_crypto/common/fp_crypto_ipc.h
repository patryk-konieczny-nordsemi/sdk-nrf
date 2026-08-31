/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_CRYPTO_IPC_H_
#define _FP_CRYPTO_IPC_H_

/**
 * @defgroup fp_crypto_ipc Fast Pair crypto IPC protocol
 * @brief Wire format shared by the NS client and the FP crypto secure partition.
 *
 * Defines opcodes and the request header passed in psa_call() in_vec[0].
 * Payload buffers are passed in separate IO vectors.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** Supported operations */
enum fp_crypto_op {
	FP_CRYPTO_OP_AES256_ECB_ENCRYPT,
	FP_CRYPTO_OP_AES256_ECB_DECRYPT,
	FP_CRYPTO_OP_SECP160R1_CALCULATE,
	FP_CRYPTO_OP_SECP256R1_CALCULATE,
};

/** IPC request header (in_vec[0] of every fp_crypto psa_call). */
struct fp_crypto_req {
	/** Operation selector (@ref fp_crypto_op). */
	uint16_t op;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_CRYPTO_IPC_H_ */
