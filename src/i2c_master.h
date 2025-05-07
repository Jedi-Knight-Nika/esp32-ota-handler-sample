#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include "ssd1306.h"

void i2c_master_init(SSD1306_t *dev, int sda_pin, int scl_pin, int reset_pin);

#endif
