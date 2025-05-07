#include "i2c_master.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_MASTER_FREQ_HZ 50000
#define I2C_TIMEOUT_MS 1000

void i2c_master_init(SSD1306_t *dev, int sda_pin, int scl_pin, int reset_pin)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda_pin;
    conf.scl_io_num = scl_pin;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;
    
    dev->port = I2C_NUM_0;
    dev->address = 0x3C;
    
    i2c_param_config(dev->port, &conf);
    i2c_driver_install(dev->port, conf.mode, 0, 0, 0);
    
    if (reset_pin >= 0) {
        esp_rom_gpio_pad_select_gpio(reset_pin);
        gpio_set_direction(reset_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(reset_pin, 0);
        vTaskDelay(50 / portTICK_PERIOD_MS);
        gpio_set_level(reset_pin, 1);
    }
}
