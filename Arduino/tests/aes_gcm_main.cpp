#include <string.h>
#include <stdio.h>
#include <string>


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


void sqrl_test_suite()
{
    printf("sqrl_test_suite\r\n");

    char* password = "1234567890ab";
    char* rescue_code = "949106491269852269220540";
    char sqrlbinary[] = "sqrldata}\x00\x01\x00-\x00\"wQ\x12""2\x0e\xb5\x89""1\xfep\x97\xef\xf2""e]\xf6\x0fg\a\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x02""3\x88\xcd\xa0\xd7WN\xf7\x8a\xd1""9\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb0""8\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1f""F\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5""e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$\"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01""e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcb""C\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce";
}

int main()
{
    sqrl_test_suite();

    return 0;
}
