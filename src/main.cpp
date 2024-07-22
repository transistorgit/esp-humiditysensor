#include <Arduino.h>
#include "mqttclient.h"
#include <Wire.h>
#include "Adafruit_SHT31.h"

#define DNSNAME "EspHumidity1"
#define TOPIC "iot/keller/EspHumiditySensor/"

// pin description: https://wolles-elektronikkiste.de/wemos-d1-mini-boards
#define LED_PIN 2 // D4
// D1 SCL
// D2 SDA
#define LEVEL_PIN D7
#define RELAIS_PIN D6

#define SLEEPTIME_US (1000000 * 10) // deep sleep needs wire between RST and XPD_DCDC on ESP-01
MqttClient *mqttClient = nullptr;

Adafruit_SHT31 sht31_in = Adafruit_SHT31();
Adafruit_SHT31 sht31_out = Adafruit_SHT31();

void drain()
{
  mqttClient->publish("pumping", "1");
  digitalWrite(RELAIS_PIN, HIGH);
  for (int i = 0; i < 3; i++)
  {
    mqttClient->operate();
    delay(10000);
  }
  digitalWrite(RELAIS_PIN, LOW);
  mqttClient->publish("pumping", "0");
}

void setup()
{
  // Serial.begin(9600);
  // Serial.println("init");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // turn off led

  pinMode(RELAIS_PIN, OUTPUT);
  digitalWrite(RELAIS_PIN, LOW); // turn relais off

  mqttClient = new MqttClient(DNSNAME, TOPIC, nullptr);
  mqttClient->publish("bucketfull", "0");
  mqttClient->publish("pumping", "0");

  sht31_in.begin(0x44);
  sht31_out.begin(0x45);
}

void loop()
{
  static uint32_t cnt = 100;

  if (mqttClient->isOk())
  {
    mqttClient->operate();

    digitalWrite(LED_PIN, LOW); // turn on led

    float t = sht31_in.readTemperature();
    float h = sht31_in.readHumidity();

    if (!isnan(t))
    { // check if 'is not a number'
      // Serial.print("In Temp *C = ");
      // Serial.print(t);
      // Serial.print("\t\t");
      mqttClient->publish("in/temp", String(t).c_str());
    }
    else
    {
      // Serial.println("Failed to read in temperature");
    }

    if (!isnan(h))
    { // check if 'is not a number'
      // Serial.print("in Hum. % = ");
      // Serial.println(h);
      mqttClient->publish("in/hum", String(h).c_str());
    }
    else
    {
      // Serial.println("Failed to read in humidity");
    }

    t = sht31_out.readTemperature();
    h = sht31_out.readHumidity();

    if (!isnan(t))
    { // check if 'is not a number'
      // Serial.print("out Temp *C = ");
      // Serial.print(t);
      // Serial.print("\t\t");
      mqttClient->publish("out/temp", String(t).c_str());
    }
    else
    {
      // Serial.println("Failed to read out temperature");
    }

    if (!isnan(h))
    {
      // Serial.print("out Hum. % = ");
      // Serial.println(h);
      mqttClient->publish("out/hum", String(h).c_str());
    }
    else
    {
      // Serial.println("Failed to read out humidity");
    }

    mqttClient->sendRssi();

    if (cnt++ >= 100)
    {
      mqttClient->sendIp();
      cnt = 0;

      if (digitalRead(LEVEL_PIN))
      {
        mqttClient->publish("bucketfull", "0");
      }
      else
      {
        drain();
        if (!digitalRead(LEVEL_PIN))
        {
          mqttClient->publish("bucketfull", "1");
        }
      }
    }

    digitalWrite(LED_PIN, HIGH); // turn off led
    delay(10000);
  }
}
