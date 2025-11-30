#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Pin definitions
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST -1

class OLEDDisplay {
private:
  Adafruit_SSD1306 display;
  bool displayAvailable;
  unsigned long lastUpdate;
  unsigned long updateInterval;
  int displayState;
  unsigned long stateChangeTime;
  
  void displayWiFiInfo(const String& ssid, const String& ip, bool isAPMode);
  void displaySensorData(float temp, float hum, const float soilMoisture[], int numSensors);
  void displayPlantStatus(const float soilMoisture[], int numSensors);
  void displayIPOnly(const String& ip);

public:
  OLEDDisplay();
  bool begin();
  void updateDisplay(const String& ssid, const String& ip, bool isAPMode, 
                    float temp, float hum, const float soilMoisture[], int numSensors);
  void clearDisplay();
  bool isAvailable() const { return displayAvailable; }
};

#endif