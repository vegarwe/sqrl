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

g++ -o TestCurve25519 tests/TestCurve25519.cpp tests/RNG_mock.cpp   \
        SqrlClient/sqrl_crypto.cpp                                  \
        ~/Documents/Arduino/libraries/Crypto/src/BigNumberUtil.cpp  \
        ~/Documents/Arduino/libraries/Crypto/src/AuthenticatedCipher.cpp    \
        ~/Documents/Arduino/libraries/Crypto/src/Crypto.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/Curve25519.cpp     \
        ~/Documents/Arduino/libraries/Crypto/src/Ed25519.cpp        \
        ~/Documents/Arduino/libraries/Crypto/src/SHA512.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/SHA256.cpp         \
        ~/Documents/Arduino/libraries/Crypto/src/Hash.cpp           \
    -I  ~/Documents/Arduino/libraries/Crypto/src                    \
    -I  ~/Documents/Arduino/libraries/base64/src                    \
    -I  SqrlClient                                                  \
    &&  ./TestCurve25519
*/


#include <string.h>
#include <stdio.h>
#include <string>

#include "sqrl_crypto.h"
#include "sqrl_conv.h"
#include "sqrl_client.h"


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


void sqrl_test_suite()
{
    // Start out with hard coded test keys
    uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
    uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};

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
    printf("\n\n");
    char sin[] = "0";
    uint8_t ins[32];
    sqrl_get_ins_from_sin(ins, ssk, sin);
    printNumber("ins ", ins);
    printf("ins0 d4389834427c0029e0919368aa0e744f85bf1157d67ef559841fe3db52ee9b93\n");

    // Test url safe base64 encode
    printf("\n\n");
    unsigned char somedata[] = {0x60,0x78,0x13,0x41,0xb4,0x36,0x30,0xfb,0x6d,0x21,0x4d,0x20,0xed,0x4b,0xf8,0x77,0xaf,0xed,0x40,0xf3,0x7c,0x87,0x1c,0x06,0x13,0x89,0xbc,0xb7,0xd0,0xbe,0xe4,0x2d};
    std::string encoded = sqrl_base64_encode(std::string((char*) somedata, sizeof(somedata)));
    printf("b64  %s\n", encoded.c_str());
    printf("b640 YHgTQbQ2MPttIU0g7Uv4d6_tQPN8hxwGE4m8t9C-5C0\n");

    // Test query
    printf("\n\n");
    ClientResponse resp = sqrl_query(imk, sks, (char*) "c3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PW9HWEVVRW1Ua1BHMHowRWthM3BISlE");
    printf("client: %s\n", resp.client.c_str());
    printf("client0 dmVyPTENCmNtZD1xdWVyeQ0KaWRrPXpaOTJfYzJfdVpweXdfWkt2VEdMdnJ1c2ZTeHpDUVkybEpud3VNYTdaTjANCm9wdD1jcHN-c3VrDQo\n");
    printf("server: %s\n", resp.server.c_str());
    printf("server0 c3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PW9HWEVVRW1Ua1BHMHowRWthM3BISlE\n");
    printf("ids:    %s\n", resp.ids.c_str());
    printf("ids0    3Y2fcPZx6d9CuHol8b48fbHQ11tCtIiiLXqj0ZXj87J-in4kYT8RtwmTsYF5Ws5bBONah5udn5JvcKHnKMMrCQ\n");

    // Test ident
    printf("\n\n");
    // TODO: Create random 32 bytes
    uint8_t rlk[32] = {0xca,0x5a,0x7b,0x6e,0xa8,0xbc,0x75,0xb3,0x94,0xd1,0xdf,0x20,0xbc,0xd9,0xcf,0x4d,0x31,0x1d,0xb0,0x67,0xd8,0x77,0xd9,0xb6,0xa7,0xda,0x74,0xd6,0x1b,0x6a,0x8d,0x69};
    char* server = (char*) "dmVyPTENCm51dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQp0aWY9NQ0KcXJ5PS9zcXJsP251dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzdWs9UEJGdWZRNmR2emgtYXB3dU1tXzR6MmFybmZNdjRDVUxVRTRWZVVFYWdWOA0KdXJsPWh0dHBzOi8vd3d3LmdyYy5jb20vc3FybC9kaWFnLmh0bT9fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzaW49MA0K";
    resp = sqrl_ident(ilk, imk, rlk, sks, server, "0", true);
    printf("client: %s\n", resp.client.c_str());
    /*
    Create new keys
    rlk b'\xcaZ{n\xa8\xbcu\xb3\x94\xd1\xdf \xbc\xd9\xcfM1\x1d\xb0g\xd8w\xd9\xb6\xa7\xdat\xd6\x1bj\x8di'
    suk  b'85307f5ae1f9e9f4f6a0f0cb21ebec4b7dc66a3b50515f4a919e1f44bd7c8909'
    vuk  b'39467eef78a2717df02f06cfbb1e170768820c308d96ec98c2fa6ffb8c70e453'
    vuk  b'39467eef78a2717df02f06cfbb1e170768820c308d96ec98c2fa6ffb8c70e453'
    ursk b'68c79dc66187569c3edb14fb9ca51bb3d33c17650b74507999fc5b7123136f0039467eef78a2717df02f06cfbb1e170768820c308d96ec98c2fa6ffb8c70e453'
    ins  b'd4389834427c0029e0919368aa0e744f85bf1157d67ef559841fe3db52ee9b93'
    b64  b'YHgTQbQ2MPttIU0g7Uv4d6_tQPN8hxwGE4m8t9C-5C0'
    client b'ver=1\r\ncmd=query\r\nidk=zZ92_c2_uZpyw_ZKvTGLvrusfSxzCQY2lJnwuMa7ZN0\r\nopt=cps~suk\r\n'
    server b'sqrl://www.grc.com/sqrl?nut=oGXEUEmTkPG0z0Eka3pHJQ'
    ids b'\xdd\x8d\x9fp\xf6q\xe9\xdfB\xb8z%\xf1\xbe<}\xb1\xd0\xd7[B\xb4\x88\xa2-z\xa3\xd1\x95\xe3\xf3\xb2~\x8a~$a?\x11\xb7\t\x93\xb1\x81yZ\xce[\x04\xe3Z\x87\x9b\x9d\x9f\x92op\xa1\xe7(\xc3+\t'
    {'client': b'dmVyPTENCmNtZD1xdWVyeQ0KaWRrPXpaOTJfYzJfdVpweXdfWkt2VEdMdnJ1c2ZTeHpDUVkybEpud3VNYTdaTjANCm9wdD1jcHN-c3VrDQo', 'server': b'c3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PW9HWEVVRW1Ua1BHMHowRWthM3BISlE', 'ids': b'3Y2fcPZx6d9CuHol8b48fbHQ11tCtIiiLXqj0ZXj87J-in4kYT8RtwmTsYF5Ws5bBONah5udn5JvcKHnKMMrCQ'}
    b'7371726c3a2f2f7777772e6772632e636f6d2f7371726c3f6e75743d6f47584555456d546b5047307a30456b613370484a51'
    b'dd8d9f70f671e9df42b87a25f1be3c7db1d0d75b42b488a22d7aa3d195e3f3b27e8a7e24613f11b70993b181795ace5b04e35a879b9d9f926f70a1e728c32b09'
    */
    printf("client0 dmVyPTENCmNtZD1xdWVyeQ0KaWRrPXpaOTJfYzJfdVpweXdfWkt2VEdMdnJ1c2ZTeHpDUVkybEpud3VNYTdaTjANCmlucz0xRGlZTkVKOEFDbmdrWk5vcWc1MFQ0V19FVmZXZnZWWmhCX2oyMUx1bTVNDQpzdWs9aFRCX1d1SDU2ZlQyb1BETElldnNTMzNHYWp0UVVWOUtrWjRmUkwxOGlRaw0KdnVrPU9VWi03M2lpY1gzd0x3YlB1eDRYQjJpQ0REQ05sdXlZd3Zwdi00eHc1Rk0NCm9wdD1jcHN-c3VrDQo\n");

    // Test somwthing else
