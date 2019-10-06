#ifndef _SQRL_S4_H_
#define _SQRL_S4_H_

#ifndef __INLINE
#define __INLINE inline
#endif

#include <stdint.h>

#include "mbedtls/gcm.h"

#include "app_util.h"

typedef struct {
    uint8_t     header[8];
    uint16_t    type1_length;
    uint16_t    type1_type;
    uint16_t    type1_pt_length;
    uint8_t     type1_aes_gcm_iv[12];
    uint8_t     type1_scrypt_random_salt[16];
    uint8_t     type1_scrypt_log_n_factor;
    uint32_t    type1_scrypt_iteration_count;
    uint16_t    type1_option_flags;
    uint8_t     type1_hint_length;
    uint8_t     type1_pw_verify_sec;
    uint16_t    type1_idle_timeout_min;
    uint8_t     type1_encrypted_identity_master_key[32];
    uint8_t     type1_encrypted_identity_lock_key[32];
    uint8_t     type1_verification_tag[16];
    uint16_t    type2_length;
    uint16_t    type2_type;
    uint8_t     type2_scrypt_random_salt[16];
    uint8_t     type2_scrypt_log_n_factor;
    uint32_t    type2_scrypt_iteration_count;
    uint8_t     type2_encrypted_identity_unlock_key[32];
    uint8_t     type2_verification_tag[16];
} sqrl_s4_identity_t;


uint32_t sqrl_s4_decode(uint8_t* sqrlbinary, sqrl_s4_identity_t* p_identity);
bool get_imk_ilk_from_scryptpassword(sqrl_s4_identity_t* p_identity, uint8_t key[32], uint8_t imk[32], uint8_t ilk[32]);


uint32_t buf_encode(uint8_t* buf, uint8_t* data, uint32_t length)
{
    memcpy(buf, data, length);
    return length;
}

uint32_t sqrl_s4_decode(uint8_t* sqrlbinary, sqrl_s4_identity_t* p_identity)
{
    int idx = 0;

    // Parse block header
    if (memcmp(sqrlbinary, "sqrldata", 8) != 0)
    {
        return 1;
    }

    memcpy(p_identity->header,                                  &sqrlbinary[idx+  0], sizeof(p_identity->header));

    // Parse Type 1 header
    idx = 8;
    p_identity->type1_length                    = uint16_decode(&sqrlbinary[idx+  0]);
    p_identity->type1_type                      = uint16_decode(&sqrlbinary[idx+  2]);

    if (p_identity->type1_type != 1)
    {
        // Only supports format version 1
        return 1;
    }

    if (p_identity->type1_length != 125)
    {
        // Only supports fixed length type1 section
        return 1;
    }

    // Parse Type 1 data
    p_identity->type1_pt_length                 = uint16_decode(&sqrlbinary[idx+  4]);
    memcpy(p_identity->type1_aes_gcm_iv,                        &sqrlbinary[idx+  6], sizeof(p_identity->type1_aes_gcm_iv));
    memcpy(p_identity->type1_scrypt_random_salt,                &sqrlbinary[idx+ 18], sizeof(p_identity->type1_scrypt_random_salt));
    p_identity->type1_scrypt_log_n_factor       =                sqrlbinary[idx+ 34];
    p_identity->type1_scrypt_iteration_count    = uint32_decode(&sqrlbinary[idx+ 35]);
    p_identity->type1_option_flags              = uint16_decode(&sqrlbinary[idx+ 39]);
    p_identity->type1_hint_length               =                sqrlbinary[idx+ 41];
    p_identity->type1_pw_verify_sec             =                sqrlbinary[idx+ 42];
    p_identity->type1_idle_timeout_min          = uint16_decode(&sqrlbinary[idx+ 43]);
    memcpy(p_identity->type1_encrypted_identity_master_key,     &sqrlbinary[idx+ 45], sizeof(p_identity->type1_encrypted_identity_master_key));
    memcpy(p_identity->type1_encrypted_identity_lock_key,       &sqrlbinary[idx+ 77], sizeof(p_identity->type1_encrypted_identity_lock_key));
    memcpy(p_identity->type1_verification_tag,                  &sqrlbinary[idx+109], sizeof(p_identity->type1_verification_tag));

    // Parse Type 2 header
    idx = 8 + p_identity->type1_length;
    p_identity->type2_length                    = uint16_decode(&sqrlbinary[idx+ 0]);
    p_identity->type2_type                      = uint16_decode(&sqrlbinary[idx+ 2]);

    if (p_identity->type2_type != 2)
    {
        // Only supports format version 2
        return 1;
    }

    if (p_identity->type2_length != 73)
    {
        // Only supports fixed length type2 section
        return 1;
    }

    // Parse Type 2 data
    memcpy(p_identity->type2_scrypt_random_salt,                &sqrlbinary[idx+  4], sizeof(p_identity->type2_scrypt_random_salt));
    p_identity->type2_scrypt_log_n_factor       =                sqrlbinary[idx+ 20];
    p_identity->type2_scrypt_iteration_count    = uint32_decode(&sqrlbinary[idx+ 21]);
    memcpy(p_identity->type2_encrypted_identity_unlock_key,     &sqrlbinary[idx+ 25], sizeof(p_identity->type1_encrypted_identity_lock_key));
    memcpy(p_identity->type2_verification_tag,                  &sqrlbinary[idx+ 57], sizeof(p_identity->type1_verification_tag));

    return 0;
}

