#include "OLEDDisplay.h"
#include <WiFi.h>

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

OLEDDisplay::OLEDDisplay() 
  : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST),
    displayAvailable(false),
    lastUpdate(0),
    updateInterval(3000), // Change display every 3 seconds
    displayState(0),
    stateChangeTime(0) {
}

bool OLEDDisplay::begin() {
  Serial.println("Initializing OLED display...");
  Serial.print("SDA Pin: "); Serial.println(OLED_SDA);
  Serial.print("SCL Pin: "); Serial.println(OLED_SCL);
  
  // Initialize I2C with defined pins
  Wire.begin(OLED_SDA, OLED_SCL);
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("❌ SSD1306 allocation failed");
    Serial.println("Please check:");
    Serial.println("1. OLED wiring (SDA, SCL, VCC, GND)");
    Serial.println("2. I2C address (usually 0x3C)");
    Serial.println("3. I2C pins configuration");
    displayAvailable = false;
    return false;
  }
  
  displayAvailable = true;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  // Show startup message
  display.setCursor(0, 0);
  display.println("Plant Monitor");
  display.println("Starting...");
  display.display();
  
  Serial.println("✅ OLED display initialized successfully");
  return true;
}

void OLEDDisplay::updateDisplay(const String& ssid, const String& ip, bool isAPMode, 
                               float temp, float hum, const float soilMoisture[], int numSensors) {
  if (!displayAvailable) return;
  
  unsigned long currentTime = millis();
  
  // Check if it's time to update display
  if (currentTime - lastUpdate < 100) return; // Update at most every 100ms
  lastUpdate = currentTime;
  
  // Check if it's time to change display state
  if (currentTime - stateChangeTime > updateInterval) {
    displayState = (displayState + 1) % 4; // Cycle through 4 states
    stateChangeTime = currentTime;
  }
  
  display.clearDisplay();
  
  switch (displayState) {
    case 0:
      displayWiFiInfo(ssid, ip, isAPMode);
      break;
    case 1:
      displaySensorData(temp, hum, soilMoisture, numSensors);
      break;
    case 2:
      displayPlantStatus(soilMoisture, numSensors);
      break;
    case 3:
      displayIPOnly(ip);
      break;
  }
  
  display.display();
}

void OLEDDisplay::displayWiFiInfo(const String& ssid, const String& ip, bool isAPMode) {
  display.setCursor(0, 0);
  display.setTextSize(1);
  
  if (isAPMode) {
    display.println("MODE: ACCESS POINT");
    display.println("SSID: " + ssid);
    display.println("IP: " + ip);
    display.println("Connect to setup WiFi");
  } else {
    display.println("MODE: STATION");
    display.println("Connected to:");
    display.println(ssid);
    display.println("IP: " + ip);
  }
  
  display.println();
  display.println("Web: http://" + ip);
}

void OLEDDisplay::displaySensorData(float temp, float hum, const float soilMoisture[], int numSensors) {
  display.setCursor(0, 0);
  display.setTextSize(1);
  
  display.println("ENVIRONMENT DATA");
  display.println("---------------");
  
  display.print("Temp: ");
  display.print(temp, 1);
  display.println(" C");
  
  display.print("Hum:  ");
  display.print(hum, 1);
  display.println(" %");
  
  display.println("---------------");
  display.println("SOIL MOISTURE:");
  
  for (int i = 0; i < numSensors && i < 3; i++) {
    display.print("P");
    display.print(i + 1);
    display.print(": ");
    display.print(soilMoisture[i], 0);
    display.println(" %");
  }
}

void OLEDDisplay::displayPlantStatus(const float soilMoisture[], int numSensors) {
  display.setCursor(0, 0);
  display.setTextSize(1);
  
  display.println("PLANT STATUS");
  display.println("---------------");
  
  for (int i = 0; i < numSensors && i < 3; i++) {
    display.print("Plant ");
    display.print(i + 1);
    display.print(": ");
    
    if (soilMoisture[i] < 30) {
      display.println("DRY");
    } else if (soilMoisture[i] < 60) {
      display.println("GOOD");
    } else {
      display.println("WET");
    }
  }
  
  display.println("---------------");
  display.println("STATUS GUIDE:");
  display.println("DRY  < 30%");
  display.println("GOOD 30-60%");
  display.println("WET  > 60%");
}

void OLEDDisplay::displayIPOnly(const String& ip) {
  display.setCursor(0, 0);
  display.setTextSize(1);
  
  display.println("PLANT MONITOR");
  display.println("Ready");
  display.println();
  display.println("IP Address:");
  display.setTextSize(2);
  display.setCursor(0, 30);
  display.println(ip);
}

void OLEDDisplay::clearDisplay() {
  if (!displayAvailable) return;
  display.clearDisplay();
  display.display();
}