/*
url       SqrlUrl(sqrl://www.grc.com/sqrl?nut=bh3VxN8YCAjU3bWF_3gdnA)
domain    www.grc.com
sks       www.grc.com
nut       ['bh3VxN8YCAjU3bWF_3gdnA']
client b'ver=1\r\ncmd=query\r\nidk=zZ92_c2_uZpyw_ZKvTGLvrusfSxzCQY2lJnwuMa7ZN0\r\nopt=cps~suk\r\n'
server b'sqrl://www.grc.com/sqrl?nut=bh3VxN8YCAjU3bWF_3gdnA'
ids b'\x8d\xf1\x9c\nnp4x\xa76\x84d\xcc\xb8\x93\x8e\x9f\xee\xe5tX\x7fq\xab[\xfe\x1a\xb9\nK\xa2\x1f\xcc%\x14E\xb0\xa9e\xc1\xbf\xf3\x98+s\x998\xfd\xae\xf3\xf1\xc4N\x80_\tBy\xef\xf0Z\xa9\xd0\x03'
<Response [200]> time 0.9843771457672119
server records {b'ver': b'1', b'nut': b'C-KgFwOHQZV2bk3GyGHg3w', b'tif': b'5', b'qry': b'/sqrl?nut=C-KgFwOHQZV2bk3GyGHg3w', b'suk': b'PBFufQ6dvzh-apwuMm_4z2arnfMv4CULUE4VeUEagV8', b'sin': b'0'}
<Response [200]> time 0.9843780994415283
dmVyPTENCm51dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQp0aWY9NQ0KcXJ5PS9zcXJsP251dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzdWs9UEJGdWZRNmR2emgtYXB3dU1tXzR6MmFybmZNdjRDVUxVRTRWZVVFYWdWOA0KdXJsPWh0dHBzOi8vd3d3LmdyYy5jb20vc3FybC9kaWFnLmh0bT9fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzaW49MA0K
server b'dmVyPTENCm51dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQp0aWY9NQ0KcXJ5PS9zcXJsP251dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzdWs9UEJGdWZRNmR2emgtYXB3dU1tXzR6MmFybmZNdjRDVUxVRTRWZVVFYWdWOA0KdXJsPWh0dHBzOi8vd3d3LmdyYy5jb20vc3FybC9kaWFnLmh0bT9fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzaW49MA0K'
url records {b'ver': b'1', b'nut': b'_Qxn6RpQRFdy94tbzIe7oQ', b'tif': b'5', b'qry': b'/sqrl?nut=_Qxn6RpQRFdy94tbzIe7oQ', b'suk': b'PBFufQ6dvzh-apwuMm_4z2arnfMv4CULUE4VeUEagV8', b'url': b'https://www.grc.com/sqrl/diag.htm?_Qxn6RpQRFdy94tbzIe7oQ', b'sin': b'0'}
redirect 'https://www.grc.com/sqrl/diag.htm?_Qxn6RpQRFdy94tbzIe7oQ'
https://www.grc.com/sqrl/diag.htm?_Qxn6RpQRFdy94tbzIe7oQ
*/
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
