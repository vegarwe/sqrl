#include <string.h>
#include <stdio.h>
#include <string>

#include "repr.h"

/*
These test cases exercises the sqrl_crypto library.

g++ -o aes_gcm_main tests/aes_gcm_main.cpp tests/RNG_mock.cpp       \
        SqrlClient/sqrl_crypto.cpp                                  \
        ~/Documents/Arduino/libraries/Crypto/src/BigNumberUtil.cpp  \
        ~/Documents/Arduino/libraries/Crypto/src/Cipher.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/Crypto.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/Curve25519.cpp     \
        ~/Documents/Arduino/libraries/Crypto/src/Ed25519.cpp        \
        ~/Documents/Arduino/libraries/Crypto/src/GCM.cpp            \
        ~/Documents/Arduino/libraries/Crypto/src/GHASH.cpp          \
        ~/Documents/Arduino/libraries/Crypto/src/GF128.cpp          \
        ~/Documents/Arduino/libraries/Crypto/src/SHA512.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/SHA256.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/Hash.cpp           \
    -I  ~/Documents/Arduino/libraries/Crypto/src                    \
    -I  SqrlClient                                                  \
    &&  ./aes_gcm_main
*/


static void printNumber(const char *name, const uint8_t *x, size_t len=32)
{
    static const char hexchars[] = "0123456789abcdef";
    printf(name);
    putchar(' ');
    for (uint8_t posn = 0; posn < len; ++posn) {
        putchar(hexchars[(x[posn] >> 4) & 0x0F]);
        putchar(hexchars[(x[posn] >> 0) & 0x0F]);
    }
    putchar('\n');
}


typedef struct __attribute__ ((packed)) {
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
    // data = struct.unpack('<8s HHH12s16s BIHBBH32s 32s16s HH16sB I32s16s', sqrlbinary)
} sqrl_s4_packed_identity_t;


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
    // data = struct.unpack('<8s HHH12s16s BIHBBH32s 32s16s HH16sB I32s16s', sqrlbinary)
} sqrl_s4_identity_t;


//static __INLINE uint16_t uint16_decode(const uint8_t * p_encoded_data)
#ifndef __INLINE
#define __INLINE
#endif

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


