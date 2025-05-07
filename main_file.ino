#include <WiFi.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include<ArduinoOTA.h>
#include <DHT.h>

#include <WiFiClientSecure.h>
// #define WIFI_NETWORK  "TECNO SPARK 20C"
// #define WIFI_PASSWORD "mahamahmed18"
#define WIFI_TIMEOUT_MS 20000
const char * ssid = "TECNO SPARK 20C";
const char * wifiPassword = "mahamahmed18";
#include <ESPmDNS.h>
#define DHTPIN 21 
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);
// const char * ssid = "TECNO SPARK 20C";
// const char * wifiPassword = "mahamahmed18";
int status = WL_IDLE_STATUS;
int incomingByte;

String FirmwareVer = "1.1";

// Important: URLs changed from https to http (non-secure)
#define URL_fw_Version "https://raw.githubusercontent.com/Mahamahmed345/gitota/refs/heads/main/version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/Mahamahmed345/gitota/main/main_file.ino.bin"


void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPassword);
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
        static int i = 0;
        if (i++ == 10) {
            ESP.restart();
        }

  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi");
  } else {
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());
  }
}

void sendDataToServer(float temperature, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://amongst-pn-slope-friend.trycloudflare.com/api/sensordata/"); // replace with your server's local IP
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> jsonDoc;
    jsonDoc["temperature"] = temperature;
    jsonDoc["humidity"] = humidity;
    String jsonData;
    serializeJson(jsonDoc, jsonData);

    int httpResponseCode = http.POST(jsonData);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}


void setup() {
  Serial.begin(115200);
  Serial.print("Active Firmware Version: ");
  Serial.println(FirmwareVer);
  connectToWiFi();
  dht.begin();
  delay(2000);
  if (!MDNS.begin("esp32")) {
  Serial.println("Error setting up MDNS responder!");
}
  ArduinoOTA.begin();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");

  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate validation

  HTTPClient http;
  if (http.begin(client, "https://raw.githubusercontent.com/Mahamahmed345/gitota/refs/heads/main/version.txt")) {
    int httpCode = http.GET();
    Serial.printf("HTTP Code: %d\n", httpCode);
    if (httpCode == 200) {
      String payload = http.getString();
      Serial.println("Payload:");
      Serial.println(payload);
    } else {
      Serial.printf("HTTP error: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("Connection failed");
  }
  
}

void loop() {
  delay(1000);
  Serial.print("Active Firmware Version: ");
  Serial.println(FirmwareVer);

  if (WiFi.status() != WL_CONNECTED) {
        reconnect();
    }

  if (Serial.available() > 0) {
        incomingByte = Serial.read();
        if (incomingByte == 'U') {
            Serial.println("Firmware Update In Progress..");
            if (FirmwareVersionCheck()) {
                firmwareUpdate();
            }
        }
    }
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (!isnan(temperature) && !isnan(humidity)) {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C  Humidity: ");
    Serial.print(humidity);
    Serial.println("%");

    sendDataToServer(temperature, humidity);
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }

  delay(2000); 
  ArduinoOTA.handle();
   delay(2000); // Send data every 5 seconds
}
void reconnect() {
    static int i = 0;
    status = WiFi.status();
    if (status != WL_CONNECTED) {
        WiFi.begin(ssid, wifiPassword);
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
            if (i++ == 10) {
                ESP.restart();
            }
        }
        Serial.println("\nConnected to AP");
    }
}

void firmwareUpdate(void) {
    WiFiClientSecure client;
    client.setInsecure();   // Now using non-secure WiFiClient
    t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

    switch (ret) {
    case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

    case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

    case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
}

int FirmwareVersionCheck(void) {
    String payload;
    int httpCode;
    String FirmwareURL = "";
    FirmwareURL += URL_fw_Version;
    FirmwareURL += "?";
    FirmwareURL += String(rand());

    Serial.println(FirmwareURL);

    WiFiClientSecure client;
    client.setInsecure();   // Again non-secure client
    HTTPClient http;

    if (http.begin(client, FirmwareURL)) {
        Serial.print("[HTTP] GET...\n");
        delay(100);
        httpCode = http.GET();
       // serial.print(httpCode)
        delay(100);

        if (httpCode == HTTP_CODE_OK) {
            payload = http.getString();
        } else {
            Serial.print("Error Occurred During Version Check: ");
            Serial.println(httpCode);
        }
        http.end();
    }

    if (httpCode == HTTP_CODE_OK) {
        payload.trim();
        if (payload.equals(FirmwareVer)) {
            Serial.printf("\nDevice IS Already on Latest Firmware Version: %s\n", FirmwareVer.c_str());
            return 0;
        } else {
            Serial.println(payload);
            Serial.println("New Firmware Detected");
            return 1;
        }
    }
    return 0;
}
