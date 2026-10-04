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

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len);
static void handleApiGetConfig(AsyncWebServerRequest* request);
static void handleApiSaveConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total);
static void handleApiRestart(AsyncWebServerRequest* request);

void webserverInit() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Web] LittleFS mount failed!");[cite: 8]
        return;
    }
    
    loadConfig();[cite: 8]
    
    ws.onEvent(onWsEvent);[cite: 8]
    server.addHandler(&ws);[cite: 8]
    
    if (LittleFS.exists("/index.html")) {[cite: 8]
        server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");[cite: 8]
    } else {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
            request->send(200, "text/html", "<h1>Camper Waterlevel</h1><p>Bitte index.html via LittleFS hochladen.</p>");
        });
    }
    
    server.on("/api/config", HTTP_GET, handleApiGetConfig);
    server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL, handleApiSaveConfig);
    server.on("/api/restart", HTTP_POST, handleApiRestart);
    server.on("/api/filter", HTTP_GET, [](AsyncWebServerRequest* request) {
        config.filter_active = !config.filter_active;
        saveConfig();[cite: 8]
        request->send(200, "application/json", "{\"filter\":\"" + String(config.filter_active ? "ON" : "OFF") + "\"}");
    });
}

void webserverStart() {
    server.begin();[cite: 8]
    Serial.println("[Web] Server started on port 80");[cite: 8]
}

void webserverLoop() {
    ws.cleanupClients(2);[cite: 8]

    if (millis() - lastWsUpdate > WS_UPDATE_MS) {[cite: 8]
        lastWsUpdate = millis();[cite: 8]
        if (ws.count() > 0) {[cite: 8]
            wsSendSensorData();[cite: 8]
        }
    }
}

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

static void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        wsSendSensorData();[cite: 8]
    }
}

static void handleApiGetConfig(AsyncWebServerRequest* request) {
    JsonDocument doc;
    
    doc["wifi_ssid"] = config.wifi_ssid;[cite: 8]
    doc["wifi_pass"] = config.wifi_pass;[cite: 8]
    doc["ap_ssid"] = config.ap_ssid;[cite: 8]
    doc["ap_pass"] = config.ap_pass;[cite: 8]
    doc["mqtt_server"] = config.mqtt_server;[cite: 8]
    doc["mqtt_port"] = config.mqtt_port;[cite: 8]
    doc["mqtt_user"] = config.mqtt_user;[cite: 8]
    doc["mqtt_pass"] = config.mqtt_pass;[cite: 8]
    doc["mqtt_prefix"] = config.mqtt_prefix;[cite: 8]
    doc["filter_active"] = config.filter_active;[cite: 8]

    // Matrix in JSON packen
    JsonArray arr = doc["calib"].to<JsonArray>();
    for (int i = 0; i < config.calibCount; i++) {
        JsonObject item = arr.add<JsonObject>();
        item["v"] = config.calibTable[i].volt;
        item["l"] = config.calibTable[i].liter;
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

static void handleApiSaveConfig(AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
    static String body;
    if (index == 0) body = "";
    body += String((char*)data, len);
    
    if (index + len == total) {
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, body);
        
        if (err) {
            request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }
        
        if (doc.containsKey("wifi_ssid")) strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "", sizeof(config.wifi_ssid));[cite: 8]
        if (doc.containsKey("wifi_pass")) strlcpy(config.wifi_pass, doc["wifi_pass"] | "", sizeof(config.wifi_pass));[cite: 8]
        if (doc.containsKey("ap_ssid")) strlcpy(config.ap_ssid, doc["ap_ssid"] | DEFAULT_AP_SSID, sizeof(config.ap_ssid));[cite: 8]
        if (doc.containsKey("ap_pass")) strlcpy(config.ap_pass, doc["ap_pass"] | DEFAULT_AP_PASSWORD, sizeof(config.ap_pass));[cite: 8]
        if (doc.containsKey("mqtt_server")) strlcpy(config.mqtt_server, doc["mqtt_server"] | "", sizeof(config.mqtt_server));[cite: 8]
        if (doc.containsKey("mqtt_port")) config.mqtt_port = doc["mqtt_port"] | DEFAULT_MQTT_PORT;[cite: 8]
        if (doc.containsKey("mqtt_user")) strlcpy(config.mqtt_user, doc["mqtt_user"] | "", sizeof(config.mqtt_user));[cite: 8]
        if (doc.containsKey("mqtt_pass")) strlcpy(config.mqtt_pass, doc["mqtt_pass"] | "", sizeof(config.mqtt_pass));[cite: 8]
        if (doc.containsKey("mqtt_prefix")) strlcpy(config.mqtt_prefix, doc["mqtt_prefix"] | DEFAULT_MQTT_PREFIX, sizeof(config.mqtt_prefix));[cite: 8]
        if (doc.containsKey("filter_active")) config.filter_active = doc["filter_active"];[cite: 8]

        // Matrix aus JSON speichern
        if (doc.containsKey("calib")) {
            JsonArray arr = doc["calib"].as<JsonArray>();
            config.calibCount = 0;
            for (JsonObject item : arr) {
                if (config.calibCount < MAX_CALIB_POINTS) {
                    config.calibTable[config.calibCount].volt = item["v"] | 0.0f;
                    config.calibTable[config.calibCount].liter = item["l"] | 0.0f;
                    config.calibCount++;
                }
            }
        }
        
        saveConfig();[cite: 8]
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    }
}

