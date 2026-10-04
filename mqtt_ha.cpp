#include "mqtt_ha.h"
#include "sensor.h"
#include "wifi_manager.h"
#include "webserver.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

static WiFiClient espClient;[cite: 3]
static PubSubClient mqtt(espClient);[cite: 3]
static unsigned long lastPublish = 0;[cite: 3]
static unsigned long lastReconnect = 0;[cite: 3]
static bool discoveryPublished = false;[cite: 3]

static void mqttCallback(char* topic, byte* payload, unsigned int length);[cite: 3]
static void publishSensor(const char* name, const char* id, const char* unit, 
                          const char* devClass, const char* icon, const char* valueTpl);[cite: 3]
static void publishSwitch(const char* name, const char* id, const char* icon, const char* cmdTopic);[cite: 3]
static void addDeviceBlock(JsonDocument& doc);[cite: 3]

void mqttInit() {
    if (strlen(config.mqtt_server) == 0) return;[cite: 3]
    
    mqtt.setServer(config.mqtt_server, config.mqtt_port);[cite: 3]
    mqtt.setCallback(mqttCallback);[cite: 3]
    mqtt.setBufferSize(1024);[cite: 3]
    mqtt.setKeepAlive(30);[cite: 3]
    mqtt.setSocketTimeout(10);[cite: 3]
    
    Serial.printf("[MQTT] Configured: %s:%d\n", config.mqtt_server, config.mqtt_port);[cite: 3]
}

void mqttLoop() {
    if (!isConnected()) return;[cite: 3]
    if (strlen(config.mqtt_server) == 0) return;[cite: 3]
    
    if (!mqtt.connected()) {[cite: 3]
        if (millis() - lastReconnect > 2000) {[cite: 3]
            lastReconnect = millis();[cite: 3]
            Serial.println("[MQTT] Connection attempt...");[cite: 3]
            
            String clientId = "camper-waterlevel-" + WiFi.macAddress();
            clientId.replace(":", "");
            bool connected;
            
            String willTopic = String(config.mqtt_prefix) + "/status";[cite: 3]
            
            if (strlen(config.mqtt_user) > 0) {[cite: 3]
                connected = mqtt.connect(clientId.c_str(), config.mqtt_user, config.mqtt_pass,
                    willTopic.c_str(), 0, true, "offline");[cite: 3]
            } else {
                connected = mqtt.connect(clientId.c_str(),
                    NULL, NULL, willTopic.c_str(), 0, true, "offline");[cite: 3]
            }
            
            if (connected) {[cite: 3]
                Serial.println("[MQTT] Connected!");[cite: 3]
                String cmdBase = String(config.mqtt_prefix) + "/cmd/#";[cite: 3]
                mqtt.subscribe(cmdBase.c_str());[cite: 3]
                
                if (!discoveryPublished) {[cite: 3]
                    mqttPublishDiscovery();[cite: 3]
                    discoveryPublished = true;[cite: 3]
                }
                
                String statusTopic = String(config.mqtt_prefix) + "/status";[cite: 3]
                mqtt.publish(statusTopic.c_str(), "online", true);[cite: 3]
            } else {
                Serial.printf("[MQTT] Failed, rc=%d\n", mqtt.state());[cite: 3]
            }
        }
        return;
    }
    
    mqtt.loop();[cite: 3]
    
    unsigned long mqttInterval = config.mqtt_fast ? MQTT_FAST_MS : MQTT_SLOW_MS;[cite: 3]
    if (millis() - lastPublish > mqttInterval) {[cite: 3]
        lastPublish = millis();[cite: 3]
        mqttPublishState();[cite: 3]
    }
}

bool mqttIsConnected() {
    return mqtt.connected();[cite: 3]
}

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);[cite: 3]
    String payloadStr;[cite: 3]
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];[cite: 3]
    }
    payloadStr.trim();[cite: 3]
    
    String cmdPrefix = String(config.mqtt_prefix) + "/cmd/";[cite: 3]
    
    if (topicStr == cmdPrefix + "filter/set") {[cite: 3]
        String fLower = payloadStr;[cite: 3]
        fLower.toLowerCase();[cite: 3]
        if (fLower == "toggle") {[cite: 3]
            config.filter_active = !config.filter_active;[cite: 3]
        } else if (fLower == "on" || fLower == "true" || fLower == "1") {[cite: 3]
            config.filter_active = true;[cite: 3]
        } else {
            config.filter_active = false;[cite: 3]
        }
        saveConfig();[cite: 3]
        String prefix = String(config.mqtt_prefix);[cite: 3]
        mqtt.publish((prefix + "/filter_active").c_str(), config.filter_active ? "ON" : "OFF", true);[cite: 3]
    }
}

