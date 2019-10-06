#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "app_timer.h"
#include "app_error.h"
#include "app_util.h"
#include "fds.h"

#include "boards.h"

#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_queue.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_log_backend_flash.h"
#include "nrf_fstorage_nvmc.h"

#include "nrf_mpu_lib.h"
#include "nrf_stack_guard.h"

#include "nrf_drv_usbd.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"

#include "sqrl_client.h"
#include "sqrl_comm.h"
#include "sqrl_crypto.h"
#include "sqrl_s4.h"

/* If enabled then CYCCNT (high resolution) timestamp is used for the logger. */
#define USE_CYCCNT_TIMESTAMP_FOR_LOG 0

#if NRF_LOG_BACKEND_FLASHLOG_ENABLED
NRF_LOG_BACKEND_FLASHLOG_DEF(m_flash_log_backend);
#endif

#if NRF_LOG_BACKEND_CRASHLOG_ENABLED
NRF_LOG_BACKEND_CRASHLOG_DEF(m_crash_log_backend);
#endif

static volatile sqrl_cmd_t* mp_cmd;

// Start out with hard coded test keys
static uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
static uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};

// Hard coded sqrl binary
static char sqrlbinary[] = "sqrldata}\x00\x01\x00-\x00\"wQ\x12""2\x0e\xb5\x89""1\xfep\x97\xef\xf2""e]\xf6\x0fg\a\x8c_\xda\xd4\xe0Z\xe0\xb8\t\x96\x00\x00\x00\xf3\x01\x04\x05\x0f\x00\x02""3\x88\xcd\xa0\xd7WN\xf7\x8a\xd1""9\xf8\x1c]\x13\x87\x06\xc6\xe8\xf8\xb0""8\xf6\x14\xd9m\x9e\xf6|\x94\xa4\x1f""F\xab}\x0e\xd3\xbf\xa3r\xa3^\xb4\xfb\xcc\xe7\x8cQ\x8d\x8dyRl\x05\xf1\x19|\x90\x03\x06\t\xe0\xb3\x85H\x8c\xe0\xa6\x0fQm\xf6\x94q6-\xee\xe0\xe9I\x00\x02\x00\xea\xde\x04q\xa1\xfaO\x8f\x1c\xf5""e\xea\xb3)-^\t\xa5\x00\x00\x00\xf9o$\"\x9e\x91\xa6\xa9k\xde\xe2z^&j\xa6\x15\xb5\x04\xf4P\x01""e\xcc\xfa\xa8V\xd7\xf4\x94L\xea\xea\xdd><\xcb""C\xc5+\xeb\xaf\x18\x88\xf9\xa6\xd4\xce";


/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif


static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            app_usbd_start();
            break;
        default:
            break;
    }
}


#define SQRL_CDC_ACM_COMM_INTERFACE 0
#define SQRL_CDC_ACM_DATA_INTERFACE 1
#define SQRL_CDC_ACM_COMM_EPIN NRF_DRV_USBD_EPIN2
#define SQRL_CDC_ACM_DATA_EPIN NRF_DRV_USBD_EPIN1
#define SQRL_CDC_ACM_DATA_EPOUT NRF_DRV_USBD_EPOUT1


static char m_rx_buffer[NRF_DRV_USBD_EPSIZE];
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

/**
 * @brief CDC_ACM class instance.
 * */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_cdc_acm,
                            cdc_acm_user_ev_handler,
                            SQRL_CDC_ACM_COMM_INTERFACE,
                            SQRL_CDC_ACM_DATA_INTERFACE,
                            SQRL_CDC_ACM_COMM_EPIN,
                            SQRL_CDC_ACM_DATA_EPIN,
                            SQRL_CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250
);

/**
 * @brief Set new buffer and process any data if already present
 *
 * This is internal function.
 * The result of its execution is the library waiting for the event of the new data.
 * If there is already any data that was returned from the CDC internal buffer
 * it would be processed here.
 */
