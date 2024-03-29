/*
 * rx888, a SDR receiver driver for the RX888 hardware, based on rtl-sdr project
 * Copyright (C) 2022-2023 by Ruslan Migirov <trapi78@gmail.com>
 *
 * Based on rtl-sdr, turns your Realtek RTL2832 based DVB dongle into a SDR receiver
 * Copyright (C) 2012-2014 by Steve Markgraf <steve@steve-m.de>
 * Copyright (C) 2012 by Dimitri Stolnikov <horiz0n@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define _POSIX_C_SOURCE 199309L
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <time.h>
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#include <stdbool.h>

#include <libusb.h>
#include "librx888.h"

enum rx888_async_status {
    RX888_INACTIVE = 0,
    RX888_CANCELING,
    RX888_RUNNING
};

enum rx888_command {
    STARTFX3 = 0xAA,
    STARTADC = 0xB2,
    STOPFX3 = 0xAB,
    R820T2STDBY = 0xB8,
    GPIOFX3 = 0xAD
};

// Bitmasks for GPIO pins
enum GPIOPin {
    ATT_LE = 1U << 0,
    ATT_CLK = 1U << 1,
    ATT_DATA = 1U << 2,
    SEL0 = 1U << 3,
    SEL1 = 1U << 4,
    SHDWN = 1U << 5,
    DITH = 1U << 6,
    RAND = 1U << 7,
    BIAS_HF = 1U << 8,
    BIAS_VHF = 1U << 9,
    LED_YELLOW = 1U << 10,
    LED_RED = 1U << 11,
    LED_BLUE = 1U << 12,
    ATT_SEL0 = 1U << 13,
    ATT_SEL1 = 1U << 14,
    
    // RX888r2
    VHF_EN = 1U << 15,
    PGA_EN = 1U << 16,
};


struct rx888_dev {
    libusb_context *ctx;
    struct libusb_device_handle *dev_handle;
    uint32_t xfer_buf_num;
    uint32_t xfer_buf_len;
    struct libusb_transfer **xfer;
    unsigned char **xfer_buf;
    rx888_read_async_cb_t cb;
    void *cb_ctx;
    enum rx888_async_status async_status;
    int async_cancel;
    uint32_t sample_rate;
    /* status */
    int dev_lost;
    int driver_active;
    unsigned int xfer_errors;
    uint32_t gpio_state;
};

typedef struct rx888 {
    uint16_t vid;
    uint16_t pid;
    const char *name;
} rx888_t;

static rx888_t known_devices[] = {
    { 0x04b4, 0x00f1, "Cypress Semiconductor Corp. RX888"},

};

#define DEFAULT_BUF_NUMBER	16
#define DEFAULT_BUF_LENGTH  (1024 * 16 * 8)
#define CTRL_TIMEOUT 0

static int rx888_send_command(struct libusb_device_handle *dev_handle,
                                 enum rx888_command cmd,uint32_t data)
{
  /* Send the control message. */
  int ret = libusb_control_transfer(
      dev_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT, cmd, 0, 0,
      (unsigned char *)&data, sizeof(data), CTRL_TIMEOUT);

  if (ret < 0) {
    fprintf(stderr, "Could not send command: 0x%X with data: %d. Error : %s.\n",
            cmd, data, libusb_error_name(ret));
    return -1;
  }

  return 0;
}

int rx888_set_hf_attenuation(rx888_dev_t *dev, double rf_gain)
{
    if (!dev)
        return -1;

    // Verify that rf_gain is one of the expected values
    if (rf_gain != 0.0 && rf_gain != -10.0 && rf_gain != -20.0)
        return -1;

    // Set the HF attenuation
    if (rf_gain == 0.0) {
        dev->gpio_state &= ~ATT_SEL0; // Clear the bit 13
        dev->gpio_state |= ATT_SEL1; // Set the bit 14
        } 
    else if (rf_gain == -10.0) {
        dev->gpio_state |= ATT_SEL0; // Set the bit 13
        dev->gpio_state |= ATT_SEL1; // Set the bit 14
        }
    else if (rf_gain == -20.0) {
        dev->gpio_state |= ATT_SEL0; // Set the bit 13
        dev->gpio_state &= ~ATT_SEL1; // Clear the bit 14
        }

    rx888_send_command(dev->dev_handle, GPIOFX3, dev->gpio_state);

    return 0;
}

