/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   librx888.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: Ruslan Migirov <trapi78@gmail.com>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/15 16:10:17 by Ruslan Migi       #+#    #+#             */
/*   Updated: 2022/06/15 16:59:09 by Ruslan Migi      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <libusb.h>
#include "librx888.h"

struct rx888_dev {
    libusb_context *ctx;
    struct libusb_device_handle *dev_handle;
    struct libusb_transfer **transfer;
    rx888_read_async_cb_t cb;
};