static void handleApiRestart(AsyncWebServerRequest* request) {
    request->send(200, "application/json", "{\"status\":\"restarting\"}");
    delay(500);
    ESP.restart();
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

    // Default Votronic Werkseinstellung[cite: 1]
    config.calibTable[0] = {0.130f, 0.0f};[cite: 1]
    config.calibTable[1] = {0.137f, 10.0f};[cite: 1]
    config.calibTable[2] = {0.973f, 20.0f};[cite: 1]
    config.calibTable[3] = {1.040f, 30.0f};[cite: 1]
    config.calibTable[4] = {1.350f, 40.0f};[cite: 1]
    config.calibTable[5] = {1.515f, 50.0f};[cite: 1]
    config.calibTable[6] = {1.770f, 60.0f};[cite: 1]
    config.calibTable[7] = {2.178f, 80.0f};[cite: 1]
    config.calibTable[8] = {2.386f, 100.0f};[cite: 1]
    config.calibCount = 9;

    if (!LittleFS.exists(CONFIG_FILE)) { saveConfig(); return; }[cite: 8]
    
    File file = LittleFS.open(CONFIG_FILE, "r");[cite: 8]
    if (!file) return;[cite: 8]
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);[cite: 8]
    file.close();[cite: 8]
    if (err) return;[cite: 8]
    
    strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "", sizeof(config.wifi_ssid));[cite: 8]
    strlcpy(config.wifi_pass, doc["wifi_pass"] | "", sizeof(config.wifi_pass));[cite: 8]
    strlcpy(config.ap_ssid, doc["ap_ssid"] | DEFAULT_AP_SSID, sizeof(config.ap_ssid));[cite: 8]
    strlcpy(config.ap_pass, doc["ap_pass"] | DEFAULT_AP_PASSWORD, sizeof(config.ap_pass));[cite: 8]
    strlcpy(config.mqtt_server, doc["mqtt_server"] | DEFAULT_MQTT_SERVER, sizeof(config.mqtt_server));[cite: 8]
    config.mqtt_port = doc["mqtt_port"] | DEFAULT_MQTT_PORT;[cite: 8]
    strlcpy(config.mqtt_user, doc["mqtt_user"] | "", sizeof(config.mqtt_user));[cite: 8]
    strlcpy(config.mqtt_pass, doc["mqtt_pass"] | "", sizeof(config.mqtt_pass));[cite: 8]
    strlcpy(config.mqtt_prefix, doc["mqtt_prefix"] | DEFAULT_MQTT_PREFIX, sizeof(config.mqtt_prefix));[cite: 8]
    config.filter_active = doc["filter_active"] | true;[cite: 8]

    if (doc.containsKey("calib")) {
        JsonArray arr = doc["calib"].as<JsonArray>();
        config.calibCount = 0;
        for (JsonObject item : arr) {
            if (config.calibCount < MAX_CALIB_POINTS) {
                config.calibTable[config.calibCount].volt = item["v"] | 0.0f;
                config.calibTable[config.calibCount].liter = item["l"] | 0.0f;
                config.calibCount++;
            }
        }
    }
}

void saveConfig() {
    JsonDocument doc;[cite: 8]
    doc["wifi_ssid"] = config.wifi_ssid;[cite: 8]
    doc["wifi_pass"] = config.wifi_pass;[cite: 8]
    doc["ap_ssid"] = config.ap_ssid;[cite: 8]
    doc["ap_pass"] = config.ap_pass;[cite: 8]
    doc["mqtt_server"] = config.mqtt_server;[cite: 8]
    doc["mqtt_port"] = config.mqtt_port;[cite: 8]
    doc["mqtt_user"] = config.mqtt_user;[cite: 8]
    doc["mqtt_pass"] = config.mqtt_pass;[cite: 8]
    doc["mqtt_prefix"] = config.mqtt_prefix;[cite: 8]
    doc["filter_active"] = config.filter_active;[cite: 8]

    JsonArray arr = doc["calib"].to<JsonArray>();
    for (int i = 0; i < config.calibCount; i++) {
        JsonObject item = arr.add<JsonObject>();
        item["v"] = config.calibTable[i].volt;
        item["l"] = config.calibTable[i].liter;
    }

    File file = LittleFS.open(CONFIG_FILE, "w");[cite: 8]
    if (!file) return;[cite: 8]
    
    serializeJsonPretty(doc, file);[cite: 8]
    file.close();[cite: 8]
}
