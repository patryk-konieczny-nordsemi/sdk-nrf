/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/util.h>

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


static int provision_validate_source(const char *name, const void *data,
				     size_t payload_length, size_t storage_length)
{
	if ((data == NULL) || (payload_length == 0U)) {
		LOG_ERR("Entry %s: missing source data", name);
		return -EINVAL;
	}

	return 0;
}

static int purge_sram_secret(void *buf, size_t len)
{
	if ((buf == NULL) || (len == 0U)) {
		return -EINVAL;
	}

	memset(buf, 0, len);

	return 0;
}

static int purge_rram_secret(const void *addr, size_t len)
{
	const struct device *flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
	uint8_t zeros[PROVISIONER_RRAM_ERASE_MAX_LEN];
	off_t off;
	int err;

	if ((addr == NULL) || (len == 0U)) {
		return -EINVAL;
	}

	/* Caller must have validated via provision_validate_rram_erase(). */
	off = (off_t)(uintptr_t)addr;
	memset(zeros, 0, sizeof(zeros));
	err = flash_write(flash_dev, off, zeros, len);
	if (err != 0) {
		LOG_ERR("Failed to purge secret from RRAM (err %d)", err);
		return -EIO;
	}

	return 0;
}

static int provision_get_payload(const void *data_in, size_t payload_length,
				 enum provisioner_data_format format, uint8_t *buf,
				 size_t buf_len, size_t *out_len)
{
	switch (format) {
	case PROVISIONER_DATA_FORMAT_RAW:
		if (payload_length > buf_len) {
			return -EINVAL;
		}

		memcpy(buf, data_in, payload_length);
		*out_len = payload_length;
		return 0;

	case PROVISIONER_DATA_FORMAT_BASE64: {
		size_t enc_len = strnlen(data_in, payload_length);
		int err;

		if (enc_len == 0U) {
			return -EINVAL;
		}

		err = base64_decode(buf, buf_len, out_len, data_in, enc_len);
		if (err != 0) {
			return -EINVAL;
		}

		return 0;
	}

	default:
		return -EINVAL;
	}
}

static int run_provision_its_entries(void)
{
	STRUCT_SECTION_FOREACH(provisioner_its_entry, entry) {
		uint8_t payload[PROVISIONER_RRAM_ERASE_MAX_LEN];
		size_t payload_len = 0;
		psa_status_t status;
		int err;

		err = provision_validate_source(entry->name, entry->data, entry->payload_length,
						entry->storage_length);
		if (err != 0) {
			return err;
		}

		err = provision_get_payload(entry->data, entry->payload_length, entry->format,
					    payload, sizeof(payload), &payload_len);
		if (err != 0) {
			LOG_ERR("Entry %s: invalid payload (err %d)", entry->name, err);
			return err;
		}

		status = psa_its_set(entry->uid, payload_len, payload, entry->create_flags);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Entry %s: ITS write failed (err %d)", entry->name, status);
			return -EIO;
		}

		LOG_INF("Entry %s: provisioned to ITS uid: %u", entry->name, (unsigned int)entry->uid);

		if (entry->storage_length != 0U) {
			err = purge_rram_secret(entry->data, entry->storage_length);
			if (err != 0) {
				return err;
			}
		}
	}

	return 0;
}

static int run_provision_kmu_entries(void)
{
	STRUCT_SECTION_FOREACH(provisioner_kmu_entry, entry) {
		uint8_t payload[PROVISIONER_RRAM_ERASE_MAX_LEN];
		size_t payload_len = 0;
		psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_key_id_t key_id = PSA_KEY_ID_NULL;
		psa_status_t status;
		int err;

		err = provision_validate_source(entry->name, entry->data, entry->payload_length,
						entry->storage_length);
		if (err != 0) {
			return err;
		}

		err = provision_get_payload(entry->data, entry->payload_length, entry->format,
					    payload, sizeof(payload), &payload_len);
		if (err != 0) {
			LOG_ERR("Entry %s: invalid payload (err %d)", entry->name, err);
			return err;
		}

		if (payload_len != (entry->key_bits / CHAR_BIT)) {
			LOG_ERR("Entry %s: decoded length %zu does not match key size %zu bits",
				entry->name, payload_len, entry->key_bits);
			return -EINVAL;
		}

		psa_set_key_id(&attr, entry->id);
		psa_set_key_type(&attr, entry->type);
		psa_set_key_bits(&attr, entry->key_bits);
		psa_set_key_lifetime(&attr, entry->lifetime);
		psa_set_key_usage_flags(&attr, entry->usage_flags);
		psa_set_key_algorithm(&attr, entry->alg);

		status = psa_import_key(&attr, payload, payload_len, &key_id);
		psa_reset_key_attributes(&attr);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Entry %s: KMU import failed (err %d)", entry->name, status);
			return -EIO;
		}

		LOG_INF("Entry %s: provisioned to KMU id: %d", entry->name, entry->id);

		err = purge_sram_secret(payload, payload_len);
		if (err != 0) {
			LOG_ERR("Entry %s: failed to purge decoded key from SRAM (err %d)",
				entry->name, err);
			return err;
		}

		// if (entry->storage_length != 0U) {
		// 	err = purge_rram_secret(entry->data, entry->storage_length);
		// 	if (err != 0) {
		// 		return err;
		// 	}
		// }
	}

	return 0;
}

static int run_provision_callbacks(void)
{
	STRUCT_SECTION_FOREACH(provisioner_generic_callback, entry) {
		int err;

		LOG_INF("Running provisioner callback: %s", entry->name);
		err = entry->callback();
		if (err != 0) {
			LOG_ERR("Provisioner callback %s failed (err %d)", entry->name, err);
			return err;
		}
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

	err = run_provision_kmu_entries();
	if (err != 0) {
		return err;
	}

	err = run_provision_its_entries();
	if (err != 0) {
		return err;
	}

	err = run_provision_callbacks();
	if (err != 0) {
		return err;
	}

	LOG_INF("Provisioning completed successfully");

	return 0;
}
