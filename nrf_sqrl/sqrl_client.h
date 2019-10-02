#ifndef _SQRL_CLIENT_H
#define _SQRL_CLIENT_H
#include <sqrl_comm.h>


typedef struct {
    char* client;
    char* ids;
} client_response_t;


void sqrl_query(client_response_t* resp, volatile sqrl_cmd_t* p_cmd, const uint8_t imk[32]);
void sqrl_ident(client_response_t* resp, volatile sqrl_cmd_t* p_cmd,
        const uint8_t ilk[32], const uint8_t imk[32], const uint8_t rlk[32]);


#endif//_SQRL_CLIENT_H
