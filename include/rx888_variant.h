


#ifndef RX888_VARIANT_H
#define RX888_VARIANT_H



int rx888_init(void *dev);
int rx888_exit(void *dev);
int rx888_set_hf_attenuation(void *dev, double rf_gain);

#endif // RX888_VARIANT_H