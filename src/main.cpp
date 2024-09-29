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

// Funktion zur Berechnung des Sättigungsdampfdrucks (in Pa) (by MS Copilot)
double saettigungsdampfdruck(double temperatur)
{
  return 6.1078 * pow(10, (7.5 * temperatur) / (237.3 + temperatur));
}

// Funktion zur Berechnung der absoluten Luftfeuchtigkeit (in g/m³)
double absoluteLuftfeuchtigkeit(double relativeFeuchte, double temperatur)
{
  double e_s = saettigungsdampfdruck(temperatur); // Sättigungsdampfdruck
  double e = (relativeFeuchte)*e_s;               // Wasserdampfdruck
  double R_d = 461.51;                            // Gaskonstante für Wasserdampf in J/(kg*K)
  double T = temperatur + 273.15;                 // Temperatur in Kelvin
  return (e * 1000) / (R_d * T);                  // Absolute Luftfeuchtigkeit in g/m³
}

void setup()
{
  // Serial.begin(9600);
  // Serial.println("init");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // turn off led

  pinMode(RELAIS_PIN, OUTPUT);
  digitalWrite(RELAIS_PIN, HIGH); // turn relais on for test
  delay(2000);
  digitalWrite(RELAIS_PIN, LOW); // turn relais off

  mqttClient = new MqttClient(DNSNAME, TOPIC, nullptr);
  mqttClient->publish("fanState", "0");

  // TODO: implement fan remote control "fanCmd"

  sht31_in.begin(0x44);
  sht31_out.begin(0x45);
}

void loop()
{
  static uint32_t cnt = 10;
  static bool fanState = false;
  if (mqttClient->isOk())
  {
    mqttClient->operate();

    digitalWrite(LED_PIN, LOW); // turn on led

    float tin = sht31_in.readTemperature();
    float hin = sht31_in.readHumidity();

    if (!isnan(tin))
    { // check if 'is not a number'
      // Serial.print("In Temp *C = ");
      // Serial.print(t);
      // Serial.print("\t\t");
      mqttClient->publish("in/temp", String(tin).c_str());
    }
    else
    {
      // Serial.println("Failed to read in temperature");
    }

    if (!isnan(hin))
    { // check if 'is not a number'
      // Serial.print("in Hum. % = ");
      // Serial.println(h);
      mqttClient->publish("in/hum", String(hin).c_str());
    }
    else
    {
      // Serial.println("Failed to read in humidity");
    }

    float tout = sht31_out.readTemperature();
    float hout = sht31_out.readHumidity();

    if (!isnan(tout))
    { // check if 'is not a number'
      // Serial.print("out Temp *C = ");
      // Serial.print(t);
      // Serial.print("\t\t");
      mqttClient->publish("out/temp", String(tout).c_str());
    }
    else
    {
      // Serial.println("Failed to read out temperature");
    }

    if (!isnan(hout))
    {
      // Serial.print("out Hum. % = ");
      // Serial.println(h);
      mqttClient->publish("out/hum", String(hout).c_str());
    }
    else
    {
      // Serial.println("Failed to read out humidity");
    }

    float absIn = absoluteLuftfeuchtigkeit(hin, tin);
    float absOut = absoluteLuftfeuchtigkeit(hout, tout);

    mqttClient->publish("in/absHum", String(absIn).c_str());
    mqttClient->publish("out/absHum", String(absOut).c_str());
    mqttClient->publish("fanState", String(fanState ? "1" : "0").c_str());

    mqttClient->sendRssi();

    if (cnt++ >= 10)
    {
      mqttClient->sendIp();
      cnt = 0;

      // turn fan on if inside air is significantly more humid than outside air
      if (absIn > absOut + 0.5)
      {
        digitalWrite(RELAIS_PIN, HIGH); // turn relais on
        fanState = true;
      }
      else
      {
        digitalWrite(RELAIS_PIN, LOW); // turn relais off
        fanState = false;
      }
    }

    digitalWrite(LED_PIN, HIGH); // turn off led
    delay(10000);
  }
}
