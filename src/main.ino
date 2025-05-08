#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "OTA_Handler.h"

const char* ssid = "________";
const char* password = "________";

const int redPin = 2;
const int greenPin = 4;
const int yellowPin = 5;
const int sdaPin = 21;
const int sclPin = 22;
const int bluePin = 27;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void displayText(const char* text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print(text);
  display.display();
}

void toggleLEDs(int pin) {
  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(yellowPin, LOW);
  switch(pin) {
    case 1:
      digitalWrite(redPin, HIGH);
      break;
    case 2:
      digitalWrite(greenPin, HIGH);
      break;
    case 3:
      digitalWrite(yellowPin, HIGH);
      break;
    default:
      break;
  }
}

void initOLED() {
  Wire.begin(sdaPin, sclPin, 50000);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while(WiFi.waitForConnectResult() != WL_CONNECTED) {
    displayText("connection failed! rebooting...");
    toggleLEDs(1); 
    delay(5000);
    ESP.restart();
  }
  
  char ipStr[20];
  sprintf(ipStr, "IP: %s", WiFi.localIP().toString().c_str());
  displayText(ipStr);
  delay(2000);
  
  toggleLEDs(2); 
}

void setup() {
  Serial.begin(9600);
  
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(yellowPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  toggleLEDs(3);
  
  initOLED();
  delay(1000);
  displayText("Booting...");
  
  initWiFi(); 
  
  setupOTA(displayText);
  
  digitalWrite(bluePin, HIGH);
}

void loop() {
  handleOTA();
}
