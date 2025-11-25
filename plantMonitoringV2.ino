// Smart Plant Monitor - Complete ESP32 Code using WiFiServer only
#include <WiFi.h>
#include <Preferences.h>
#include <DHT.h>
#include <LittleFS.h> // Changed from SPIFFS

// --------- CONFIG ----------
static const uint8_t DHTPIN = 4;        // DHT data pin
static const uint8_t DHTTYPE = DHT11;   // DHT11 type
static const uint8_t SOIL_PIN = 34;     // ADC pin for soil sensor
// ---------------------------

// Global objects
static DHT dht(DHTPIN, DHTTYPE);
static WiFiServer server(80);
static Preferences prefs;

// Sensor cached variables
static float _temp = NAN;
static float _hum = NAN;
static int _soilRaw = 0;
static float _soilPct = NAN;

// Wi-Fi config
static const char* AP_SSID = "SmartPlantSetup";
static const char* AP_PASS = "smartplant";
static unsigned long lastAttempt = 0;
static bool isInAPMode = false;

// ========== SENSORS FUNCTIONS ==========
void sensorsBegin() {
  dht.begin();
  analogReadResolution(12);   // 0-4095
  sensorsUpdate();
}

void sensorsUpdate() {
  // Read DHT with better error handling
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Check if readings failed and retry
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000);
    h = dht.readHumidity();
    t = dht.readTemperature();

    // If still failed, set to 0 instead of NaN
    if (isnan(h)) _hum = 0;
    else _hum = h;

    if (isnan(t)) _temp = 0;
    else _temp = t;
  } else {
    _hum = h;
    _temp = t;
  }

  // Soil sensor reading with validation
  _soilRaw = analogRead(SOIL_PIN);

  // Map to percentage (inverted - higher value = drier)
  const int dryValue = 4095;  // Value when dry (in air)
  const int wetValue = 1500;  // Value when wet (in water)

  int clamped = _soilRaw;
  if (clamped > dryValue) clamped = dryValue;
  if (clamped < wetValue) clamped = wetValue;

  _soilPct = 100.0f * (1.0f - float(clamped - wetValue) / float(dryValue - wetValue));
  if (_soilPct < 0) _soilPct = 0;
  if (_soilPct > 100) _soilPct = 100;
}

float getTemperature() {
  return _temp;
}
float getHumidity() {
  return _hum;
}
int getSoilRaw() {
  return _soilRaw;
}
float getSoilPercent() {
  return _soilPct;
}

// ========== WIFI MANAGER FUNCTIONS ==========
void wifiBegin() {
  // Try to read saved credentials and connect
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.length() > 0) {
    Serial.println("Found saved Wi-Fi credentials, attempting to connect...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    // Wait for connection with timeout
    unsigned long start = millis();
    while (millis() - start < 30000) {  // Wait 30 seconds
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected to Wi-Fi: " + ssid);
        Serial.println("Device IP: " + WiFi.localIP().toString());
        isInAPMode = false;
        return;  // Successfully connected
      }
      Serial.print(".");
      delay(500);
    }
    
    // If we get here, connection failed after 30 seconds
    Serial.println("Failed to connect to saved Wi-Fi after 30 seconds");
  } else {
    Serial.println("No saved Wi-Fi credentials found");
  }

  // If connection failed or no credentials, start AP mode immediately
  Serial.println("Starting Access Point for configuration...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.println("AP Started: " + String(AP_SSID));
  Serial.println("AP IP: " + WiFi.softAPIP().toString());
  isInAPMode = true;
}

void wifiHandle() {
  // Check if we lost Wi-Fi connection and need to switch to AP mode
  if (!isInAPMode && WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastAttempt > 15000) {
      lastAttempt = now;
      Serial.println("Wi-Fi disconnected. Attempting reconnection...");

      prefs.begin("wifi", true);
      String ssid = prefs.getString("ssid", "");
      String pass = prefs.getString("pass", "");
      prefs.end();

      if (ssid.length() > 0) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), pass.c_str());
        unsigned long start = millis();
        while (millis() - start < 5000) {
          if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Reconnected to: " + ssid);
            return;
          }
          delay(500);
        }
        Serial.println("Reconnect failed. Switching to AP mode...");
      }
      
      // If reconnection failed or no saved credentials, switch to AP mode
      Serial.println("Starting Access Point mode...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID, AP_PASS);
      isInAPMode = true;
      Serial.println("AP Started: " + String(AP_SSID));
      Serial.println("AP IP: " + WiFi.softAPIP().toString());
    }
  }
}

String getWifiMode() {
  if (isInAPMode) return "AP";
  if (WiFi.status() == WL_CONNECTED) return "STA";
  return "CONNECTING";
}

String getWifiSSID() {
  if (isInAPMode) return AP_SSID;
  return WiFi.SSID();
}

String getWifiIP() {
  if (isInAPMode) return WiFi.softAPIP().toString();
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return "0.0.0.0";
}

// ========== FILE SERVING ==========
String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".json")) return "application/json";
  else if (filename.endsWith(".png")) return "image/png";
  else if (filename.endsWith(".jpg")) return "image/jpeg";
  return "text/plain";
}

