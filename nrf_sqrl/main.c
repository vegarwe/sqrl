/**
 * Copyright (c) 2018 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @defgroup nrf_crypto_eddsa_example
 * @{
 * @ingroup nrf_crypto_eddsa
 * @brief EdDSA Example Application main file.
 *
 * This file contains the source code for a sample application that demonstrates the usage of the
 * nrf_crypto library to generate and verify an EdDSA signature. Different backends can be
 * used by adjusting @ref sdk_config.h.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "nrf_assert.h"
#include "nrf_drv_uart.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_crypto.h"
#include "nrf_crypto_hash.h"
#include "occ_curve25519.h"
#include "occ_ed25519.h"
#include "mbedtls/base64.h"
#include "sqrl_comm.h"


/** @brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


static void print_array(const uint8_t *string, size_t len)
{
  #if NRF_LOG_ENABLED
  for(size_t i = 0; i < len; i++)
  {
    NRF_LOG_RAW_INFO("%02x", string[i]);
  }
  #endif // NRF_LOG_ENABLED
}

#define PRINT_HEX(msg, res, len)    \
do                                  \
{                                   \
    NRF_LOG_RAW_INFO(msg);          \
    NRF_LOG_RAW_INFO(" ");          \
    print_array(res, len);          \
    NRF_LOG_RAW_INFO("\n")          \
} while(0)



void sqrl_hmac(uint8_t digest[32], const uint8_t key[32], const char* data, size_t data_len)
{
    static nrf_crypto_hmac_context_t    s_context;

    ret_code_t  err_code;
    size_t      digest_len = NRF_CRYPTO_HASH_SIZE_SHA256;

    err_code = nrf_crypto_hmac_init(&s_context, &g_nrf_crypto_hmac_sha256_info, key, 32);
    APP_ERROR_CHECK(err_code);

    // Push all data in one go (could be done repeatedly)
    err_code = nrf_crypto_hmac_update(&s_context, (uint8_t*)data, data_len);
    APP_ERROR_CHECK(err_code);

    // Finish calculation
    err_code = nrf_crypto_hmac_finalize(&s_context, digest, &digest_len);
    APP_ERROR_CHECK(err_code);
}

void sqrl_get_idk_for_site(uint8_t idk[32], uint8_t ssk[32], const uint8_t imk[32], const char* sks)
{
    sqrl_hmac(ssk, imk, sks, strlen(sks));
    occ_ed25519_public_key(idk, ssk);
}

void sqrl_make_public(uint8_t publ[32], const uint8_t priv[32])
{
    occ_curve25519_scalarmult_base(publ, priv);
}

void sqrl_get_ilk_from_iuk(uint8_t ilk[32], uint8_t iuk[32])
{
    sqrl_make_public(ilk, iuk);
}

void sqrl_get_unlock_request_signing_key(uint8_t ursk[32], uint8_t vuk[32], const uint8_t suk[32], const uint8_t iuk[32])
{
    occ_curve25519_scalarmult(ursk, iuk, suk);
    occ_ed25519_public_key(vuk, ursk);
}

void EnHash(uint8_t digest[32], const uint8_t data[32])
{
    uint8_t tmp_data[32];
    memcpy(tmp_data, data, sizeof(tmp_data));

    memset(digest, 0, 32);
    for (int i = 0; i < 16; i++)
    {
        occ_sha256(tmp_data, tmp_data, sizeof(tmp_data)); // TODO: Safe to reuse tmp_data here?

        for (int j = 0; j < 32; j++) {
            digest[j] ^= tmp_data[j];
        }
    }
}


void sqrl_get_ins_from_sin(uint8_t ins[32], const uint8_t ssk[32], const char* sin)
{
    EnHash(ins, ssk);
    sqrl_hmac(ins, ins, sin, strlen(sin)); // Note: reusing ins here is safe... :-)
}


void sqrl_idlock_keys(uint8_t suk[32], uint8_t vuk[32], const uint8_t rlk[32], const uint8_t ilk[32])
{
    uint8_t dhka[32];
    sqrl_make_public(suk, rlk);
    occ_curve25519_scalarmult(dhka, rlk, ilk);
    occ_ed25519_public_key(vuk, dhka);
}


void sqrl_sha256(uint8_t digest[32], const char* data, size_t data_len)
{
    static nrf_crypto_hash_context_t s_hash_context;

    ret_code_t  err_code;
    size_t      digest_len = NRF_CRYPTO_HASH_SIZE_SHA256;

    err_code = nrf_crypto_hash_init(&s_hash_context, &g_nrf_crypto_hash_sha256_info);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_crypto_hash_update(&s_hash_context, (uint8_t*)data, data_len);
    APP_ERROR_CHECK(err_code);

    // Run the finalize when all data has been fed to the update function.
    // this gives you the result
    err_code = nrf_crypto_hash_finalize(&s_hash_context, digest, &digest_len);
    APP_ERROR_CHECK(err_code);
}

size_t sqrl_base64_size_calc(char* input, size_t input_len)
{
    size_t olen;
    mbedtls_base64_encode(NULL, 0, &olen, (uint8_t*)input, input_len);
    return olen;
//int base64Length = bytes.length / 3 * 4; // Strictly integer division
//if (bytes.length % 3 != 0)
//{
//    base64Length += 4; // Extra padding characters will be added
//}
// // or....
// return (sizeof(somedata) / 3 * 4) + 4;
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

typedef struct {
    char* client;
    char* ids;
} client_response_t;


size_t append_string(char* dst, const char* data)
{
    size_t len = strlen(data);
    memcpy(dst, data, len);

    return len;
}


void sqrl_query(client_response_t* resp, sqrl_cmd_t* p_cmd, const uint8_t imk[32])
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
    NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
    idx += encoded_len;
    idx += append_string(&client[idx], "\r\n");
    idx += append_string(&client[idx], "opt=cps~suk\r\n"); // TODO: Always cps and suk?

    encoded_len = sqrl_base64_size_calc(client, idx);
    resp->client = malloc(encoded_len);
    ret = sqrl_base64_encode(resp->client, &encoded_len, client, idx);
    NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);

    memcpy(&client[0], resp->client, encoded_len);
    size_t server_len = strlen(p_cmd->server);
    memcpy(&client[encoded_len], p_cmd->server, server_len);

    uint8_t signature[64];
    occ_ed25519_sign(signature, (uint8_t*)client, encoded_len + server_len, ssk, idk);

    encoded_len = sqrl_base64_size_calc((char*)signature, sizeof(signature));
    resp->ids = malloc(encoded_len);
    ret = sqrl_base64_encode(resp->ids, &encoded_len, (char*)signature, sizeof(signature));
    NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
}


void sqrl_ident(client_response_t* resp, sqrl_cmd_t* p_cmd,
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
    NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
    idx += encoded_len;
    idx += append_string(&client[idx], "\r\n");

    if (p_cmd->sin) {
        uint8_t ins[32];
        sqrl_get_ins_from_sin(ins, ssk, p_cmd->sin);

        idx += append_string(&client[idx], "ins=");
        encoded_len = 2048; // TODO: Truce is somewhat smaller
        ret = sqrl_base64_encode(&client[idx], &encoded_len, (char*)ins, sizeof(ins));
        NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
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
        NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
        idx += encoded_len;
        idx += append_string(&client[idx], "\r\n");

        idx += append_string(&client[idx], "vuk=");
        encoded_len = 2048; // TODO: Truce is somewhat smaller
        ret = sqrl_base64_encode(&client[idx], &encoded_len, (char*)vuk, sizeof(vuk));
        NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
        idx += encoded_len;
        idx += append_string(&client[idx], "\r\n");
    }

    idx += append_string(&client[idx], "opt=cps~suk\r\n"); // TODO: Always cps and suk?

    //NRF_LOG_RAW_INFO("client1 %s\n", client);
    encoded_len = sqrl_base64_size_calc(client, idx);
    resp->client = malloc(encoded_len);
    ret = sqrl_base64_encode(resp->client, &encoded_len, client, idx);
    NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);

    memcpy(&client[0], resp->client, encoded_len);
    size_t server_len = strlen(p_cmd->server);
    memcpy(&client[encoded_len], p_cmd->server, server_len);

    uint8_t signature[64];
    occ_ed25519_sign(signature, (uint8_t*)client, encoded_len + server_len, ssk, idk);

    encoded_len = sqrl_base64_size_calc((char*)signature, sizeof(signature));
    resp->ids = malloc(encoded_len);
    ret = sqrl_base64_encode(resp->ids, &encoded_len, (char*)signature, sizeof(signature));
    NRF_LOG_RAW_INFO("ret %d, encoded_len %d\n", ret, encoded_len);
}


static nrf_drv_uart_t m_uart = NRF_DRV_UART_INSTANCE(0);
static volatile bool    m_xfer_done;
static uint8_t          rx_buffer[1];
static volatile bool    cmd_newline = false;


static void uart_evt_handler(nrf_drv_uart_event_t * p_event, void * p_context)
{
    if (p_event->type == NRF_DRV_UART_EVT_RX_DONE)
    {
        (void)nrf_drv_uart_rx(&m_uart, rx_buffer, 1);

        sqrl_comm_handle_input(p_event->data.rxtx.p_data[0]);
    }
    else if (p_event->type == NRF_DRV_UART_EVT_ERROR)
    {
        // p_event->data.error.error_mask;
        (void)nrf_drv_uart_rx(&m_uart, rx_buffer, 1);
    }
    else if (p_event->type == NRF_DRV_UART_EVT_TX_DONE)
    {
        m_xfer_done = true;
    }
}

void comm_uart_init(void)
{
    nrf_drv_uart_config_t config = NRF_DRV_UART_DEFAULT_CONFIG;
    config.pseltxd  = 6;
    config.pselrxd  = 8; // TODO: Check
    config.pselcts  = NRF_UART_PSEL_DISCONNECTED;
    config.pselrts  = NRF_UART_PSEL_DISCONNECTED;
    config.baudrate = NRF_UART_BAUDRATE_115200;
    ret_code_t err_code = nrf_drv_uart_init(&m_uart, &config, uart_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_uart_rx(&m_uart, rx_buffer, 1);
    APP_ERROR_CHECK(err_code);
}

static void serial_tx(char const * p_buffer, size_t len)
{
    while (len > 0)
    {
        uint8_t len8 = (uint8_t)(len & 0xFF);

        if (len > 0xff)
        {
            len8 = 0xff;
        }

        m_xfer_done = false;
        ret_code_t err_code = nrf_drv_uart_tx(&m_uart, (uint8_t *)p_buffer, len8);
        APP_ERROR_CHECK(err_code);

        /* wait for completion since buffer is reused*/
        while (m_xfer_done == false)
        {
        }

        len -= len8;
        p_buffer = &p_buffer[len8];
    }
}