int rx888_set_sample_rate(rx888_dev_t *dev, uint32_t samp_rate)
{
    if (!dev)
        return -1;

    /* check if the rate is supported by the device */
    if ((samp_rate <= 10000) || (samp_rate > 150000000)) {
        fprintf(stderr, "Invalid sample rate: %u Hz\n", samp_rate);
        return -EINVAL;
    }

    dev->sample_rate = samp_rate;

    rx888_send_command(dev->dev_handle, STARTADC, samp_rate);

    return 0;
}

uint32_t rx888_get_sample_rate(rx888_dev_t *dev)
{
    if (!dev)
        return 0;

    return dev->sample_rate;
}

int rx888_get_usb_strings(rx888_dev_t *dev, char *manufact, char *product,
                char *serial)
{
    if (!dev || !dev->dev_handle)
        return -1;

    libusb_device *device = libusb_get_device(dev->dev_handle);

    struct libusb_device_descriptor dd;
    int r = libusb_get_device_descriptor(device, &dd);
    if (r < 0)
        return -1;

    const int buf_max = 256;
    if (manufact) {
        memset(manufact, 0, buf_max);
        libusb_get_string_descriptor_ascii(dev->dev_handle, dd.iManufacturer,
                           (unsigned char *)manufact,
                           buf_max);
    }

    if (product) {
        memset(product, 0, buf_max);
        libusb_get_string_descriptor_ascii(dev->dev_handle, dd.iProduct,
                           (unsigned char *)product,
                           buf_max);
    }

    if (serial) {
        memset(serial, 0, buf_max);
        libusb_get_string_descriptor_ascii(dev->dev_handle, dd.iSerialNumber,
                           (unsigned char *)serial,
                           buf_max);
    }

    return 0;
}

static rx888_t *find_known_device(uint16_t vid, uint16_t pid)
{
    unsigned int i;
    rx888_t *device = NULL;

    for (i = 0; i < sizeof(known_devices)/sizeof(rx888_t); i++ ) {
        if (known_devices[i].vid == vid && known_devices[i].pid == pid) {
            device = &known_devices[i];
            break;
        }
    }

    return device;
}

uint32_t rx888_get_device_count(void)
{
    libusb_context *ctx;
    int r = libusb_init(&ctx);
    if(r < 0)
        return 0;

    libusb_device **list;
    ssize_t cnt = libusb_get_device_list(ctx, &list);

    uint32_t device_count = 0;
    struct libusb_device_descriptor dd;
    for (int i = 0; i < cnt; i++) {
        libusb_get_device_descriptor(list[i], &dd);

        if (find_known_device(dd.idVendor, dd.idProduct))
            device_count++;
    }

    libusb_free_device_list(list, 1);

    libusb_exit(ctx);

    return device_count;
}

const char *rx888_get_device_name(uint32_t index)
{
    libusb_context *ctx;
    int r = libusb_init(&ctx);
    if(r < 0)
        return "";

    libusb_device **list;
    ssize_t cnt = libusb_get_device_list(ctx, &list);

    struct libusb_device_descriptor dd;
    uint32_t device_count = 0;
    rx888_t *device = NULL;
    for (int i = 0; i < cnt; i++) {
        libusb_get_device_descriptor(list[i], &dd);

        device = find_known_device(dd.idVendor, dd.idProduct);

        if (device) {
            device_count++;

            if (index == device_count - 1)
                break;
        }
    }

    libusb_free_device_list(list, 1);

    libusb_exit(ctx);

    if (device)
        return device->name;
    else
        return "";
}

