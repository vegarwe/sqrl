#ifndef _SQRL_CLIENT_H
#define _SQRL_CLIENT_H

#include <string>

#include "base64.hpp"
#include "Ed25519.h"


struct ClientResponse {
    std::string client;
    std::string server;
    std::string ids;
};


ClientResponse sqrl_query(const uint8_t imk[32], const char* sks, char* server)
{
    uint8_t idk[32];
    uint8_t ssk[32];
    sqrl_get_idk_for_site(idk, ssk, imk, sks);

    std::string client("ver=1\r\n");
    client  += "cmd=query\r\n";
    client  += "idk=" + sqrl_base64_encode(std::string((char*) idk, 32)) + "\r\n";
    client  += "opt=cps~suk\r\n"; // TODO: Always cps and suk?

    ClientResponse resp;
    resp.client = sqrl_base64_encode(client);
    resp.server = server;

    uint8_t signature[64];
    std::string toSign(resp.client);
    toSign += resp.server;
    Ed25519::sign(signature, ssk, idk, toSign.data(), toSign.size());

    resp.ids = sqrl_base64_encode(std::string((char*)signature, sizeof(signature)));

    return resp;
}
//    # Get site specific keys
//    idk, ssk = sqrl_get_idk_for_site(imk, sks)
//
//    client  =  b'ver=1\r\n'
//    client  += b'cmd=query\r\n'
//    client  += b'idk=%s\r\n' % sqrl_conv.base64_encode(idk)
//    client  += b'opt=cps~suk\r\n'
//    print('client', client)
//    client  = sqrl_conv.base64_encode(client)
//
//    print('server', server)
//    server  = sqrl_conv.base64_encode(server)
//
//    ids     = sqrl_sign(ssk, client + server)
//    form    = {'client': client,
//               'server': server,
//               'ids':    sqrl_conv.base64_encode(ids)}
//    return form

#endif//_SQRL_CLIENT_H
