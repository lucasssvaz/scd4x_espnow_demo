/*
  SCD4x with ESP-NOW example
  Created by: Lucas Saavedra Vaz, 2024
*/

#include <Arduino.h>

#include <ESP32_NOW_Serial.h>
#include <MacAddress.h>
#include <WiFi.h>
#include <Wire.h>

#include "SparkFun_SCD4x_Arduino_Library.h" // Click here to get the library: http://librarymanager/All#SparkFun_SCD4x

#define ESPNOW_WIFI_CHANNEL 4                   // WiFi Radio Channel
#define ESPNOW_WIFI_MODE    WIFI_STA            // WiFi Mode
#define ESPNOW_WIFI_IF      WIFI_IF_STA         // WiFi Interface
#define ESPNOW_PMK          "ESPNOW_PMK_SCD4x"  // ESP-NOW PMK
#define ESPNOW_LMK          "ESPNOW_LMK_SCD4x"  // ESP-NOW LMK

// Set the MAC address of the device that will receive the data
// For example: f4:12:fa:42:b6:e8
const MacAddress peer_mac({0xF4, 0x12, 0xFA, 0x42, 0xB6, 0xE8});

// Class for Point to Point serial communication
ESP_NOW_Serial_Class NowSerial(peer_mac, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF, (const uint8_t *)ESPNOW_LMK);

SCD4x sensor;

void setup() {
  Serial.begin(115200);
  Serial.println("SCD4x with ESP-NOW example");

  Serial.println("Initializing WiFi");
  WiFi.mode(ESPNOW_WIFI_MODE);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  while (!WiFi.STA.started()) {
    delay(100);
  }

  Serial.print("STA MAC Address: ");
  Serial.println(WiFi.macAddress());

  Serial.println("Initializing ESP-NOW");
  if (!ESP_NOW.begin((const uint8_t *)ESPNOW_PMK)) {
    Serial.println("ESP-NOW initialization failed");
    while (1) {
      delay(1000);
    }
  }
  NowSerial.begin(115200);

  Serial.println("Initializing I2C");
  Wire.begin(42, 40); // SDA, SCL

  Serial.println("Initializing SCD4x");
  while (!sensor.begin()) {
    Serial.println("SCD4x not detected. Please check wiring.");
    delay(1000);
  }

  Serial.println("Device initialized\n");
}

void loop() {
  if (sensor.readMeasurement()) { // readMeasurement will return true when fresh data is available
    char buffer[32];

    uint16_t co2 = sensor.getCO2();
    float temperature = sensor.getTemperature();
    float humidity = sensor.getHumidity();

    snprintf(buffer, sizeof(buffer), "%u,%.2f,%.2f\n", co2, temperature, humidity);

    Serial.printf("Read data (CO2, Temperature, Humidity): %s\n", buffer);
    Serial.println("Sending data to peer device\n");

    NowSerial.write((uint8_t *)buffer, strlen(buffer));
  }

  delay(500);
}
