#ifndef _SQRL_CRYPTO_H
#define _SQRL_CRYPTO_H

#include <stddef.h>
#include <stdint.h>


void sqrl_crypto_init(void);
void sqrl_make_public(uint8_t publ[32], const uint8_t priv[32]);
void sqrl_get_ilk_from_iuk(uint8_t ilk[32], uint8_t iuk[32]);
void sqrl_get_unlock_request_signing_key(uint8_t ursk[32], uint8_t vuk[32], const uint8_t suk[32], const uint8_t iuk[32]);
void sqrl_get_idk_for_site(uint8_t idk[32], uint8_t ssk[32], const uint8_t imk[32], const char* sks);
void EnHash(uint8_t digest[32], const uint8_t data[32]);
void sqrl_get_ins_from_sin(uint8_t ins[32], const uint8_t ssk[32], const char* sin);
void sqrl_idlock_keys(uint8_t suk[32], uint8_t vuk[32], const uint8_t rlk[32], const uint8_t ilk[32]);


#endif//_SQRL_CRYPTO_H
