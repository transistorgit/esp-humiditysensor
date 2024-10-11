#pragma once

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <functional>
#include <string>

class MqttClient{
    const char* mqtt_server = "192.168.168.112";
    std::string name;
    std::string prefix;
    bool wifiOk = false;
    int mqttRetries = 0;
    int wifiRetries = 0;
    const int Max_Wifi_Retries = 30;
    const int Max_Mqtt_Retries = 20;


    WiFiClient espClient;
    PubSubClient client {espClient};

    unsigned int lastMsg = 0;
    std::string status {"offline"};

    bool setupWifi();
    void initOta();
    void reconnect();
    
public:
    MqttClient(std::string name, std::string prefix, std::function<void(char* topic, byte* payload, unsigned int length)> cmdCallback);
    ~MqttClient();
    void operate();
    void publish(std::string subtopic, std::string payload);
    void sendRssi();
    void sendIp();
    bool isOk() { return wifiOk;}
    bool subscribe(std::string subtopic);
};

