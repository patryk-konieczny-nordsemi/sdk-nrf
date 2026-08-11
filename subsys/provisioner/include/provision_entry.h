/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PROVISION_ENTRY_H_
#define PROVISION_ENTRY_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/util.h>

#include <psa/crypto.h>
#include <psa/crypto_extra.h>
#include <psa/internal_trusted_storage.h>
#include <psa/storage_common.h>

/**
 * @brief Minimum RRAM erase granularity (nRF54L 128-bit wordline).
 *
 * Used for compile-time checks in @ref REGISTER_PROVISION_KMU_ENTRY_SECRET and
 * @ref REGISTER_PROVISION_ITS_ENTRY_SECRET. The provisioner re-checks alignment
 * at runtime against the flash driver write block size.
 */
#define PROVISION_RRAM_ERASE_ALIGN 16U

/** @brief Maximum number of RRAM bytes the provisioner can erase in one entry. */
#define PROVISION_RRAM_ERASE_MAX_LEN 64U

/** @brief Source data encoding for a provision entry. */
enum provision_data_format {
	/** @brief Binary payload; @a payload_length is the exact byte count to read. */
	PROVISION_DATA_FORMAT_RAW = 0,
	/**
	 * @brief Base64-encoded, NUL-terminated text at the start of @a data.
	 *
	 * @a payload_length bounds the decode (strnlen). It must cover the full
	 * encoded string and its terminating NUL, and must not exceed the allocated
	 * @a storage_length when using a @c _SECRET registration macro.
	 */
	PROVISION_DATA_FORMAT_BASE64,
};

/** @brief Generic post-provisioning callback descriptor. */
struct provision_generic_callback {
	/** Entry name (STRINGIFY of the registration symbol). */
	const char *name;
	/** Callback invoked after core provisioning completes. */
	int (*callback)(void);
};

/** @brief PSA Internal Trusted Storage provision entry. */
struct provision_its_entry {
	/** Entry name (STRINGIFY of the registration symbol). */
	const char *name;
	/** Source data pointer. Must remain valid for the life of the image. */
	const void *data;
	/**
	 * Payload extent read from @a data.
	 *
	 * RAW: exact byte count. BASE64: upper bound for strnlen (string + NUL).
	 */
	size_t payload_length;
	/** Encoding of @a data. */
	enum provision_data_format format;
	/** PSA ITS uid. */
	psa_storage_uid_t uid;
	/** Flags passed to psa_its_set(). */
	psa_storage_create_flags_t create_flags;
	/**
	 * RRAM storage extent to erase after a successful write, or 0 to retain
	 * @a data in the image. When non-zero, must be write-block aligned and
	 * be at least @a payload_length (see @ref REGISTER_PROVISION_ITS_ENTRY_SECRET).
	 */
	size_t storage_length;
};

/** @brief CRACEN KMU provision entry. */
struct provision_kmu_entry {
	/** Entry name (STRINGIFY of the registration symbol). */
	const char *name;
	/** Source data pointer. Must remain valid for the life of the image. */
	const void *data;
	/**
	 * Payload extent read from @a data.
	 *
	 * RAW: exact byte count. BASE64: upper bound for strnlen (string + NUL).
	 */
	size_t payload_length;
	/** Key size in bits passed to psa_set_key_bits(). */
	size_t key_bits;
	/** Encoding of @a data. */
	enum provision_data_format format;
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
	/**
	 * RRAM storage extent to erase after a successful import, or 0 to retain
	 * @a data in the image. When non-zero, must be write-block aligned and be
	 * at least @a payload_length (see @ref REGISTER_PROVISION_KMU_ENTRY_SECRET).
	 */
	size_t storage_length;
};

/** @brief Compile-time checks shared by ITS and KMU secret registration macros. */
#define PROVISION_SECRET_STORAGE_ASSERT(_data, _payload_length, _storage_length)                   	\
	BUILD_ASSERT((_storage_length) > 0U, "storage_length must be non-zero for secret entries");		\
	BUILD_ASSERT((_storage_length) <= PROVISION_RRAM_ERASE_MAX_LEN,                            		\
		     "storage_length exceeds provisioner limit");                                  			\
	BUILD_ASSERT((_storage_length) >= (_payload_length),                                       		\
		     "storage_length must cover the payload region");                              			\
	BUILD_ASSERT(((_storage_length) % PROVISION_RRAM_ERASE_ALIGN) == 0U,                       		\
		     "storage_length must be RRAM write-block aligned");                           			\
	BUILD_ASSERT((((uintptr_t)(_data)) % PROVISION_RRAM_ERASE_ALIGN) == 0U,                   		\
		     "secret data address must be RRAM write-block aligned")

