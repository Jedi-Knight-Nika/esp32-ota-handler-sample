#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

typedef void (*display_callback_t)(const char* text);

void ota_setup(display_callback_t display_func);
void ota_handle(void);

#endif
