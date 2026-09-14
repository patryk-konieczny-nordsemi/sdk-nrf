/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FHN_LENGTHS_H_
#define _FP_FHN_LENGTHS_H_

/**
 * @file fp_fhn_lengths.h
 * @brief FHN size constants shared by NS, SPE, and core EIK routines.
 */

/* Length in bytes of the Ephemeral Identity Key (EIK). */
#define FP_FHN_STATE_EIK_LEN 32

/* Length in bytes of an encrypted EIK (two AES-128-ECB blocks). */
#define FP_FHN_ENCRYPTED_EIK_LEN FP_FHN_STATE_EIK_LEN

/* Length in bytes of the Account Key used to encrypt or decrypt the EIK. */
#define FP_FHN_ACCOUNT_KEY_LEN 16U

/* Maximum length in bytes of a derived EIK auth key (full SHA-256 output). */
#define FP_FHN_EIK_DERIVED_KEY_MAX_LEN 32U

/* Length in bytes of the EIK hash compared in beacon actions (first 8 bytes of SHA-256). */
#define FP_FHN_EIK_HASH_COMPARE_LEN 8U

/* Byte length of fields used to generate a seed for Ephemeral Identifier. */
#define FP_FHN_EID_SEED_PADDING_LEN        11
#define FP_FHN_EID_SEED_ROT_PERIOD_EXP_LEN 1
#define FP_FHN_EID_SEED_FHN_CLOCK_LEN      4

#define FP_FHN_EID_SEED_LEN \
	((FP_FHN_EID_SEED_PADDING_LEN + \
	  FP_FHN_EID_SEED_ROT_PERIOD_EXP_LEN + \
	  FP_FHN_EID_SEED_FHN_CLOCK_LEN) * 2)

/* Length in bytes of the Hashed Flags XOR operand in the FHN frame. */
#define FP_FHN_FRAME_HASHED_FLAGS_XOR_LEN 1

/* Length in bytes of the Ephemeral Identifier (EID). */
#define FP_FHN_STATE_EID_LEN CONFIG_BT_FAST_PAIR_FHN_ECC_LEN

#endif /* _FP_FHN_LENGTHS_H_ */
