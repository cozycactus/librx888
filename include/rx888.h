#ifndef RX888_H
#define RX888_H

#include "librx888.h"
#include <libusb.h>


enum rx888_command {
    STARTFX3 = 0xAA,
    STOPFX3 = 0xAB,
    TESTFX3 = 0xAC,
    GPIOFX3 = 0xAD,
    STARTADC = 0xB2,
    R820T2STDBY = 0xB8
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

enum rx888_async_status {
    RX888_INACTIVE = 0,
    RX888_CANCELING,
    RX888_RUNNING
};

typedef struct rx888_variant_iface {
    /* variant interface */
    int (*init)(void *);
    int (*exit)(void *);
    int (*set_hf_attenuation)(void *, double rf_gain);
} rx888_variant_iface_t;

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
    /* variant context */
    rx888_variant_iface_t *variant;
    /* status */
    int dev_lost;
    int driver_active;
    unsigned int xfer_errors;
    uint32_t gpio_state;
};

int rx888_send_command(struct libusb_device_handle *dev_handle,
                                 enum rx888_command cmd,uint32_t data);

#endif // RX888_H
