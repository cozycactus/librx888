/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   librx888.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: Ruslan Migirov <trapi78@gmail.com>         +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/06/15 16:09:44 by Ruslan Migi       #+#    #+#             */
/*   Updated: 2022/06/15 16:58:09 by Ruslan Migi      ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef C7B7DE77_F735_4B88_A492_1ED83430C335
#define C7B7DE77_F735_4B88_A492_1ED83430C335

#include <stdint.h>

typedef struct rx888_dev rx888_dev_t;

typedef void(*rx888_read_async_cb_t)(unsigned char *buf, uint32_t len, void *ctx);


#endif /* C7B7DE77_F735_4B88_A492_1ED83430C335 */
