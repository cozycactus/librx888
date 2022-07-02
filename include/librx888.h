#ifndef C7B7DE77_F735_4B88_A492_1ED83430C335
#define C7B7DE77_F735_4B88_A492_1ED83430C335

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct rx888_dev rx888_dev_t;

uint32_t rx888_get_device_count(void);

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

int rx888_open(rx888_dev_t **dev, uint32_t index);

int rx888_close(rx888_dev_t *dev);

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

int rx888_read_sync(rx888_dev_t *dev, void *buf, int len, int *n_read);

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
