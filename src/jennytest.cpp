/*
 * temperature_sensor.ino
 *
 * This example shows how to:
 * 1. define a temperature sensor accessory and its characteristics (in my_accessory.c).
 * 2. report the sensor value to HomeKit (just random value here, you need to change it to your real sensor value).
 *
 *  Created on: 2020-05-15
 *      Author: Mixiaoxiao (Wang Bin)
 *
 * Note:
 *
 * You are recommended to read the Apple's HAP doc before using this library.
 * https://developer.apple.com/support/homekit-accessory-protocol/
 *
 * This HomeKit library is mostly written in C,
 * you can define your accessory/service/characteristic in a .c file,
 * since the library provides convenient Macro (C only, CPP can not compile) to do this.
 * But it is possible to do this in .cpp or .ino (just not so conveniently), do it yourself if you like.
 * Check out homekit/characteristics.h and use the Macro provided to define your accessory.
 *
 * Generally, the Arduino libraries (e.g. sensors, ws2812) are written in cpp,
 * you can include and use them in a .ino or a .cpp file (but can NOT in .c).
 * A .ino is a .cpp indeed.
 *
 * You can define some variables in a .c file, e.g. int my_value = 1;,
 * and you can access this variable in a .ino or a .cpp by writing extern "C" int my_value;.
 *
 * So, if you want use this HomeKit library and other Arduino Libraries together,
 * 1. define your HomeKit accessory/service/characteristic in a .c file
 * 2. in your .ino, include some Arduino Libraries and you can use them normally
 *                  write extern "C" homekit_characteristic_t xxxx; to access the characteristic defined in your .c file
 *                  write your logic code (eg. read sensors) and
 *                  report your data by writing your_characteristic.value.xxxx_value = some_data; homekit_characteristic_notify(..., ...)
 * done.
 */

#include <Arduino.h>
#include <arduino_homekit_server.h>
#include "wifi_info.h"
//include the Arduino library for your real sensor here, e.g. <DHT.h>
#include <DHTesp.h>
#define OCCUPANCY_SENSOR_PIN D1
#define DHTPIN D3
#define DHTTYPE DHT11

#define REPORTING_TIME_MS 2 * 1000 //
#define SHOW_HEAP_MS 10 * 1000 //

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

DHTesp dht;

// static byte blink_value = HIGH;
// Function declarations
void my_homekit_loop();
void my_homekit_setup();
void my_homekit_report();
void my_dht_setup();

void setup() {
  Serial.begin(115200);
  wifi_connect(); // in wifi_info.h
  my_homekit_setup();
  my_dht_setup();
}

void loop() {
  my_homekit_loop();
  delay(10);
}

//==============================
// Homekit setup and loop
//==============================

// access your homekit characteristics defined in my_accessory.c
extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t cha_occupancy_detected;
extern "C" homekit_characteristic_t cha_temperature;
extern "C" homekit_characteristic_t cha_humidity;

static uint32_t next_heap_millis = 0;
static uint32_t next_report_millis = 0;

void my_homekit_setup() {
  pinMode(OCCUPANCY_SENSOR_PIN, INPUT);
  pinMode(D0,OUTPUT); 
  
  arduino_homekit_setup(&config);
}

void my_dht_setup() {
  dht.setup(DHTPIN, DHTesp::DHTTYPE);
}

void my_homekit_loop() {
  arduino_homekit_loop();
  const uint32_t t = millis();
  if (t > next_report_millis) {
    // report sensor values every 2 seconds
    next_report_millis = t + REPORTING_TIME_MS;
    my_homekit_report();
  }
  if (t > next_heap_millis) {
    // show heap info every 5 seconds
    int connected_clients = arduino_homekit_connected_clients_count();
    next_heap_millis = t + 5 * 1000;
    LOG_D("Free heap: %d, HomeKit clients: %d",
      ESP.getFreeHeap(), connected_clients);
  }
}

void my_homekit_report() {
  // float temperature_value = random_value(10, 30); // FIXME, read your real sensor here.
  // cha_current_temperature.value.float_value = temperature_value;
  float temp = dht.getTemperature() - 3.0;
  float hum = dht.getHumidity();
  bool occupancy_sensor_value = ((bool)digitalRead(OCCUPANCY_SENSOR_PIN));

  digitalWrite(D0, occupancy_sensor_value ? HIGH : LOW);

  if (isnan(hum) || isnan(temp)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  } else {
    cha_temperature.value.float_value = temp;
    homekit_characteristic_notify(&cha_temperature, cha_temperature.value);

    cha_humidity.value.float_value = hum;
    homekit_characteristic_notify(&cha_humidity, cha_humidity.value);
    
    LOG_D("Temp (C): %.1f", temp); 
    LOG_D("Hum (percent): %.0f", hum);
  }
  
  cha_occupancy_detected.value.bool_value = occupancy_sensor_value;
  homekit_characteristic_notify(&cha_occupancy_detected, cha_occupancy_detected.value);
  LOG_D("Occupancy sensor is: %s", occupancy_sensor_value ? "active" : "inactive");
  
}
