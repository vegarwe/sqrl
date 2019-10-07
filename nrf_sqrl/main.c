#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nrf_assert.h"
#include "nrf_drv_uart.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "sqrl_client.h"
#include "sqrl_comm.h"
#include "sqrl_crypto.h"
#include "sqrl_s4.h"


static nrf_drv_uart_t       m_uart = NRF_DRV_UART_INSTANCE(0);
static volatile bool        m_xfer_done;
static uint8_t              m_rx_buffer[1];
static volatile sqrl_cmd_t* mp_cmd;


// Start out with hard coded test keys
static uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
static uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};

// Hard coded sqrl binary
static char sqrlbinary[] = "sqrldata}\x00\x01\x00-\x00\"wQ\x12""2\x0e\xb5\x89""1\xfep\x97\xef\xf2""e]\xf6\x0fg\a\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x02""3\x88\xcd\xa0\xd7WN\xf7\x8a\xd1""9\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb0""8\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1f""F\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5""e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$\"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01""e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcb""C\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce";


static void uart_evt_handler(nrf_drv_uart_event_t * p_event, void * p_context)
{
    if (p_event->type == NRF_DRV_UART_EVT_RX_DONE)
    {
        (void)nrf_drv_uart_rx(&m_uart, m_rx_buffer, 1);

        sqrl_comm_handle_input((char*)&p_event->data.rxtx.p_data[0], 1);
    }
    else if (p_event->type == NRF_DRV_UART_EVT_ERROR)
    {
        // p_event->data.error.error_mask;
        (void)nrf_drv_uart_rx(&m_uart, m_rx_buffer, 1);
    }
    else if (p_event->type == NRF_DRV_UART_EVT_TX_DONE)
    {
        m_xfer_done = true;
    }
}


/* Initialize transport */
static void comm_uart_init(void)
{
    nrf_drv_uart_config_t config = NRF_DRV_UART_DEFAULT_CONFIG;
    config.pseltxd  = 6;
    config.pselrxd  = 8;
    config.pselcts  = NRF_UART_PSEL_DISCONNECTED;
    config.pselrts  = NRF_UART_PSEL_DISCONNECTED;
    config.baudrate = NRF_UART_BAUDRATE_115200;
    ret_code_t err_code = nrf_drv_uart_init(&m_uart, &config, uart_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_uart_rx(&m_uart, m_rx_buffer, 1);
    APP_ERROR_CHECK(err_code);
}


/* Write data back to PC */
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

/** Initilize logging module. */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}



static void on_sqrl_comm_evt(sqrl_comm_evt_t * p_evt)
{
    if (p_evt->evt_type == SQRL_COMM_EVT_COMMAND)
    {
        mp_cmd = p_evt->evt.p_cmd;
    }
}


static void sqrl_client_loop(void)
{
    char outputbuffer[2048];
    uint8_t ilk[32];
    sqrl_get_ilk_from_iuk(ilk, iuk);

    sprintf(outputbuffer, "\n\nlog: sqrl_client_loop\n");
    serial_tx(outputbuffer, strlen(outputbuffer));
    NRF_LOG_INFO("sqrl_client_loop");

    while (true)
    {

        if (mp_cmd == NULL) {
            // TODO: sleep // TODO: Add sleep (wfe_ or wfi_)
            continue;
        }

        NRF_LOG_INFO("sqrl_client_loop: command received");

        client_response_t resp = {0};
        memset(&resp, 0, sizeof(resp));

        if (mp_cmd->type == SQRL_CMD_QUERY)
        {
            sqrl_query(&resp, mp_cmd, imk);

            sprintf(outputbuffer, "\x02log\x1e cmd:    '%s'\x03\n", "query"); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "\x02log\x1e sks:    '%s'\x03\n", mp_cmd->params.sqrl_cmd.sks); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "\x02log\x1e client: '%s'\x03\n", resp.client); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "\x02log\x1e server: '%s'\x03\n", mp_cmd->params.sqrl_cmd.server); serial_tx(outputbuffer, strlen(outputbuffer));
            sprintf(outputbuffer, "\x02log\x1e ids:    '%s'\x03\n", resp.ids);    serial_tx(outputbuffer, strlen(outputbuffer));

            sprintf(outputbuffer, "\x02resp\x1equery\x1e%s\x1e%s\x1e%s\x03\n", resp.client, mp_cmd->params.sqrl_cmd.server, resp.ids);
            serial_tx(outputbuffer, strlen(outputbuffer));
        }
        else if (mp_cmd->type == SQRL_CMD_IDENT)
        {
            // TODO: Create random 32 bytes
            uint8_t rlk[] = {0xca,0x5a,0x7b,0x6e,0xa8,0xbc,0x75,0xb3,0x94,0xd1,0xdf,0x20,0xbc,0xd9,0xcf,0x4d,0x31,0x1d,0xb0,0x67,0xd8,0x77,0xd9,0xb6,0xa7,0xda,0x74,0xd6,0x1b,0x6a,0x8d,0x69};
            sqrl_ident(&resp, mp_cmd, ilk, imk, rlk);

            sprintf(outputbuffer, "\x02resp\x1eident\x1e%s\x1e%s\x1e%s\x03\n", resp.client, mp_cmd->params.sqrl_cmd.server, resp.ids);
            serial_tx(outputbuffer, strlen(outputbuffer));
        }
        else if (mp_cmd->type == SQRL_CMD_UNLOCK)
        {
            // Output from sqrl_s4.py:get_imk_ilk_from_password
            uint8_t key[] = {0x44,0xb6,0xb3,0xea,0x66,0x01,0x59,0x82,0xe5,0x5f,0xe8,0xf0,0xea,0x5c,0x11,0xe0,0x10,0x67,0x61,0x59,0xfe,0x02,0xed,0x70,0xd6,0xfb,0xf6,0x87,0x6b,0x49,0x77,0x4d};
            uint8_t tmpimk[32];
            uint8_t tmpilk[32];

            sqrl_s4_identity_t identity;
            int res = sqrl_s4_decode((uint8_t*)sqrlbinary, &identity);
            if (res != 0)
            {
                sprintf(outputbuffer, "\x02idresp\x1eunlock\x1e""failed\x1eUnable to parse identity\x03\n");
                serial_tx(outputbuffer, strlen(outputbuffer));
                continue;
            }

            if (! get_imk_ilk_from_scryptpassword(&identity, key, tmpimk, tmpilk))
            {
                sprintf(outputbuffer, "\x02idresp\x1eunlock\x1e""failed\x1eIdentity decryption failed\x03\n");
                serial_tx(outputbuffer, strlen(outputbuffer));
                continue;
            }

            sprintf(outputbuffer, "\x02idresp\x1eunlock\x1esuccess\x1epass\x03\n");
            serial_tx(outputbuffer, strlen(outputbuffer));
        }
        else
        {
            sprintf(outputbuffer, "\x02log\x1e err: Invalid command\x03\n");
            serial_tx(outputbuffer, strlen(outputbuffer));
        }

        free(resp.client);
        free(resp.ids);
        memset(&resp, 0, sizeof(resp));
        mp_cmd = NULL;
        sqrl_comm_command_handled();
    }
}


/** @brief Function for the application main entry. */
int main(void)
{
    mp_cmd = NULL;

    sqrl_crypto_init();

    log_init();
    sqrl_comm_init(on_sqrl_comm_evt);
    comm_uart_init();

    sqrl_client_loop();

    for (;;)
    {
    }
}

