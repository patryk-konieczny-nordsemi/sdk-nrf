/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PROVISIONER_H_
#define PROVISIONER_H_

/**
 * @file provisioner.h
 * @brief Public API for the runtime data provisioner.
 *
 * User applications include this header and call provision_data() from their
 * provisioner sub-application main (see provisioner/provisioner_main.c).
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Run runtime credential provisioning (KMU, ITS, registered plugins).
 *
 * @return 0 on success, negative errno on failure.
 */
int provision_data(void);

#ifdef __cplusplus
}
#endif

#endif /* PROVISIONER_H_ */
