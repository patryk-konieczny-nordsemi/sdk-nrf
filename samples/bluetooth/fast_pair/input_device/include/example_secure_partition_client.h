#ifndef EXAMPLE_SECURE_PARTITION_CLIENT_H
#define EXAMPLE_SECURE_PARTITION_CLIENT_H

#include <stdint.h>
#include "psa/client.h"
#include "psa_manifest/sid.h"
#include "tfm_ns_interface.h"

psa_status_t psa_example_secure_partition_get(uint64_t *value);

#endif /* EXAMPLE_SECURE_PARTITION_CLIENT_H */