#ifndef SECRETS_H
#define SECRETS_H

// NOTE: Replace the following with your actual WiFi and MQTT credentials
// and rename this file to Secrets.h before building the project.
#define WIFI_SSID       "your-ssid"
#define WIFI_PASSWORD   "your-password"
#define MQTT_BROKER     "192.168.x.x"
#define MQTT_PORT       8883

// Paste the contents of mosquitto/certs/ca.crt here
static const char* CA_CERT = R"EOF(
-----BEGIN CERTIFICATE-----
REPLACE_WITH_YOUR_CA_CRT_CONTENTS
-----END CERTIFICATE-----
)EOF";

#endif // SECRETS_H
