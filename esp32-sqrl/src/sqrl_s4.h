#ifndef _SQRL_S4_H_
#define _SQRL_S4_H_

#ifndef __INLINE
#define __INLINE inline
#endif

#include <stdint.h>
#include <AES.h>
#include "GCM.h"

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




static __INLINE uint16_t uint16_decode(const void * p_encoded_data)
{
    return ( (((uint16_t)((uint8_t *)p_encoded_data)[0])) |
             (((uint16_t)((uint8_t *)p_encoded_data)[1]) << 8 ));
}


static __INLINE uint32_t uint32_decode(const void * p_encoded_data)
{
    return ( (((uint32_t)((uint8_t *)p_encoded_data)[0]) << 0)  |
             (((uint32_t)((uint8_t *)p_encoded_data)[1]) << 8)  |
             (((uint32_t)((uint8_t *)p_encoded_data)[2]) << 16) |
             (((uint32_t)((uint8_t *)p_encoded_data)[3]) << 24 ));
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
    GCM<AES256> gcmaes256;
    if (!gcmaes256.setKey(key, 32))
    {
        return false;
    }

    if (!gcmaes256.setIV(p_identity->type1_aes_gcm_iv, sizeof(p_identity->type1_aes_gcm_iv)))
    {
        return false;
    }

    gcmaes256.addAuthData((uint8_t*)&p_identity->type1_length,                          sizeof(p_identity->type1_length));
    gcmaes256.addAuthData((uint8_t*)&p_identity->type1_type,                            sizeof(p_identity->type1_type));
    gcmaes256.addAuthData((uint8_t*)&p_identity->type1_pt_length,                       sizeof(p_identity->type1_pt_length));
    gcmaes256.addAuthData(           p_identity->type1_aes_gcm_iv,                      sizeof(p_identity->type1_aes_gcm_iv));
    gcmaes256.addAuthData(           p_identity->type1_scrypt_random_salt,              sizeof(p_identity->type1_scrypt_random_salt));
    gcmaes256.addAuthData((uint8_t*)&p_identity->type1_scrypt_log_n_factor,             sizeof(p_identity->type1_scrypt_log_n_factor));
    gcmaes256.addAuthData((uint8_t*)&p_identity->type1_scrypt_iteration_count,          sizeof(p_identity->type1_scrypt_iteration_count));
    gcmaes256.addAuthData((uint8_t*)&p_identity->type1_option_flags,                    sizeof(p_identity->type1_option_flags));
    gcmaes256.addAuthData((uint8_t*)&p_identity->type1_hint_length,                     sizeof(p_identity->type1_hint_length));
    gcmaes256.addAuthData((uint8_t*)&p_identity->type1_pw_verify_sec,                   sizeof(p_identity->type1_pw_verify_sec));
    gcmaes256.addAuthData((uint8_t*)&p_identity->type1_idle_timeout_min,                sizeof(p_identity->type1_idle_timeout_min));

    gcmaes256.decrypt(imk, (uint8_t*)&p_identity->type1_encrypted_identity_master_key,  sizeof(p_identity->type1_encrypted_identity_master_key));
    gcmaes256.decrypt(ilk, (uint8_t*)&p_identity->type1_encrypted_identity_lock_key,    sizeof(p_identity->type1_encrypted_identity_lock_key));

    return gcmaes256.checkTag((uint8_t*)&p_identity->type1_verification_tag,            sizeof(p_identity->type1_verification_tag));
}

#endif//_SQRL_S4_H_
