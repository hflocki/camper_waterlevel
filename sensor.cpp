#include "sensor.h"

static unsigned long lastSensorRead = 0;

// Moving-Average-Filter-Puffer
const int WINDOW_SIZE = 15;[cite: 1]
static float readings[WINDOW_SIZE];
static int readIndex = 0;
static float total = 0.0f;

// Kalibrierungstabelle (Volt -> Liter)[cite: 1]
struct CalibPoint {
    float volt;
    float liter;
};

static const CalibPoint calibTable[] = {
    {0.130f, 0.0f},
    {0.137f, 10.0f},
    {0.973f, 20.0f},
    {1.040f, 30.0f},
    {1.350f, 40.0f},
    {1.515f, 50.0f},
    {1.770f, 60.0f},
    {2.178f, 80.0f},
    {2.386f, 100.0f}
};
static const int calibSize = sizeof(calibTable) / sizeof(calibTable[0]);

// Lineare Interpolation (entspricht calibrate_linear)[cite: 1]
static float interpolateLiter(float volt) {
    if (volt <= calibTable[0].volt) return calibTable[0].liter;
    if (volt >= calibTable[calibSize - 1].volt) return calibTable[calibSize - 1].liter;

    for (int i = 0; i < calibSize - 1; i++) {
        if (volt >= calibTable[i].volt && volt <= calibTable[i + 1].volt) {
            float t = (volt - calibTable[i].volt) / (calibTable[i + 1].volt - calibTable[i].volt);
            return calibTable[i].liter + t * (calibTable[i + 1].liter - calibTable[i].liter);
        }
    }
    return 0.0f;
}

bool sensorInit() {
    Serial.println("[Sensor] Initialisiere Analogeingang (GPIO0)...");
    analogReadResolution(12); // 12-Bit Auflösung (0–4095)
    
    for (int i = 0; i < WINDOW_SIZE; i++) {
        readings[i] = 0.0f;
    }
    sensorData.sensor_ok = true;
    return true;
}

void sensorLoop() {
    if (millis() - lastSensorRead < SENSOR_UPDATE_MS) return;
    lastSensorRead = millis();

    // Raw-ADC lesen
    int raw = analogRead(ADC_PIN);
    
    // Nährungsumrechnung ADC-Value (12-bit, 3.3V Max) -> Volt
    float voltage = (raw / 4095.0f) * 3.3f; 
    sensorData.raw_voltage = voltage;

    // Moving Average Filter[cite: 1]
    total = total - readings[readIndex];
    readings[readIndex] = voltage;
    total = total + readings[readIndex];
    readIndex = (readIndex + 1) % WINDOW_SIZE;
    
    float avgVoltage = config.filter_active ? (total / WINDOW_SIZE) : voltage;
    sensorData.avg_voltage = avgVoltage;

    // Umrechnung Volt in Liter und Prozent[cite: 1]
    float liters = interpolateLiter(avgVoltage);
    sensorData.liters = liters;
    
    // Clamp 0.0 bis 100.0 Liter[cite: 1]
    if (sensorData.liters < 0.0f) sensorData.liters = 0.0f;
    if (sensorData.liters > 100.0f) sensorData.liters = 100.0f;

    sensorData.percent = (sensorData.liters / 100.0f) * 100.0f;
}