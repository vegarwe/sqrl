#ifndef _SQRL_CONV_H
#define _SQRL_CONV_H

#include "base64.hpp"


std::string sqrl_base64_encode(const std::string& data)
{
    size_t encoded_len = encode_base64_length(data.size());
    unsigned char encoded[encoded_len];
    encoded_len = encode_base64((unsigned char*) data.data(), data.size(), encoded);

    for (int i = 0; i < encoded_len; i++) {
        if (encoded[i] == '+') {
            encoded[i] = '-';
        }
        if (encoded[i] == '/') {
            encoded[i] = '_';
        }
        if (encoded[i] == '=') {
            encoded[i] = '\0';
            encoded_len = i;
            break;
        }
    }

    return std::string((char*) encoded, encoded_len);
}


#endif//_SQRL_CONV_H
