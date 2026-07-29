/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/*
 * Provisioning variant of the Fast Pair registration data module.
 *
 * This file is an alternative to fp_registration_data.c and is built instead of it
 * when CONFIG_BT_FAST_PAIR_PROVISION is enabled (selected from the CMakeLists.txt).
 *
 * The standard module serves the Model ID and the Anti-Spoofing private key directly
 * from the bt_fast_pair flash partition (RRAM) on every access. This variant instead
 * performs a one-time migration ("provisioning") of that data into hardware-protected
 * storage and then serves it from there:
 *
 *   - Anti-Spoofing private key -> CRACEN KMU slot   CONFIG_BT_FAST_PAIR_KMU_SLOT
 *   - Model ID                  -> PSA Protected Storage uid CONFIG_BT_FAST_PAIR_PS_ID
 *
 * After a successful migration the flash partition is erased so the Anti-Spoofing
 * private key no longer resides in plaintext. The migration runs when the Fast Pair
 * registration data module is initialized (from bt_fast_pair_enable()) and is
 * idempotent: on subsequent boots the data is already in KMU/Protected Storage, the
 * flash partition is empty, and provisioning is skipped.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/storage/flash_map.h>

#include <psa/crypto.h>
#include <psa/protected_storage.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/fast_pair/fast_pair.h>
#include "fp_activation.h"
#include "fp_registration_data.h"

#if defined(CONFIG_PARTITION_MANAGER_ENABLED)
#include <pm_config.h>
#define FP_PARTITION_ID		PM_BT_FAST_PAIR_ID
#define FP_PARTITION_SIZE	PM_BT_FAST_PAIR_SIZE
#else
BUILD_ASSERT(PARTITION_EXISTS(bt_fast_pair_partition));
#define FP_PARTITION_ID		PARTITION_ID(bt_fast_pair_partition)
#define FP_PARTITION_SIZE	PARTITION_SIZE(bt_fast_pair_partition)
#endif

/*
 * Layout of the provisioning data blob stored in the flash partition. This must
 * match the layout produced by the provisioning tooling and used by the standard
 * fp_registration_data.c module:
 *
 *   [ magic (4B) ][ Model ID (3B, padded to 4B) ]
 *   [ Anti-Spoofing private key (32B) ][ SHA-256 hash (32B) ]
 *
 * The SHA-256 hash covers everything that precedes it (offset 0 .. FP_HASH_OFF).
 */
#define FP_MAGIC_SIZE		4
#define FP_SHA256_HASH_LEN	32
#define FP_DATA_ALIGN		4

#define FP_MAGIC_OFF		0
#define FP_MODEL_ID_OFF		(FP_MAGIC_OFF + ROUND_UP(FP_MAGIC_SIZE, FP_DATA_ALIGN))
#define FP_ANTI_SPOOFING_KEY_OFF \
	(FP_MODEL_ID_OFF + ROUND_UP(FP_REG_DATA_MODEL_ID_LEN, FP_DATA_ALIGN))
#define FP_HASH_OFF \
	(FP_ANTI_SPOOFING_KEY_OFF + ROUND_UP(FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN, FP_DATA_ALIGN))
#define FP_BLOB_SIZE		(FP_HASH_OFF + ROUND_UP(FP_SHA256_HASH_LEN, FP_DATA_ALIGN))

BUILD_ASSERT(FP_BLOB_SIZE <= FP_PARTITION_SIZE, "Fast Pair registration data partition is too small");

/* Uncompressed SECP256R1 public key: 0x04 || X (32B) || Y (32B). */
#define FP_ECC_PUB_KEY_LEN	(1 + 2 * 32)

/* Handle of the Anti-Spoofing private key in the KMU (RAW usage scheme). */
#define FP_KMU_KEY_ID \
	PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, CONFIG_BT_FAST_PAIR_KMU_SLOT)

/* Protected Storage uid holding the Model ID. */
#define FP_PS_MODEL_ID_UID	((psa_storage_uid_t)CONFIG_BT_FAST_PAIR_PS_ID)

static const uint8_t fp_magic[FP_MAGIC_SIZE] = {0xFA, 0x57, 0xFA, 0x57};

static bool is_enabled;

/* ------------------------------------------------------------------------- *
 *  Provisioning flow helpers
 * ------------------------------------------------------------------------- */

/* Is the Anti-Spoofing private key present in the KMU slot? */
static bool prov_kmu_key_present(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status = psa_get_key_attributes(FP_KMU_KEY_ID, &attr);

	psa_reset_key_attributes(&attr);

	return (status == PSA_SUCCESS);
}

