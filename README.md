# Camper Waterlevel – ESP32 Frischwasser Levelsensor

Präziser DIY-Frischwassersensor für Wohnmobile und Wohnwagen mit Votronic Tankgeber, interaktiver Web-UI, Echtzeit-Tankanzeige, Glättungsfilter und automatischer Home-Assistant-Anbindung über MQTT.

---

## Danksagung / Vorlage

Dieses Projekt basiert konzeptionell und strukturell auf dem Open-Source-Projekt [ESP32-Adrafruit-BNO085-HomeAssistant-Caravan-Levelsensor-MQTT](https://github.com/klehnst/ESP32-Adrafruit-BNO085-HomeAssistant-Caravan-Levelsensor-MQTT) von **klehnst** und wurde speziell für die Auswertung analoger Votronic-Wassersensoren (Volt -> Liter/Prozent) umgebaut.

---

## Hardware

- **MCU:** ESP32-C3 Super Mini (oder kompatibles ESP32 DevKit)
- **Sensor:** Votronic Tankgeber (Analoge Spannungsmessung)
- **Stromversorgung:** 5V VIN / Buck-Converter oder USB-C

## Pin-Belegung

| ESP32-C3 | Votronic Sensor | Funktion |
| --- | --- | --- |
| GPIO0 | Signal (Volt) | Analog-Eingang (ADC) |
| GND | GND | Gemeinsame Masse |

---

## Arduino IDE Setup

### Board

- **Board Manager URL:** `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- **Board:** ESP32C3 Dev Module
- **Flash Size:** 4MB
- **Partition Scheme:** Default 4MB with LittleFS

### Benötigte Libraries (Library Manager)

1. **PubSubClient** (by Nick O'Leary)
2. **ArduinoJson** (by Benoit Blanchon) – Version 7.x
3. **ESPAsyncWebServer** (ESP32Async-Fork)
4. **AsyncTCP** (ESP32Async-Fork)

---

## Features

- ✅ **Präzise Füllstandsmessung:** Liest die analoge Spannung des Votronic-Sensors am ADC-Pin (GPIO0) aus.
- ✅ **Multi-Punkt-Kalibrierung:** Lineare Interpolation von Spannungswerten zu echten Literangaben (0–100 Litern).
- ✅ **Glättungsfilter (Moving Average):** Filtert Spannungsschwankungen und Rauschen des Bordnetzes/Buck-Converters über 15 Messwerte.
- ✅ **WiFi Manager & AP Fallback:** Automatische Verbindung mit dem WLAN. Bei fehlendem Netz wird ein eigener Access Point (`Camper-Waterlevel-AP`) mit IP `192.168.4.1` gestartet.
- ✅ **MQTT & Home Assistant Auto-Discovery:** Automatische Erkennung in Home Assistant inkl. Last-Will-Entdeckung (`camper/waterlevel`).
- ✅ **Echtzeit Web-UI:** Integrierte responsive Web-Oberfläche mit visueller Tankanzeige via WebSocket.

---

## Tankvolumen & Spannungs-Kalibrierung anpassen

Die Umrechnung von der gemessenen Sensor-Spannung (Volt) in Liter erfolgt über eine Multi-Punkt-Kalibrierungstabelle mit linearer Interpolation in der Datei `sensor.cpp`.

### Kalibrierungstabelle ändern (`sensor.cpp`)

Öffne `sensor.cpp` und passe das Array `calibTable` an dein Sensor-Modell und dein Tankvolumen an:

```cpp
static const CalibPoint calibTable[] = {
    // Volt , Liter
    {0.130f,   0.0f},
    {0.137f,  10.0f},
    {0.973f,  20.0f},
    {1.040f,  30.0f},
    {1.350f,  40.0f},
    {1.515f,  50.0f},
    {1.770f,  60.0f},
    {2.178f,  80.0f},
    {2.386f, 100.0f}
};
```

## Home Assistant Integration

Das Modul meldet sich automatisch über MQTT Discovery in Home Assistant an und stellt folgende Entitäten bereit:

- **Sensoren:**
  - `Frischwasser Inhalt` (Liter)
  - `Frischwasser Prozent` (%)
  - `Tank Spannung` (Volt)
  - `WLAN Signal` (dBm)
- **Schalter:**
  - `Glättungsfilter` (Ein / Aus)

---

## MQTT Topics (Default Prefix: `camper/waterlevel`)

| Topic | Typ | Beschreibung |
| --- | --- | --- |
| `.../voltage` | Sensor | Aktuelle gefilterte Spannung in Volt |
| `.../liters` | Sensor | Berechneter Tankinhalt in Litern (0–100 L) |
| `.../percent` | Sensor | Füllstand in Prozent (0–100 %) |
| `.../rssi` | Sensor | WLAN Signalstärke (dBm) |
| `.../status` | LWT | Status des Moduls (`online` / `offline`) |
| `.../filter_active` | State | Status des Moving-Average-Filters (`ON` / `OFF`) |
| `.../cmd/filter/set` | Command | Filter umschalten (`ON` / `OFF` / `toggle`) |

---

## Erstbenutzung

1. **Flashen:** Code sowie den `data/`-Ordner (LittleFS Upload für die `index.html`) auf den ESP32 flashen.
2. **Erstverbindung:** Wenn noch kein WLAN konfiguriert ist, startet der ESP32 den Access Point `Camper-Waterlevel-AP` (Passwort: `camperlevel`).
3. **Konfiguration:** Verbinde dich mit dem Access Point, öffne `http://192.168.4.1` im Browser und trage im Tab **📶 WiFi** deine WLAN-Zugangsdaten sowie unter **⚙️ MQTT** deinen MQTT-Broker ein.
4. **Speichern & Neustart:** Nach dem Speichern startet das Modul neu und verbindet sich automatisch mit deinem Netzwerk.

---

## Projektstruktur

```text
.
├── data/                  – Dateisystem für LittleFS (enthält index.html)
├── config.h               – Systemkonfiguration & Datenstrukturen
├── mqtt_ha.h / .cpp       – MQTT Client & Home Assistant Auto-Discovery
├── sensor.h / .cpp        – ADC Spannungsmessung & Linear-Kalibrierung (Volt zu Liter)
├── webserver.h / .cpp     – Webserver, REST-API & WebSocket Broadcast
├── wifi_manager.h / .cpp  – WLAN-Verbindungssteuerung & AP-Fallback
└── camper.ino             – Hauptsketch (Setup & Hauptschleife)