void sqrl_test_suite()
{
    printf("sqrl_test_suite\r\n");

    char* password = "1234567890ab";
    char* rescue_code = "949106491269852269220540";
    char sqrlbinary[] = "sqrldata}\x00\x01\x00-\x00\"wQ\x12""2\x0e\xb5\x89""1\xfep\x97\xef\xf2""e]\xf6\x0fg\a\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x02""3\x88\xcd\xa0\xd7WN\xf7\x8a\xd1""9\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb0""8\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1f""F\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5""e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$\"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01""e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcb""C\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce";

    //sqrl_s4_identity_t* identity = (sqrl_s4_identity_t*)sqrlbinary;
    sqrl_s4_identity_t fisk = {0};
    sqrl_s4_identity_t* identity = &fisk;

    memcpy(fisk.header,                                 &sqrlbinary[  0], sizeof(fisk.header));
    fisk.type1_length                   = uint16_decode(&sqrlbinary[  8]);
    fisk.type1_type                     = uint16_decode(&sqrlbinary[ 10]);
    fisk.type1_pt_length                = uint16_decode(&sqrlbinary[ 12]);
    memcpy(fisk.type1_aes_gcm_iv,                       &sqrlbinary[ 14], sizeof(fisk.type1_aes_gcm_iv));
    memcpy(fisk.type1_scrypt_random_salt,               &sqrlbinary[ 26], sizeof(fisk.type1_scrypt_random_salt));
    fisk.type1_scrypt_log_n_factor      =                sqrlbinary[ 42];
    fisk.type1_scrypt_iteration_count   = uint32_decode(&sqrlbinary[ 43]);
    fisk.type1_option_flags             = uint16_decode(&sqrlbinary[ 47]);
    fisk.type1_hint_length              =                sqrlbinary[ 49];
    fisk.type1_pw_verify_sec            =                sqrlbinary[ 50];
    fisk.type1_idle_timeout_min         = uint16_decode(&sqrlbinary[ 51]);
    memcpy(fisk.type1_encrypted_identity_master_key,    &sqrlbinary[ 53], sizeof(fisk.type1_encrypted_identity_master_key));
    memcpy(fisk.type1_encrypted_identity_lock_key,      &sqrlbinary[ 85], sizeof(fisk.type1_encrypted_identity_lock_key));
    memcpy(fisk.type1_verification_tag,                 &sqrlbinary[117], sizeof(fisk.type1_verification_tag));

    fisk.type2_length                   = uint16_decode(&sqrlbinary[133]);
    fisk.type2_type                     = uint16_decode(&sqrlbinary[135]);
    memcpy(fisk.type2_scrypt_random_salt,               &sqrlbinary[137], sizeof(fisk.type2_scrypt_random_salt));
    fisk.type2_scrypt_log_n_factor      =                sqrlbinary[153];
    fisk.type2_scrypt_iteration_count   = uint32_decode(&sqrlbinary[154]);
    memcpy(fisk.type2_encrypted_identity_unlock_key,    &sqrlbinary[158], sizeof(fisk.type1_encrypted_identity_lock_key));
    memcpy(fisk.type2_verification_tag,                 &sqrlbinary[190], sizeof(fisk.type1_verification_tag));

    printf("Type1: user access password protected data\n");
    printf("  length                            %u\n", identity->type1_length);
    printf("  type                              %u\n", identity->type1_type);
    printf("  pt_length                         %u\n", identity->type1_pt_length);
    printNumber("  aes_gcm_iv                       ", identity->type1_aes_gcm_iv, sizeof(fisk.type1_aes_gcm_iv));
    printNumber("  scrypt_random_salt               ", identity->type1_scrypt_random_salt, sizeof(fisk.type1_scrypt_random_salt));
    printf("  scrypt_log_n_factor               %u\n", identity->type1_scrypt_log_n_factor);
    printf("  scrypt_iteration_count            %u\n", identity->type1_scrypt_iteration_count);
    printf("  option_flags                      %u\n", identity->type1_option_flags);
    printf("  hint_length                       %u\n", identity->type1_hint_length);
    printf("  pw_verify_sec                     %u\n", identity->type1_pw_verify_sec);
    printf("  idle_timeout_min                  %u\n", identity->type1_idle_timeout_min);
    printNumber("  encrypted_identity_master_key    ", identity->type1_encrypted_identity_master_key, sizeof(fisk.type1_encrypted_identity_master_key));
    printNumber("  encrypted_identity_lock_key      ", identity->type1_encrypted_identity_lock_key, sizeof(fisk.type1_encrypted_identity_lock_key));
    printNumber("  verification_tag                 ", identity->type1_verification_tag, sizeof(fisk.type1_verification_tag));
    printf("Type2: rescue code data\n");
    printf("  length                            %u\n", identity->type2_length);
    printf("  type                              %u\n", identity->type2_type);
    printNumber("  scrypt_random_salt               ", identity->type2_scrypt_random_salt, sizeof(fisk.type2_scrypt_random_salt));
    printf("  scrypt_log_n_factor               %u\n", identity->type2_scrypt_log_n_factor);
    printf("  scrypt_iteration_count            %u\n", identity->type2_scrypt_iteration_count);
    printNumber("  encrypted_identity_unlock_key    ", identity->type2_encrypted_identity_unlock_key, sizeof(fisk.type2_encrypted_identity_unlock_key));
    printNumber("  verification_tag                 ", identity->type2_verification_tag, sizeof(fisk.type2_verification_tag));

    printf("Type1: user access password protected data\n");
    printf("  magic:                            sqrldata,\n");
    printf("  length:                           125,\n");
    printf("  type:                             1,\n");
    printf("  pt_length:                        45,\n");
    printf("  aes_gcm_iv:                       \n");
    printf("  scrypt_random_salt:               \n");
    printf("  scrypt_log_n_factor:              9,\n");
    printf("  scrypt_iteration_count:           150,\n");
    printf("  option_flags:                     499,\n");
    printf("  hint_length:                      4,\n");
    printf("  pw_verify_sec:                    5,\n");
    printf("  idle_timeout_min:                 15,\n");
    printf("  encrypted_identity_master_key:    023388cda0d7574ef78ad139f81c5d138706c6e8f8b038f614d96d9ef67c94a4\n");
    printf("  encrypted_identity_lock_key:      1f46ab7d0ed3bfa372a35eb4fbcce78c518d8d79526c05f1197c90030609e0b3\n");
    printf("  verification_tag:                 85488ce0a60f516df69471362deee0e9\n");
    printf("Type2: rescue code data\n");
    printf("  length:                           73,\n");
    printf("  type:                             2,\n");
    printf("  scrypt_random_salt:               eade0471a1fa4f8f1cf565eab3292d5e\n");
    printf("  scrypt_log_n_factor:              9,\n");
    printf("  scrypt_iteration_count:           165,\n");
    printf("  encrypted_identity_unlock_key:    f96f24229e91a6a96bdee27a5e266aa615b504f4500165ccfaa856d7f4944cea\n");
    printf("  verification_tag:                 eadd3e3ccb43c52bebaf1888f9a6d4ce\n");
}

int main()
{
    sqrl_test_suite();

    return 0;
}
