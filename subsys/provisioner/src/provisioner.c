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
#include <cracen_psa_kmu.h>

#include <provisioner/provisioner.h>

LOG_MODULE_REGISTER(provisioner, CONFIG_PROVISIONER_LOG_LEVEL);

static int provision_init(void)
{
	psa_status_t status = psa_crypto_init();

	if (status != PSA_SUCCESS) {
		LOG_ERR("psa_crypto_init failed (err: %d)", status);
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

static int provision_its_entry_verify(const struct provisioner_its_entry *entry,
				      const uint8_t *payload, size_t payload_len,
				      const struct psa_storage_info_t *info)
{
	uint8_t stored[CONFIG_PROVISIONER_MAX_DATA_SIZE];
	size_t stored_len = 0;
	psa_status_t status;
	int ret = 0;

	if (info->flags != entry->config.create_flags || info->size != payload_len) {
		LOG_ERR("Entry %s: data already stored (configuration mismatch) "
			"in ITS uid: 0x%08x", entry->name, (unsigned int)entry->config.uid);
		return -EEXIST;
	}

	status = psa_its_get(entry->config.uid, 0, info->size, stored, &stored_len);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Entry %s: ITS read failed (err: %d)", entry->name, status);
		return -EIO;
	}

	if (stored_len == payload_len && memcmp(stored, payload, payload_len) == 0) {
		LOG_WRN("Entry %s: identical data already provisioned in ITS uid: 0x%08x",
			entry->name, (unsigned int)entry->config.uid);
	} else {
		LOG_ERR("Entry %s: data already stored (different value) in ITS uid: 0x%08x",
			entry->name, (unsigned int)entry->config.uid);
		ret = -EEXIST;
	}

	mbedtls_platform_zeroize(stored, sizeof(stored));

	return ret;
}

static int provision_its_entries_run(void)
{
	int ret_val = 0;

	STRUCT_SECTION_FOREACH(provisioner_its_entry, entry) {
		uint8_t payload[CONFIG_PROVISIONER_MAX_DATA_SIZE];
		struct psa_storage_info_t info;
		size_t payload_len = 0;
		psa_status_t status;
		int err;

		err = provision_payload_get(&entry->prov_data, payload, sizeof(payload),
					    &payload_len);
		if (err != 0) {
			LOG_ERR("Entry %s: invalid payload (err: %d)", entry->name, err);
			mbedtls_platform_zeroize(payload, sizeof(payload));
			ret_val = (ret_val) ? ret_val : err;
			continue;
		}

		status = psa_its_get_info(entry->config.uid, &info);
		if (status != PSA_SUCCESS && status != PSA_ERROR_DOES_NOT_EXIST) {
			LOG_ERR("Entry %s: ITS info check failed (err: %d)", entry->name, status);
			mbedtls_platform_zeroize(payload, sizeof(payload));
			ret_val = (ret_val) ? ret_val : -EIO;
			continue;
		}

		if (status == PSA_SUCCESS) {
			err = provision_its_entry_verify(entry, payload, payload_len, &info);
			if (err != 0) {
				ret_val = (ret_val) ? ret_val : err;
			}
			mbedtls_platform_zeroize(payload, sizeof(payload));
			continue;
		}

		status = psa_its_set(entry->config.uid, payload_len, payload,
				     entry->config.create_flags);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Entry %s: ITS write failed (err: %d)", entry->name, status);
			mbedtls_platform_zeroize(payload, sizeof(payload));
			ret_val = (ret_val) ? ret_val : -EIO;
			continue;
		}

		LOG_INF("Entry %s: provisioned to ITS uid: 0x%08x", entry->name,
			(unsigned int)entry->config.uid);

		mbedtls_platform_zeroize(payload, sizeof(payload));
	}

	return ret_val;
}

static int provision_kmu_entries_run(void)
{
	int ret_val = 0;

	STRUCT_SECTION_FOREACH(provisioner_kmu_entry, entry) {
		unsigned int slot = CRACEN_PSA_GET_KMU_SLOT(entry->config.id);
		uint8_t payload[CONFIG_PROVISIONER_MAX_DATA_SIZE];
		size_t payload_len = 0;
		psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
		psa_key_id_t key_id = PSA_KEY_ID_NULL;
		psa_status_t status;
		int err;

		err = provision_payload_get(&entry->prov_data, payload, sizeof(payload),
					    &payload_len);
		if (err != 0) {
			LOG_ERR("Entry %s: invalid payload (err: %d)", entry->name, err);
			mbedtls_platform_zeroize(payload, sizeof(payload));
			ret_val = (ret_val) ? ret_val : err;
			continue;
		}

		if (payload_len != PSA_BITS_TO_BYTES(entry->config.key_bits)) {
			LOG_ERR("Entry %s: decoded length %zu does not match key size %zu bits",
				entry->name, payload_len, entry->config.key_bits);
			mbedtls_platform_zeroize(payload, sizeof(payload));
			ret_val = (ret_val) ? ret_val : -EINVAL;
			continue;
		}

		psa_set_key_id(&attr, entry->config.id);
		psa_set_key_type(&attr, entry->config.type);
		psa_set_key_bits(&attr, entry->config.key_bits);
		psa_set_key_lifetime(&attr, entry->config.lifetime);
		psa_set_key_usage_flags(&attr, entry->config.usage_flags);
		psa_set_key_algorithm(&attr, entry->config.alg);

		status = psa_import_key(&attr, payload, payload_len, &key_id);
		psa_reset_key_attributes(&attr);
		if (status == PSA_ERROR_ALREADY_EXISTS) {
			LOG_ERR("Entry %s: key collision - KMU slot %u already occupied",
				entry->name, slot);
			mbedtls_platform_zeroize(payload, sizeof(payload));
			ret_val = (ret_val) ? ret_val : -EEXIST;
			continue;
		}
		if (status != PSA_SUCCESS) {
			LOG_ERR("Entry %s: KMU import failed (err: %d)", entry->name, status);
			mbedtls_platform_zeroize(payload, sizeof(payload));
			ret_val = (ret_val) ? ret_val : -EIO;
			continue;
		}

		status = psa_purge_key(key_id);
		if (status != PSA_SUCCESS) {
			LOG_ERR("Entry %s: KMU psa_purge_key failed (err: %d)", entry->name,
				status);
			mbedtls_platform_zeroize(payload, sizeof(payload));
			ret_val = (ret_val) ? ret_val : -EIO;
			continue;
		}

		LOG_INF("Entry %s: provisioned to KMU slot: %u", entry->name, slot);

		mbedtls_platform_zeroize(payload, sizeof(payload));
	}

	return ret_val;
}

int provisioner_run(void)
{
	int err;
	int kmu_err;
	int its_err;

	LOG_INF("Provisioner started");

	err = provision_init();
	if (err != 0) {
		return err;
	}

	kmu_err = provision_kmu_entries_run();
	if (kmu_err != 0) {
		LOG_ERR("KMU provisioning failed (err: %d); see entry logs above for details",
			kmu_err);
	} else {
		LOG_INF("KMU provisioning finished - all entries OK");
	}

	its_err = provision_its_entries_run();
	if (its_err != 0) {
		LOG_ERR("ITS provisioning failed (err: %d); see entry logs above for details",
			its_err);
	} else {
		LOG_INF("ITS provisioning finished - all entries OK");
	}

	if (kmu_err != 0 || its_err != 0) {
		LOG_ERR("Provisioning failed");
		return (kmu_err != 0) ? kmu_err : its_err;
	}

	LOG_INF("Provisioning completed successfully");
	return 0;
}