void serveFile(WiFiClient& client, String path) {
  // Check using LittleFS.exists instead of SPIFFS.exists
  if (!LittleFS.exists(path)) { 
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-type:text/plain");
    client.println();
    client.println("404 Not Found");
    return;
  }

  // Open file using LittleFS.open instead of SPIFFS.open
  File file = LittleFS.open(path, "r"); 
  if (!file) {
    client.println("HTTP/1.1 500 Internal Server Error");
    client.println("Content-type:text/plain");
    client.println();
    client.println("Failed to open file");
    return;
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:" + getContentType(path));
  client.println();

  while (file.available()) {
    client.write(file.read());
  }
  file.close();
}

// ========== WEB SERVER HANDLERS ==========
void handleClientRequest(WiFiClient& client, String method, String url, String body) {
  Serial.println(" " + method + " " + url);

  // Handle POST /savewifi
  if (method == "POST" && url == "/api/savewifi") {
    Serial.println("POST Body: " + body);

    // Parse JSON body
    String ssid = "";
    String pass = "";

    int ssidIndex = body.indexOf("\"ssid\":\"");
    if (ssidIndex >= 0) {
      int ssidStart = ssidIndex + 8;
      int ssidEnd = body.indexOf("\"", ssidStart);
      ssid = body.substring(ssidStart, ssidEnd);
    }

    int passIndex = body.indexOf("\"password\":\"");
    if (passIndex >= 0) {
      int passStart = passIndex + 12;
      int passEnd = body.indexOf("\"", passStart);
      pass = body.substring(passStart, passEnd);
    }

    Serial.println("SSID: " + ssid + ", Pass: " + pass);

    if (ssid.length() == 0) {
      client.println("HTTP/1.1 400 Bad Request");
      client.println("Content-type:application/json");
      client.println();
      client.println("{\"success\":false,\"message\":\"SSID required\"}");
      return;
    }

    // Save credentials
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();

    // Success response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:application/json");
    client.println();
    client.println("{\"success\":true,\"message\":\"WiFi credentials saved\"}");

    // Restart Wi-Fi after a delay
    delay(1000);
    wifiBegin();
    return;
  }

  // Handle GET /api/plants - Get all plants data
  if (method == "GET" && url == "/api/plants") {
    sensorsUpdate();
    
    // Return data for 3 plants (you can modify based on number of sensors)
    String payload = "[";
    payload += "{\"id\":1,\"name\":\"Monstera Deliciosa\",\"temperature\":" + String(getTemperature(), 1);
    payload += ",\"humidity\":" + String(getHumidity(), 1);
    payload += ",\"soilMoisture\":" + String(getSoilPercent(), 1);
    payload += ",\"status\":\"";
    
    // Determine status based on soil moisture
    float soil = getSoilPercent();
    if (soil < 30) payload += "critical";
    else if (soil < 60) payload += "warning";
    else payload += "healthy";
    
    payload += "\"}]";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.println(payload);
    return;
  }

  // Handle GET /api/status - Get WiFi status
  if (method == "GET" && url == "/api/status") {
    String payload = "{\"wifiConfigured\":";
    payload += isInAPMode ? "false" : "true";
    payload += ",\"ssid\":\"" + getWifiSSID() + "\"";
    payload += ",\"ip\":\"" + getWifiIP() + "\"}";
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:application/json");
    client.println("Access-Control-Allow-Origin: *");
    client.println();
    client.println(payload);
    return;
  }

  // Handle static files
  if (method == "GET") {
    if (url == "/" || url == "/ ") {
      serveFile(client, "/index.html");
    } else if (url == "/esp32-styles.css") {
      serveFile(client, "/esp32-styles.css");
    } else if (url == "/esp32-script.js") {
      serveFile(client, "/esp32-script.js");
    } else {
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-type:text/plain");
      client.println();
      client.println("404 Not Found");
    }
  }
}

// ========== WEB SERVER HANDLING ==========
void webserverBegin() {
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void webserverHandle() {
  WiFiClient client = server.available();

  if (client) {
    String header = "";
    String currentLine = "";
    String method = "";
    String url = "";
    bool isPost = false;
    int contentLength = 0;

    // Read request headers
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Parse method and URL from first line
            int firstSpace = header.indexOf(' ');
            int secondSpace = header.indexOf(' ', firstSpace + 1);
            if (firstSpace != -1 && secondSpace != -1) {
              method = header.substring(0, firstSpace);
              url = header.substring(firstSpace + 1, secondSpace);
              isPost = (method == "POST");
            }
            break;
          } else {
            // Check for Content-Length header
            if (currentLine.startsWith("Content-Length:")) {
              contentLength = currentLine.substring(15).toInt();
            }
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    // Read POST body if present
    String body = "";
    if (isPost && contentLength > 0) {
      for (int i = 0; i < contentLength; i++) {
        if (client.available()) {
          body += (char)client.read();
        }
      }
    }

    // Handle the request
    handleClientRequest(client, method, url, body);

    client.stop();
  }
}

// ========== ARDUINO SETUP & LOOP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nSmart Plant Monitor Starting...");
  Serial.println("=================================");

  // Initialize LittleFS (Changed from SPIFFS)
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }
  Serial.println("LittleFS mounted successfully"); // Changed from SPIFFS

  // Initialize all components
  sensorsBegin();
  wifiBegin();
  webserverBegin();

  Serial.println("\nSYSTEM READY");
  if (isInAPMode) {
    Serial.println("MODE: Access Point");
    Serial.println("Connect to: SmartPlantSetup");
    Serial.println("Password: smartplant");
    Serial.println("Open: http://192.168.4.1");
  } else {
    Serial.println("MODE: Station (Connected to Wi-Fi)");
    Serial.println("Open: http://" + WiFi.localIP().toString());
  }
  Serial.println("=================================\n");
}

void loop() {
  webserverHandle();

  // Update sensors every 5 seconds
  static unsigned long lastSensorUpdate = 0;
  if (millis() - lastSensorUpdate > 5000) {
    lastSensorUpdate = millis();
    sensorsUpdate();
  }

  // Handle Wi-Fi reconnection
  wifiHandle();
  delay(1);
}