bool get_imk_ilk_from_scryptpassword(sqrl_s4_identity_t* p_identity, uint8_t key[32], uint8_t imk[32], uint8_t ilk[32])
{
    // 'Collect' additional data into contigous buffer
    uint8_t add[46]; // TODO: use sizeof(...)
    uint32_t aix = 0;
    aix += buf_encode(&add[aix], (uint8_t*)&p_identity->type1_length,                          sizeof(p_identity->type1_length));
    aix += buf_encode(&add[aix], (uint8_t*)&p_identity->type1_type,                            sizeof(p_identity->type1_type));
    aix += buf_encode(&add[aix], (uint8_t*)&p_identity->type1_pt_length,                       sizeof(p_identity->type1_pt_length));
    aix += buf_encode(&add[aix],            p_identity->type1_aes_gcm_iv,                      sizeof(p_identity->type1_aes_gcm_iv));
    aix += buf_encode(&add[aix],            p_identity->type1_scrypt_random_salt,              sizeof(p_identity->type1_scrypt_random_salt));
    aix += buf_encode(&add[aix], (uint8_t*)&p_identity->type1_scrypt_log_n_factor,             sizeof(p_identity->type1_scrypt_log_n_factor));
    aix += buf_encode(&add[aix], (uint8_t*)&p_identity->type1_scrypt_iteration_count,          sizeof(p_identity->type1_scrypt_iteration_count));
    aix += buf_encode(&add[aix], (uint8_t*)&p_identity->type1_option_flags,                    sizeof(p_identity->type1_option_flags));
    aix += buf_encode(&add[aix], (uint8_t*)&p_identity->type1_hint_length,                     sizeof(p_identity->type1_hint_length));
    aix += buf_encode(&add[aix], (uint8_t*)&p_identity->type1_pw_verify_sec,                   sizeof(p_identity->type1_pw_verify_sec));
    aix += buf_encode(&add[aix], (uint8_t*)&p_identity->type1_idle_timeout_min,                sizeof(p_identity->type1_idle_timeout_min));

    if (aix >= sizeof(add))
    {
        return false;
    }

    // Start
    mbedtls_gcm_context aes;

    mbedtls_gcm_init(&aes);
    mbedtls_gcm_setkey(&aes, MBEDTLS_CIPHER_ID_AES, key, 32 * 8);

    int ret = mbedtls_gcm_starts(&aes, MBEDTLS_GCM_DECRYPT,
            p_identity->type1_aes_gcm_iv, sizeof(p_identity->type1_aes_gcm_iv),
            add, aix);
    if (ret != 0) return false;

    // Decrypt data
    ret = mbedtls_gcm_update(&aes, sizeof(p_identity->type1_encrypted_identity_master_key), (uint8_t*)&p_identity->type1_encrypted_identity_master_key, imk);
    if (ret != 0) return false;
    ret = mbedtls_gcm_update(&aes, sizeof(p_identity->type1_encrypted_identity_lock_key), (uint8_t*)&p_identity->type1_encrypted_identity_lock_key, ilk);
    if (ret != 0) return false;

    uint8_t check_tag[sizeof(p_identity->type1_verification_tag)];
    if (mbedtls_gcm_finish(&aes, check_tag, sizeof(p_identity->type1_verification_tag)) != 0)
    {
        memset(imk, 0, sizeof(p_identity->type1_encrypted_identity_master_key));
        memset(ilk, 0, sizeof(p_identity->type1_encrypted_identity_lock_key));
        mbedtls_gcm_free(&aes);
        return false;
    }

    mbedtls_gcm_free(&aes);

    // Check tag
    ret = 0;
    for (uint32_t i = 0; i < sizeof(p_identity->type1_verification_tag); i++)
    {
        ret |= p_identity->type1_verification_tag[i] ^ check_tag[i];
    }

    if (ret != 0)
    {
        memset(imk, 0, sizeof(p_identity->type1_encrypted_identity_master_key));
        memset(ilk, 0, sizeof(p_identity->type1_encrypted_identity_lock_key));
        return false;
    }

    return true;

}

#endif//_SQRL_S4_H_
