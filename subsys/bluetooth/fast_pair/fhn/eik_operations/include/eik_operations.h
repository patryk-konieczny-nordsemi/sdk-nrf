/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _EIK_OPERATIONS_H_
#define _EIK_OPERATIONS_H_

#include <sys/types.h>

/**
 * @defgroup Ephemeral Identity Key (EIK) operations for the FHN extension
 * @brief API of Fast Pair Ephemeral Identity Key (EIK) operations for the FHN extension
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

int eik_delete(void);
int eik_is_provisioned(void);
bool eik_hash_compare(const uint8_t *eik_hash, const uint8_t *random_nonce);
int eik_core_eid_encode(uint32_t fhn_clock, const uint8_t *eid_seed_buf_data, const uint8_t *fhn_eid, const uint8_t *fhn_frame_hashed_flags_xor_operand);
int eik_provision_encrypted(const uint8_t *encrypted_eik, const uint8_t *account_key);
int eik_get_encrypted(const uint8_t *owner_account_key, const uint8_t *encrypted_eik);
int eik_derive_key(uint8_t seed_end_byte, uint8_t *eik_derived_key, size_t eik_derived_key_len);
 
#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _EIK_OPERATIONS_H_ */