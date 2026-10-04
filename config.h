#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================
// PIN DEFINITIONS & CONSTANTS
// ============================================================
#define ADC_PIN               0      // GPIO0 (ADC1_CH0) for the Votronic tank sender
#define ADC_SAMPLES           8      // Oversampling per measurement (reduces ADC noise)

#define DEFAULT_AP_SSID       "Camper-Waterlevel-AP"
#define DEFAULT_AP_PASSWORD   "camperlevel"   // Change before flashing!
#define DEFAULT_OTA_PASSWORD  "camper2026"    // Change before flashing!
#define DEFAULT_MQTT_SERVER   "192.168.8.1"
#define DEFAULT_MQTT_PORT     1883
#define DEFAULT_MQTT_PREFIX   "camper/waterlevel"

#define SENSOR_UPDATE_MS      1000   // Read sensor every second
#define MQTT_FAST_MS          1000   // Fast publish interval
#define MQTT_SLOW_MS          5000   // Normal publish interval
#define WS_UPDATE_MS          1000   // Web UI live update interval

#define CONFIG_FILE           "/config.json"
#define OTA_HOSTNAME          "camper-waterlevel"

#define MAX_CALIB_POINTS      15     // Max. number of calibration points

// ============================================================
// DYNAMIC CALIBRATION TABLE
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

    // Dynamic volt-to-liter table (sorted by voltage, ascending)
    CalibPoint calibTable[MAX_CALIB_POINTS];
    uint8_t calibCount;
};

// ============================================================
// SENSOR DATA STRUCT
// ============================================================
struct SensorData {
    float raw_voltage;     // Raw voltage from the ADC
    float avg_voltage;     // Filtered voltage
    float liters;          // Calculated content in liters
    float percent;         // Calculated content in % (0-100)
    bool sensor_ok;
    int rssi;
};

extern AppConfig config;
extern SensorData sensorData;

#endif // CONFIG_H
