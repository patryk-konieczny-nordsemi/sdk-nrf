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

#include "fp_fhn_lengths.h"

/**
 * @defgroup eik_operations Ephemeral Identity Key (EIK) operations for the FHN extension
 * @brief API for Fast Pair Ephemeral Identity Key (EIK) operations
 *
 * This module is the single access point for FHN code that needs to use the EIK.
 * The plaintext key is never returned to the caller; each operation loads or uses
 * the provisioned EIK internally.
 *
 * When @kconfig{CONFIG_TFM_IPC} is enabled, calls are forwarded over PSA IPC to the
 * EIK secure partition in SPE. Otherwise, the legacy path uses device settings or
 * RRAM storage together with @ref eik_operations_core in NS.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Delete the provisioned Ephemeral Identity Key (EIK).
 *
 * Removes the stored EIK. Deleting an already unprovisioned EIK is treated as
 * success on both backends.
 *
 * @retval 0 EIK deleted, or no EIK was provisioned.
 * @retval -EIO IPC or storage backend failure.
 */
int eik_delete(void);

/** Check whether an Ephemeral Identity Key (EIK) is provisioned.
 *
 * On the TF-M path, provisioning is considered valid only when the ITS object
 * exists and its size matches @ref FP_FHN_STATE_EIK_LEN.
 *
 * @retval 1 EIK is provisioned.
 * @retval 0 EIK is not provisioned.
 * @retval -EIO IPC or storage backend failure.
 */
int eik_is_provisioned(void);

/** Verify a received EIK hash against the locally provisioned EIK.
 *
 * Compares the first @ref FP_FHN_EIK_HASH_COMPARE_LEN bytes of
 * SHA256(EIK || random_nonce) with @p eik_hash.
 *
 * @param[in] eik_hash Received EIK hash (@ref FP_FHN_EIK_HASH_COMPARE_LEN bytes).
 * @param[in] random_nonce Random nonce used to generate the hash. Its length is
 *                         given by @kconfig{CONFIG_BT_FAST_PAIR_FHN_RANDOM_NONCE_LEN}.
 *
 * @retval true Hash matches the provisioned EIK.
 * @retval false Hash does not match, the EIK is not available, or the operation failed.
 */
bool eik_hash_compare(const uint8_t *eik_hash, const uint8_t *random_nonce);

/** Encode the Ephemeral Identifier (EID) from the EID seed.
 *
 * Uses the provisioned EIK to derive the EID and the Hashed Flags XOR operand
 * for the current FHN frame.
 *
 * @param[in] eid_seed_buf_data EID seed data (@ref FP_FHN_EID_SEED_LEN bytes).
 * @param[out] fhn_eid Buffer to receive the calculated EID
 *                     (@ref FP_FHN_STATE_EID_LEN bytes).
 * @param[out] fhn_frame_hashed_flags_xor_operand Buffer to receive the Hashed
 *             Flags XOR operand (@ref FP_FHN_FRAME_HASHED_FLAGS_XOR_LEN byte).
 *
 * @retval 0 EID encoded successfully.
 * @retval -EIO IPC or storage backend failure, or EIK could not be read.
 * @retval other Negative errno from the underlying crypto implementation.
 */
int eik_eid_encode(const uint8_t *eid_seed_buf_data, uint8_t *fhn_eid,
		   uint8_t *fhn_frame_hashed_flags_xor_operand);

/** Decrypt an encrypted Ephemeral Identity Key (EIK) and store it.
 *
 * Decrypts @p encrypted_eik with @p account_key and persists the resulting EIK.
 *
 * @param[in] encrypted_eik Encrypted EIK (@ref FP_FHN_ENCRYPTED_EIK_LEN bytes).
 * @param[in] account_key Account Key (@ref FP_FHN_ACCOUNT_KEY_LEN bytes) used to
 *                        decrypt the EIK.
 *
 * @retval 0 EIK decrypted and stored.
 * @retval -EIO IPC or storage backend failure.
 * @retval other Negative errno from the underlying crypto or storage layer.
 */
int eik_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key);

/** Read the provisioned Ephemeral Identity Key (EIK) encrypted with the Owner Account Key.
 *
 * Loads the provisioned EIK, encrypts it with @p owner_account_key, and writes the
 * result to @p encrypted_eik.
 *
 * @param[in] owner_account_key Owner Account Key (@ref FP_FHN_ACCOUNT_KEY_LEN bytes)
 *                              used to encrypt the EIK.
 * @param[out] encrypted_eik Buffer to receive the encrypted EIK
 *                           (@ref FP_FHN_ENCRYPTED_EIK_LEN bytes).
 *
 * @retval 0 Encrypted EIK written successfully.
 * @retval -EIO IPC or storage backend failure.
 * @retval other Negative errno from the underlying crypto or storage layer.
 */
int eik_get_encrypted(const uint8_t *owner_account_key, uint8_t *encrypted_eik);

/** Derive an EIK auth key from the provisioned Ephemeral Identity Key (EIK).
 *
 * Calculates the first @p eik_derived_key_len bytes of SHA256(EIK || seed_end_byte).
 *
 * @param[in] seed_end_byte Seed end byte selecting the derived key type.
 * @param[out] eik_derived_key Buffer to receive the derived key.
 * @param[in] eik_derived_key_len Length of the derived key to output. Must be greater
 *                                than 0 and at most @ref FP_FHN_EIK_DERIVED_KEY_MAX_LEN.
 *
 * @retval 0 Derived key written successfully.
 * @retval -EINVAL @p eik_derived_key_len is out of range.
 * @retval -EIO IPC or storage backend failure.
 * @retval other Negative errno from the underlying crypto layer.
 */
int eik_derive_key(uint8_t seed_end_byte, uint8_t *eik_derived_key, size_t eik_derived_key_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EIK_OPERATIONS_H_ */
