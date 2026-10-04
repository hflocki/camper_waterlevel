#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================
// PIN DEFINITIONEN & KONSTANTEN FÜR WASSERMESSUNG
// ============================================================
#define ADC_PIN               0      // GPIO0 für Votronic Sensor[cite: 1]
#define DEFAULT_AP_SSID       "Camper-Waterlevel-AP"
#define DEFAULT_AP_PASSWORD   "camperlevel"
#define DEFAULT_MQTT_SERVER   "192.168.8.1"[cite: 1]
#define DEFAULT_MQTT_PORT     1883
#define DEFAULT_MQTT_PREFIX   "camper/waterlevel"

#define SENSOR_UPDATE_MS      1000   // Sensor alle 1 Sekunde lesen
#define MQTT_FAST_MS          1000   // Fast Updates
#define MQTT_SLOW_MS          5000   // Dauerbetrieb (5 Sek.)[cite: 1]
#define WS_UPDATE_MS          1000   // Web UI Live-Interval

#define CONFIG_FILE           "/config.json"[cite: 2]
#define OTA_HOSTNAME          "camper-waterlevel"

#define MAX_CALIB_POINTS      15     // Max. 15 Matrix-Messpunkte

// ============================================================
// DYNAMISCHE KALIBRIERUNGSMATRIX
// ============================================================
struct CalibPoint {
    float volt;
    float liter;
};

// ============================================================
// CONFIGURATION STRUCT
// ============================================================
struct AppConfig {
    char wifi_ssid[64];
    char wifi_pass[64];
    char ap_ssid[32];
    char ap_pass[32];
    
    char mqtt_server[64];
    uint16_t mqtt_port;
    char mqtt_user[32];
    char mqtt_pass[32];
    char mqtt_prefix[32];
    
    bool filter_active;
    bool mqtt_fast;

    // Dynamische Volt-zu-Liter-Matrix
    CalibPoint calibTable[MAX_CALIB_POINTS];
    uint8_t calibCount;
};

// ============================================================
// SENSOR DATA STRUCT
// ============================================================
struct SensorData {
    float raw_voltage;     // Rohspannung vom ADC
    float avg_voltage;     // Gefilterte Spannung
    float liters;          // Berechneter Inhalt in Litern
    float percent;         // Berechneter Inhalt in % (0-100%)
    bool sensor_ok;
    int rssi;
};

extern AppConfig config;[cite: 2]
extern SensorData sensorData;[cite: 2]

#endif // CONFIG_H
