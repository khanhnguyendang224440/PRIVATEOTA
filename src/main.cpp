#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

// ====== CẤU HÌNH WIFI ======
const char *WIFI_SSID = "IOT_2";
const char *WIFI_PASS = "iot@1234";

// ====== OTA Innoway ======
const char *FW_API_URL = "http://apivcloud.innoway.vn/api/downloadota/b58819b8-12ba-4a4d-a550-a8de15f16090/download";
const char *BEARER_TOKEN = "rPFJ3brJwk5ZK6H0QQ3zVGnWprMbrA7";  // Token thiết bị từ Innoway

// ====== PHIÊN BẢN HIỆN TẠI ======
#define FW_VERSION 4.0                       // <-- Sửa theo firmware hiện tại
#define FW_BUILD_TIME __DATE__ " " __TIME__

void connectWiFi()
{
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

bool otaUpdateFromInnoway(const String &url)
{
  WiFiClient client;
  HTTPClient http;

  Serial.println("📡 Connecting to OTA API...");
  http.begin(client, url);
  http.addHeader("Authorization", String("Bearer ") + BEARER_TOKEN);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST("");  // POST rỗng

  if (httpCode != HTTP_CODE_OK)
  {
    Serial.printf("❌ OTA API request failed! HTTP %d\n", httpCode);
    http.end();
    return false;
  }

  int contentLength = http.getSize();
  if (contentLength <= 0)
  {
    Serial.println("❌ Invalid firmware size!");
    http.end();
    return false;
  }
  Serial.printf("📦 Firmware size: %d bytes\n", contentLength);

  if (!Update.begin(contentLength))
  {
    Serial.println("❌ Not enough space for OTA!");
    http.end();
    return false;
  }

  Serial.println("✍️  Writing firmware to flash...");
  size_t written = Update.writeStream(http.getStream());

  if (written != (size_t)contentLength)
  {
    Serial.printf("❌ Written only %d/%d bytes\n", (int)written, contentLength);
    Update.abort();
    http.end();
    return false;
  }

  if (!Update.end())
  {
    Serial.printf("❌ Update.end() failed! Error %d\n", Update.getError());
    http.end();
    return false;
  }

  if (!Update.isFinished())
  {
    Serial.println("❌ OTA not finished correctly!");
    http.end();
    return false;
  }

  Serial.println("✅ OTA SUCCESS! Rebooting...");
  http.end();
  delay(2000);
  ESP.restart();  // reboot sau khi update thành công
  return true;
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println("🔧 Booting ESP32");
  Serial.printf("FW version: %.1f | Built at: %s\n", FW_VERSION, FW_BUILD_TIME);

  connectWiFi();

  Serial.println("🚀 Checking Innoway OTA...");
  otaUpdateFromInnoway(FW_API_URL);  // chỉ gọi 1 lần khi boot
}

void loop()
{
  // Code chính của bạn ở đây
  Serial.println("🏃 ESP32 running normally...");
  delay(5000);
}
