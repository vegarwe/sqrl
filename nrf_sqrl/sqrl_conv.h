#ifndef _SQRL_CONV_H
#define _SQRL_CONV_H
#include <stddef.h>


size_t sqrl_base64_size_calc(char* input, size_t input_len);
int sqrl_base64_encode(char* encoded, size_t* encoded_len, const char* input, size_t input_len);


#endif//_SQRL_CONV_H
