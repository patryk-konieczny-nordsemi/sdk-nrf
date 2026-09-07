/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _EIK_SECURE_IPC_H_
#define _EIK_SECURE_IPC_H_

/**
 * @defgroup eik_secure_ipc Secure EIK operations IPC protocol
 * @brief Wire format shared by the NS client and the EIK secure partition.
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
enum eik_secure_op {
	EIK_SECURE_OP_DELETE,
	EIK_SECURE_OP_IS_PROVISIONED,
	EIK_SECURE_OP_HASH_COMPARE,
	EIK_SECURE_OP_EID_ENCODE,
	EIK_SECURE_OP_PROVISION_ENCRYPTED,
	EIK_SECURE_OP_GET_ENCRYPTED,
	EIK_SECURE_OP_DERIVE_KEY
};

/** IPC request header (in_vec[0] of every eik_secure psa_call). */
struct eik_secure_req {
	/** Operation selector (@ref eik_secure_op). */
	uint8_t op;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EIK_SECURE_IPC_H_ */
