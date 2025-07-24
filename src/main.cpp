#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

// ====== CẤU HÌNH WIFI ======
const char *WIFI_SSID     = "IOT_2";
const char *WIFI_PASS     = "iot@1234";

// ====== SERVER OTA ======
// Đảm bảo ESP32 & máy tính cùng mạng LAN
const char *OTA_SERVER    = "http://192.168.100.29:8000/";
const char *VERSION_FILE  = "version.json";

// ====== PHIÊN BẢN HIỆN TẠI ======
#define FW_VERSION 5.0                       // <-- Tăng số này khi build bản mới
#define FW_BUILD_TIME __DATE__ " " __TIME__  // Thời gian build firmware

void connectWiFi() {
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

bool downloadAndUpdate(String fwUrl) {
  WiFiClient client;
  HTTPClient httpFW;

  Serial.println("📥 Downloading firmware: " + fwUrl);
  httpFW.begin(client, fwUrl);
  int fwCode = httpFW.GET();

  if (fwCode != HTTP_CODE_OK) {
    Serial.printf("❌ Firmware download failed! HTTP %d\n", fwCode);
    httpFW.end();
    return false;
  }

  int contentLength = httpFW.getSize();
  if (contentLength <= 0) {
    Serial.println("❌ Invalid firmware size!");
    httpFW.end();
    return false;
  }
  Serial.printf("📦 Firmware size: %d bytes\n", contentLength);

  if (!Update.begin(contentLength)) {
    Serial.println("❌ Not enough space for OTA!");
    httpFW.end();
    return false;
  }

  Serial.println("✍️  Writing firmware to flash...");
  size_t written = Update.writeStream(httpFW.getStream());

  if (written != (size_t)contentLength) {
    Serial.printf("❌ Written only %d/%d bytes\n", (int)written, contentLength);
    Update.abort();
    httpFW.end();
    return false;
  }

  if (!Update.end()) {
    Serial.printf("❌ Update.end() failed! Error %d\n", Update.getError());
    httpFW.end();
    return false;
  }

  if (!Update.isFinished()) {
    Serial.println("❌ OTA not finished correctly!");
    httpFW.end();
    return false;
  }

  Serial.println("✅ OTA SUCCESS! Rebooting...");
  httpFW.end();
  delay(2000);
  ESP.restart();  // reboot sau khi update thành công
  return true;
}

void checkForUpdate() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected, cannot check OTA!");
    return;
  }

  HTTPClient httpVer;
  String versionUrl = String(OTA_SERVER) + VERSION_FILE;

  Serial.println("🔎 Checking OTA version: " + versionUrl);
  httpVer.begin(versionUrl);
  int httpCode = httpVer.GET();

  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("❌ Failed to fetch version.json (HTTP %d)\n", httpCode);
    httpVer.end();
    return;
  }

  String payload = httpVer.getString();
  Serial.println("📄 Version JSON: " + payload);

  DynamicJsonDocument doc(256);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("❌ JSON parse failed!");
    httpVer.end();
    return;
  }

  float latestVersion = doc["version"].as<float>();
  String fwUrl = doc["firmware_url"].as<String>();

  Serial.printf("📌 Current FW: %.1f (Built %s) | Latest FW: %.1f\n",
                FW_VERSION, FW_BUILD_TIME, latestVersion);

  if (latestVersion > FW_VERSION) {
    Serial.println("🚀 New firmware available! Start OTA update...");
    httpVer.end();
    downloadAndUpdate(fwUrl);
  } else {
    Serial.println("✅ Already up-to-date, no update needed.");
    httpVer.end();
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("🔧 Booting ESP32");
  Serial.printf("FW version: %.1f | Built at: %s\n", FW_VERSION, FW_BUILD_TIME);

  connectWiFi();
  checkForUpdate();  // chỉ check 1 lần khi boot
}

void loop() {
  // Code chính của bạn ở đây
  Serial.println("🏃 ESP32 running normally...");
  delay(5000);
}
