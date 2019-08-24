#include "sqrl_crypto.h"

#include <string.h>

#include "app_error.h"
#include "occ_curve25519.h"
#include "occ_ed25519.h"
#include "occ_sha256.h"
#include "occ_hmac_sha256.h"

void sqrl_get_idk_for_site(uint8_t idk[32], uint8_t ssk[32], const uint8_t imk[32], const char* sks)
{
    occ_hmac_sha256(ssk, imk, 32, (uint8_t *)sks, strlen(sks));
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
    uint8_t tmp[32];
    EnHash(tmp, ssk);
    occ_hmac_sha256(tmp, ins, 32, (uint8_t *)sin, strlen(sin));
}


void sqrl_idlock_keys(uint8_t suk[32], uint8_t vuk[32], const uint8_t rlk[32], const uint8_t ilk[32])
{
    uint8_t dhka[32];
    sqrl_make_public(suk, rlk);
    occ_curve25519_scalarmult(dhka, rlk, ilk);
    occ_ed25519_public_key(vuk, dhka);
}


void sqrl_crypto_init(void)
{
}
