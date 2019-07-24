#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "Crypto.h"
#include "Curve25519.h"
#include "Ed25519.h"
#include "SHA256.h"


bool sqrl_make_public(uint8_t publ[32], const uint8_t priv[32])
{
    uint8_t key[32];
    memcpy(key, priv, sizeof(key));

    key[ 0] &= 248;
    key[31] &= 127;
    key[31] |= 64;

    return Curve25519::eval(publ, key, NULL);
}


bool sqrl_scalar_mult(uint8_t res[32], const uint8_t priv[32], const uint8_t publ[32])
{
    uint8_t key[32];
    memcpy(key, priv, sizeof(key));

    key[ 0] &= 248;
    key[31] &= 127;
    key[31] |= 64;

    return Curve25519::eval(res, key, publ);
}


bool sqrl_get_ilk_from_iuk(uint8_t ilk[32], uint8_t iuk[32])
{
    return sqrl_make_public(ilk, iuk);
}


bool sqrl_get_unlock_request_signing_key(uint8_t ursk[32], uint8_t vuk[32], const uint8_t suk[32], const uint8_t iuk[32])
{
    if (! sqrl_scalar_mult(ursk, iuk, suk)) return false;
    Ed25519::derivePublicKey(vuk, ursk);
    return true;
}


void sqrl_hmac(uint8_t digest[32], const uint8_t key[32], const char* data, size_t data_len)
{
    SHA256 hash;

    hash.resetHMAC(key, 32);
    hash.update(data, data_len);
    hash.finalizeHMAC(key, 32, digest, 32);
}


bool sqrl_get_idk_for_site(uint8_t idk[32], uint8_t ssk[32], const uint8_t imk[32], const char* sks)
{
    sqrl_hmac(ssk, imk, sks, strlen(sks));
    Ed25519::derivePublicKey(idk, ssk);
}


void EnHash(uint8_t digest[32], const char* data, size_t data_len)
{
    SHA256 hash;

    uint8_t* tmp_data = (uint8_t*) malloc(data_len);
    memcpy(tmp_data, data, data_len);

    memset(digest, 0, 32);
    for (int i = 0; i < 16; i++) {
        hash.reset();
        hash.update(tmp_data, data_len);
        hash.finalize(tmp_data, 32);

        for (int j = 0; j < 32; j++) {
            digest[j] ^= tmp_data[j];
        }
    }

    free(tmp_data);
}


void sqrl_get_ins_from_sin(uint8_t ins[32], const uint8_t ssk[32], const char* sin)
{
    EnHash(ins, (char*) ssk, 32);
    sqrl_hmac(ins, ins, sin, strlen(sin)); // Note: reusing ins here is safe... :-)
}


void sqrl_idlock_keys(uint8_t suk[32], uint8_t vuk[32], const uint8_t rlk[32], const uint8_t ilk[32])
{
    uint8_t dhka[32];
    sqrl_make_public(suk, rlk);
    sqrl_scalar_mult(dhka, rlk, ilk);
    Ed25519::derivePublicKey(vuk, dhka);
}
