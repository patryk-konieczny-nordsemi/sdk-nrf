#include <stdint.h>
#include <string.h>
#include "psa/protected_storage.h"
#include "psa/service.h"
#include "psa_manifest/tfm_example_secure_partition.h"

#define FIB_UID_PREV  ((psa_storage_uid_t)0x2500)
#define FIB_UID_CURR  ((psa_storage_uid_t)0x2000)
#define FIB_VAL_SIZE  sizeof(uint64_t)

static psa_status_t fib_load_or_init(uint64_t *prev, uint64_t *curr)
{
    size_t len;
    psa_status_t st;

    st = psa_ps_get(FIB_UID_PREV, 0, FIB_VAL_SIZE, prev, &len);
    if (st == PSA_SUCCESS && len == FIB_VAL_SIZE) {
        st = psa_ps_get(FIB_UID_CURR, 0, FIB_VAL_SIZE, curr, &len);
        if (st == PSA_SUCCESS && len == FIB_VAL_SIZE) {
            return PSA_SUCCESS;
        }
    }

    *prev = 0;
    *curr = 1;
    st = psa_ps_set(FIB_UID_PREV, FIB_VAL_SIZE, prev, PSA_STORAGE_FLAG_NONE);
    if (st != PSA_SUCCESS) {
        return st;
    }
    return psa_ps_set(FIB_UID_CURR, FIB_VAL_SIZE, curr, PSA_STORAGE_FLAG_NONE);
}

static psa_status_t fib_store(uint64_t prev, uint64_t curr)
{
    psa_status_t st;

    st = psa_ps_set(FIB_UID_PREV, FIB_VAL_SIZE, &prev, PSA_STORAGE_FLAG_NONE);
    if (st != PSA_SUCCESS) {
        return st;
    }
    return psa_ps_set(FIB_UID_CURR, FIB_VAL_SIZE, &curr, PSA_STORAGE_FLAG_NONE);
}

static psa_status_t fibonacci_get_handler(psa_msg_t *msg)
{
    uint64_t prev, curr, next, result;
    psa_status_t st;

    if (msg->out_size[0] != FIB_VAL_SIZE) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    st = fib_load_or_init(&prev, &curr);
    if (st != PSA_SUCCESS) {
        return st;
    }

    result = curr;
    next = prev + curr;   /* watch overflow in production */
    st = fib_store(curr, next);
    if (st != PSA_SUCCESS) {
        return st;
    }

    psa_write(msg->handle, 0, &result, FIB_VAL_SIZE);
    return PSA_SUCCESS;
}

static void tfm_example_get(void){
    psa_msg_t msg;
    psa_status_t st = psa_get(TFM_EXAMPLE_GET_SIGNAL, &msg);

    switch (msg.type) {
    case PSA_IPC_CONNECT:
    case PSA_IPC_DISCONNECT:
        psa_reply(msg.handle, PSA_SUCCESS);
        break;
    case PSA_IPC_CALL:
        st = fibonacci_get_handler(&msg);
        psa_reply(msg.handle, st);
        break;
    default:
        psa_panic();
        break;
    }
}

/* IPC entry — mirror dummy_partition.c signal loop */
psa_status_t tfm_example_main(void)
{
    psa_signal_t signals = 0;

    while (1) {
        signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
        if (signals & TFM_EXAMPLE_GET_SIGNAL) {
            tfm_example_get();
        } else {
            psa_panic();
        }
    }
}