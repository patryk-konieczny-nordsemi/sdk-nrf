/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <psa/crypto.h>

#include <psa/internal_trusted_storage.h>

#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(fast_pair, CONFIG_BT_FAST_PAIR_LOG_LEVEL);

#include <bluetooth/fast_pair/fast_pair.h>
#include "fp_activation.h"
#include "fp_registration_data.h"

/* Length of an exported SECP-R1 256-bit public key in uncompressed form
 * (0x04 || X || Y). Only used to size the scratch buffer for the key usability
 * check.
 */
#define FP_ECC_PUB_KEY_LEN	65U

/* Handle of the Anti-Spoofing private key in the KMU (RAW usage scheme). */
#define FP_KMU_KEY_ID \
	PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(
		CRACEN_KMU_KEY_USAGE_SCHEME_RAW, CONFIG_BT_FAST_PAIR_KMU_SLOT)

/* Internal Trusted Storage uid holding the Model ID. */
#define FP_ITS_MODEL_ID_UID	((psa_storage_uid_t)CONFIG_BT_FAST_PAIR_ITS_ID)

static bool is_enabled;

/* Is the Anti-Spoofing private key present in the KMU slot? */
static bool prov_kmu_key_present(void)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_status_t status = psa_get_key_attributes(FP_KMU_KEY_ID, &attr);

	psa_reset_key_attributes(&attr);

	return (status == PSA_SUCCESS);
}

/* Is the Model ID present in Internal Trusted Storage? */
static bool prov_model_id_present(void)
{
	struct psa_storage_info_t info;

	return (psa_its_get_info(FP_ITS_MODEL_ID_UID, &info) == PSA_SUCCESS);
}

/* Has the data been provisioned into both the KMU and Internal Trusted Storage? */
static bool prov_data_present(void)
{
	return prov_kmu_key_present() && prov_model_id_present();
}

/* Confirm the provisioned data is usable. */
static bool prov_data_valid(void)
{
	uint8_t pub_key[FP_ECC_PUB_KEY_LEN];
	size_t pub_key_len;
	struct psa_storage_info_t info;

	if (psa_export_public_key(FP_KMU_KEY_ID, pub_key, sizeof(pub_key),
				&pub_key_len) != PSA_SUCCESS) {
		LOG_ERR("Anti-Spoofing key in KMU slot %d is not usable",
			CONFIG_BT_FAST_PAIR_KMU_SLOT);
		return false;
	}

	if ((psa_its_get_info(FP_ITS_MODEL_ID_UID, &info) != PSA_SUCCESS) ||
		(info.size != FP_REG_DATA_MODEL_ID_LEN)) {
		LOG_ERR("Model ID in Internal Trusted Storage (uid %u) is missing or malformed",
			(unsigned int)FP_ITS_MODEL_ID_UID);
		return false;
	}

	return true;
}

int fp_reg_data_get_model_id(uint8_t *buf, size_t size)
{
	__ASSERT_NO_MSG(is_enabled);

	psa_status_t status;
	size_t read_len;

	if (size < FP_REG_DATA_MODEL_ID_LEN) {
		return -EINVAL;
	}

	status = psa_its_get(FP_ITS_MODEL_ID_UID, 0, FP_REG_DATA_MODEL_ID_LEN, buf, &read_len);
	if ((status != PSA_SUCCESS) || (read_len != FP_REG_DATA_MODEL_ID_LEN)) {
		LOG_ERR("Failed to read Model ID from Internal Trusted Storage (err %d)", status);
		return -EIO;
	}

	return 0;
}

static int fp_reg_data_init(void)
{
	if (is_enabled) {
		LOG_WRN("fp_registration_data module already initialized");
		return 0;
	}

	if (!prov_data_present() || !prov_data_valid()) {
		LOG_ERR("Fast Pair data not provisioned - build and flash the provisioner "
			"image (FILE_SUFFIX=provisioner) before running this application");
		return -EINVAL;
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