int rx888_get_device_usb_strings(uint32_t index, char *manufact,
                   char *product, char *serial)
{
    libusb_context *ctx;
    int r = libusb_init(&ctx);
    if(r < 0)
        return r;

    libusb_device **list;
    ssize_t cnt = libusb_get_device_list(ctx, &list);

    uint32_t device_count = 0;
    rx888_dev_t devt;
    rx888_t *device = NULL;
    struct libusb_device_descriptor dd;
    for (int i = 0; i < cnt; i++) {
        libusb_get_device_descriptor(list[i], &dd);

        device = find_known_device(dd.idVendor, dd.idProduct);

        if (device) {
            device_count++;

            if (index == device_count - 1) {
                r = libusb_open(list[i], &devt.dev_handle);
                if (!r) {
                    r = rx888_get_usb_strings(&devt,
                                   manufact,
                                   product,
                                   serial);
                    libusb_close(devt.dev_handle);
                }
                break;
            }
        }
    }

    libusb_free_device_list(list, 1);

    libusb_exit(ctx);

    return r;
}

int rx888_get_index_by_serial(const char *serial)
{
    if (!serial)
        return -1;

    int cnt = rx888_get_device_count();

    if (!cnt)
        return -2;
    
    int r;
    char str[256];
    for (int i = 0; i < cnt; i++) {
        r = rx888_get_device_usb_strings(i, NULL, NULL, str);
        if (!r && !strcmp(serial, str))
            return i;
    }

    return -3;
}

int rx888_open(rx888_dev_t **out_dev, uint32_t index)
{
    rx888_dev_t *dev = calloc(1, sizeof(rx888_dev_t));
    if (!dev)
        return -ENOMEM;

    int r = libusb_init(&dev->ctx);
    if(r < 0) {
        free(dev);
        return -1;
    }

    libusb_device **list;
    ssize_t num_of_devs = libusb_get_device_list(dev->ctx, &list);

    libusb_device *device = NULL;
    struct libusb_device_descriptor dev_desc;
    uint32_t device_count = 0;
    for (int i =0; i < num_of_devs; i++) {
        device = list[i];

        libusb_get_device_descriptor(list[i], &dev_desc);

        if (find_known_device(dev_desc.idVendor, dev_desc.idProduct)) {
            device_count++;
        }

        if (index == device_count - 1)
            break;
        
        device = NULL;
    }

    if(!device) {
        r = -1;
        goto err;
    }

    r = libusb_open(device, &dev->dev_handle);
    if (r < 0) {
        libusb_free_device_list(list, 1);
        fprintf(stderr, "usb_open error %d\n", r);
        if(r == LIBUSB_ERROR_ACCESS)
            fprintf(stderr, "Please fix the device permissions.\n");
        goto err;
    }

    libusb_free_device_list(list, 1);

    if (libusb_kernel_driver_active(dev->dev_handle, 0) == 1) {
        dev->driver_active = 1;

#ifdef DETACH_KERNEL_DRIVER
        if (!libusb_detach_kernel_driver(dev->dev_handle, 0)) {
            fprintf(stderr, "Detached kernel driver\n");
        } else {
            fprintf(stderr, "Detaching kernel driver failed!");
            goto err;
        }
#else
        fprintf(stderr, "\nKernel driver is active, or device is "
                "claimed by second instance of librx888."
                "\nIn the first case, please either detach"
                " or blacklist the kernel module\n"
                " or enable automatic"
                " detaching at compile time.\n\n");
#endif
    }

    r = libusb_claim_interface(dev->dev_handle, 0);
    if (r < 0) {
        fprintf(stderr, "usb_claim_interface error %d\n", r);
        goto err;
    }

    dev->dev_lost = false;

    dev->gpio_state = BIAS_HF;
    *out_dev = dev;
    rx888_send_command(dev->dev_handle, R820T2STDBY, 0);
    rx888_send_command(dev->dev_handle, STOPFX3, 0);
    rx888_send_command(dev->dev_handle, STARTADC, dev->sample_rate);
    rx888_send_command(dev->dev_handle, STARTFX3, 0);
    return 0;
err:
    if (dev) {
        if (dev->dev_handle)
            libusb_close(dev->dev_handle);

        if (dev->ctx)
            libusb_exit(dev->ctx);

        free(dev);
    }

    return r;
    
}

