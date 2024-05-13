/*
  SCD4x with ESP-NOW example (Receiver)
  Created by: Lucas Saavedra Vaz, 2024
*/

#include <Arduino.h>

#include <ESP32_NOW_Serial.h>
#include <MacAddress.h>
#include <WiFi.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

#define ESPNOW_WIFI_CHANNEL    4                   // WiFi Radio Channel
#define ESPNOW_WIFI_MODE       WIFI_STA            // WiFi Mode
#define ESPNOW_WIFI_IF         WIFI_IF_STA         // WiFi Interface
#define ESPNOW_PMK             "ESPNOW_PMK_SCD4x"  // ESP-NOW PMK
#define ESPNOW_LMK             "ESPNOW_LMK_SCD4x"  // ESP-NOW LMK

#define SPI_PIN_SCK            6
#define SPI_PIN_MISO           3
#define SPI_PIN_MOSI           7
#define DISPLAY_WIDTH          240
#define DISPLAY_HEIGHT         240
#define SPI_DISPLAY_FREQUENCY  40000000
#define SPI_DISPLAY_PIN_CS     5
#define SPI_DISPLAY_PIN_DC     4
#define SPI_DISPLAY_PIN_RST    8
#define DISPLAY_PIN_BACKLIGHT  9

#define CO2_MIN_VALUE          0
#define CO2_MAX_VALUE          5000
#define TEMPERATURE_MIN_VALUE  15.0
#define TEMPERATURE_MAX_VALUE  35.0
#define HUMIDITY_MIN_VALUE     0.0
#define HUMIDITY_MAX_VALUE     100.0

typedef struct espnow_data_s
{
  uint16_t co2;
  float temperature;
  float humidity;
} __attribute__((packed)) espnow_data_t;

espnow_data_t data;

// Set the MAC address of the device that will receive the data
// For example: F4:12:FA:FB:F0:30
const MacAddress peer_mac({0xF4, 0x12, 0xFA, 0xFB, 0xF0, 0x30});

ESP_NOW_Serial_Class NowSerial(peer_mac, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF, (const uint8_t *)ESPNOW_LMK);
Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, SPI_DISPLAY_PIN_CS, SPI_DISPLAY_PIN_DC, SPI_DISPLAY_PIN_RST);

template <typename T>
uint8_t get_rectangle_size(T value, T min_value, T max_value) {
  uint16_t size = static_cast<uint16_t>(DISPLAY_WIDTH * (value - min_value) / (max_value - min_value));
  return (size > DISPLAY_WIDTH) ? DISPLAY_WIDTH : size;
}

uint16_t calculate_rectangle_color(uint16_t in) {
  uint16_t color565;

  if (in <= 1000) {
    // Fully green
    color565 = 0x07E0; // Green: 5 bits, Green = 31, Blue: 6 bits, Blue = 0
  } else if (in <= 3000) {
    // Interpolating between green and yellow
    float ratio = static_cast<float>(in - 1000) / 2000;
    uint8_t green = 31;
    uint8_t red = static_cast<uint8_t>(31 * (1 - ratio));
    color565 = (red << 11) | (green << 5); // Red: 5 bits, Green: 6 bits
  } else if (in <= 5000) {
    // Interpolating between yellow and red
    float ratio = static_cast<float>(in - 3000) / 2000;
    uint8_t red = 31;
    uint8_t green = static_cast<uint8_t>(31 * (1 - ratio));
    color565 = (red << 11) | (green << 5); // Red: 5 bits, Green: 6 bits
  } else {
    // Fully red
    color565 = 0xF800; // Red: 5 bits, Red = 31, Green: 6 bits, Green = 0
  }

  return color565;
}


void setup() {
  Serial.begin(115200);
  Serial.println("SCD4x with ESP-NOW example (Receiver)");

  Serial.println("Initializing Wi-Fi");
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

  Serial.println("Initializing SPI");
  SPI.begin(SPI_PIN_SCK, SPI_PIN_MISO, SPI_PIN_MOSI);
  SPI.setFrequency(SPI_DISPLAY_FREQUENCY);

  Serial.println("Initializing display");
  tft.init(DISPLAY_WIDTH, DISPLAY_HEIGHT);
  tft.setRotation(2);
  tft.setTextSize(2);
  tft.fillScreen(ST77XX_BLACK);
  pinMode(DISPLAY_PIN_BACKLIGHT, OUTPUT);
  digitalWrite(DISPLAY_PIN_BACKLIGHT, HIGH);

  Serial.println("Device initialized\n");
}

void loop() {
  if (NowSerial.available()) {
    uint8_t rectangle_size;
    uint16_t color;

    // Clear the previous data by redrawing everything in black
    tft.setTextColor(ST77XX_BLACK);

    tft.setCursor(10, 5);
    tft.print("CO2: ");
    tft.print(data.co2);
    rectangle_size = get_rectangle_size<uint16_t>(data.co2, CO2_MIN_VALUE, CO2_MAX_VALUE);
    tft.fillRect(0, 25, rectangle_size, 20, ST77XX_BLACK);

    tft.setCursor(10, 60);
    tft.print("Temperature: ");
    tft.print(data.temperature, 2);
    rectangle_size = get_rectangle_size<float>(data.temperature, TEMPERATURE_MIN_VALUE, TEMPERATURE_MAX_VALUE);
    tft.fillRect(0, 80, rectangle_size, 20, ST77XX_BLACK);

    tft.setCursor(10, 115);
    tft.print("Humidity: ");
    tft.print(data.humidity, 2);
    rectangle_size = get_rectangle_size<float>(data.humidity, HUMIDITY_MIN_VALUE, HUMIDITY_MAX_VALUE);
    tft.fillRect(0, 135, rectangle_size, 20, ST77XX_BLACK);

    // Read the new data
    NowSerial.read((uint8_t *)&data, sizeof(data));

    // Draw the new data
    tft.setTextColor(ST77XX_WHITE);

    tft.setCursor(10, 5);
    tft.print("CO2: ");
    tft.print(data.co2);
    rectangle_size = get_rectangle_size<uint16_t>(data.co2, CO2_MIN_VALUE, CO2_MAX_VALUE);
    color = calculate_rectangle_color(data.co2);
    tft.fillRect(0, 25, rectangle_size, 20, color);

    tft.setCursor(10, 60);
    tft.print("Temperature: ");
    tft.print(data.temperature, 2);
    rectangle_size = get_rectangle_size<float>(data.temperature, TEMPERATURE_MIN_VALUE, TEMPERATURE_MAX_VALUE);
    tft.fillRect(0, 80, rectangle_size, 20, ST77XX_CYAN);

    tft.setCursor(10, 115);
    tft.print("Humidity: ");
    tft.print(data.humidity, 2);
    rectangle_size = get_rectangle_size<float>(data.humidity, HUMIDITY_MIN_VALUE, HUMIDITY_MAX_VALUE);
    tft.fillRect(0, 135, rectangle_size, 20, ST77XX_BLUE);
  }

  delay(100);
}
