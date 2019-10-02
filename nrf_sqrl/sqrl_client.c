#include "sqrl_client.h"

#include <string.h>
#include <stdlib.h>

#include "occ_ed25519.h"
#include "sqrl_conv.h"
#include "sqrl_crypto.h"


static size_t append_string(char* dst, const char* data)
{
    size_t len = strlen(data);
    memcpy(dst, data, len);

    return len;
}


void sqrl_query(client_response_t* resp, volatile sqrl_cmd_t* p_cmd, const uint8_t imk[32])
{
    uint8_t idk[32];
    uint8_t ssk[32];
    sqrl_get_idk_for_site(idk, ssk, imk, p_cmd->sks);

    static char client[2048];

    size_t idx = 0;
    size_t encoded_len = 2048; // TODO: Truce is somewhat smaller

    memset(client, 0, sizeof(client));
    idx += append_string(&client[idx], "ver=1\r\n");
    idx += append_string(&client[idx], "cmd=query\r\n");
    idx += append_string(&client[idx], "idk=");
    int ret = sqrl_base64_encode(&client[idx], &encoded_len, (char*)idk, sizeof(idk));
    (void) ret; // TODO: Check?
    //NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
    idx += encoded_len;
    idx += append_string(&client[idx], "\r\n");
    idx += append_string(&client[idx], "opt=cps~suk\r\n"); // TODO: Always cps and suk?

    encoded_len = sqrl_base64_size_calc(client, idx);
    resp->client = malloc(encoded_len);
    ret = sqrl_base64_encode(resp->client, &encoded_len, client, idx);
    //NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);

    memcpy(&client[0], resp->client, encoded_len);
    size_t server_len = strlen(p_cmd->server);
    memcpy(&client[encoded_len], p_cmd->server, server_len);

    uint8_t signature[64];
    occ_ed25519_sign(signature, (uint8_t*)client, encoded_len + server_len, ssk, idk);

    encoded_len = sqrl_base64_size_calc((char*)signature, sizeof(signature));
    resp->ids = malloc(encoded_len);
    ret = sqrl_base64_encode(resp->ids, &encoded_len, (char*)signature, sizeof(signature));
    //NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
}


void sqrl_ident(client_response_t* resp, volatile sqrl_cmd_t* p_cmd,
        const uint8_t ilk[32], const uint8_t imk[32], const uint8_t rlk[32])
{
    uint8_t idk[32];
    uint8_t ssk[32];
    sqrl_get_idk_for_site(idk, ssk, imk, p_cmd->sks);

    static char client[2048];

    size_t idx = 0;
    size_t encoded_len = 2048; // TODO: Truce is somewhat smaller

    memset(client, 0, sizeof(client));
    idx += append_string(&client[idx], "ver=1\r\n");
    idx += append_string(&client[idx], "cmd=ident\r\n");
    idx += append_string(&client[idx], "idk=");
    int ret = sqrl_base64_encode(&client[idx], &encoded_len, (char*)idk, sizeof(idk));
    (void) ret; // TODO: Check?
    //NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
    idx += encoded_len;
    idx += append_string(&client[idx], "\r\n");

    if (p_cmd->sin && strlen(p_cmd->sin) > 0) {
        uint8_t ins[32];
        sqrl_get_ins_from_sin(ins, ssk, p_cmd->sin);

        idx += append_string(&client[idx], "ins=");
        encoded_len = 2048; // TODO: Truce is somewhat smaller
        ret = sqrl_base64_encode(&client[idx], &encoded_len, (char*)ins, sizeof(ins));
        //NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
        idx += encoded_len;
        idx += append_string(&client[idx], "\r\n");
    }

    if (p_cmd->create_suk) {
        uint8_t suk[32];
        uint8_t vuk[32];
        sqrl_idlock_keys(suk, vuk, rlk, ilk);

        idx += append_string(&client[idx], "suk=");
        encoded_len = 2048; // TODO: Truce is somewhat smaller
        ret = sqrl_base64_encode(&client[idx], &encoded_len, (char*)suk, sizeof(suk));
        //NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
        idx += encoded_len;
        idx += append_string(&client[idx], "\r\n");

        idx += append_string(&client[idx], "vuk=");
        encoded_len = 2048; // TODO: Truce is somewhat smaller
        ret = sqrl_base64_encode(&client[idx], &encoded_len, (char*)vuk, sizeof(vuk));
        //NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
        idx += encoded_len;
        idx += append_string(&client[idx], "\r\n");
    }

    idx += append_string(&client[idx], "opt=cps~suk\r\n"); // TODO: Always cps and suk?

    //NRF_LOG_RAW_INFO("client1 %s\n", client);
    encoded_len = sqrl_base64_size_calc(client, idx);
    resp->client = malloc(encoded_len);
    ret = sqrl_base64_encode(resp->client, &encoded_len, client, idx);
    //NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);

    memcpy(&client[0], resp->client, encoded_len);
    size_t server_len = strlen(p_cmd->server);
    memcpy(&client[encoded_len], p_cmd->server, server_len);

    uint8_t signature[64];
    occ_ed25519_sign(signature, (uint8_t*)client, encoded_len + server_len, ssk, idk);

    encoded_len = sqrl_base64_size_calc((char*)signature, sizeof(signature));
    resp->ids = malloc(encoded_len);
    ret = sqrl_base64_encode(resp->ids, &encoded_len, (char*)signature, sizeof(signature));
    //NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
}


