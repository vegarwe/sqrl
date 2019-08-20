#include "sqrl_conv.h"

#include "mbedtls/base64.h"

#include <stdint.h>


size_t sqrl_base64_size_calc(char* input, size_t input_len)
{
    size_t olen;
    mbedtls_base64_encode(NULL, 0, &olen, (uint8_t*)input, input_len);
    return olen;
}


int sqrl_base64_encode(char* encoded, size_t* encoded_len, const char* input, size_t input_len)
{
    size_t olen;
    int retval = mbedtls_base64_encode(
            (uint8_t*)encoded, *encoded_len, &olen, (uint8_t*)input, input_len);
    *encoded_len = olen;

    if (retval != 0)
    {
        return retval;
    }

    for (int i = 0; i < *encoded_len; i++)
    {
        if (encoded[i] == '+')
        {
            encoded[i] = '-';
        }
        if (encoded[i] == '/')
        {
            encoded[i] = '_';
        }
        if (encoded[i] == '=')
        {
            encoded[i] = '\0';
            *encoded_len = i;
            break;
        }
    }


    return retval;
}

