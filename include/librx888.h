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
 
#ifndef C7B7DE77_F735_4B88_A492_1ED83430C335
#define C7B7DE77_F735_4B88_A492_1ED83430C335

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct rx888_dev rx888_dev_t;

/*!
 * Get the number of available devices.
 *
 * \return the number of available devices
 */
uint32_t rx888_get_device_count(void);

/*!
 * Get the name of a device.
 *
 * \param index the device index
 * \return the device name
 */
const char* rx888_get_device_name(uint32_t index);

/*!
 * Get USB device strings.
 *
 * NOTE: The string arguments must provide space for up to 256 bytes.
 *
 * \param index the device index
 * \param manufact manufacturer name, may be NULL
 * \param product product name, may be NULL
 * \param serial serial number, may be NULL
 * \return 0 on success
 */
int rx888_get_device_usb_strings(uint32_t index,
                         char *manufact,
                         char *product,
                         char *serial);


/*!
 * Get device index by USB serial string descriptor.
 *
 * \param serial serial string of the device
 * \return device index of first device where the name matched
 * \return -1 if name is NULL
 * \return -2 if no devices were found at all
 * \return -3 if devices were found, but none with matching name
 */
int rx888_get_index_by_serial(const char *serial);

/*!
 * Open a device by index.
 *
 * \param dev pointer to the device handle to be created
 * \param index the device index
 * \return 0 on success
 */
int rx888_open(rx888_dev_t **dev, uint32_t index);

/*!
 * Close a device.
 *
 * \param dev the device handle given by rx888_open()
 * \return 0 on success
 */
int rx888_close(rx888_dev_t *dev);

enum rx888_device {
    RX888_DEVICE = 0
};

enum rx888_variant {
    RX888_VARIANT_UNKNOWN = 0x00,
    RX888_VARIANT_BBRF103 = 0x01,
    RX888_VARIANT_HF103 = 0x02,
    RX888_VARIANT_RX888 = 0x03,
    RX888_VARIANT_RX888MK2 = 0x04,
    RX888_VARIANT_RX999 = 0x05,
    RX888_VARIANT_RXLUCY = 0x06,
    RX888_VARIANT_RX888MK3 = 0x07,
};

int rx888_set_hf_attenuation(void *dev, double rf_gain);

/*!
 * Set the sample rate for the device.
 *
 * \param dev the device handle given by rx888_open()
 * \param samp_rate the sample rate to be set, possible values are:
 * 		    10000 - 150000000 Hz
 * 		    sample loss is to be expected for rates > 150000000
 * \return 0 on success, -EINVAL on invalid rate
 */
int rx888_set_sample_rate(rx888_dev_t *dev, uint32_t rate);

/*!
 * Get actual sample rate the device is configured to.
 *
 * \param dev the device handle given by rx888_open()
 * \return 0 on error, sample rate in Hz otherwise
 */
uint32_t rx888_get_sample_rate(rx888_dev_t *dev);

typedef void(*rx888_read_async_cb_t)(unsigned char *buf, uint32_t len, void *ctx);

/*!
 * Read samples from the device asynchronously. This function will block until
 * it is being canceled using rx888_cancel_async()
 *
 * \param dev the device handle given by rx888_open()
 * \param cb callback function to return received samples
 * \param ctx user specific context to pass via the callback function
 * \param buf_num optional buffer count, buf_num * buf_len = overall buffer size
 *		  set to 0 for default buffer count (15)
 * \param buf_len optional buffer length, must be multiple of 512,
 *		  should be a multiple of 16384 (URB size), set to 0
 *		  for default buffer length (16 * 32 * 512)
 * \return 0 on success
 */
int rx888_read_async(rx888_dev_t *dev,
                 rx888_read_async_cb_t cb,
                 void *ctx,
                 uint32_t buf_num,
                 uint32_t buf_len);

/*!
 * Cancel all pending asynchronous operations on the device.
 *
 * \param dev the device handle given by rx888_open()
 * \return 0 on success
 */
int rx888_cancel_async(rx888_dev_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* C7B7DE77_F735_4B88_A492_1ED83430C335 */
