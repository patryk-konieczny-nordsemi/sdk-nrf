/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <psa/internal_trusted_storage.h>
#include <psa/storage_common.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>

LOG_MODULE_REGISTER(fp_provision, LOG_LEVEL_INF);

/* RRAM write granularity on this SoC: 128-bit (16-byte) wordline. */
#define FP_RRAM_WRITE_BLOCK 16U

/* Fast Pair Model ID length (24 bits = 3 bytes). */
#define FP_MODEL_ID_LEN		3U

/* Fast Pair Anti-Spoofing private key length (256 bits = 32 bytes). */
#define FP_PROVISION_ANTI_SPOOFING_KEY_LEN 32U

/* Longest expected base64 encoding of the 32-byte Anti-Spoofing key (+ NUL). */
#define FP_PROVISION_ANTI_SPOOFING_KEY_B64_MAX_LEN 64U

/* Handle of the Anti-Spoofing private key in the KMU (RAW usage scheme). */
#define FP_KMU_KEY_ID \
	PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW, \
					CONFIG_FP_PROVISION_KMU_SLOT)

/* Internal Trusted Storage uid holding the Model ID. */
#define FP_ITS_MODEL_ID_UID	((psa_storage_uid_t)CONFIG_FP_PROVISION_ITS_ID)

BUILD_ASSERT(FP_PROVISION_ANTI_SPOOFING_KEY_B64_MAX_LEN % FP_RRAM_WRITE_BLOCK == 0U,
	"Key storage must be a whole number of RRAM write blocks");

static const char fp_anti_spoofing_key_b64[FP_PROVISION_ANTI_SPOOFING_KEY_B64_MAX_LEN] __aligned(16) =
        CONFIG_FP_PROVISION_ANTI_SPOOFING_KEY;


static int purge_sram_secret(void *buf, size_t len)
{
	if ((buf == NULL) || (len == 0U)) {
		LOG_ERR("Invalid SRAM purge request");
		return -EINVAL;
	}

	memset(buf, 0, len);

	return 0;
}

static int purge_rram_secret(const void *addr, size_t len)
{
	const struct device *flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	uint8_t zeros[FP_PROVISION_ANTI_SPOOFING_KEY_B64_MAX_LEN] = {0};
	size_t wbs;
	off_t off;
	int err;

	if ((addr == NULL) || (len == 0U) || (len > sizeof(zeros))) {
		LOG_ERR("Invalid RRAM purge request");
		return -EINVAL;
	}

	if (!device_is_ready(flash_dev)) {
		LOG_ERR("Flash device not ready");
		return -ENODEV;
	}

	wbs = flash_get_write_block_size(flash_dev);
	off = (off_t)(uintptr_t)addr;


	if ((((size_t)off % wbs) != 0U) || ((len % wbs) != 0U)) {
		LOG_ERR("RRAM purge region not write-block aligned "
			"(off %ld, len %zu, wbs %zu)", (long)off, len, wbs);
		return -EINVAL;
	}

	err = flash_write(flash_dev, off, zeros, len);
	if (err != 0) {
		LOG_ERR("Failed to purge secret from RRAM (err %d)", err);
		return -EIO;
	}

	return 0;
}

static int provision_init(void)
{
	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed (err %d)", status);
		return -EIO;
	}

	return 0;
}

static int provision_anti_spoofing_key(void)
{
	const size_t key_b64_len = strlen(fp_anti_spoofing_key_b64);
	uint8_t fp_provision_anti_spoofing_key[FP_PROVISION_ANTI_SPOOFING_KEY_LEN] = {0};
	size_t bytes_written = 0;
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;
	int err;

	if (key_b64_len == 0U) {
		LOG_ERR("Anti-Spoofing key is empty");
		return -EINVAL;
	}

	err = base64_decode(fp_provision_anti_spoofing_key,
			    sizeof(fp_provision_anti_spoofing_key), &bytes_written,
			    (const uint8_t *)fp_anti_spoofing_key_b64, key_b64_len);
	if (err != 0) {
		LOG_ERR("Anti-Spoofing key base64 decode failed (err %d)", err);
		return -EINVAL;
	}

	if (bytes_written != FP_PROVISION_ANTI_SPOOFING_KEY_LEN) {
		LOG_ERR("Anti-Spoofing key has invalid length (%zu)", bytes_written);
		return -EINVAL;
	}

	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, FP_PROVISION_ANTI_SPOOFING_KEY_LEN * __CHAR_BIT__);
	psa_set_key_lifetime(&attr,
			     PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
				     CRACEN_KEY_PERSISTENCE_READ_ONLY,
				     PSA_KEY_LOCATION_CRACEN_KMU));
	psa_set_key_id(&attr, FP_KMU_KEY_ID);

	status = psa_import_key(&attr, fp_provision_anti_spoofing_key,
				FP_PROVISION_ANTI_SPOOFING_KEY_LEN, &key_id);
	psa_reset_key_attributes(&attr);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Anti-Spoofing key provisioning failed (err %d)", status);
		return -EIO;
	}

	LOG_INF("Anti-Spoofing key provisioned to KMU slot %d", CONFIG_FP_PROVISION_KMU_SLOT);

	err = purge_rram_secret(fp_anti_spoofing_key_b64, FP_PROVISION_ANTI_SPOOFING_KEY_B64_MAX_LEN);
	if (err != 0) {
		return err;
	}

	err = purge_sram_secret(fp_provision_anti_spoofing_key,
				sizeof(fp_provision_anti_spoofing_key));
	if (err != 0) {
		LOG_ERR("Failed to purge Anti-Spoofing key from SRAM (err %d)", err);
		return err;
	}

	return 0;
}

static int provision_model_id(void)
{
	uint8_t model_id[FP_MODEL_ID_LEN];
	psa_status_t status;

	sys_put_be24(CONFIG_FP_PROVISION_MODEL_ID, model_id);

	status = psa_its_set(FP_ITS_MODEL_ID_UID, sizeof(model_id), model_id,
			     PSA_STORAGE_FLAG_WRITE_ONCE);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Model ID provisioning failed (err %d)", status);
		return -EIO;
	}

	LOG_INF("Model ID 0x%06x provisioned to Internal Trusted Storage (uid %u)",
		CONFIG_FP_PROVISION_MODEL_ID, (unsigned int)FP_ITS_MODEL_ID_UID);

	return 0;
}

int main(void)
{
	LOG_INF("Fast Pair provisioner started");

	if (provision_init()) {
		return 0;
	}

	if (provision_anti_spoofing_key()) {
		return 0;
	}

	if (provision_model_id()) {
		return 0;
	}

	LOG_INF("Fast Pair provisioning completed successfully");

	return 0;
}
