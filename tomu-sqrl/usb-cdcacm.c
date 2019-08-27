/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 * Copyright (C) 2015 Piotr Esden-Tempski <piotr@esden.net>
 * Copyright (C) 2018 Seb Holzapfel <schnommus@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/vector.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/efm32/wdog.h>
#include <libopencm3/efm32/gpio.h>
#include <libopencm3/efm32/cmu.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

//#include <cmsis_armcc.h>

//#include "toboot.h"
//TOBOOT_CONFIGURATION(0);

#include "occ_hmac_sha256.h"
#include "sqrl_comm.h"

/* Default AHB (core clock) frequency of Tomu board */
#define AHB_FREQUENCY 14000000

#define LED_GREEN_PORT GPIOA
#define LED_GREEN_PIN  GPIO0
#define LED_RED_PORT   GPIOB
#define LED_RED_PIN    GPIO7

#define VENDOR_ID                 0x1209    /* pid.code */
#define PRODUCT_ID                0x70b1    /* Assigned to Tomu project */
#define DEVICE_VER                0x0101    /* Program version */


static volatile bool        g_usbd_is_connected = false;
static usbd_device*         g_usbd_dev = 0;
static volatile bool        m_tx_done = false;
static volatile sqrl_cmd_t* mp_cmd = NULL;


static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_CDC,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = VENDOR_ID,
    .idProduct = PRODUCT_ID,
    .bcdDevice = DEVICE_VER,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec its
 * optional, but its absence causes a NULL pointer dereference in Linux
 * cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x83,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 16,
    .bInterval = 255,
}};

static const struct usb_endpoint_descriptor data_endp[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x01,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x82,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 1,
}};

static const struct {
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
    .header = {
        .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
        .bcdCDC = 0x0110,
    },
    .call_mgmt = {
        .bFunctionLength =
            sizeof(struct usb_cdc_call_management_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
        .bmCapabilities = 0,
        .bDataInterface = 1,
    },
    .acm = {
        .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_ACM,
        .bmCapabilities = 6,
    },
    .cdc_union = {
        .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_UNION,
        .bControlInterface = 0,
        .bSubordinateInterface0 = 1,
     }
};

static const struct usb_interface_descriptor comm_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
    .iInterface = 0,

    .endpoint = comm_endp,

    .extra = &cdcacm_functional_descriptors,
    .extralen = sizeof(cdcacm_functional_descriptors)
}};

static const struct usb_interface_descriptor data_iface[] = {{
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,

    .endpoint = data_endp,
}};

static const struct usb_interface ifaces[] = {{
    .num_altsetting = 1,
    .altsetting = comm_iface,
}, {
    .num_altsetting = 1,
    .altsetting = data_iface,
}};

static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 2,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0x80,
    .bMaxPower = 0x32,

    .interface = ifaces,
};

static const char *usb_strings[] = {
    "Tomu",
    "CDC-ACM Demo",
    "DEMO",
};

/* This busywait loop is roughly accurate when running at 24 MHz. */
void udelay_busy(uint32_t usecs)
{
    while (usecs --> 0) {
        /* This inner loop is 3 instructions, one of which is a branch.
         * This gives us 4 cycles total.
         * We want to sleep for 1 usec, and there are cycles per usec at 24 MHz.
         * Therefore, loop 6 times, as 6*4=24.
         */
        asm("mov   r1, #6");
        asm("retry:");
        asm("sub r1, #1");
        asm("bne retry");
        asm("nop");
    }
}

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[128];

static enum usbd_request_return_codes cdcacm_control_request(usbd_device *usbd_dev, struct usb_setup_data *req, uint8_t **buf,
        uint16_t *len, void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
    (void)complete;
    (void)buf;
    (void)usbd_dev;

    switch(req->bRequest) {
    case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
        g_usbd_is_connected = req->wValue & 1; /* Check RTS bit */
        if (!g_usbd_is_connected) /* Note: GPIO polarity is inverted */
            gpio_set(LED_GREEN_PORT, LED_GREEN_PIN);
        else
            gpio_clear(LED_GREEN_PORT, LED_GREEN_PIN);
        return USBD_REQ_HANDLED;
        }
    case USB_CDC_REQ_SET_LINE_CODING:
        if(*len < sizeof(struct usb_cdc_line_coding))
            return 0;

        return USBD_REQ_HANDLED;
    }
    return 0;
}

/* Simple callback that echoes whatever is sent */
static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)ep;

    char buf[64];
    int len = usbd_ep_read_packet(usbd_dev, 0x01, buf, sizeof(buf));

    sqrl_comm_handle_input(buf, len);
}


static void cdcacm_data_tx_cb(usbd_device *usbd_dev, uint8_t ep)
{
    (void)usbd_dev;
    (void)ep;

    m_tx_done = true;
}


static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
    (void)wValue;

    usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_rx_cb);
    usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, cdcacm_data_tx_cb);
    usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, 0);

    usbd_register_control_callback(
                usbd_dev,
                USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
                USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                cdcacm_control_request);
}


static void usb_puts(char *s)
{
    if (! g_usbd_is_connected) {
        return;
    }

    usbd_ep_write_packet(g_usbd_dev, 0x82, s, strnlen(s, 64));
    udelay_busy(200); // Why is this needed?

    /* wait for completion */
    while (m_tx_done == false)
    {
    }
}