/** @brief Register a generic post-provisioning callback. */
#define REGISTER_PROVISION_GENERIC_CALLBACK(_name, _callback)                                   \
	static const STRUCT_SECTION_ITERABLE(provision_generic_callback, _name) = {                 \
		.name = STRINGIFY(_name),                                                              	\
		.callback = (_callback),                                                               	\
	}

/** @brief Register a PSA ITS entry (source data retained in the image). */
#define REGISTER_PROVISION_ITS_ENTRY(_name, _data, _payload_length, _format, _uid,              \
				     _create_flags)                                                				\
	static const STRUCT_SECTION_ITERABLE(provision_its_entry, _name) = {      	\
		.name = STRINGIFY(_name),                                             	\
		.data = (_data),                                                      	\
		.payload_length = (_payload_length),                                  	\
		.format = (_format),                                                  	\
		.uid = (_uid),                                                        	\
		.create_flags = (_create_flags),                                      	\
		.storage_length = 0,                                                  	\
	}

/**
 * @brief Register a PSA ITS entry backed by RRAM-resident secret data.
 *
 * @param _payload_length Bytes read from @p _data (payload / string + NUL).
 * @param _storage_length Full RRAM allocation erased after success (aligned,
 *                        >= @p _payload_length, typically `sizeof(_data)`).
 */
#define REGISTER_PROVISION_ITS_ENTRY_SECRET(_name, _data, _payload_length, _storage_length,     \
					    _format, _uid, _create_flags)                            				\
	PROVISION_SECRET_STORAGE_ASSERT(_data, _payload_length, _storage_length);	\
	static const STRUCT_SECTION_ITERABLE(provision_its_entry, _name) = {     	\
		.name = STRINGIFY(_name),                                            	\
		.data = (_data),                                                     	\
		.payload_length = (_payload_length),                                 	\
		.format = (_format),                                                 	\
		.uid = (_uid),                                                       	\
		.create_flags = (_create_flags),                                     	\
		.storage_length = (_storage_length),                                 	\
	}

/** @brief Register a CRACEN KMU entry (source data retained in the image). */
#define REGISTER_PROVISION_KMU_ENTRY(_name, _data, _payload_length, _key_bits, _format, _id,   	\
				     _type, _lifetime, _usage_flags, _alg)                         				\
	static const STRUCT_SECTION_ITERABLE(provision_kmu_entry, _name) = {		\
		.name = STRINGIFY(_name),                                       		\
		.data = (_data),                                                		\
		.payload_length = (_payload_length),                            		\
		.key_bits = (_key_bits),                                        		\
		.format = (_format),                                            		\
		.id = (_id),                                                    		\
		.type = (_type),                                                		\
		.lifetime = (_lifetime),                                        		\
		.usage_flags = (_usage_flags),                                  		\
		.alg = (_alg),                                                  		\
		.storage_length = 0,                                            		\
	}

/**
 * @brief Register a CRACEN KMU entry backed by RRAM-resident secret data.
 *
 * @param _payload_length Bytes read from @p _data (payload / string + NUL).
 * @param _storage_length Full RRAM allocation erased after success (aligned,
 *                        >= @p _payload_length, typically `sizeof(_data)`).
 */
#define REGISTER_PROVISION_KMU_ENTRY_SECRET(_name, _data, _payload_length, _storage_length,		\
					    _key_bits, _format, _id, _type, _lifetime,             					\
					    _usage_flags, _alg)                                     				\
	PROVISION_SECRET_STORAGE_ASSERT(_data, _payload_length, _storage_length);	\
	static const STRUCT_SECTION_ITERABLE(provision_kmu_entry, _name) = {        \
		.name = STRINGIFY(_name),                                               \
		.data = (_data),                                                        \
		.payload_length = (_payload_length),                                    \
		.key_bits = (_key_bits),                                                \
		.format = (_format),                                                    \
		.id = (_id),                                                            \
		.type = (_type),                                                        \
		.lifetime = (_lifetime),                                                \
		.usage_flags = (_usage_flags),                                          \
		.alg = (_alg),                                                          \
		.storage_length = (_storage_length),                                    \
	}

#endif /* PROVISION_ENTRY_H_ */
