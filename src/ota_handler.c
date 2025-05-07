#include "ota_handler.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include <string.h>

#define OTA_PORT 3232
#define BUFFSIZE 1024

static display_callback_t display_callback = NULL;
static TaskHandle_t ota_task_handle = NULL;
static const char *TAG = "OTA";

static void ota_server_task(void *pvParameter);

void ota_setup(display_callback_t display_func)
{
    display_callback = display_func;
    
    xTaskCreate(&ota_server_task, "ota_server", 4096, NULL, 5, &ota_task_handle);
    
    if (display_callback) {
        display_callback("OTA Ready");
    }
}

void ota_handle(void)
{
}

static void ota_server_task(void *pvParameter)
{
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(OTA_PORT);
    
    bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(listen_sock, 1);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
        
        ESP_LOGI(TAG, "OTA request received");
        if (display_callback) {
            display_callback("OTA request received");
        }
        
        esp_ota_handle_t ota_handle;
        esp_err_t err;
        const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
        
        err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
            close(sock);
            continue;
        }
        
        char buf[BUFFSIZE];
        bool flag = true;
        
        while (flag) {
            int len = recv(sock, buf, BUFFSIZE, 0);
            if (len < 0) {
                ESP_LOGE(TAG, "Error: recv failed");
                break;
            }
            else if (len == 0) {
                ESP_LOGI(TAG, "Connection closed");
                flag = false;
                break;
            }
            
            err = esp_ota_write(ota_handle, (const void *)buf, len);
            if (err != ESP_OK) {
                flag = false;
                break;
            }
        }
        
        if (flag) {
            err = esp_ota_end(ota_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
            } else {
                err = esp_ota_set_boot_partition(update_partition);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
                } else {
                    if (display_callback) {
                        display_callback("OTA success! Rebooting...");
                    }
                    ESP_LOGI(TAG, "OTA success! Rebooting...");
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    esp_restart();
                }
            }
        } else {
            esp_ota_end(ota_handle);
            if (display_callback) {
                display_callback("OTA failed");
            }
            ESP_LOGE(TAG, "OTA failed");
        }
        
        close(sock);
    }
    
    close(listen_sock);
    vTaskDelete(NULL);
}