static sqrl_cmd_t* mp_cmd;

void on_sqrl_comm_evt(sqrl_comm_evt_t * p_evt)
{
    if (p_evt->evt_type == SQRL_COMM_EVT_COMMAND)
    {
        mp_cmd = p_evt->evt.p_cmd;
    }
}


void sqrl_client_loop(void)
{
    char outputbuffer[2048];

    // Start out with hard coded test keys
    uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
    uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};

    uint8_t ilk[32];
    sqrl_get_ilk_from_iuk(ilk, iuk);

    sprintf(outputbuffer, "\n\nlog: sqrl_client_loop\n");    serial_tx(outputbuffer, strlen(outputbuffer));

    client_response_t resp = {0};
    while (true)
    {
        while (mp_cmd == NULL) { }

        if (mp_cmd->type == SQRL_CMD_QUERY)
        {
            sqrl_query(&resp, mp_cmd, imk);

            sprintf(outputbuffer, "\ncmd:     %s\n", "query"); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "sks:    '%s'\n", mp_cmd->sks); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "client: '%s'\n", resp.client); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "server: '%s'\n", mp_cmd->server); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "ids:    '%s'\n", resp.ids);    serial_tx(outputbuffer, strlen(outputbuffer));
        }
        else if (mp_cmd->type == SQRL_CMD_IDENT)
        {
            // TODO: Create random 32 bytes
            uint8_t rlk[] = {0xca,0x5a,0x7b,0x6e,0xa8,0xbc,0x75,0xb3,0x94,0xd1,0xdf,0x20,0xbc,0xd9,0xcf,0x4d,0x31,0x1d,0xb0,0x67,0xd8,0x77,0xd9,0xb6,0xa7,0xda,0x74,0xd6,0x1b,0x6a,0x8d,0x69};
            sqrl_ident(&resp, mp_cmd, ilk, imk, rlk);

            sprintf(outputbuffer, "\ncmd:     %s\n", "ident"); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "sks:    '%s'\n", mp_cmd->sks); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "client: '%s'\n", resp.client); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "server: '%s'\n", mp_cmd->server); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "ids:    '%s'\n", resp.ids);    serial_tx(outputbuffer, strlen(outputbuffer));
        }
        else
        {
            sprintf(outputbuffer, "Invalid command\n"); serial_tx(outputbuffer, strlen(outputbuffer));
        }

        free(resp.client);
        free(resp.ids);
        memset(&resp, 0, sizeof(resp));
        mp_cmd = NULL;
        sqrl_comm_command_handled();
    }
}


