/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FHN_EIK_SECURE_IPC_H_
#define _FP_FHN_EIK_SECURE_IPC_H_

/**
 * @defgroup fp_fhn_eik_secure_ipc Secure EIK operations IPC protocol
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
enum fp_fhn_eik_secure_op {
	FP_FHN_EIK_SECURE_OP_DELETE,
	FP_FHN_EIK_SECURE_OP_IS_PROVISIONED,
	FP_FHN_EIK_SECURE_OP_HASH_COMPARE,
	FP_FHN_EIK_SECURE_OP_EID_ENCODE,
	FP_FHN_EIK_SECURE_OP_PROVISION_ENCRYPTED,
	FP_FHN_EIK_SECURE_OP_GET_ENCRYPTED,
	FP_FHN_EIK_SECURE_OP_DERIVE_KEY
};

/** IPC request header (in_vec[0] of every eik_secure psa_call). */
struct fp_fhn_eik_secure_req {
	/** Operation selector (@ref fp_fhn_eik_secure_op). */
	uint8_t op;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FHN_EIK_SECURE_IPC_H_ */
