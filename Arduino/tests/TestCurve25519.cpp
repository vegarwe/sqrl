/*
 * Copyright (C) 2015 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
These test cases exercises the sqrl_crypto library.

g++ -o TestCurve25519 SqrlClient/sqrl_crypto.cpp                    \
        tests/TestCurve25519.cpp tests/RNG_mock.cpp                 \
        ~/Documents/Arduino/libraries/Crypto/src/BigNumberUtil.cpp  \
        ~/Documents/Arduino/libraries/Crypto/src/Crypto.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/Curve25519.cpp     \
        ~/Documents/Arduino/libraries/Crypto/src/Ed25519.cpp        \
        ~/Documents/Arduino/libraries/Crypto/src/SHA512.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/SHA256.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/Hashcpp            \
    -I  ~/Documents/Arduino/libraries/Crypto/src                    \
    -I  SqrlClient                                                  \
    &&  ./TestCurve25519
*/


#include <string.h>
#include <stdio.h>

#include "sqrl_crypto.h"


void printNumber(const char *name, const uint8_t *x)
{
    static const char hexchars[] = "0123456789abcdef";
    printf(name);
    putchar(' ');
    for (uint8_t posn = 0; posn < 32; ++posn) {
        putchar(hexchars[(x[posn] >> 4) & 0x0F]);
        putchar(hexchars[(x[posn] >> 0) & 0x0F]);
    }
    putchar('\n');
}


void sqrl_test_suite()
{
    uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
    uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};
    //printf("iuk0 a243bfb0ed04565d0461a7731a8fad18de4bddf8e3833853926ccfaf2e2e04a6\n");
    //printNumber("iuk ", iuk);

    uint8_t ilk[32];
    sqrl_get_ilk_from_iuk(ilk, iuk);
    printf("ilk0 00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878\n");
    printNumber("ilk ", ilk);

    // Get idk for site
    printf("\n\n");
    char sks[] = "www.grc.com";
    uint8_t ssk[32];
    uint8_t idk[32];
    sqrl_get_idk_for_site(idk, ssk, imk, sks);
    printNumber("ssk ", ssk);
    printf("ssk0 a9f02ccd2ef61146a0f4c9101f4dbf285059d9687cba62b136b9a188623943d4\n");
    printNumber("idk ", idk);
    printf("idk0 cd9f76fdcdbfb99a72c3f64abd318bbebbac7d2c730906369499f0b8c6bb64dd\n");

    // Get ursk and vuk from suk and iuk
    printf("\n\n");
    uint8_t suk[] = {0x0c,0x49,0xb7,0xd7,0x01,0x06,0xec,0xc3,0x3c,0x68,0xc7,0xc9,0x93,0x12,0x33,0xdf,0xc6,0x84,0x98,0xf4,0x6a,0xb7,0xda,0x4e,0xea,0x20,0xdf,0xb6,0x92,0x5e,0xc2,0x7f};
    uint8_t vuk[32];
    uint8_t ursk[32];
    if(! sqrl_get_unlock_request_signing_key(ursk, vuk, suk, iuk)) { printf("failed\n"); }
    printNumber("suk ", suk);
    printf("suk0 0c49b7d70106ecc33c68c7c9931233dfc68498f46ab7da4eea20dfb6925ec27f\n");
    printNumber("vuk ", vuk);
    printf("vuk0 580a13634186a5aca95a99f94f36bad7c5aed58e3d95e00a8b63a559a5543817\n");
    printNumber("ursk", ursk);
    printf("ursk0793d0e4c49ea722e7d59b6c874f2a0198ccb53bd465c4022ab5019c14737050a\n");

    // Test EnHash by creating ins from sin
    uint8_t tmp[32];
    EnHash(tmp, (char*) ssk, 32);
    printNumber("tmp        ", tmp);
    printf("EnHash(ssk) 38ce2f61fb17f9007ba24f8ab83e42833194e852cfbdf0fe475bed46b18b1b2d\n");
}

int main()
{
    sqrl_test_suite();

    return 0;
}

// Decoding data, this will take a while
// iuk  b'a243bfb0ed04565d0461a7731a8fad18de4bddf8e3833853926ccfaf2e2e04a6'
// imk  b'21d70894575e6b6efe991fb86a9868a49f3a72040e88252d82be5a3ac6c3aa23'
// ilk  b'00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878'
// Decoding data, this will take a while
// imk  b'21d70894575e6b6efe991fb86a9868a49f3a72040e88252d82be5a3ac6c3aa23'
// ilk  b'00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878'
// Test crypto
// idk  b'cd9f76fdcdbfb99a72c3f64abd318bbebbac7d2c730906369499f0b8c6bb64dd'
// suk  b'60781341b43630fb6d214d20ed4bf877afed40f37c871c061389bcb7d0bee42d'
// vuk  b'dfaf52eb1f58976b17e1c080190d371780dd714b8e82da4faae509d167d6e2a5'
// ursk b'eb18a00f1a1919e3e593be9289cfa403590097e6a84094d90d074fdb3acaf751'
// Create new keys
// suk  b'0c49b7d70106ecc33c68c7c9931233dfc68498f46ab7da4eea20dfb6925ec27f'
// vuk  b'580a13634186a5aca95a99f94f36bad7c5aed58e3d95e00a8b63a559a5543817'
// vuk  b'580a13634186a5aca95a99f94f36bad7c5aed58e3d95e00a8b63a559a5543817'
// ursk b'793d0e4c49ea722e7d59b6c874f2a0198ccb53bd465c4022ab5019c14737050a'
