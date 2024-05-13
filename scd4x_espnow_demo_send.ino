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

#define RGB_LED_PIN         4                   //  WS2812 Pin

typedef struct espnow_data_s
{
  uint16_t co2;
  float temperature;
  float humidity;
} __attribute__((packed)) espnow_data_t;

// Set the MAC address of the device that will receive the data
// For example: DC:54:75:C7:E1:D0
const MacAddress peer_mac({0xDC, 0x54, 0x75, 0xC7, 0xE1, 0xD0});

// Class for Point to Point serial communication
ESP_NOW_Serial_Class NowSerial(peer_mac, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF, (const uint8_t *)ESPNOW_LMK);

SCD4x sensor;

void calculateLEDColor(uint16_t in, uint8_t& red, uint8_t& green, uint8_t& blue) {
  if (in <= 1000) {
    // Fully green
    red = 0;
    green = 255;
    blue = 0;
  } else if (in <= 3000) {
    // Interpolating between green and yellow
    float ratio = static_cast<float>(in - 1000) / 2000;
    red = 255 * (1 - ratio);
    green = 255;
    blue = 0;
  } else if (in <= 5000) {
    // Interpolating between yellow and red
    float ratio = static_cast<float>(in - 3000) / 2000;
    red = 255;
    green = 255 * (1 - ratio);
    blue = 0;
  } else {
    // Fully red
    red = 255;
    green = 0;
    blue = 0;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("SCD4x with ESP-NOW example (Sender)");

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
    espnow_data_t data;
    uint8_t red, green, blue;

    data.co2 = sensor.getCO2();
    data.temperature = sensor.getTemperature();
    data.humidity = sensor.getHumidity();

    calculateLEDColor(data.co2, red, green, blue);
    neopixelWrite(RGB_LED_PIN, red, green, blue);

    Serial.printf("Read data (CO2, Temperature, Humidity): %u, %.2f, %.2f\n", data.co2, data.temperature, data.humidity);
    Serial.println("Sending data to peer device\n");

    NowSerial.write((uint8_t *) &data, sizeof(data));
  }

  delay(500);
}
