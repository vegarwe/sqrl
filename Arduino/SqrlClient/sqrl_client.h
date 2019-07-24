#ifndef _SQRL_CLIENT_H
#define _SQRL_CLIENT_H

#include <string>

#include "base64.hpp"
#include "Ed25519.h"
#include "sqrl_crypto.h"
#include "sqrl_conv.h"


struct ClientResponse {
    std::string client;
    std::string server;
    std::string ids;
};


ClientResponse sqrl_query(const uint8_t imk[32], const char* sks, const char* server)
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


ClientResponse sqrl_ident(const uint8_t ilk[32], const uint8_t imk[32], const uint8_t rlk[32],
        const char* sks, const char* server,
        const char* sin, bool create_suk)
{
    uint8_t idk[32];
    uint8_t ssk[32];
    sqrl_get_idk_for_site(idk, ssk, imk, sks);

    std::string client("ver=1\r\n");
    client  += "cmd=ident\r\n";
    client  += "idk=" + sqrl_base64_encode(std::string((char*) idk, 32)) + "\r\n";

    if (sin) {
        uint8_t ins[32];
        sqrl_get_ins_from_sin(ins, ssk, sin);
        client += "ins=" + sqrl_base64_encode(std::string((char*) ins, 32)) + "\r\n";
    }

    if (create_suk) {
        uint8_t suk[32];
        uint8_t vuk[32];
        sqrl_idlock_keys(suk, vuk, rlk, ilk);
        client += "suk=" + sqrl_base64_encode(std::string((char*) suk, 32)) + "\r\n";
        client += "vuk=" + sqrl_base64_encode(std::string((char*) vuk, 32)) + "\r\n";
    }

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


#endif//_SQRL_CLIENT_H
