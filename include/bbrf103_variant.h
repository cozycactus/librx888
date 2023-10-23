#ifndef BBRF103_VARIANT_H
#define BBRF103_VARIANT_H






int bbrf103_init(void *dev);
int bbrf103_exit(void *dev);


int bbrf103_set_hf_attenuation(void *dev, double rf_gain);




#endif // BBRF103_VARIANT_H