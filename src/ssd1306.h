#ifndef SSD1306_H
#define SSD1306_H

#include "driver/i2c.h"

typedef struct {
    i2c_port_t port;
    int address;
    int width;
    int height;
} SSD1306_t;

void ssd1306_init(SSD1306_t *dev, int width, int height);
void ssd1306_clear_screen(SSD1306_t *dev, bool invert);
void ssd1306_display_text(SSD1306_t *dev, int page, const char *text, int text_len, bool invert);

#endif