static void cdc_acm_process_and_prepare_buffer(app_usbd_cdc_acm_t const * p_cdc_acm)
{
    for (;;)
    {
        ret_code_t ret = app_usbd_cdc_acm_read_any(&m_cdc_acm, m_rx_buffer, sizeof(m_rx_buffer));

        if (ret == NRF_SUCCESS)
        {
            /* Get amount of data transfered */
            size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);

            sqrl_comm_handle_input(m_rx_buffer, size);
        }
        else if (ret == NRF_ERROR_IO_PENDING)
        {
            break;
        }
        else
        {
            APP_ERROR_CHECK(ret);
            break;
        }
    }
}


/**
 * @brief User event handler.
 */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);


    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
            /* Setup first transfer */
            cdc_acm_process_and_prepare_buffer(p_cdc_acm);
            break;
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            break;
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            break;
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            /* Get amount of data transfered, send to comm module */
            size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);

            sqrl_comm_handle_input(m_rx_buffer, size);

            /* Setup next transfer */
            cdc_acm_process_and_prepare_buffer(p_cdc_acm);
            break;
        }
        default:
            break;
    }
}


static ret_code_t cdc_acm_write(void const* p_data, size_t length, size_t* p_cnt)
{
    ASSERT(p_cnt);
    ret_code_t ret;

    ret = app_usbd_cdc_acm_write(&m_cdc_acm, p_data, length);
    if (ret == NRF_SUCCESS || ret == NRF_ERROR_INVALID_STATE)
    {
        *p_cnt = length;
        ret = NRF_SUCCESS;
    }
    else if (ret == NRF_ERROR_BUSY)
    {
        *p_cnt = 0;
        ret = NRF_SUCCESS;
    }
    else
    {
        /* Nothing to do */
    }

    return ret;
}


static void usbd_init(void)
{
    ret_code_t ret;
    static const app_usbd_config_t usbd_config = {
        .ev_handler = app_usbd_event_execute,
        .ev_state_proc = usbd_user_ev_handler
    };
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    app_usbd_class_inst_t const * class_cdc_acm =
            app_usbd_cdc_acm_class_inst_get(&m_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }

    /* Give some time for the host to enumerate and connect to the USB CDC port */
    nrf_delay_ms(1000);
}


