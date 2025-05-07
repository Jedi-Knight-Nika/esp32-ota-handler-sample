#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_netif_ip_addr.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "ota_handler.h"
#include "ssd1306.h"
#include "i2c_master.h"

#define RED_PIN     2
#define GREEN_PIN   4
#define YELLOW_PIN  5
#define BLUE_PIN    27
#define SDA_PIN     21
#define SCL_PIN     22

#define WIFI_SSID     "______"
#define WIFI_PASSWORD "______"

SSD1306_t oled;
static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

void init_gpio(void);
void init_wifi(void);
void display_text(const char* text);
void toggle_leds(int pin);
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    init_gpio();
    
    i2c_master_init(&oled, SDA_PIN, SCL_PIN, -1);
    ssd1306_init(&oled, 128, 64);
    ssd1306_clear_screen(&oled, false);
    display_text("Booting...");
    
    init_wifi();
    
    ota_setup(display_text);
    
    gpio_set_level(BLUE_PIN, 1);
    
    while (1) {
        ota_handle();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void init_gpio(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<RED_PIN) | (1ULL<<GREEN_PIN) | (1ULL<<YELLOW_PIN) | (1ULL<<BLUE_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

void toggle_leds(int pin)
{
    gpio_set_level(RED_PIN, 0);
    gpio_set_level(GREEN_PIN, 0);
    gpio_set_level(YELLOW_PIN, 0);
    
    switch(pin) {
        case 1:
            gpio_set_level(RED_PIN, 1);
            break;
        case 2:
            gpio_set_level(GREEN_PIN, 1);
            break;
        case 3:
            gpio_set_level(YELLOW_PIN, 1);
            break;
        default:
            break;
    }
}

void display_text(const char* text)
{
    ssd1306_clear_screen(&oled, false);
    ssd1306_display_text(&oled, 0, text, strlen(text), false);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        display_text("WiFi disconnected");
        toggle_leds(1);
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
 } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        char ip_str[20];
        snprintf(ip_str, sizeof(ip_str), "IP: %d.%d.%d.%d", 
          esp_ip4_addr1(&event->ip_info.ip),
          esp_ip4_addr2(&event->ip_info.ip),
          esp_ip4_addr3(&event->ip_info.ip),
          esp_ip4_addr4(&event->ip_info.ip));
        display_text(ip_str);
        toggle_leds(2);
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}

void init_wifi(void)
{
    wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}
