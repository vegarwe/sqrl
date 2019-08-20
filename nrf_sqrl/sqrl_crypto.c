#include "sqrl_crypto.h"

#include "app_error.h"
#include "nrf_crypto.h"
#include "nrf_crypto_hash.h"
#include "occ_curve25519.h"
#include "occ_ed25519.h"

void sqrl_hmac(uint8_t digest[32], const uint8_t key[32], const char* data, size_t data_len)
{
    static nrf_crypto_hmac_context_t    s_context;

    ret_code_t  err_code;
    size_t      digest_len = NRF_CRYPTO_HASH_SIZE_SHA256;

    err_code = nrf_crypto_hmac_init(&s_context, &g_nrf_crypto_hmac_sha256_info, key, 32);
    APP_ERROR_CHECK(err_code);

    // Push all data in one go (could be done repeatedly)
    err_code = nrf_crypto_hmac_update(&s_context, (uint8_t*)data, data_len);
    APP_ERROR_CHECK(err_code);

    // Finish calculation
    err_code = nrf_crypto_hmac_finalize(&s_context, digest, &digest_len);
    APP_ERROR_CHECK(err_code);
}

void sqrl_get_idk_for_site(uint8_t idk[32], uint8_t ssk[32], const uint8_t imk[32], const char* sks)
{
    sqrl_hmac(ssk, imk, sks, strlen(sks));
    occ_ed25519_public_key(idk, ssk);
}

void sqrl_make_public(uint8_t publ[32], const uint8_t priv[32])
{
    occ_curve25519_scalarmult_base(publ, priv);
}

void sqrl_get_ilk_from_iuk(uint8_t ilk[32], uint8_t iuk[32])
{
    sqrl_make_public(ilk, iuk);
}

void sqrl_get_unlock_request_signing_key(uint8_t ursk[32], uint8_t vuk[32], const uint8_t suk[32], const uint8_t iuk[32])
{
    occ_curve25519_scalarmult(ursk, iuk, suk);
    occ_ed25519_public_key(vuk, ursk);
}

void EnHash(uint8_t digest[32], const uint8_t data[32])
{
    uint8_t tmp_data[32];
    memcpy(tmp_data, data, sizeof(tmp_data));

    memset(digest, 0, 32);
    for (int i = 0; i < 16; i++)
    {
        occ_sha256(tmp_data, tmp_data, sizeof(tmp_data)); // TODO: Safe to reuse tmp_data here?

        for (int j = 0; j < 32; j++) {
            digest[j] ^= tmp_data[j];
        }
    }
}


void sqrl_get_ins_from_sin(uint8_t ins[32], const uint8_t ssk[32], const char* sin)
{
    EnHash(ins, ssk);
    sqrl_hmac(ins, ins, sin, strlen(sin)); // Note: reusing ins here is safe... :-)
}


void sqrl_idlock_keys(uint8_t suk[32], uint8_t vuk[32], const uint8_t rlk[32], const uint8_t ilk[32])
{
    uint8_t dhka[32];
    sqrl_make_public(suk, rlk);
    occ_curve25519_scalarmult(dhka, rlk, ilk);
    occ_ed25519_public_key(vuk, dhka);
}


void sqrl_sha256(uint8_t digest[32], const char* data, size_t data_len)
{
    static nrf_crypto_hash_context_t s_hash_context;

    ret_code_t  err_code;
    size_t      digest_len = NRF_CRYPTO_HASH_SIZE_SHA256;

    err_code = nrf_crypto_hash_init(&s_hash_context, &g_nrf_crypto_hash_sha256_info);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_crypto_hash_update(&s_hash_context, (uint8_t*)data, data_len);
    APP_ERROR_CHECK(err_code);

    // Run the finalize when all data has been fed to the update function.
    // this gives you the result
    err_code = nrf_crypto_hash_finalize(&s_hash_context, digest, &digest_len);
    APP_ERROR_CHECK(err_code);
}


void sqrl_crypto_init(void)
{
    ret_code_t err_code = nrf_crypto_init();
    APP_ERROR_CHECK(err_code);
}
