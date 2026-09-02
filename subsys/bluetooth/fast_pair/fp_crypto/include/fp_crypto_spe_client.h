/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

 #ifndef _FP_CRYPTO_SPE_CLIENT_H_
 #define _FP_CRYPTO_SPE_CLIENT_H_

#include <stdint.h>
#include "psa/client.h"
#include "psa_manifest/sid.h"

 /**
 * @defgroup fp_crypto Fast Pair crypto SPE client
 * @brief Internal API for Fast Pair secure partition crypto
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

psa_status_t fp_crypto_spe_ecc_secp160r1_calculate(uint8_t *out, uint8_t *mod, const uint8_t *in, size_t datalen);
psa_status_t fp_crypto_spe_ecc_secp256r1_calculate(uint8_t *out, uint8_t *mod, const uint8_t *in, size_t datalen);

 #ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_CRYPTO_SPE_CLIENT_H_ */