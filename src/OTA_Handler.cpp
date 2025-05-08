#include "OTA_Handler.h"

void (*displayTextCallback)(const char*) = NULL;

void setupOTA(void (*displayCallback)(const char*)) {
  displayTextCallback = displayCallback;
  
  ArduinoOTA.onStart([]() {
    String type;
    if(ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {
      type = "filesystem";
    }
    String message = "Start updating " + type;
    if(displayTextCallback) displayTextCallback(message.c_str());
  });
  
  ArduinoOTA.onEnd([]() {
    if(displayTextCallback) displayTextCallback("End updating");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    String message = "Progress: " + String((progress / (total / 100))) + "%";
    if(displayTextCallback) displayTextCallback(message.c_str());
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    String message;
    switch(error) {
      case OTA_AUTH_ERROR:
        message = "Auth Failed";
        break;
      case OTA_BEGIN_ERROR:
        message = "Begin Failed";
        break;
      case OTA_CONNECT_ERROR:
        message = "Connect Failed";
        break;
      case OTA_RECEIVE_ERROR:
        message = "Receive Failed";
        break;
      case OTA_END_ERROR:
        message = "End Failed";
        break;
      default:
        message = "Error: " + String(error);
        break;
    }
    if(displayTextCallback) displayTextCallback(message.c_str());
  });
  
  ArduinoOTA.begin();
  if(displayTextCallback) displayTextCallback("OTA Ready");
}

void handleOTA() {
  ArduinoOTA.handle();
}