void test(void)
{
    NRF_LOG_RAW_INFO("\n\nEdDSA example started\n");

    // Start out with hard coded test keys
    uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
    uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};

    uint8_t ilk[32];
    sqrl_get_ilk_from_iuk(ilk, iuk);
    PRINT_HEX("ilk ", ilk, 32);
    NRF_LOG_RAW_INFO("ilk  00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878\n\n");

    // Get idk for site
    char sks[] = "www.grc.com";
    uint8_t ssk[32];
    uint8_t idk[32];
    sqrl_get_idk_for_site(idk, ssk, imk, sks);
    PRINT_HEX("ssk ", ssk, 32);
    NRF_LOG_RAW_INFO("ssk  a9f02ccd2ef61146a0f4c9101f4dbf285059d9687cba62b136b9a188623943d4\n");
    PRINT_HEX("idk ", idk, 32);
    NRF_LOG_RAW_INFO("idk  cd9f76fdcdbfb99a72c3f64abd318bbebbac7d2c730906369499f0b8c6bb64dd\n\n");

    // Get ursk and vuk from suk and iuk
    uint8_t suk[] = {0x0c,0x49,0xb7,0xd7,0x01,0x06,0xec,0xc3,0x3c,0x68,0xc7,0xc9,0x93,0x12,0x33,0xdf,0xc6,0x84,0x98,0xf4,0x6a,0xb7,0xda,0x4e,0xea,0x20,0xdf,0xb6,0x92,0x5e,0xc2,0x7f};
    uint8_t vuk[32];
    uint8_t ursk[32];
    sqrl_get_unlock_request_signing_key(ursk, vuk, suk, iuk);
    PRINT_HEX("suk ", suk, 32);
    NRF_LOG_RAW_INFO("suk  0c49b7d70106ecc33c68c7c9931233dfc68498f46ab7da4eea20dfb6925ec27f\n");
    PRINT_HEX("vuk ", vuk, 32);
    NRF_LOG_RAW_INFO("vuk  580a13634186a5aca95a99f94f36bad7c5aed58e3d95e00a8b63a559a5543817\n");
    PRINT_HEX("ursk", ursk, 32);
    NRF_LOG_RAW_INFO("ursk0793d0e4c49ea722e7d59b6c874f2a0198ccb53bd465c4022ab5019c14737050a\n\n");

    // Test EnHash by creating ins from sin
    char sin[] = "0";
    uint8_t ins[32];
    sqrl_get_ins_from_sin(ins, ssk, sin);
    PRINT_HEX("ins ", ins, 32);
    NRF_LOG_RAW_INFO("ins  d4389834427c0029e0919368aa0e744f85bf1157d67ef559841fe3db52ee9b93\n");

    // Test sha256
    uint8_t sha[32];
    sqrl_sha256(sha, sks, strlen(sks));
    PRINT_HEX("sha ", sha, 32);
    NRF_LOG_RAW_INFO("sha  8fa88b71d0f8efd1d0c77341f2d39b1c7c1ceac995dc13c273e6f438b04745c9\n\n");

    // Test url safe base64 encode
    char somedata[] = {0x60,0x78,0x13,0x41,0xb4,0x36,0x30,0xfb,0x6d,0x21,0x4d,0x20,0xed,0x4b,0xf8,0x77,0xaf,0xed,0x40,0xf3,0x7c,0x87,0x1c,0x06,0x13,0x89,0xbc,0xb7,0xd0,0xbe,0xe4,0x2d};
    size_t encoded_len = sqrl_base64_size_calc(somedata, sizeof(somedata));
    NRF_LOG_RAW_INFO("encoded_len %d, needed? %d\n", encoded_len, (sizeof(somedata) / 3 * 4) + 4);
    char encoded[encoded_len];
    int ret = sqrl_base64_encode(encoded, &encoded_len, somedata, sizeof(somedata));
    NRF_LOG_RAW_INFO("ret %d, encoded_len %d, sizeof(encoded) %d, sizeof(somedata) %d\n",
            ret, encoded_len, sizeof(encoded), sizeof(somedata));
    NRF_LOG_RAW_INFO("b64  %s\n", encoded);
    NRF_LOG_RAW_INFO("b640 YHgTQbQ2MPttIU0g7Uv4d6_tQPN8hxwGE4m8t9C-5C0\n\n");


    client_response_t resp;
    sqrl_cmd_t cmd;

    // Test query
    cmd.sks = sks;
    cmd.server = (char*) "c3FybDovL3d3dy5ncmMuY29tL3Nxcmw_bnV0PW9HWEVVRW1Ua1BHMHowRWthM3BISlE";
    sqrl_query(&resp, &cmd, imk);
    NRF_LOG_RAW_INFO("client: %s\n", resp.client);
    NRF_LOG_RAW_INFO("client0 dmVyPTENCmNtZD1xdWVyeQ0KaWRrPXpaOTJfYzJfdVpweXdfWkt2VEdMdnJ1c2ZTeHpDUVkybEpud3VNYTdaTjANCm9wdD1jcHN-c3VrDQo\n");
    NRF_LOG_RAW_INFO("ids:    %s\n", resp.ids);
    NRF_LOG_RAW_INFO("ids0    3Y2fcPZx6d9CuHol8b48fbHQ11tCtIiiLXqj0ZXj87J-in4kYT8RtwmTsYF5Ws5bBONah5udn5JvcKHnKMMrCQ\n\n");
    free(resp.client);
    free(resp.ids);



    // Test ident
    // TODO: Create random 32 bytes
    uint8_t rlk[32] = {0xca,0x5a,0x7b,0x6e,0xa8,0xbc,0x75,0xb3,0x94,0xd1,0xdf,0x20,0xbc,0xd9,0xcf,0x4d,0x31,0x1d,0xb0,0x67,0xd8,0x77,0xd9,0xb6,0xa7,0xda,0x74,0xd6,0x1b,0x6a,0x8d,0x69};
    cmd.sks = sks;
    cmd.server = (char*) "dmVyPTENCm51dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQp0aWY9NQ0KcXJ5PS9zcXJsP251dD1fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzdWs9UEJGdWZRNmR2emgtYXB3dU1tXzR6MmFybmZNdjRDVUxVRTRWZVVFYWdWOA0KdXJsPWh0dHBzOi8vd3d3LmdyYy5jb20vc3FybC9kaWFnLmh0bT9fUXhuNlJwUVJGZHk5NHRiekllN29RDQpzaW49MA0K";
    cmd.sin = "0";
    cmd.create_suk = true;
    sqrl_ident(&resp, &cmd, ilk, imk, rlk);
    NRF_LOG_RAW_INFO("client: %s\n", resp.client);
    NRF_LOG_RAW_INFO("client0 dmVyPTENCmNtZD1xdWVyeQ0KaWRrPXpaOTJfYzJfdVpweXdfWkt2VEdMdnJ1c2ZTeHpDUVkybEpud3VNYTdaTjANCmlucz0xRGlZTkVKOEFDbmdrWk5vcWc1MFQ0V19FVmZXZnZWWmhCX2oyMUx1bTVNDQpzdWs9aFRCX1d1SDU2ZlQyb1BETElldnNTMzNHYWp0UVVWOUtrWjRmUkwxOGlRaw0KdnVrPU9VWi03M2lpY1gzd0x3YlB1eDRYQjJpQ0REQ05sdXlZd3Zwdi00eHc1Rk0NCm9wdD1jcHN-c3VrDQo\n\n");


    NRF_LOG_RAW_INFO("EdDSA example executed successfully\n");
}


/** @brief Function for the application main entry. */
int main(void)
{
    ret_code_t err_code;

    mp_cmd = NULL;

    err_code = nrf_crypto_init();
    APP_ERROR_CHECK(err_code);

    log_init();
    sqrl_comm_init(on_sqrl_comm_evt);
    comm_uart_init();

    sqrl_client_loop();

    for (;;)
    {
    }
}


/** @}
 */
