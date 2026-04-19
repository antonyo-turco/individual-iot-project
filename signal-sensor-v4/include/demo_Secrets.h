#ifndef SECRETS_H
#define SECRETS_H

// NOTE: Replace the following with your actual WiFi and MQTT credentials and rename this file to Secrets.h before building the project.
#define WIFI_SSID       "your-ssid"             // your WiFi name here
#define WIFI_PASSWORD   "your-password"         // your WiFi password here
#define MQTT_BROKER     "192.168.x.x"           // ipconfig on Windows
#define MQTT_PORT       1883                    // leave as is
#define MQTT_TOPIC      "iot/sensor/aggregate"  // leave as is

// NOTE: If you want to test LoRaWAN connectivity, also fill in the following with your LoRaWAN credentials. The current codebase does not implement LoRaWAN functionality, but these are included here for future expansion.
#define LORA_APPEUI   "0000000000000000"
#define LORA_DEVEUI   "your-deveui-here"
#define LORA_APPKEY   "your-appkey-here"

#endif // SECRETS_H
