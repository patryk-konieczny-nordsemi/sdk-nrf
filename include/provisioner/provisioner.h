/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PROVISIONER_H_
#define PROVISIONER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file provisioner.h
 *
 * @defgroup provisioner Public API for runtime provisioning
 *
 * @{
 *
 * @brief Public API for the runtime data provisioner.
 * Provisioner application images include this header and call provisioner_run()
 * from main (see samples/bluetooth/fast_pair/input_device/provisioner/).
 * Provisioned data needs to be registered using registration macros 
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <psa/internal_trusted_storage.h>
#include <psa/storage_common.h>

/** @brief Source data encoding for a provision entry. */
enum provisioner_data_format {
    /** @brief Binary payload; @a payload_length is the exact byte count to read. */
    PROVISIONER_DATA_FORMAT_RAW = 0,
    /**
    * @brief Base64-encoded, NUL-terminated text at the start of @a data.
    * @a payload_length bounds the decode (strnlen). 
    */
    PROVISIONER_DATA_FORMAT_BASE64,
};

typedef struct{
    /** Source data pointer. Must remain valid for the life of the image. */
    const void *data;
    /**
    * Payload extent read frBASE64om @a data.
    *
    * RAW: exact byte count. BASE64: upper bound for strnlen (string + NUL).
    */
    size_t payload_length;
    /** Encoding of @a data. */
    enum provisioner_data_format format;
} provisioner_data;

typedef struct{
    /** PSA ITS uid. */
    psa_storage_uid_t uid;
    /** Flags passed to psa_its_set(). */
    psa_storage_create_flags_t create_flags;
} provisioner_its_config;

typedef struct{
    /** Key size in bits passed to psa_set_key_bits(). */
    size_t key_bits;
    /** PSA key identifier (KMU slot mapping). */
    psa_key_id_t id;
    /** PSA key type. */
    psa_key_type_t type;
    /** PSA key lifetime. */
    psa_key_lifetime_t lifetime;
    /** PSA key usage flags. */
    psa_key_usage_t usage_flags;
    /** PSA key algorithm. */
    psa_algorithm_t alg;
} provisioner_kmu_config;

/** @brief PSA Internal Trusted Storage provision entry. */
struct provisioner_its_entry {
    /** Entry name (STRINGIFY of the registration symbol). */
    const char *name;
    /** Provisioner data */
    provisioner_data prov_data;
    /** ITS entry config */
    provisioner_its_config config;
};

/** @brief CRACEN KMU provision entry. */
struct provisioner_kmu_entry {
    /** Entry name (STRINGIFY of the registration symbol). */
    const char *name;
    /** Provisioner data */
    provisioner_data prov_data;
    /** KMU entry config */
    provisioner_kmu_config config;
};

/** @brief Register a PSA ITS entry (source data retained in the image). */
#define PROVISIONER_ENTRY_ITS_REGISTER(_name, _provision_data, _config)                     \
    static const STRUCT_SECTION_ITERABLE(provisioner_its_entry, _name) = {                  \
        .name = STRINGIFY(_name),                                                           \
        .prov_data = (_provision_data),                                                     \
        .config = (_config),                                                                \
    }

/** @brief Register a CRACEN KMU entry (source data retained in the image). */
#define PROVISIONER_ENTRY_KMU_REGISTER(_name, _provision_data, _config)                     \
    static const STRUCT_SECTION_ITERABLE(provisioner_kmu_entry, _name) = {                  \
        .name = STRINGIFY(_name),                                                           \
        .prov_data = (_provision_data),                                                     \
        .config = (_config),                                                                \
    }


/**
 * Run runtime credential provisioning (KMU, ITS, registered plugins).
 *
 * @return 0 on success, negative errno on failure.
 */
int provisioner_run(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PROVISIONER_H_ */
