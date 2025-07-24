#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h> // <<<<<< GIỮ LẠI THƯ VIỆN NÀY <<<<<<

// Thông tin Wi-Fi của bạn
const char *ssid = "IOT_2";        // <<<<<< THAY THẾ SSID CỦA BẠN <<<<<<
const char *password = "iot@1234"; // <<<<<< THAY THẾ MẬT KHẨU WIFI CỦA BẠN <<<<<<

// Địa chỉ IP của máy tính đang chạy máy chủ OTA và cổng
// Đảm bảo ESP32 và máy tính ở cùng mạng cục bộ
const char *ota_server_url_base = "http://192.168.100.29:8000/"; // <<<<<< THAY THẾ IP MÁY TÍNH CỦA BẠN <<<<<<
const char *firmware_file_name = "firmware_v3.0.bin";            // Tên file firmware mới trên server
const char *version_file_name = "version.json";                  // Tên file chứa thông tin phiên bản

// Phiên bản firmware hiện tại của ESP32
// Chúng ta sẽ tăng số này khi có bản cập nhật
const float CURRENT_FIRMWARE_VERSION = 3.0; // Giữ nguyên 1.0 cho lần upload đầu tiên

void connectToWiFi()
{
  Serial.print("Connecting to WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void check_for_updates()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String versionUrl = String(ota_server_url_base) + version_file_name;
    Serial.print("Checking for updates at: ");
    Serial.println(versionUrl);

    http.begin(versionUrl);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    { // Nếu request thành công
      String payload = http.getString();
      Serial.print("Received version info: ");
      Serial.println(payload);

      DynamicJsonDocument doc(256); // Kích thước đủ lớn cho JSON
      DeserializationError error = deserializeJson(doc, payload);

      if (error)
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        http.end();
        return;
      }

      float latestVersion = doc["version"].as<float>();
      String latestFirmwareUrl = doc["firmware_url"].as<String>();

      Serial.print("Current firmware version: ");
      Serial.println(CURRENT_FIRMWARE_VERSION);
      Serial.print("Latest available version: ");
      Serial.println(latestVersion);

      if (latestVersion > CURRENT_FIRMWARE_VERSION)
      {
        Serial.println("New firmware available! Starting update...");
        Serial.print("Downloading from: ");
        Serial.println(latestFirmwareUrl);

        // LOGIC CẬP NHẬT VỚI Update.h và HTTPClient.h
        WiFiClient client;                     // Sử dụng WiFiClient cho HTTPClient
        http.begin(client, latestFirmwareUrl); // Bắt đầu HTTP request cho firmware
        int httpResponseCode = http.GET();     // Gửi GET request

        if (httpResponseCode == HTTP_CODE_OK)
        {
          // Lấy tổng kích thước của file firmware
          int contentLength = http.getSize();
          Serial.printf("Firmware size: %d bytes\n", contentLength);

          // Kiểm tra xem có đủ không gian để cập nhật không
          bool canBegin = Update.begin(contentLength);

          if (canBegin)
          {
            Serial.println("Update begin successful. Writing firmware...");
            // Ghi dữ liệu từ stream HTTP vào bộ nhớ flash
            size_t writtenBytes = Update.writeStream(http.getStream());

            if (writtenBytes == contentLength)
            {
              Serial.println("Written: " + String(writtenBytes) + " successfully");
            }
            else
            {
              Serial.println("Written only: " + String(writtenBytes) + "/" + String(contentLength) + ". Error!");
              Update.abort(); // Hủy bỏ quá trình cập nhật nếu có lỗi
            }

            if (Update.end())
            { // Kết thúc quá trình cập nhật
              Serial.println("Update finished successfully!");
              if (Update.isFinished())
              { // Kiểm tra xem quá trình có thành công không
                Serial.println("OTA Success, restarting...");
                delay(2000);
                ESP.restart(); // Khởi động lại sau khi cập nhật
              }
              else
              {
                Serial.println("Update not finished successfully, but ended.");
              }
            }
            else
            {
              Serial.println("Update failed! Error: " + String(Update.getError()));
            }
          }
          else
          {
            Serial.println("Not enough space to begin OTA update.");
          }
        }
        else
        {
          Serial.printf("Error downloading firmware: HTTP code %d\n", httpResponseCode);
        }
        http.end(); // Đóng kết nối HTTP
      }
      else
      {
        Serial.println("No new firmware available.");
      }

      http.end(); // Đóng kết nối HTTP cho version.json
    }
    else
    {
      Serial.printf("Error getting version info: %s, HTTP code: %d\n", http.errorToString(httpCode).c_str(), httpCode);
      http.end(); // Đảm bảo kết thúc HTTP client ngay cả khi có lỗi
    }
  }
  else
  {
    Serial.println("WiFi not connected. Cannot check for updates.");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  connectToWiFi();
  Serial.printf("Current firmware version: %.1f\n", CURRENT_FIRMWARE_VERSION);
}

void loop()
{
  // Kiểm tra cập nhật mỗi 10 giây (chỉ để minh họa, trong thực tế nên ít hơn)
  static unsigned long lastCheckTime = 0;
  if (millis() - lastCheckTime > 10000)
  { // Check every 10 seconds
    check_for_updates();
    lastCheckTime = millis();
  }

  // Các tác vụ khác của ESP32 sẽ chạy ở đây
  // Ví dụ: đọc cảm biến, điều khiển relay, v.v.
  Serial.println("ESP32 is running current firmware...");
  delay(2000);
}