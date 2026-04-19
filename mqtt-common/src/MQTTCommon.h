#ifndef MQTT_COMMON_H
#define MQTT_COMMON_H

#include <Arduino.h>
#include <PubSubClient.h>

typedef void (*StatusCallback)(const char* message);

extern PubSubClient mqttClient;

void mqttCommonInit(const char* ssid, const char* password,
                    const char* broker, int port, const char* clientId,
                    const char* caCert = nullptr,
                    StatusCallback statusCb = nullptr);
void mqttCommonSetCallback(MQTT_CALLBACK_SIGNATURE);
void mqttCommonSubscribe(const char* topic);
bool mqttCommonPublish(const char* topic, const char* payload);
void mqttCommonLoop();
bool mqttCommonConnected();

#endif // MQTT_COMMON_H
