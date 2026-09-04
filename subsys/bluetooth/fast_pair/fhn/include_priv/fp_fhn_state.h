/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FHN_STATE_H_
#define _FP_FHN_STATE_H_

#include <stdint.h>
#include <stddef.h>

#include <bluetooth/fast_pair/fhn/fhn.h>

/**
 * @defgroup fp_fhn_state Fast Pair FHN state
 * @brief Internal API for Fast Pair FHN state
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Length in bytes of the Ephemeral Identifier (EID). */
#define FP_FHN_STATE_EID_LEN CONFIG_BT_FAST_PAIR_FHN_ECC_LEN
/* Length in bytes of the Ephemeral Identity Key (EIK). */
#define FP_FHN_STATE_EIK_LEN 32

/** Read the currently used Ephemeral Identifier.
 *
 *  Length of buffer used to store the Ephemeral Identifier must at least be equal
 *  to 20 bytes (for the SECP160R1 variant) or 32 bytes (for the SECP256R1 variant).
 *
 * @param[out] eid Ephemeral Identifier.

 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fhn_state_eid_read(uint8_t *eid);

/** Encode the Elliptic Curve type configuration.
 *  The configuration is encoded as required by the FHN Accessory specification.
 *
 * @return Byte with an encoded information about the Elliptic Curve type.
 */
uint8_t fp_fhn_state_ecc_type_encode(void);

/** Encode the TX power configuration in dBm.
 *  The TX power is encoded as required by the FHN Accessory specification.
 *  The return value is a sum of TX power readout from the Bluetooth controller
 *  and the TX power correction value defined in Kconfig:
 *  CONFIG_BT_FAST_PAIR_FHN_TX_POWER_CORRECTION_VAL.
 *
 * @return Byte with information about the TX power, encoded as a signed integer.
 */
int8_t fp_fhn_state_tx_power_encode(void);

/** Provision or reprovision the beacon with a new Ephemeral Identity Key (EIK).
 *
 *  The encrypted EIK is decrypted and stored through the EIK operations module.
 *
 * @param[in] encrypted_eik Encrypted Ephemeral Identity Key (EIK) (32 bytes).
 * @param[in] account_key Account Key (16 bytes) used to decrypt the EIK.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fhn_state_eik_provision(const uint8_t *encrypted_eik, const uint8_t *account_key);

/** Unprovision the beacon and delete the Ephemeral Identity Key (EIK).
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fhn_state_eik_unprovision(void);

/** Activate the Unwanted Tracking Protection (UTP) mode.
 *
 * @param[in] control_flags Control Flags.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fhn_state_utp_mode_activate(uint8_t control_flags);

/** Deactivate the Unwanted Tracking Protection (UTP) mode.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fhn_state_utp_mode_deactivate(void);

/** Check if the beacon should skip the authentication step for the ringing request.
 *
 * @return True if the ringing request shouldn't be authenticated, False Otherwise.
 */
bool fp_fhn_state_utp_mode_ring_auth_skip(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FHN_STATE_H_ */
