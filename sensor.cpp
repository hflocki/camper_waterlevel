#include "sensor.h"

static unsigned long lastSensorRead = 0;

// Moving-Average-Filter-Puffer
const int WINDOW_SIZE = 15;[cite: 1]
static float readings[WINDOW_SIZE];
static int readIndex = 0;
static float total = 0.0f;

// Lineare Interpolation der dynamischen Matrix aus config.calibTable
static float interpolateLiter(float volt) {
    if (config.calibCount == 0) return 0.0f;
    
    // Sortierter Puffer-Check: Unterer und oberer Randbereich
    if (volt <= config.calibTable[0].volt) return config.calibTable[0].liter;
    if (volt >= config.calibTable[config.calibCount - 1].volt) return config.calibTable[config.calibCount - 1].liter;

    for (int i = 0; i < config.calibCount - 1; i++) {
        if (volt >= config.calibTable[i].volt && volt <= config.calibTable[i + 1].volt) {
            float vDiff = config.calibTable[i + 1].volt - config.calibTable[i].volt;
            if (vDiff <= 0.0001f) return config.calibTable[i].liter; // Schutz vor Division durch 0
            
            float t = (volt - config.calibTable[i].volt) / vDiff;
            return config.calibTable[i].liter + t * (config.calibTable[i + 1].liter - config.calibTable[i].liter);
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

    // Umrechnung Volt in Liter und Prozent aus der dynamischen Tabelle
    float liters = interpolateLiter(avgVoltage);
    sensorData.liters = liters;
    if (sensorData.liters < 0.0f) sensorData.liters = 0.0f;

    // Max. Volumen entspricht dem höchsten Liter-Eintrag der Matrix
    float maxLiters = (config.calibCount > 0) ? config.calibTable[config.calibCount - 1].liter : 100.0f;
    if (maxLiters <= 0.0f) maxLiters = 100.0f;

    sensorData.percent = (sensorData.liters / maxLiters) * 100.0f;
    if (sensorData.percent > 100.0f) sensorData.percent = 100.0f;
}
