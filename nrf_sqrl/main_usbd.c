#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_queue.h"

#include "app_timer.h"
#include "fds.h"
#include "app_error.h"
#include "app_util.h"

#include "boards.h"

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

#include "sqrl_comm.h"

/* If enabled then CYCCNT (high resolution) timestamp is used for the logger. */
#define USE_CYCCNT_TIMESTAMP_FOR_LOG 0

#if NRF_LOG_BACKEND_FLASHLOG_ENABLED
NRF_LOG_BACKEND_FLASHLOG_DEF(m_flash_log_backend);
#endif

#if NRF_LOG_BACKEND_CRASHLOG_ENABLED
NRF_LOG_BACKEND_CRASHLOG_DEF(m_crash_log_backend);
#endif

static sqrl_cmd_t* mp_cmd;

/* Counter timer. */
APP_TIMER_DEF(m_timer_0);


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


NRF_QUEUE_DEF(uint8_t,
              m_rx_queue,
              2*NRF_DRV_USBD_EPSIZE,
              NRF_QUEUE_MODE_OVERFLOW);

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
        if (!nrf_queue_is_empty(&m_rx_queue))
        {
            //mp_internal->p_cb->handler(NRF_CLI_TRANSPORT_EVT_RX_RDY, mp_internal->p_cb->p_context);
        }
        ret_code_t ret = app_usbd_cdc_acm_read_any(&m_cdc_acm,
                                                   m_rx_buffer,
                                                   sizeof(m_rx_buffer));
        if (ret == NRF_SUCCESS)
        {
            size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
            size_t qsize = nrf_queue_in(&m_rx_queue, m_rx_buffer, size);
            ASSERT(size == qsize);
            UNUSED_VARIABLE(qsize);
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
 * */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);


    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            /*Setup first transfer*/
            cdc_acm_process_and_prepare_buffer(p_cdc_acm);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            break;
        case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
            //mp_internal->p_cb->handler(NRF_CLI_TRANSPORT_EVT_TX_RDY, mp_internal->p_cb->p_context);
            break;
        case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
            /*Get amount of data transfered*/
            size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
            size_t qsize = nrf_queue_in(&m_rx_queue, m_rx_buffer, size);
            ASSERT(size == qsize);
            UNUSED_VARIABLE(qsize);

            sqrl_comm_handle_input(m_rx_buffer, size);

            /*Setup next transfer*/
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




static void timer_handle(void * p_context)
{
    UNUSED_PARAMETER(p_context);
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

    ret = app_timer_init();
    APP_ERROR_CHECK(ret);

    ret = app_timer_create(&m_timer_0, APP_TIMER_MODE_REPEATED, timer_handle);
    APP_ERROR_CHECK(ret);

    ret = app_timer_start(m_timer_0, APP_TIMER_TICKS(1000), NULL);
    APP_ERROR_CHECK(ret);

    sqrl_comm_init(on_sqrl_comm_evt);
    usbd_init();

    ret = fds_init();
    APP_ERROR_CHECK(ret);


    UNUSED_RETURN_VALUE(nrf_log_config_load());

    flashlog_init();

    stack_guard_init();

    while (true)
    {
        UNUSED_RETURN_VALUE(NRF_LOG_PROCESS());
#if APP_USBD_CONFIG_EVENT_QUEUE_ENABLE
        while (app_usbd_event_queue_process())
        {
            /* Nothing to do */
        }
#endif

        char fisken[] = "\x02log\x1e Hello my name is doctor green thumb\x03\n";
        size_t cnt;
        //if (!nrf_queue_is_empty(&m_rx_queue))
        //{
        //    cdc_acm_write(fjaser, sizeof(fjaser) - 1, &cnt);
        //}
        //else
        //{
        //    cdc_acm_write(fisken, sizeof(fisken) - 1, &cnt);
        //}
        //nrf_delay_ms(500);

        if (mp_cmd != NULL) {
            cdc_acm_write(fisken, sizeof(fisken) - 1, &cnt);

            mp_cmd = NULL;
            sqrl_comm_command_handled();
        }
    }
}

