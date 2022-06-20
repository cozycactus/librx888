/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   librx888.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: Ruslan Migirov <trapi78@gmail.com>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/15 16:10:17 by Ruslan Migi       #+#    #+#             */
/*   Updated: 2022/06/20 20:26:52 by Ruslan Migi      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <libusb.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "librx888.h"

enum rx888_async_status {
	RX888_INACTIVE = 0,
	RX888_CANCELING,
	RX888_RUNNING
};

struct rx888_dev {
    libusb_context *ctx;
    struct libusb_device_handle *dev_handle;
    struct libusb_transfer **transfer;
    rx888_read_async_cb_t cb;
    uint32_t sample_rate;
    
};

typedef struct rx888 {
    uint16_t vid;
    uint16_t pid;
    const char *name;
} rx888_t;

static rx888_t known_devices[] = {
    { 0x04b4, 0x00f1, "Cypress Semiconductor Corp. RX888"},

};

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
    if (dev == NULL)
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

    *out_dev = dev;
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

