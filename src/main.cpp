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
#define RELAY_PIN D6

#define SLEEPTIME_US (1000000 * 10) // deep sleep needs wire between RST and XPD_DCDC on ESP-01
MqttClient *mqttClient = nullptr;

Adafruit_SHT31 sht31_in = Adafruit_SHT31();
Adafruit_SHT31 sht31_out = Adafruit_SHT31();

// Standard trigger values
float fanOnLevel = 3.0;
float fanOffLevel = 2.0;

// loop counter
uint32_t cnt = 100;

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

// Funktion, um MQTT-Nachrichten zu verarbeiten  -  doesn't work, receives garbage
void handleMqttMessage(char *topic, byte *payload, unsigned int length)
{
  mqttClient->publish("mqttCmdRcvd", "1");
  Serial.println(topic);
  char data[10];
  snprintf(data, min((int)length, 10), "%s", payload);
  Serial.println(data);

  if (strcmp(topic, "iot/keller/EspHumiditySensor/fanOnLevel") == 0)
  {
    Serial.println("fanOnLevel");
    char data[10];
    snprintf(data, min((int)length, 10), "%s", payload);
    fanOnLevel = atof(data);
    mqttClient->publish("fanOnLevelReturn", String(fanOnLevel).c_str());
    cnt = 100; // make changes effective immediately
  }
  else if (strcmp(topic, "iot/keller/EspHumiditySensor/fanOffLevel") == 0)
  {
    Serial.println("fanOffLevel");
    char data[10];
    snprintf(data, min((int)length, 10), "%s", payload);
    fanOffLevel = atof(data);
    mqttClient->publish("fanOffLevelReturn", String(fanOffLevel).c_str());
    cnt = 100; // make changes effective immediately
  }
  else if (strcmp(topic, "iot/keller/EspHumiditySensor/fanCmd") == 0)
  {
    Serial.println("fanCmd");
    digitalWrite(RELAY_PIN, (*payload) & 0xff != 0);
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("init");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // turn off led

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // turn relay on for test
  delay(2000);
  digitalWrite(RELAY_PIN, LOW); // turn relay off

  mqttClient = new MqttClient(DNSNAME, TOPIC, handleMqttMessage);
  mqttClient->operate(); //hack to improve immediate connection
  mqttClient->publish("fanState", "0");
  mqttClient->subscribe("fanOnLevel");
  mqttClient->subscribe("fanOffLevel");
  bool succ = mqttClient->subscribe("fanCmd");
  Serial.println(succ ? "succ" : "fail");

  // TODO: implement fan remote control "fanCmd"

  sht31_in.begin(0x44);
  sht31_out.begin(0x45);
}

void loop()
{
  static bool fanState = false;
  float absIn = 0.0f;
  float absOut = 0.0f;
  if (mqttClient->isOk())
  {
    Serial.println("Loop");
    mqttClient->operate();

    if(cnt % 10 == 0){
      Serial.println("Loop/10");
      digitalWrite(LED_PIN, LOW); // turn on led

      float tin = sht31_in.readTemperature();
      float hin = sht31_in.readHumidity();

      if (!isnan(tin))
      {
        mqttClient->publish("in/temp", String(tin).c_str());
      }

      if (!isnan(hin))
      {
        mqttClient->publish("in/hum", String(hin).c_str());
      }

      float tout = sht31_out.readTemperature();
      float hout = sht31_out.readHumidity();

      if (!isnan(tout))
      {
        mqttClient->publish("out/temp", String(tout).c_str());
      }

      if (!isnan(hout))
      {
        mqttClient->publish("out/hum", String(hout).c_str());
      }

      absIn = absoluteLuftfeuchtigkeit(hin, tin);
      absOut = absoluteLuftfeuchtigkeit(hout, tout);

      mqttClient->publish("in/absHum", String(absIn).c_str());
      mqttClient->publish("out/absHum", String(absOut).c_str());
      mqttClient->publish("fanState", String(fanState ? "1" : "0").c_str());

      mqttClient->sendRssi();
    }
    if (cnt++ >= 100)
    {
      Serial.println("Loop/100");
      mqttClient->sendIp();
      cnt = 0;

      // Turn fan on if the inside absolute humidity is at least 2.0 g/m³ higher than outside
      if (absIn > absOut + fanOnLevel)
      {
        digitalWrite(RELAY_PIN, HIGH); // turn relay on
        fanState = true;
      }
      // Turn fan off if the inside absolute humidity is 1.0 g/m³ higher than outside
      else if (absIn <= absOut + fanOffLevel)
      {
        digitalWrite(RELAY_PIN, LOW); // turn relay off
        fanState = false;
      }
    }

    digitalWrite(LED_PIN, HIGH); // turn off led
    delay(1000);
  }
}
