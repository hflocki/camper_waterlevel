#include "webserver.h"
#include "wifi_manager.h"
#include "sensor.h"
#include "mqtt_ha.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);[cite: 8]
static AsyncWebSocket ws("/ws");[cite: 8]
static unsigned long lastWsUpdate = 0;[cite: 8]

void wsSendSensorData() {
    JsonDocument doc;
    
    doc["voltage"] = round(sensorData.avg_voltage * 1000.0f) / 1000.0f;
    doc["liters"] = round(sensorData.liters * 10.0f) / 10.0f;
    doc["percent"] = round(sensorData.percent);
    doc["rssi"] = sensorData.rssi;[cite: 8]
    doc["filter"] = config.filter_active;[cite: 8]
    doc["wifi"] = getWiFiStateString();[cite: 8]
    doc["mqtt"] = mqttIsConnected();[cite: 8]
    
    char buffer[256];
    size_t len = serializeJson(doc, buffer);[cite: 8]
    ws.textAll(buffer, len);[cite: 8]
}

void loadConfig() {
    strlcpy(config.ap_ssid, DEFAULT_AP_SSID, sizeof(config.ap_ssid));[cite: 8]
    strlcpy(config.ap_pass, DEFAULT_AP_PASSWORD, sizeof(config.ap_pass));[cite: 8]
    memset(config.wifi_ssid, 0, sizeof(config.wifi_ssid));[cite: 8]
    memset(config.wifi_pass, 0, sizeof(config.wifi_pass));[cite: 8]
    strlcpy(config.mqtt_server, DEFAULT_MQTT_SERVER, sizeof(config.mqtt_server));[cite: 8]
    config.mqtt_port = DEFAULT_MQTT_PORT;[cite: 8]
    memset(config.mqtt_user, 0, sizeof(config.mqtt_user));[cite: 8]
    memset(config.mqtt_pass, 0, sizeof(config.mqtt_pass));[cite: 8]
    strlcpy(config.mqtt_prefix, DEFAULT_MQTT_PREFIX, sizeof(config.mqtt_prefix));[cite: 8]
    config.filter_active = true;
    config.mqtt_fast = false;[cite: 8]
    
    if (!LittleFS.exists(CONFIG_FILE)) {[cite: 8]
        saveConfig();[cite: 8]
        return;
    }
    
    File file = LittleFS.open(CONFIG_FILE, "r");[cite: 8]
    if (!file) return;[cite: 8]
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);[cite: 8]
    file.close();[cite: 8]
    if (err) return;[cite: 8]
    
    strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "", sizeof(config.wifi_ssid));[cite: 8]
    strlcpy(config.wifi_pass, doc["wifi_pass"] | "", sizeof(config.wifi_pass));[cite: 8]
    strlcpy(config.mqtt_server, doc["mqtt_server"] | DEFAULT_MQTT_SERVER, sizeof(config.mqtt_server));[cite: 8]
    config.mqtt_port = doc["mqtt_port"] | DEFAULT_MQTT_PORT;[cite: 8]
    strlcpy(config.mqtt_prefix, doc["mqtt_prefix"] | DEFAULT_MQTT_PREFIX, sizeof(config.mqtt_prefix));[cite: 8]
    config.filter_active = doc["filter_active"] | true;[cite: 8]
}

void saveConfig() {
    JsonDocument doc;[cite: 8]
    doc["wifi_ssid"] = config.wifi_ssid;[cite: 8]
    doc["wifi_pass"] = config.wifi_pass;[cite: 8]
    doc["mqtt_server"] = config.mqtt_server;[cite: 8]
    doc["mqtt_port"] = config.mqtt_port;[cite: 8]
    doc["mqtt_prefix"] = config.mqtt_prefix;[cite: 8]
    doc["filter_active"] = config.filter_active;[cite: 8]
    
    File file = LittleFS.open(CONFIG_FILE, "w");[cite: 8]
    if (!file) return;[cite: 8]
    
    serializeJsonPretty(doc, file);[cite: 8]
    file.close();[cite: 8]
}