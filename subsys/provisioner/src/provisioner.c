/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/base64.h>
#include <mbedtls/platform_util.h>

#include <psa/crypto.h>
#include <psa/internal_trusted_storage.h>

#include <provisioner/provisioner.h>

LOG_MODULE_REGISTER(provisioner, CONFIG_PROVISIONER_LOG_LEVEL);

static int provision_init(void)
{
	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed (err %d)", status);
		return -EIO;
	}

	return 0;
}

static int provision_payload_get(const struct provisioner_data *prov_data, uint8_t *buf,
				 size_t buf_len, size_t *out_len)
{
	if (prov_data == NULL || prov_data->data == NULL || buf == NULL || out_len == NULL) {
		return -EINVAL;
	}

	switch (prov_data->format) {
	case PROVISIONER_DATA_FORMAT_RAW:
		if (prov_data->payload_length > buf_len) {
			return -EINVAL;
		}

		memcpy(buf, prov_data->data, prov_data->payload_length);
		*out_len = prov_data->payload_length;
		return 0;

	case PROVISIONER_DATA_FORMAT_BASE64: {
		size_t enc_len = strnlen(prov_data->data, prov_data->payload_length);
		int err;

		if (enc_len == 0U) {
			return -EINVAL;
		}

		err = base64_decode(buf, buf_len, out_len, prov_data->data, enc_len);
		if (err != 0) {
			return -EINVAL;
		}

		return 0;
	}

	default:
		return -EINVAL;
	}
}

static int provision_its_entries_run(void)
{
	STRUCT_SECTION_FOREACH(provisioner_its_entry, entry) {
		uint8_t payload[CONFIG_PROVISIONER_MAX_DATA_SIZE];
		size_t payload_len = 0;
		psa_status_t status;
		int err;

		err = provision_payload_get(&entry->prov_data, payload, sizeof(payload),
					    &payload_len);
		if (err != 0) {
			LOG_ERR("Entry %s: invalid payload (err %d)", entry->name, err);
			mbedtls_platform_zeroize(payload, payload_len);
			return err;
		}

		status = psa_its_set(entry->config.uid, payload_len, payload,
				     entry->config.create_flags);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Entry %s: ITS write failed (err %d)", entry->name, status);
			mbedtls_platform_zeroize(payload, payload_len);
			return -EIO;
		}

		LOG_INF("Entry %s: provisioned to ITS uid: 0x%08x", entry->name,
			(unsigned int)entry->config.uid);

		mbedtls_platform_zeroize(payload, payload_len);
	}

	return 0;
}

static int provision_kmu_entries_run(void)
{
	STRUCT_SECTION_FOREACH(provisioner_kmu_entry, entry) {
		uint8_t payload[CONFIG_PROVISIONER_MAX_DATA_SIZE];
		size_t payload_len = 0;
		psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_key_id_t key_id = PSA_KEY_ID_NULL;
		psa_status_t status;
		int err;

		err = provision_payload_get(&entry->prov_data, payload, sizeof(payload),
					    &payload_len);
		if (err != 0) {
			LOG_ERR("Entry %s: invalid payload (err %d)", entry->name, err);
			mbedtls_platform_zeroize(payload, payload_len);
			return err;
		}

		if (payload_len != (entry->config.key_bits / CHAR_BIT)) {
			LOG_ERR("Entry %s: decoded length %zu does not match key size %zu bits",
				entry->name, payload_len, entry->config.key_bits);
			mbedtls_platform_zeroize(payload, payload_len);
			return -EINVAL;
		}

		psa_set_key_id(&attr, entry->config.id);
		psa_set_key_type(&attr, entry->config.type);
		psa_set_key_bits(&attr, entry->config.key_bits);
		psa_set_key_lifetime(&attr, entry->config.lifetime);
		psa_set_key_usage_flags(&attr, entry->config.usage_flags);
		psa_set_key_algorithm(&attr, entry->config.alg);

		status = psa_import_key(&attr, payload, payload_len, &key_id);
		psa_reset_key_attributes(&attr);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Entry %s: KMU import failed (err %d)", entry->name, status);
			mbedtls_platform_zeroize(payload, payload_len);
			return -EIO;
		}

		status = psa_purge_key(key_id);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Entry %s: KMU psa_purge_key failed (err: %d)", entry->name,
				status);
			mbedtls_platform_zeroize(payload, payload_len);
			return -ECANCELED;
		}

		LOG_INF("Entry %s: provisioned to KMU id: %u", entry->name, entry->config.id);

		mbedtls_platform_zeroize(payload, payload_len);
	}

	return 0;
}

int provisioner_run(void)
{
	int err;

	LOG_INF("Provisioner started");

	err = provision_init();
	if (err != 0) {
		return err;
	}

	err = provision_kmu_entries_run();
	if (err != 0) {
		return err;
	}

	err = provision_its_entries_run();
	if (err != 0) {
		return err;
	}

	LOG_INF("Provisioning completed successfully");

	return 0;
}