int rx888_close(rx888_dev_t *dev)
{
    if (!dev)
        return -1;

    if(!dev->dev_lost) {
        /* block until all async operations have been completed (if any) */
        while (RX888_INACTIVE != dev->async_status) {
#ifdef _WIN32
            Sleep(1);
#else
            nanosleep((const struct timespec[]){{0, 1000000L}}, NULL);
#endif
        }
        rx888_send_command(dev->dev_handle, STOPFX3, 0);


    }

    libusb_release_interface(dev->dev_handle, 0);

#ifdef DETACH_KERNEL_DRIVER
    if (dev->driver_active) {
        if (!libusb_attach_kernel_driver(dev->dev_handle, 0))
            fprintf(stderr, "Reattached kernel driver\n");
        else
            fprintf(stderr, "Reattaching kernel driver failed!\n");
    }
#endif

    libusb_close(dev->dev_handle);

    libusb_exit(dev->ctx);

    free(dev);

    return 0;
}

int rx888_read_sync(rx888_dev_t *dev, void *buf, int len, int *n_read)
{
    if (!dev)
        return -1;

    return libusb_bulk_transfer(dev->dev_handle, 0x81,
             buf, len, n_read, 0);
}

static void LIBUSB_CALL _libusb_callback(struct libusb_transfer *xfer)
{
    rx888_dev_t *dev = (rx888_dev_t *)xfer->user_data;

    if (LIBUSB_TRANSFER_COMPLETED == xfer->status) {
        if (dev->cb)
            dev->cb(xfer->buffer, xfer->actual_length, dev->cb_ctx);

        libusb_submit_transfer(xfer); /* resubmit transfer */
        dev->xfer_errors = 0;
    } else if (LIBUSB_TRANSFER_CANCELLED != xfer->status) {
#ifndef _WIN32
        if (LIBUSB_TRANSFER_ERROR == xfer->status)
            dev->xfer_errors++;

        if (dev->xfer_errors >= dev->xfer_buf_num ||
            LIBUSB_TRANSFER_NO_DEVICE == xfer->status) {
#endif
            dev->dev_lost = true;
            rx888_cancel_async(dev);
            fprintf(stderr, "cb transfer status: %d, "
                "canceling...\n", xfer->status);
#ifndef _WIN32
        }
#endif
    }
}

static int _rx888_alloc_async_buffers(rx888_dev_t *dev)
{
    unsigned int i;

    if (!dev)
        return -1;

    if (!dev->xfer) {
        dev->xfer = malloc(dev->xfer_buf_num *
                   sizeof(struct libusb_transfer *));

        for(i = 0; i < dev->xfer_buf_num; ++i)
            dev->xfer[i] = libusb_alloc_transfer(0);
    }

    if (dev->xfer_buf)
        return -2;

    dev->xfer_buf = calloc(dev->xfer_buf_num, sizeof(unsigned char *));
    
    for (i = 0; i < dev->xfer_buf_num; ++i) {
        dev->xfer_buf[i] = malloc(dev->xfer_buf_len);
        if (!dev->xfer_buf[i])
            return -ENOMEM;
    }
    

    return 0;
}

static int _rx888_free_async_buffers(rx888_dev_t *dev)
{
    unsigned int i;

    if (!dev)
        return -1;

    if (dev->xfer) {
        for(i = 0; i < dev->xfer_buf_num; ++i) {
            if (dev->xfer[i]) {
                libusb_free_transfer(dev->xfer[i]);
            }
        }

        free(dev->xfer);
        dev->xfer = NULL;
    }

    if (dev->xfer_buf) {
        for (i = 0; i < dev->xfer_buf_num; ++i) {
            if (dev->xfer_buf[i]) {
                free(dev->xfer_buf[i]);
            }
        }

        free(dev->xfer_buf);
        dev->xfer_buf = NULL;
    }

    return 0;
}


