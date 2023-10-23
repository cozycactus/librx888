#ifndef HF103_VARIANT_H
#define HF103_VARIANT_H




int hf103_init(void *dev);
int hf103_exit(void *dev);

int hf103_set_hf_attenuation(void *dev, double rf_gain);








#endif // HF103_VARIANT_H
