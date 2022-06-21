/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   librx888.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: Ruslan Migirov <trapi78@gmail.com>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/15 16:09:44 by Ruslan Migi       #+#    #+#             */
/*   Updated: 2022/06/21 10:38:15 by Ruslan Migi      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef C7B7DE77_F735_4B88_A492_1ED83430C335
#define C7B7DE77_F735_4B88_A492_1ED83430C335

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct rx888_dev rx888_dev_t;

typedef void(*rx888_read_async_cb_t)(unsigned char *buf, uint32_t len, void *ctx);

int rx888_open(rx888_dev_t **dev, uint32_t index);

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