int rx888_read_async(rx888_dev_t *dev, rx888_read_async_cb_t cb, void *ctx,
              uint32_t buf_num, uint32_t buf_len)
{
    int r = 0;
    struct timeval tv = { 1, 0 };
    struct timeval zerotv = { 0, 0 };
    enum rx888_async_status next_status = RX888_INACTIVE;

    if (!dev)
        return -1;

    if (RX888_INACTIVE != dev->async_status)
        return -2;

    dev->async_status = RX888_RUNNING;
    dev->async_cancel = false;

    dev->cb = cb;
    dev->cb_ctx = ctx;

    if (buf_num > 0)
        dev->xfer_buf_num = buf_num;
    else
        dev->xfer_buf_num = DEFAULT_BUF_NUMBER;

    if (buf_len > 0 && buf_len % 512 == 0) /* len must be multiple of 512 */
        dev->xfer_buf_len = buf_len;
    else
        dev->xfer_buf_len = DEFAULT_BUF_LENGTH;

    _rx888_alloc_async_buffers(dev);

    for(uint32_t i = 0; i < dev->xfer_buf_num; ++i) {
        libusb_fill_bulk_transfer(dev->xfer[i],
                      dev->dev_handle,
                      0x81,
                      dev->xfer_buf[i],
                      dev->xfer_buf_len,
                      _libusb_callback,
                      (void *)dev,
                      0);

        r = libusb_submit_transfer(dev->xfer[i]);
        if (r < 0) {
            fprintf(stderr, "Failed to submit transfer %i\n"
                    "Please increase your allowed " 
                    "usbfs buffer size with the "
                    "following command:\n"
                    "echo 0 > /sys/module/usbcore"
                    "/parameters/usbfs_memory_mb\n", i);
            dev->async_status = RX888_CANCELING;
            break;
        }
    }
    
    //rx888_send_command(dev->dev_handle, STARTADC, dev->sample_rate);
    //rx888_send_command(dev->dev_handle, STARTFX3, 0);

    while (RX888_INACTIVE != dev->async_status) {
        r = libusb_handle_events_timeout_completed(dev->ctx, &tv,
                               &dev->async_cancel);
        if (r < 0) {
            /*fprintf(stderr, "handle_events returned: %d\n", r);*/
            if (r == LIBUSB_ERROR_INTERRUPTED) /* stray signal */
                continue;
            break;
        }

        if (RX888_CANCELING == dev->async_status) {
            next_status = RX888_INACTIVE;

            if (!dev->xfer)
                break;

            for(uint32_t i = 0; i < dev->xfer_buf_num; ++i) {
                if (!dev->xfer[i])
                    continue;

                if (LIBUSB_TRANSFER_CANCELLED !=
                        dev->xfer[i]->status) {
                    r = libusb_cancel_transfer(dev->xfer[i]);
                    /* handle events after canceling
                     * to allow transfer status to
                     * propagate */
#ifdef _WIN32
                    Sleep(1);
#endif
                    libusb_handle_events_timeout_completed(dev->ctx,
                                           &zerotv, NULL);
                    if (r < 0)
                        continue;

                    next_status = RX888_CANCELING;
                }
            }

            if (dev->dev_lost || RX888_INACTIVE == next_status) {
                /* handle any events that still need to
                 * be handled before exiting after we
                 * just cancelled all transfers */
                libusb_handle_events_timeout_completed(dev->ctx,
                                       &zerotv, NULL);
                break;
            }
        }
    }

    _rx888_free_async_buffers(dev);

    dev->async_status = next_status;

    return r;
}

int rx888_cancel_async(rx888_dev_t *dev)
{
    if (!dev)
        return -1;

    /* if streaming, try to cancel gracefully */
    if (RX888_RUNNING == dev->async_status) {
        dev->async_status = RX888_CANCELING;
        dev->async_cancel = true;
        //rx888_send_command(dev->dev_handle, STOPFX3, 0);
        return 0;
    }

    /* if called while in pending state, change the state forcefully */
#if 0
    if (RX888_INACTIVE != dev->async_status) {
        dev->async_status = RX888_INACTIVE;
        return 0;
    }
#endif
    return -2;
}