static void flashlog_init(void)
{
#if NRF_MODULE_ENABLED(NRF_LOG_BACKEND_FLASH)
    ret_code_t ret;
    int32_t backend_id;

    ret = nrf_log_backend_flash_init(&nrf_fstorage_nvmc);
    APP_ERROR_CHECK(ret);
#if NRF_LOG_BACKEND_FLASHLOG_ENABLED
    backend_id = nrf_log_backend_add(&m_flash_log_backend, NRF_LOG_SEVERITY_WARNING);
    APP_ERROR_CHECK_BOOL(backend_id >= 0);

    nrf_log_backend_enable(&m_flash_log_backend);
#endif

#if NRF_LOG_BACKEND_CRASHLOG_ENABLED
    backend_id = nrf_log_backend_add(&m_crash_log_backend, NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK_BOOL(backend_id >= 0);

    nrf_log_backend_enable(&m_crash_log_backend);
#endif
#endif
}

static inline void stack_guard_init(void)
{
    APP_ERROR_CHECK(nrf_mpu_lib_init());
    APP_ERROR_CHECK(nrf_stack_guard_init());
}

uint32_t cyccnt_get(void)
{
    return DWT->CYCCNT;
}


static void on_sqrl_comm_evt(sqrl_comm_evt_t * p_evt)
{
    if (p_evt->evt_type == SQRL_COMM_EVT_COMMAND)
    {
        mp_cmd = p_evt->evt.p_cmd;
    }
}


int main(void)
{
    ret_code_t ret;

    if (USE_CYCCNT_TIMESTAMP_FOR_LOG)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        DWT->CYCCNT = 0;
        APP_ERROR_CHECK(NRF_LOG_INIT(cyccnt_get, 64000000));
    }
    else
    {
        APP_ERROR_CHECK(NRF_LOG_INIT(app_timer_cnt_get));
    }

    ret = nrf_drv_clock_init();
    APP_ERROR_CHECK(ret);
    nrf_drv_clock_lfclk_request(NULL);

    sqrl_comm_init(on_sqrl_comm_evt);
    usbd_init();

    ret = fds_init();
    APP_ERROR_CHECK(ret);


    UNUSED_RETURN_VALUE(nrf_log_config_load());

    flashlog_init();

    stack_guard_init();


    uint8_t ilk[32];
    sqrl_get_ilk_from_iuk(ilk, iuk);

    (void)iuk;
    (void)imk;
    (void)ilk;

    char outputbuffer[2048];
    //sprintf(outputbuffer, "\n\nlog: sqrl_client_loop\n");
    //serial_tx(outputbuffer, strlen(outputbuffer));
    //cdc_acm_write(outputbuffer, strlen(outputbuffer), &cnt);

    while (true)
    {
        UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
#if APP_USBD_CONFIG_EVENT_QUEUE_ENABLE
        while (app_usbd_event_queue_process())
        {
            /* Nothing to do */
        }
#endif

        if (mp_cmd == NULL) {
            // TODO: sleep
            continue;
        }

        client_response_t resp = {0};
        memset(&resp, 0, sizeof(resp));

        size_t cnt;

        if (mp_cmd->type == SQRL_CMD_QUERY)
        {
            sqrl_query(&resp, mp_cmd, imk);

            //sprintf(outputbuffer, "\x02log\x1e cmd:    '%s'\x03\n", "query"); serial_tx(outputbuffer, strlen(outputbuffer));
            //sprintf(outputbuffer, "\x02log\x1e sks:    '%s'\x03\n", mp_cmd->params.sqrl_cmd.sks); serial_tx(outputbuffer, strlen(outputbuffer));
            //sprintf(outputbuffer, "\x02log\x1e client: '%s'\x03\n", resp.client); serial_tx(outputbuffer, strlen(outputbuffer));
            //sprintf(outputbuffer, "\x02log\x1e server: '%s'\x03\n", mp_cmd->params.sqrl_cmd.server); serial_tx(outputbuffer, strlen(outputbuffer));
            //sprintf(outputbuffer, "\x02log\x1e ids:    '%s'\x03\n", resp.ids);    serial_tx(outputbuffer, strlen(outputbuffer));

            sprintf(outputbuffer, "\x02resp\x1equery\x1e%s\x1e%s\x1e%s\x03\n", resp.client, mp_cmd->params.sqrl_cmd.server, resp.ids);
            cdc_acm_write(outputbuffer, strlen(outputbuffer), &cnt);
        }
        else if (mp_cmd->type == SQRL_CMD_IDENT)
        {
            // TODO: Create random 32 bytes
            uint8_t rlk[] = {0xca,0x5a,0x7b,0x6e,0xa8,0xbc,0x75,0xb3,0x94,0xd1,0xdf,0x20,0xbc,0xd9,0xcf,0x4d,0x31,0x1d,0xb0,0x67,0xd8,0x77,0xd9,0xb6,0xa7,0xda,0x74,0xd6,0x1b,0x6a,0x8d,0x69};
            sqrl_ident(&resp, mp_cmd, ilk, imk, rlk);

            sprintf(outputbuffer, "\x02resp\x1eident\x1e%s\x1e%s\x1e%s\x03\n", resp.client, mp_cmd->params.sqrl_cmd.server, resp.ids);
            cdc_acm_write(outputbuffer, strlen(outputbuffer), &cnt);
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
                cdc_acm_write(outputbuffer, strlen(outputbuffer), &cnt);
                continue;
            }

            if (! get_imk_ilk_from_scryptpassword(&identity, key, tmpimk, tmpilk))
            {
                sprintf(outputbuffer, "\x02idresp\x1eunlock\x1e""failed\x1eIdentity decryption failed\x03\n");
                cdc_acm_write(outputbuffer, strlen(outputbuffer), &cnt);
                continue;
            }

            sprintf(outputbuffer, "\x02idresp\x1eunlock\x1esuccess\x1epass\x03\n");
            cdc_acm_write(outputbuffer, strlen(outputbuffer), &cnt);
        }
        else
        {
            sprintf(outputbuffer, "\x02log\x1e err: Invalid command\x03\n");
            cdc_acm_write(outputbuffer, strlen(outputbuffer), &cnt);
        }

        free(resp.client);
        free(resp.ids);
        memset(&resp, 0, sizeof(resp));
        mp_cmd = NULL;
        sqrl_comm_command_handled();
    }
}

