#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <ArduinoOTA.h>

void setupOTA(void (*displayCallback)(const char*));
void handleOTA();

#endif
