/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <psa/crypto.h>
#include <psa/internal_trusted_storage.h>
#include <cracen_psa_kmu.h>
#include <cracen_psa_key_ids.h>

#include <zephyr/ztest.h>

#define FP_KMU_KEY_ID										\
	PSA_KEY_ID_FROM_CRACEN_KMU_SLOT(CRACEN_KMU_KEY_USAGE_SCHEME_RAW,		        \
					CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT)

#define FP_ITS_MODEL_ID_UID ((psa_storage_uid_t)CONFIG_BT_FAST_PAIR_MODEL_ID_ITS_ID)

#define FP_PRIV_KEY_LEN PSA_BITS_TO_BYTES(256)
#define FP_PUB_KEY_LEN  PSA_EXPORT_PUBLIC_KEY_OUTPUT_SIZE( \
	PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1), 256)
	
static const uint8_t expected_model_id[] = {
	(CONFIG_TEST_BT_FAST_PAIR_EXPECTED_MODEL_ID >> 16) & 0xFF,
	(CONFIG_TEST_BT_FAST_PAIR_EXPECTED_MODEL_ID >> 8) & 0xFF,
	CONFIG_TEST_BT_FAST_PAIR_EXPECTED_MODEL_ID & 0xFF,
};

/* Valid secp256r1 private key (scalar 1) used only to attempt overwriting the KMU slot. */
static const uint8_t dummy_priv_key[FP_PRIV_KEY_LEN] = {[FP_PRIV_KEY_LEN - 1] = 0x01};

static void assert_pub_key_matches_expected(void)
{
	const char *hex = CONFIG_TEST_BT_FAST_PAIR_EXPECTED_PUB_KEY;
	uint8_t expected_pub_key[FP_PUB_KEY_LEN];
	uint8_t pub_key[FP_PUB_KEY_LEN];
	psa_status_t status;
	size_t len;

	zassert_equal(hex2bin(hex, strlen(hex), expected_pub_key, sizeof(expected_pub_key)),
		      sizeof(expected_pub_key),
		      "CONFIG_TEST_BT_FAST_PAIR_EXPECTED_PUB_KEY is not a %d-byte hex string",
		      FP_PUB_KEY_LEN);

	status = psa_export_public_key(FP_KMU_KEY_ID, pub_key, sizeof(pub_key), &len);
	zassert_equal(status, PSA_SUCCESS,
		      "Anti-Spoofing public key export from KMU slot %d failed (status %d)",
		      CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT, status);
	zassert_equal(len, sizeof(expected_pub_key),
		      "Anti-Spoofing public key has %zu bytes, expected %zu", len,
		      sizeof(expected_pub_key));
	zassert_mem_equal(pub_key, expected_pub_key, sizeof(expected_pub_key),
			  "Anti-Spoofing public key does not match the expected public key");
}

static void assert_model_id_matches_expected(void)
{
	uint8_t buf[sizeof(expected_model_id)];
	psa_status_t status;
	size_t len;

	status = psa_its_get(FP_ITS_MODEL_ID_UID, 0, sizeof(buf), buf, &len);
	zassert_equal(status, PSA_SUCCESS, "Model ID read from ITS uid 0x%08x failed (status %d)",
		      (unsigned int)FP_ITS_MODEL_ID_UID, status);
	zassert_equal(len, sizeof(expected_model_id), "Model ID has %zu bytes, expected %zu",
		      len, sizeof(expected_model_id));
	zassert_mem_equal(buf, expected_model_id, sizeof(expected_model_id),
			  "Model ID 0x%02x%02x%02x does not match the expected 0x%06x",
			  buf[0], buf[1], buf[2], CONFIG_TEST_BT_FAST_PAIR_EXPECTED_MODEL_ID);
}

ZTEST(fp_provisioner_post_prov, test_ask_validate)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_lifetime_t lifetime;
	psa_status_t status;

	status = psa_get_key_attributes(FP_KMU_KEY_ID, &attr);
	zassert_equal(status, PSA_SUCCESS,
		      "Anti-Spoofing key attributes read from KMU slot %d failed (status %d)",
		      CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT, status);

	lifetime = psa_get_key_lifetime(&attr);

	zexpect_equal(psa_get_key_type(&attr), PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1),
		      "Key type 0x%04x is not a secp256r1 key pair", psa_get_key_type(&attr));
	zexpect_equal(psa_get_key_bits(&attr), 256, "Key size is %zu bits, expected 256",
		      psa_get_key_bits(&attr));
	zexpect_equal(PSA_KEY_LIFETIME_GET_LOCATION(lifetime), PSA_KEY_LOCATION_CRACEN_KMU,
		      "Key location 0x%06x is not the CRACEN KMU",
		      PSA_KEY_LIFETIME_GET_LOCATION(lifetime));
	zexpect_equal(PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime), CRACEN_KEY_PERSISTENCE_READ_ONLY,
		      "Key persistence 0x%02x is not read-only",
		      PSA_KEY_LIFETIME_GET_PERSISTENCE(lifetime));
	zexpect_equal(psa_get_key_usage_flags(&attr), PSA_KEY_USAGE_DERIVE,
		      "Key usage flags 0x%08x, expected only PSA_KEY_USAGE_DERIVE",
		      psa_get_key_usage_flags(&attr));
	zexpect_equal(psa_get_key_algorithm(&attr), PSA_ALG_ECDH,
		      "Key algorithm 0x%08x, expected PSA_ALG_ECDH", psa_get_key_algorithm(&attr));

	psa_reset_key_attributes(&attr);
}

