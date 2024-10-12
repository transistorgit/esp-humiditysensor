#include "mqttclient.h"
#include "secrets.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

MqttClient::MqttClient(std::string name, std::string prefix, std::function<void(char* topic, byte* payload, unsigned int length)> cmdCallback)
:name(name)
,prefix(prefix)
{
  if( prefix.back() != '/'){
    prefix.append("/");
  }

  wifiOk = setupWifi();

  if (wifiOk)
  {
    client.setServer(mqtt_server, 1883);
    if (cmdCallback)
    {
      client.setCallback(cmdCallback);
    }
  }
}

MqttClient::~MqttClient()
{
  publish("WifiRetries", String(wifiRetries).c_str());
  publish("MqttRetries", String(mqttRetries).c_str());
  sendRssi();

  if (client.connected())
  {
    // wait for last transmission
    for (int i = 0; i < 20; i++)
    {
      client.loop();
      delay(100);
    }

    client.disconnect();

    // wait 20s for clean disconnect, then shutdown anyhow
    for (int i = 0; i < 100; i++)
    {
      if (!client.connected())
      {
        break;
      }
      delay(200);
    }
  }

  // Serial.println(".");//make blue led blink
  WiFi.disconnect();
}

bool MqttClient::setupWifi()
{
  delay(100);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(name.c_str());
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

        if (++wifiRetries > Max_Wifi_Retries)
        {
            return false;
        }
        delay(2000);
        Serial.print("+");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  initOta();
  return true;
}

void MqttClient::initOta(){
  ArduinoOTA.setHostname(name.c_str());
  ArduinoOTA.setPassword(OTAPASSWORD);
  ArduinoOTA.onStart([]()
                     { Serial.println("Start"); });
  ArduinoOTA.onEnd([]()
                   { Serial.println("\nEnd"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
  ArduinoOTA.begin();
}

void MqttClient::reconnect() {
  // Loop until we're reconnected
  Serial.print("Attempting MQTT reconnection...");
  if (client.connect(name.c_str()))
  {
    Serial.println("connected");
    return;
  }

  // Serial.print("failed, rc=");
  // Serial.print(client.state());
  // Serial.println(" try again in 1 second");
  if (mqttRetries++ > Max_Mqtt_Retries)
  {
    wifiOk = false;
    return;
  }

    // Wait 1 seconds before retrying
    delay(1000);
}

void MqttClient::operate(){
    if (wifiOk && !client.connected())
    {
        reconnect();
    }
  client.loop();
  ArduinoOTA.handle();
}

void MqttClient::publish(std::string subtopic, std::string payload)
{
  if (client.connected())
  {
    client.publish(std::string(prefix + subtopic).c_str(), payload.c_str());
  }
}

void MqttClient::sendRssi(){
  char buf[10]; 
  std::sprintf(buf, "%d", WiFi.RSSI()); 
  client.publish(std::string(prefix + "RSSI").c_str(), buf);
}

void MqttClient::sendIp(){
  client.publish(std::string(prefix + "IP").c_str(), WiFi.localIP().toString().c_str());
}

bool MqttClient::subscribe(std::string subtopic){
  if (!client.connected())
  {
    return false;
  }
  Serial.println(std::string(prefix + subtopic).c_str());
  return client.subscribe(std::string(prefix + subtopic).c_str());
}