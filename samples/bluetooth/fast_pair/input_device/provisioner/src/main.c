/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 #include <string.h>
 #include <zephyr/kernel.h>
 #include <zephyr/logging/log.h>

 #include <psa/crypto.h>
 #include <psa/crypto_extra.h>
 #include <psa/protected_storage.h>
 #include <psa/storage_common.h>
 #include <cracen_psa_kmu.h>
 #include <stdint.h>
 #include <zephyr/sys/byteorder.h>
 #include <zephyr/sys/base64.h>

LOG_MODULE_REGISTER(provisioner, LOG_LEVEL_INF);

int main(void)
{
	LOG_INF("Provisioner image running");

	LOG_INF("Fast Pair Model ID: 0x%06x", CONFIG_PROV_FP_MODEL_ID);
	LOG_INF("Anti-Spoofing key (base64, %zu chars): %s",
		strlen(CONFIG_PROV_FP_ANTI_SPOOFING_KEY),
		CONFIG_PROV_FP_ANTI_SPOOFING_KEY);
	
	uint8_t fp_device_id[3] = {};
	sys_put_be24(CONFIG_PROV_FP_MODEL_ID, fp_device_id);
	
	const char *fp_anti_spoofing_key_b64 = CONFIG_PROV_FP_ANTI_SPOOFING_KEY;
	uint8_t fp_anti_spoofing_key[32] = {};
	size_t bytes_written = 0;
	int ret = base64_decode(fp_anti_spoofing_key, sizeof(fp_anti_spoofing_key), &bytes_written, 
                            (const uint8_t *)fp_anti_spoofing_key_b64, strlen(fp_anti_spoofing_key_b64));
	
	LOG_HEXDUMP_INF(fp_device_id, sizeof(fp_device_id),"Device ID: ");
	LOG_HEXDUMP_INF(fp_anti_spoofing_key, sizeof(fp_anti_spoofing_key),"Anti-Spoofing Key: ");

	if (ret == 0) {
		LOG_INF("Decoding successful!");
		LOG_INF("Decoded %zu bytes.", bytes_written);
	} else {
		LOG_ERR("Decoding failed with error code: %d\n", ret);
		return 0;
	}

	psa_status_t status;
	status = psa_crypto_init();
	if (status != PSA_SUCCESS) {
		LOG_ERR("PSA Crypto initialization failed! Error code: %d", status);
		return -1;
	}
	LOG_INF("PSA Crypto initialized successfully!");
	
	psa_key_attributes_t key_attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;

	psa_set_key_usage_flags(&key_attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&key_attr, PSA_ALG_ECDH);
	psa_set_key_type(&key_attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&key_attr, 256);

	psa_set_key_lifetime(&key_attr,
        PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
            PSA_KEY_PERSISTENCE_DEFAULT,
            PSA_KEY_LOCATION_CRACEN_KMU));

	psa_set_key_id(&key_attr,
		PSA_KEY_HANDLE_FROM_CRACEN_KMU_SLOT(
			CRACEN_KMU_KEY_USAGE_SCHEME_RAW,
			170));

	status = psa_import_key(&key_attr, fp_anti_spoofing_key, bytes_written, &key_id);
	if (status != PSA_SUCCESS) {
		LOG_ERR("Anti-spoofing key provision failed: %d", status);
		psa_reset_key_attributes(&key_attr);
		return -1;
	}

	psa_reset_key_attributes(&key_attr);
	LOG_INF("Anti-spoofing key provisioned successfully!");


    status = psa_ps_set(
        0x00000001,          /* uid: unique identifier */
        sizeof(fp_device_id),      /* data length (3 bytes) */
        fp_device_id,              /* pointer to data */
        PSA_STORAGE_FLAG_NONE);    /* create flags */

    if (status != PSA_SUCCESS) {
        LOG_ERR("Failed to store device ID: %d", status);
    }
	LOG_INF("Device ID provisioned successfully!");

	LOG_INF("Provisioning completed");
	return 0;
}