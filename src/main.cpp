#include <Arduino.h>
#include "mqttclient.h"

#define SENSORNO "9"
#define DNSNAME "EspSolarMoisture2"
#define TOPIC "iot/garten/EspSolarMoisture/"

#define LED_PIN 2      // ESP-01 -> Pin 1 / ESP-01S -> Pin 2 //esp32 dollatec -> 16 (nicht benutzen wenn deep sleep verwendet wird)
#define SENSOR_POWER 3 // pin 3 (RX), set to high to power the sensor
// wakeup each 5min = 300s
// #define SLEEPTIME_US (1000000 * 300)
#define SLEEPTIME_US (1000000 * 10) // deep sleep needs wire between RST and XPD_DCDC on ESP-01
MqttClient *mqttClient = nullptr;
const float SCALE = 1;
const float OFFSET = 0;

int clip(int value, int min = 0, int max = 100)
{
  if (value < min)
  {
    return min;
  }
  if (value > max)
  {
    return max;
  }
  return value;
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // turn off led
  // power sensor and wait for it to settle
  pinMode(SENSOR_POWER, OUTPUT);
  digitalWrite(SENSOR_POWER, HIGH);
  delay(1000);

  mqttClient = new MqttClient(DNSNAME, TOPIC, nullptr);
  if (mqttClient->isOk())
  {
    mqttClient->operate();

    auto value = 0;
    for (int i = 0; i < 10; i++)
    {
      value += analogRead(A0);
      delay(100);
    }
    value /= 10;
    digitalWrite(SENSOR_POWER, LOW);

    digitalWrite(LED_PIN, LOW); // turn on led
    //mqttClient->publish(SENSORNO, std::to_string(100 - clip(value * SCALE + OFFSET)).c_str());
    mqttClient->publish(SENSORNO, std::to_string(value).c_str());
    digitalWrite(LED_PIN, HIGH); // turn off led
  }
  delete mqttClient;
  ESP.deepSleep(SLEEPTIME_US);
}

void loop()
{
}
