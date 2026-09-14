/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FHN_EIK_OPERATIONS_COMMON_H_
#define _FP_FHN_EIK_OPERATIONS_COMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "fp_fhn_lengths.h"

/**
 * @defgroup fp_fhn_eik_operations_common Ephemeral Identity Key (EIK) common operations
 * @brief Internal API of Fast Pair Ephemeral Identity Key (EIK) operations for the FHN extension
 *
 * These routines operate on the plaintext EIK passed by the caller and contain no
 * storage access, so they can be reused in a secure processing environment (SPE).
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Length in bytes of the EIK hash compared in beacon actions (first 8 bytes of SHA-256). */
#define FP_FHN_EIK_COMMON_HASH_COMPARE_LEN FP_FHN_EIK_HASH_COMPARE_LEN

/* Length in bytes of the random nonce used in EIK hash verification. */
#define FP_FHN_EIK_COMMON_RANDOM_NONCE_LEN CONFIG_BT_FAST_PAIR_FHN_RANDOM_NONCE_LEN

/* Maximum length in bytes of output from @ref fp_fhn_eik_common_derive_key. */
#define FP_FHN_EIK_COMMON_DERIVED_KEY_MAX_LEN FP_FHN_EIK_DERIVED_KEY_MAX_LEN

/** Verify a received EIK hash against the provided Ephemeral Identity Key (EIK).
 *
 * Compares the first 8 bytes of SHA256(EIK || random_nonce).
 *
 * @param[in] eik Ephemeral Identity Key.
 * @param[in] eik_hash Received EIK hash.
 * @param[in] random_nonce Random nonce used to generate the hash.
 *
 * @return True if the hash matches, false otherwise.
 */
bool fp_fhn_eik_common_hash_compare(const uint8_t *eik, const uint8_t *eik_hash,
				    const uint8_t *random_nonce);

/** Encode the Ephemeral Identifier (EID) from the EID seed and the provided EIK.
 *
 * @param[in] eik Ephemeral Identity Key.
 * @param[in] eid_seed_buf_data EID seed data.
 * @param[out] fhn_eid Buffer to receive the calculated EID.
 * @param[out] fhn_frame_hashed_flags_xor_operand Buffer to receive the Hashed Flags XOR operand.
 *
 * @return 0 on success. Otherwise, a (negative) error code is returned.
 */
int fp_fhn_eik_common_eid_encode(const uint8_t *eik, const uint8_t *eid_seed_buf_data,
				 uint8_t *fhn_eid, uint8_t *fhn_frame_hashed_flags_xor_operand);

/** Decrypt an encrypted Ephemeral Identity Key (EIK) with the Account Key.
 *
 * @param[in] encrypted_eik Encrypted EIK.
 * @param[in] account_key Account Key used to decrypt the EIK.
 * @param[out] eik Buffer to receive the decrypted EIK.
 *
 * @return 0 on success. Otherwise, a (negative) error code is returned.
 */
int fp_fhn_eik_common_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key,
					  uint8_t *eik);

/** Encrypt the provided Ephemeral Identity Key (EIK) with the Owner Account Key.
 *
 * @param[in] eik Ephemeral Identity Key.
 * @param[in] owner_account_key Owner Account Key used to encrypt the EIK.
 * @param[out] encrypted_eik Buffer to receive the encrypted EIK.
 *
 * @return 0 on success. Otherwise, a (negative) error code is returned.
 */
int fp_fhn_eik_common_get_encrypted(const uint8_t *eik, const uint8_t *owner_account_key,
				    uint8_t *encrypted_eik);

/** Derive an EIK auth key from the provided Ephemeral Identity Key (EIK).
 *
 * Calculates the first @p eik_derived_key_len bytes of SHA256(EIK || seed_end_byte).
 *
 * @param[in] eik Ephemeral Identity Key.
 * @param[in] seed_end_byte Seed end byte selecting the derived key type.
 * @param[out] eik_derived_key Buffer to receive the derived key.
 * @param[in] eik_derived_key_len Length of the derived key to output.
 *
 * @return 0 on success. Otherwise, a (negative) error code is returned.
 */
int fp_fhn_eik_common_derive_key(const uint8_t *eik, uint8_t seed_end_byte,
				 uint8_t *eik_derived_key, size_t eik_derived_key_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FHN_EIK_OPERATIONS_COMMON_H_ */
