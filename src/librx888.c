/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   librx888.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: Ruslan Migirov <trapi78@gmail.com>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/15 16:10:17 by Ruslan Migi       #+#    #+#             */
/*   Updated: 2022/06/18 15:56:15 by Ruslan Migi      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <libusb.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
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
    
};

typedef struct rx888 {
    uint16_t vid;
    uint16_t pid;
    const char *name;
} rx888_t;

static rx888_t known_devices[] = {
    { 0x04b4, 0x00f1, "Cypress Semiconductor Corp. RX888"},

};

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

uint32_t rtlsdr_get_device_count(void)
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