/* Is the Model ID present in Protected Storage? */
static bool prov_model_id_present(void)
{
	struct psa_storage_info_t info;

	return (psa_ps_get_info(FP_PS_MODEL_ID_UID, &info) == PSA_SUCCESS);
}

/* Has the data already been provisioned into KMU and Protected Storage? */
static bool prov_data_present(void)
{
	return prov_kmu_key_present() && prov_model_id_present();
}

/* Confirm the provisioned data is usable (used both as the runtime validity check
 * and as the pre-erase verification during migration).
 */
static bool prov_data_valid(void)
{
	uint8_t pub_key[FP_ECC_PUB_KEY_LEN];
	size_t pub_key_len;
	struct psa_storage_info_t info;

	/* The private key never leaves the KMU, but its public part can always be
	 * derived - a successful export proves the key is present and usable.
	 */
	if (psa_export_public_key(FP_KMU_KEY_ID, pub_key, sizeof(pub_key),
				  &pub_key_len) != PSA_SUCCESS) {
		LOG_ERR("Anti-Spoofing key in KMU slot %d is not usable",
			CONFIG_BT_FAST_PAIR_KMU_SLOT);
		return false;
	}

	if ((psa_ps_get_info(FP_PS_MODEL_ID_UID, &info) != PSA_SUCCESS) ||
	    (info.size != FP_REG_DATA_MODEL_ID_LEN)) {
		LOG_ERR("Model ID in Protected Storage (uid %u) is missing or malformed",
			(unsigned int)FP_PS_MODEL_ID_UID);
		return false;
	}

	return true;
}

/* Read the provisioning blob from the flash partition and validate it
 * (magic marker + SHA-256 integrity hash).
 */
static int prov_flash_read_validate(uint8_t *blob)
{
	const struct flash_area *fa;
	uint8_t hash[FP_SHA256_HASH_LEN];
	size_t hash_len;
	int err;

	err = flash_area_open(FP_PARTITION_ID, &fa);
	if (err) {
		return err;
	}

	err = flash_area_read(fa, 0, blob, FP_BLOB_SIZE);
	flash_area_close(fa);
	if (err) {
		return err;
	}

	if (memcmp(&blob[FP_MAGIC_OFF], fp_magic, FP_MAGIC_SIZE) != 0) {
		LOG_ERR("Fast Pair partition: invalid magic marker");
		return -EINVAL;
	}

	if (psa_hash_compute(PSA_ALG_SHA_256, blob, FP_HASH_OFF,
			     hash, sizeof(hash), &hash_len) != PSA_SUCCESS) {
		LOG_ERR("Fast Pair partition: failed to compute integrity hash");
		return -EIO;
	}

	if (memcmp(hash, &blob[FP_HASH_OFF], FP_SHA256_HASH_LEN) != 0) {
		LOG_ERR("Fast Pair partition: integrity hash mismatch (data corrupted)");
		return -EINVAL;
	}

	return 0;
}

/* Import the Anti-Spoofing private key into the KMU slot as a persistent,
 * non-exportable ECDH key pair.
 */
static int prov_import_key_to_kmu(const uint8_t *priv_key)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id;
	psa_status_t status;

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN * __CHAR_BIT__);
	psa_set_key_lifetime(&attr,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     CRACEN_KEY_PERSISTENCE_READ_ONLY, PSA_KEY_LOCATION_CRACEN_KMU));
	psa_set_key_id(&attr, FP_KMU_KEY_ID);

	status = psa_import_key(&attr, priv_key, FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN, &key_id);
	psa_reset_key_attributes(&attr);

	if (status == PSA_ERROR_ALREADY_EXISTS) {
		LOG_WRN("KMU slot %d already populated - keeping the existing key",
			CONFIG_BT_FAST_PAIR_KMU_SLOT);
		return 0;
	}

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to import Anti-Spoofing key into KMU slot %d (err %d)",
			CONFIG_BT_FAST_PAIR_KMU_SLOT, status);
		return -EIO;
	}

	return 0;
}

/* Store the Model ID in Protected Storage. */
static int prov_store_model_id_to_ps(const uint8_t *model_id)
{
	psa_status_t status = psa_ps_set(FP_PS_MODEL_ID_UID, FP_REG_DATA_MODEL_ID_LEN,
					 model_id, PSA_STORAGE_FLAG_WRITE_ONCE);

	if (status != PSA_SUCCESS) {
		LOG_ERR("Failed to store Model ID in Protected Storage uid %u (err %d)",
			(unsigned int)FP_PS_MODEL_ID_UID, status);
		return -EIO;
	}

	return 0;
}