ZTEST(fp_provisioner_post_prov, test_ask_value)
{
	assert_pub_key_matches_expected();
}

ZTEST(fp_provisioner_post_prov, test_ask_raw_export_blocked)
{
	uint8_t buf[FP_PRIV_KEY_LEN];
	size_t len = SIZE_MAX;
	psa_status_t status;

	status = psa_export_key(FP_KMU_KEY_ID, buf, sizeof(buf), &len);
	zassert_equal(status, PSA_ERROR_NOT_PERMITTED,
		      "Raw export of the Anti-Spoofing private key was not blocked (status %d)",
		      status);
	zassert_equal(len, 0, "Failed export reported %zu bytes of key material", len);
}

ZTEST(fp_provisioner_post_prov, test_ask_destroy_blocked)
{
	psa_status_t status = psa_destroy_key(FP_KMU_KEY_ID);

	zassert_equal(status, PSA_ERROR_NOT_PERMITTED,
		      "Destroying the read-only Anti-Spoofing key was not blocked (status %d)",
		      status);

	assert_pub_key_matches_expected();
}

ZTEST(fp_provisioner_post_prov, test_ask_overwrite_blocked)
{
	psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t key_id = PSA_KEY_ID_NULL;
	psa_status_t status;

	psa_set_key_id(&attr, FP_KMU_KEY_ID);
	psa_set_key_type(&attr, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
	psa_set_key_bits(&attr, FP_PRIV_KEY_LEN * 8);
	psa_set_key_lifetime(&attr, PSA_KEY_LIFETIME_FROM_PERSISTENCE_AND_LOCATION(
					    CRACEN_KEY_PERSISTENCE_READ_ONLY,
					    PSA_KEY_LOCATION_CRACEN_KMU));
	psa_set_key_usage_flags(&attr, PSA_KEY_USAGE_DERIVE);
	psa_set_key_algorithm(&attr, PSA_ALG_ECDH);

	status = psa_import_key(&attr, dummy_priv_key, sizeof(dummy_priv_key), &key_id);
	psa_reset_key_attributes(&attr);

	zassert_equal(status, PSA_ERROR_ALREADY_EXISTS,
		      "Overwriting the Anti-Spoofing key in KMU slot %d was not blocked (status %d)",
		      CONFIG_BT_FAST_PAIR_ANTI_SPOOFING_PRIVATE_KEY_KMU_SLOT, status);

	assert_pub_key_matches_expected();
}

ZTEST(fp_provisioner_post_prov, test_mid_validate)
{
	struct psa_storage_info_t info;
	psa_status_t status;

	status = psa_its_get_info(FP_ITS_MODEL_ID_UID, &info);
	zassert_equal(status, PSA_SUCCESS,
		      "Model ID info read from ITS uid 0x%08x failed (status %d)",
		      (unsigned int)FP_ITS_MODEL_ID_UID, status);

	zexpect_equal(info.size, sizeof(expected_model_id),
		      "Model ID entry size is %zu bytes, expected %zu", info.size,
		      sizeof(expected_model_id));
	zexpect_equal(info.flags, PSA_STORAGE_FLAG_WRITE_ONCE,
		      "Model ID entry flags 0x%08x, expected PSA_STORAGE_FLAG_WRITE_ONCE",
		      info.flags);
}

ZTEST(fp_provisioner_post_prov, test_mid_value)
{
	assert_model_id_matches_expected();
}

ZTEST(fp_provisioner_post_prov, test_mid_remove_blocked)
{
	psa_status_t status = psa_its_remove(FP_ITS_MODEL_ID_UID);

	zassert_equal(status, PSA_ERROR_NOT_PERMITTED,
		      "Removing the write-once Model ID from ITS uid 0x%08x was not blocked "
		      "(status %d)",
		      (unsigned int)FP_ITS_MODEL_ID_UID, status);

	assert_model_id_matches_expected();
}

ZTEST_SUITE(fp_provisioner_post_prov, NULL, NULL, NULL, NULL, NULL);