static void serial_tx(char const * p_buffer, size_t len)
{
    if (! g_usbd_is_connected) {
        return;
    }

    while (len > 0)
    {
        uint8_t len8 = (uint8_t)(len & 0xFF);

        if (len > 64)
        {
            len8 = 64;
        }

        m_tx_done = false;
        usbd_ep_write_packet(g_usbd_dev, 0x82, p_buffer, len8);
        udelay_busy(200); // Why is this needed?

        /* wait for completion */
        while (m_tx_done == false)
        {
        }

        len -= len8;
        p_buffer = &p_buffer[len8];
    }
}

void usb_isr(void)
{
    usbd_poll(g_usbd_dev);
}

void hard_fault_handler(void)
{
    gpio_clear(LED_RED_PORT, LED_RED_PIN);
    gpio_clear(LED_GREEN_PORT, LED_GREEN_PIN);
    while(1);
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
    /* Disable the watchdog that the bootloader started. */
    WDOG_CTRL = 0;

    /* GPIO peripheral clock is necessary for us to set up the GPIO pins as outputs */
    cmu_periph_clock_enable(CMU_GPIO);

    /* Set up both LEDs as outputs */
    gpio_mode_setup(LED_RED_PORT, GPIO_MODE_WIRED_AND, LED_RED_PIN);
    gpio_mode_setup(LED_GREEN_PORT, GPIO_MODE_WIRED_AND, LED_GREEN_PIN);

    gpio_set(LED_GREEN_PORT, LED_GREEN_PIN);
    gpio_set(LED_RED_PORT, LED_RED_PIN);
    gpio_clear(LED_RED_PORT, LED_RED_PIN); // TODO: Just for 'unplug - program' cycle

    /* Configure the USB core & stack */
    g_usbd_dev = usbd_init(&efm32hg_usb_driver, &dev, &config, usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(g_usbd_dev, cdcacm_set_config);

    /* Set the CPU Core to run from the trimmed USB clock, divided by 2.
     * This will give the CPU Core a frequency of 24 MHz +/- 1% */
    CMU_CMD = (5 << 0);
    while (! (CMU_STATUS & (1 << 26)))
        ;

    /* Enable USB IRQs */
    nvic_enable_irq(NVIC_USB_IRQ);

    sqrl_comm_init(on_sqrl_comm_evt);

    // Start out with hard coded test keys
    //uint8_t iuk[] = {0xa2,0x43,0xbf,0xb0,0xed,0x04,0x56,0x5d,0x04,0x61,0xa7,0x73,0x1a,0x8f,0xad,0x18,0xde,0x4b,0xdd,0xf8,0xe3,0x83,0x38,0x53,0x92,0x6c,0xcf,0xaf,0x2e,0x2e,0x04,0xa6};
    uint8_t imk[] = {0x21,0xd7,0x08,0x94,0x57,0x5e,0x6b,0x6e,0xfe,0x99,0x1f,0xb8,0x6a,0x98,0x68,0xa4,0x9f,0x3a,0x72,0x04,0x0e,0x88,0x25,0x2d,0x82,0xbe,0x5a,0x3a,0xc6,0xc3,0xaa,0x23};

    //uint8_t ilk[32];
    //sqrl_get_ilk_from_iuk(ilk, iuk);
    //PRINT_HEX("ilk ", ilk, 32);
    //NRF_LOG_RAW_INFO("ilk  00d3a56b500bca7908eb89a6f5fe0931388797d42930798d2ffe88d436c94878\n\n");

    while(1) {
        asm("nop"); // TODO: Why the fuck is this needed???
        asm ("wfe");
        //__asm ("wfi");
        //__WFE();

        if (! g_usbd_is_connected || mp_cmd == NULL) {
            // TODO: sleep?
            continue;
        }

        gpio_clear(LED_RED_PORT, LED_RED_PIN);
        usb_puts("\x02log\x1e asdf asdf sadf\x03\n");

        //client_response_t resp = {0};

        if (mp_cmd->type == SQRL_CMD_QUERY)
        {
            usb_puts("\x02resp\x1equery\x1e");
            serial_tx(mp_cmd->sks, strlen(mp_cmd->sks));
            usb_puts("\x1e");
            serial_tx(mp_cmd->server, strlen(mp_cmd->server));
            usb_puts("\x1e<... resp.ids ...>");
            usb_puts("\x03\n");

            char sks[] = "www.grc.com";
            uint8_t ssk[32];
            occ_hmac_sha256(ssk, imk, 32, (uint8_t *)sks, strlen(sks));
        }
        else if (mp_cmd->type == SQRL_CMD_IDENT)
        {
            usb_puts("\x02resp\x1eident\x1e");
            serial_tx(mp_cmd->sks, strlen(mp_cmd->sks));
            usb_puts("\x1e");
            serial_tx(mp_cmd->server, strlen(mp_cmd->server));
            usb_puts("\x1e<... resp.ids ...>");
            usb_puts("\x03\n");
        }
        else
        {
            usb_puts("\x02log\x1e err: Invalid command\x03\n");
        }

        //free(resp.client);
        //free(resp.ids);
        mp_cmd = NULL;
        sqrl_comm_command_handled();
        //gpio_set(LED_RED_PORT, LED_RED_PIN); // TODO: Uncommented just for 'unplug - program' cycle
    }
}

