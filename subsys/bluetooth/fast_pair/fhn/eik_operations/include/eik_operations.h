/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _EIK_OPERATIONS_H_
#define _EIK_OPERATIONS_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @defgroup eik_operations Ephemeral Identity Key (EIK) operations for the FHN extension
 * @brief API of Fast Pair Ephemeral Identity Key (EIK) operations for the FHN extension
 *
 * This module is the single access point to the EIK. It loads the EIK from the
 * storage backend internally and forwards the plaintext key to the core routines,
 * so that callers never handle the raw EIK.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Delete the Ephemeral Identity Key (EIK) from the storage backend.
 *
 * @return 0 on success. Otherwise, a (negative) error code is returned.
 */
int eik_delete(void);

/** Check whether the Ephemeral Identity Key (EIK) is provisioned.
 *
 * @return 1 if provisioned, 0 if not provisioned.
 *         Otherwise, a (negative) error code is returned.
 */
int eik_is_provisioned(void);

/** Verify a received EIK hash against the locally provisioned EIK.
 *
 *  Compares the first 8 bytes of SHA256(EIK || random_nonce).
 *
 * @param[in] eik_hash Received EIK hash (8 bytes).
 * @param[in] random_nonce Random nonce used to generate the hash.
 *
 * @return True if the hash matches, false otherwise.
 */
bool eik_hash_compare(const uint8_t *eik_hash, const uint8_t *random_nonce);

/** Encode the Ephemeral Identifier (EID) from the EID seed.
 *
 * @param[in] eid_seed_buf_data EID seed data (32 bytes).
 * @param[out] fhn_eid Buffer to receive the calculated EID.
 * @param[out] fhn_frame_hashed_flags_xor_operand Buffer to receive the Hashed Flags XOR operand.
 *
 * @return 0 on success. Otherwise, a (negative) error code is returned.
 */
int eik_eid_encode(const uint8_t *eid_seed_buf_data, uint8_t *fhn_eid,
		   uint8_t *fhn_frame_hashed_flags_xor_operand);

/** Decrypt the provided Ephemeral Identity Key (EIK) and save it to the storage backend.
 *
 * @param[in] encrypted_eik Encrypted EIK (32 bytes).
 * @param[in] account_key Account Key (16 bytes) used to decrypt the EIK.
 *
 * @return 0 on success. Otherwise, a (negative) error code is returned.
 */
int eik_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key);

/** Read the provisioned Ephemeral Identity Key (EIK) encrypted with the Owner Account Key.
 *
 * @param[in] owner_account_key Owner Account Key (16 bytes) used to encrypt the EIK.
 * @param[out] encrypted_eik Buffer to receive the encrypted EIK (32 bytes).
 *
 * @return 0 on success. Otherwise, a (negative) error code is returned.
 */
int eik_get_encrypted(const uint8_t *owner_account_key, uint8_t *encrypted_eik);

/** Derive an EIK auth key from the provisioned Ephemeral Identity Key (EIK).
 *
 *  Calculates the first @p eik_derived_key_len bytes of SHA256(EIK || seed_end_byte).
 *
 * @param[in] seed_end_byte Seed end byte selecting the derived key type.
 * @param[out] eik_derived_key Buffer to receive the derived key.
 * @param[in] eik_derived_key_len Length of the derived key to output.
 *
 * @return 0 on success. Otherwise, a (negative) error code is returned.
 */
int eik_derive_key(uint8_t seed_end_byte, uint8_t *eik_derived_key, size_t eik_derived_key_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EIK_OPERATIONS_H_ */
