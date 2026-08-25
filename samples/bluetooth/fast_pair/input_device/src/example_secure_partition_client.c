#include <stdint.h>
#include "psa/client.h"
#include "psa_manifest/sid.h"
#include "tfm_ns_interface.h"
#include "example_secure_partition_client.h"

psa_status_t psa_example_secure_partition_get(uint64_t *value)
{
    psa_handle_t handle;
    psa_status_t status;
    psa_outvec out = { .base = value, .len = sizeof(*value) };

    if (!value) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    handle = psa_connect(TFM_EXAMPLE_GET_SID, TFM_EXAMPLE_GET_VERSION);
    if (!PSA_HANDLE_IS_VALID(handle)) {
        return PSA_ERROR_GENERIC_ERROR;
    }
    
    status = psa_call(handle, PSA_IPC_CALL, NULL, 0, &out, 1);
    psa_close(handle);
    return status;
}