void mqttPublishState() {
    if (!mqtt.connected()) return;[cite: 3]
    
    String prefix = String(config.mqtt_prefix);[cite: 3]
    char buf[16];[cite: 3]
    
    dtostrf(sensorData.avg_voltage, 1, 3, buf);
    mqtt.publish((prefix + "/voltage").c_str(), buf, true);
    
    dtostrf(sensorData.liters, 1, 1, buf);
    mqtt.publish((prefix + "/liters").c_str(), buf, true);
    
    dtostrf(sensorData.percent, 1, 0, buf);
    mqtt.publish((prefix + "/percent").c_str(), buf, true);
    
    itoa(sensorData.rssi, buf, 10);[cite: 3]
    mqtt.publish((prefix + "/rssi").c_str(), buf, true);[cite: 3]
    
    mqtt.publish((prefix + "/filter_active").c_str(), config.filter_active ? "ON" : "OFF", true);[cite: 3]
    mqtt.publish((prefix + "/status").c_str(), "online", true);[cite: 3]
}

void mqttPublishDiscovery() {
    Serial.println("[MQTT] Publishing HA Discovery...");[cite: 3]
    
    publishSensor("Tank Spannung", "voltage", "V", "voltage", "mdi:current-dc", "{{ value }}");
    publishSensor("Frischwasser Inhalt", "liters", "L", "volume", "mdi:water", "{{ value }}");
    publishSensor("Frischwasser Prozent", "percent", "%", "battery", "mdi:water-percent", "{{ value }}");
    publishSensor("WLAN Signal", "rssi", "dBm", "signal_strength", "mdi:wifi", "{{ value }}");[cite: 3]
    publishSensor("Status", "status", "", "None", "mdi:check-circle", "{{ value }}");[cite: 3]
    
    String cmdBase = String(config.mqtt_prefix) + "/cmd/";[cite: 3]
    publishSwitch("Glättungsfilter", "filter_active", "mdi:blur", (cmdBase + "filter/set").c_str());[cite: 3]
    
    Serial.println("[MQTT] Discovery complete");[cite: 3]
}

static void addDeviceBlock(JsonDocument& doc) {
    JsonObject device = doc["device"].to<JsonObject>();[cite: 3]
    device["identifiers"][0] = "camper_waterlevel_sensor";
    device["name"] = "Camper Wasserlevel Sensor";
    device["model"] = "ESP32 Water Sensor";
    device["manufacturer"] = "DIY";[cite: 3]
    device["sw_version"] = "1.0.0";[cite: 3]
    
    doc["availability_topic"] = String(config.mqtt_prefix) + "/status";[cite: 3]
    doc["payload_available"] = "online";[cite: 3]
    doc["payload_not_available"] = "offline";[cite: 3]
}

static void publishSensor(const char* name, const char* id, const char* unit,
                          const char* devClass, const char* icon, const char* valueTpl) {
    JsonDocument doc;[cite: 3]
    String uniqueId = String("camper_water_") + id;
    String stateTopic = String(config.mqtt_prefix) + "/" + id;[cite: 3]
    String configTopic = String("homeassistant/sensor/camper_water/") + id + "/config";
    
    doc["name"] = name;[cite: 3]
    doc["unique_id"] = uniqueId;[cite: 3]
    doc["state_topic"] = stateTopic;[cite: 3]
    doc["value_template"] = valueTpl;[cite: 3]
    doc["icon"] = icon;[cite: 3]
    
    if (strlen(unit) > 0) doc["unit_of_measurement"] = unit;[cite: 3]
    if (strcmp(devClass, "None") != 0) doc["device_class"] = devClass;[cite: 3]
    
    addDeviceBlock(doc);[cite: 3]
    
    char buffer[768];[cite: 3]
    serializeJson(doc, buffer);[cite: 3]
    mqtt.publish(configTopic.c_str(), buffer, true);[cite: 3]
}

static void publishSwitch(const char* name, const char* id, const char* icon,
                           const char* cmdTopic) {
    JsonDocument doc;[cite: 3]
    String uniqueId = String("camper_water_") + id + "_switch";
    String stateTopic = String(config.mqtt_prefix) + "/" + id;[cite: 3]
    String configTopic = String("homeassistant/switch/camper_water/") + id + "/config";
    
    doc["name"] = name;[cite: 3]
    doc["unique_id"] = uniqueId;[cite: 3]
    doc["state_topic"] = stateTopic;[cite: 3]
    doc["command_topic"] = cmdTopic;[cite: 3]
    doc["payload_on"] = "ON";[cite: 3]
    doc["payload_off"] = "OFF";[cite: 3]
    doc["icon"] = icon;[cite: 3]
    
    addDeviceBlock(doc);[cite: 3]
    
    char buffer[512];[cite: 3]
    serializeJson(doc, buffer);[cite: 3]
    mqtt.publish(configTopic.c_str(), buffer, true);[cite: 3]
}