/* Erase the plaintext provisioning data from the flash partition. */
static int prov_flash_erase(void)
{
	const struct flash_area *fa;
	int err = flash_area_open(FP_PARTITION_ID, &fa);

	if (!err) {
		err = flash_area_erase(fa, 0, FP_PARTITION_SIZE);
		flash_area_close(fa);
	}

	return err;
}

static int fp_provision(void)
{
	uint8_t blob[FP_BLOB_SIZE];
	int err;

	if (prov_data_present()) {
		if (prov_data_valid()) {
			LOG_INF("Fast Pair data already provisioned "
				"(Anti-Spoofing key: KMU slot %d, Model ID: PS uid %u)",
				CONFIG_BT_FAST_PAIR_KMU_SLOT, (unsigned int)FP_PS_MODEL_ID_UID);
			
			memset(blob, 0, sizeof(blob));
			return 0;
		}

		LOG_ERR("Provisioned Fast Pair data is present but invalid");
		memset(blob, 0, sizeof(blob));
		return -EINVAL;
	}

	LOG_INF("Fast Pair data not provisioned - migrating from the flash partition");

	err = prov_flash_read_validate(blob);
	if (err) {
		LOG_ERR("No valid Fast Pair data available to provision (err %d)", err);
		err = -ENODATA;
		memset(blob, 0, sizeof(blob));
		return err;
	}

	err = prov_import_key_to_kmu(&blob[FP_ANTI_SPOOFING_KEY_OFF]);
	if (err) {
		memset(blob, 0, sizeof(blob));
		return err;
	}
	LOG_INF("Anti-Spoofing private key provisioned to KMU slot %d",
		CONFIG_BT_FAST_PAIR_KMU_SLOT);

	err = prov_store_model_id_to_ps(&blob[FP_MODEL_ID_OFF]);
	if (err) {
		memset(blob, 0, sizeof(blob));
		return err;
	}
	LOG_HEXDUMP_INF(&blob[FP_MODEL_ID_OFF], FP_REG_DATA_MODEL_ID_LEN,
			"Model ID provisioned to Protected Storage:");

	if (!prov_data_valid()) {
		LOG_ERR("Provisioned data verification failed");
		err = -EIO;
		memset(blob, 0, sizeof(blob));
		return err;
	}

	memset(blob, 0, sizeof(blob));
	LOG_INF("Fast Pair provisioning completed successfully");
	
	return err;
}

/* ------------------------------------------------------------------------- *
 *  Fast Pair registration data API (KMU / Protected Storage backed)
 * ------------------------------------------------------------------------- */

int fp_reg_data_get_model_id(uint8_t *buf, size_t size)
{
	__ASSERT_NO_MSG(is_enabled);

	psa_status_t status;
	size_t read_len;

	if (size < FP_REG_DATA_MODEL_ID_LEN) {
		return -EINVAL;
	}

	status = psa_ps_get(FP_PS_MODEL_ID_UID, 0, FP_REG_DATA_MODEL_ID_LEN, buf, &read_len);
	if ((status != PSA_SUCCESS) || (read_len != FP_REG_DATA_MODEL_ID_LEN)) {
		LOG_ERR("Failed to read Model ID from Protected Storage (err %d)", status);
		return -EIO;
	}

	return 0;
}

int fp_get_anti_spoofing_priv_key(uint8_t *buf, size_t size)
{
	__ASSERT_NO_MSG(bt_fast_pair_is_ready());

	if (size < FP_REG_DATA_ANTI_SPOOFING_PRIV_KEY_LEN) {
		return -EINVAL;
	}

	// Anti-Spoofing key never leaves the KMU in plaintext - return empty memory space
	memset(buf, 0, size);
	LOG_DBG("Anti-Spoofing key stays in KMU slot %d - no plaintext key returned",
		CONFIG_BT_FAST_PAIR_KMU_SLOT);

	return 0;
}

/* ------------------------------------------------------------------------- *
 *  Fast Pair activation module hooks
 * ------------------------------------------------------------------------- */

static int fp_reg_data_init(void)
{
	if (is_enabled) {
		LOG_WRN("fp_registration_data_provision module already initialized");
		return 0;
	}

	int err_prov = fp_provision();

	int err = prov_flash_erase();
	if (err) {
		LOG_ERR("Failed to erase the Fast Pair flash partition (err %d)", err);
	} else {
		LOG_INF("Fast Pair flash partition erased");
	}

	if (err_prov) {
		return err;
	}

	is_enabled = true;

	return 0;
}

static int fp_reg_data_uninit(void)
{
	is_enabled = false;

	return 0;
}

FP_ACTIVATION_MODULE_REGISTER(fp_reg_data, CONFIG_BT_FAST_PAIR_REGISTRATION_DATA_INIT_PRIORITY,
			      fp_reg_data_init, fp_reg_data_uninit);
