#include "ssd1306.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include <string.h>

#define TAG "SSD1306"

#define SSD1306_ADDR 0x3C
#define SSD1306_CONTROL_BYTE_CMD_SINGLE 0x80
#define SSD1306_CONTROL_BYTE_DATA_STREAM 0x40

#define SSD1306_CMD_SET_MUX_RATIO 0xA8
#define SSD1306_CMD_DISPLAY_OFFSET 0xD3
#define SSD1306_CMD_DISPLAY_ON 0xAF
#define SSD1306_CMD_DISPLAY_OFF 0xAE
#define SSD1306_CMD_SET_DISPLAY_CLK_DIV 0xD5
#define SSD1306_CMD_SET_PRECHARGE 0xD9
#define SSD1306_CMD_SET_VCOM_DESELECT 0xDB
#define SSD1306_CMD_SET_SEGMENT_REMAP 0xA1
#define SSD1306_CMD_SET_COM_SCAN_MODE 0xC8
#define SSD1306_CMD_SET_COM_PIN_MAP 0xDA
#define SSD1306_CMD_SET_CONTRAST 0x81
#define SSD1306_CMD_DISPLAY_RAM 0xA4
#define SSD1306_CMD_DISPLAY_NORMAL 0xA6
#define SSD1306_CMD_SCROLL_DEACTIVATE 0x2E
#define SSD1306_CMD_SET_COLUMN_RANGE 0x21
#define SSD1306_CMD_SET_PAGE_RANGE 0x22
#define SSD1306_CMD_SET_MEMORY_ADDR_MODE 0x20
#define SSD1306_CMD_SET_CHARGE_PUMP 0x8D

static esp_err_t i2c_write_cmd(SSD1306_t *dev, uint8_t cmd)
{
    uint8_t buffer[2];
    buffer[0] = SSD1306_CONTROL_BYTE_CMD_SINGLE;
    buffer[1] = cmd;
    
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, dev->address << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd_handle, buffer, 2, true);
    i2c_master_stop(cmd_handle);
    esp_err_t ret = i2c_master_cmd_begin(dev->port, cmd_handle, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
    
    return ret;
}

static esp_err_t i2c_write_data(SSD1306_t *dev, const uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, dev->address << 1 | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, SSD1306_CONTROL_BYTE_DATA_STREAM, true);
    i2c_master_write(cmd_handle, data, len, true);
    i2c_master_stop(cmd_handle);
    esp_err_t ret = i2c_master_cmd_begin(dev->port, cmd_handle, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd_handle);
    
    return ret;
}

void ssd1306_init(SSD1306_t *dev, int width, int height)
{
    dev->width = width;
    dev->height = height;
    dev->address = SSD1306_ADDR;
    
    i2c_write_cmd(dev, SSD1306_CMD_DISPLAY_OFF);
    i2c_write_cmd(dev, SSD1306_CMD_SET_MUX_RATIO);
    i2c_write_cmd(dev, height - 1);
    
    i2c_write_cmd(dev, SSD1306_CMD_DISPLAY_OFFSET);
    i2c_write_cmd(dev, 0x00);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_DISPLAY_CLK_DIV);
    i2c_write_cmd(dev, 0x80);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_PRECHARGE);
    i2c_write_cmd(dev, 0xF1);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_VCOM_DESELECT);
    i2c_write_cmd(dev, 0x30);
    
    i2c_write_cmd(dev, SSD1306_CMD_DISPLAY_RAM);
    i2c_write_cmd(dev, SSD1306_CMD_DISPLAY_NORMAL);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_SEGMENT_REMAP);
    i2c_write_cmd(dev, SSD1306_CMD_SET_COM_SCAN_MODE);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_COM_PIN_MAP);
    if (dev->height == 32) {
        i2c_write_cmd(dev, 0x02);
    } else {
        i2c_write_cmd(dev, 0x12);
    }
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_CONTRAST);
    i2c_write_cmd(dev, 0xFF);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_CHARGE_PUMP);
    i2c_write_cmd(dev, 0x14);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_MEMORY_ADDR_MODE);
    i2c_write_cmd(dev, 0x00);
    
    i2c_write_cmd(dev, SSD1306_CMD_SCROLL_DEACTIVATE);
    i2c_write_cmd(dev, SSD1306_CMD_DISPLAY_ON);
    
    ssd1306_clear_screen(dev, false);
}

void ssd1306_clear_screen(SSD1306_t *dev, bool invert)
{
    uint8_t *buf = malloc(dev->width * dev->height / 8);
    if (buf == NULL) {
        ESP_LOGE(TAG, "malloc failed");
        return;
    }
    
    memset(buf, invert ? 0xFF : 0x00, dev->width * dev->height / 8);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_COLUMN_RANGE);
    i2c_write_cmd(dev, 0);
    i2c_write_cmd(dev, dev->width - 1);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_PAGE_RANGE);
    i2c_write_cmd(dev, 0);
    i2c_write_cmd(dev, dev->height/8 - 1);
    
    i2c_write_data(dev, buf, dev->width * dev->height / 8);
    
    free(buf);
}

void ssd1306_display_text(SSD1306_t *dev, int page, const char *text, int text_len, bool invert)
{
    if (page >= dev->height/8) {
        return;
    }
    
    int width = dev->width;
    if (text_len > width) {
        text_len = width;
    }
    
    uint8_t *buffer = malloc(width);
    if (buffer == NULL) {
        ESP_LOGE(TAG, "malloc failed");
        return;
    }
    
    memset(buffer, 0, width);
    
    for (int i = 0; i < text_len; i++) {
        uint8_t c = text[i] & 0x7F;
        if (c < 32 || c > 127) {
            c = 32;
        }
        c -= 32;
        
        // Simple 5x7 font data (first 5 bytes for each character)
        // This is a simplified implementation for brevity
        for (int j = 0; j < 5; j++) {
            if (c == 0) { // space
                buffer[i*6 + j] = 0;
            } else if (c == 1) { // !
                buffer[i*6 + j] = j == 2 ? 0x7E : 0;
            } else {
                // Simplified - just fill with a pattern for visibility
                buffer[i*6 + j] = 0x7F;
            }
        }
        buffer[i*6 + 5] = 0; // Spacing between characters
    }
    
    if (invert) {
        for (int i = 0; i < width; i++) {
            buffer[i] = ~buffer[i];
        }
    }
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_COLUMN_RANGE);
    i2c_write_cmd(dev, 0);
    i2c_write_cmd(dev, width - 1);
    
    i2c_write_cmd(dev, SSD1306_CMD_SET_PAGE_RANGE);
    i2c_write_cmd(dev, page);
    i2c_write_cmd(dev, page);
    
    i2c_write_data(dev, buffer, width);
    
    free(buffer);
}
