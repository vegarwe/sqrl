#include <stdint.h>

bool sqrl_make_public(uint8_t publ[32], const uint8_t priv[32]);
bool sqrl_scalar_mult(uint8_t res[32], const uint8_t priv[32], const uint8_t publ[32]);
bool sqrl_get_ilk_from_iuk(uint8_t ilk[32], uint8_t iuk[32]);
bool sqrl_get_unlock_request_signing_key(uint8_t ursk[32], uint8_t vuk[32], const uint8_t suk[32], const uint8_t iuk[32]);
void sqrl_hmac(uint8_t digest[32], const uint8_t key[32], const char* msg, size_t msg_len);
bool sqrl_get_idk_for_site(uint8_t idk[32], uint8_t ssk[32], const uint8_t imk[32], const char* sks);
void EnHash(uint8_t digest[32], const char* data, size_t data_len);
void sqrl_get_ins_from_sin(uint8_t ins[32], const uint8_t ssk[32], const char* sin);
void sqrl_idlock_keys(uint8_t suk[32], uint8_t vuk[32], uint8_t rlk[32], uint8_t ilk[32]);
