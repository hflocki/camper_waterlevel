# Camper Waterlevel – ESP32 Frischwasser-Levelsensor

Präziser DIY-Frischwassersensor für Wohnmobile und Wohnwagen mit Votronic Tankgeber, interaktiver Web-UI inklusive dynamischem Matrix-Editor, Echtzeit-Tankanzeige, Glättungsfilter und automatischer Home-Assistant-Anbindung über MQTT.

---

## Danksagung / Vorlage

Dieses Projekt basiert konzeptionell und strukturell auf dem Open-Source-Projekt [ESP32-Adrafruit-BNO085-HomeAssistant-Caravan-Levelsensor-MQTT](https://github.com/klehnst/ESP32-Adrafruit-BNO085-HomeAssistant-Caravan-Levelsensor-MQTT) von **klehnst** und wurde speziell für die Auswertung analoger Votronic-Wassersensoren (Volt -> Liter/Prozent) umgebaut.

---

## Hardware

- **MCU:** ESP32-C3 Super Mini (oder kompatibles ESP32 DevKit)
- **Sensor:** Votronic Tankgeber (analoge Spannungsmessung)
- **Stromversorgung:** 5V VIN / Buck-Converter oder USB-C

### Pin-Belegung

| ESP32-C3 | Votronic Sensor | Funktion             |
| -------- | --------------- | -------------------- |
| GPIO0    | Signal (Volt)   | Analog-Eingang (ADC) |
| GND      | GND             | Gemeinsame Masse     |

### ⚠️ Messbereich des ADC

Der ADC des ESP32-C3 wird mit 11 dB Dämpfung betrieben und misst etwa **0 – 2,5 V** zuverlässig. Die Spannung wird über `analogReadMilliVolts()` mit der werkseitigen Chip-Kalibrierung ausgelesen. Der Standardwert der Matrix (bis 2,386 V) liegt knapp unter dieser Grenze. Der Eingang verträgt maximal 3,3 V. Kann der Tankgeber mehr liefern, ist ein Spannungsteiler nötig und die Matrix muss mit den geteilten Werten kalibriert werden.

---

## Arduino IDE Setup

### Board

- **Board Manager URL:** `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
- **Board:** ESP32C3 Dev Module
- **Flash Size:** 4MB
- **Partition Scheme:** Default 4MB with spiffs (1.2MB APP / 1.5MB SPIFFS) – diese Partition wird von LittleFS verwendet und bietet Platz für OTA
- **USB CDC On Boot:** Enabled (nötig für die Serial-Ausgabe beim ESP32-C3 Super Mini)

### Benötigte Libraries (Library Manager)

1. **PubSubClient** (by Nick O'Leary)
2. **ArduinoJson** (by Benoit Blanchon) – Version 7.x
3. **ESPAsyncWebServer** (ESP32Async-Fork)
4. **AsyncTCP** (ESP32Async-Fork)

### Dateisystem (`data/`-Ordner) hochladen

Die Arduino IDE 2.x hat keinen eingebauten LittleFS-Upload. Dafür wird das Plugin [arduino-littlefs-upload](https://github.com/earlephilhower/arduino-littlefs-upload) benötigt. Alternativ funktioniert PlatformIO mit `pio run -t uploadfs`.

---

## Features

- ✅ **Präzise Füllstandsmessung:** Liest die analoge Spannung des Votronic-Sensors am ADC-Pin (GPIO0) aus, 8-fach gemittelt pro Messung.
- ✅ **Dynamische Multi-Punkt-Kalibrierung:** Die Volt-zu-Liter-Matrix lässt sich direkt über die Web-UI (Tab `📊 Matrix`) anpassen, erweitern oder löschen – **kein Neu-Flashen nötig!**
- ✅ **Glättungsfilter (Moving Average):** Filtert Spannungsschwankungen und Rauschen des Bordnetzes/Buck-Converters über 15 Messwerte. Der Filter wird beim Start mit dem ersten Messwert vorbelegt, es gibt also keinen Anlauf von 0.
- ✅ **WiFi Manager & AP Fallback:** Automatische Verbindung mit dem WLAN. Bei fehlendem Netz wird ein eigener Access Point (`Camper-Waterlevel-AP`) mit IP `192.168.4.1` gestartet.
- ✅ **MQTT & Home Assistant Auto-Discovery:** Automatische Erkennung in Home Assistant inkl. Last Will (`camper/waterlevel/status`).
- ✅ **Echtzeit Web-UI:** Integrierte responsive Web-Oberfläche mit visueller Tankanzeige via WebSocket.
- ✅ **OTA-Updates:** Firmware-Update über WLAN (Hostname `camper-waterlevel`).

---

## Volt-zu-Liter Kalibrierungsmatrix (Web-UI)

Die Füllstandsberechnung nutzt eine Tabelle aus Spannungswerten (Volt) und dem zugehörigen Tankinhalt (Liter). Die Zwischenwerte werden automatisch linear interpoliert. Es sind maximal 15 Punkte möglich, mindestens 2 werden benötigt.

1. Öffne die Web-UI im Browser oder auf einem Android-Autoradio.
2. Gehe auf den Tab **📊 Matrix**.
3. Du kannst bestehende Punkte anpassen, mit **+ Punkt hinzufügen** neue Messpunkte anlegen oder Einträge löschen.
4. Nach dem Klick auf **💾 Matrix Speichern** wird die Tabelle nach Spannung sortiert, vom Gerät geprüft und dauerhaft in der `config.json` auf dem ESP32 (LittleFS) abgelegt.

> **Werkseinstellung:** Standardmäßig sind 9 Votronic-Referenzpunkte hinterlegt (0.130 V = 0 L bis 2.386 V = 100 L).

---

## Home Assistant Integration

Das Modul meldet sich automatisch über MQTT Discovery in Home Assistant an und stellt folgende Entitäten bereit:

- **Sensoren:**
  * `Frischwasser Inhalt` (Liter)
  * `Frischwasser Prozent` (%)
  * `Tank Spannung` (Volt)
  * `WLAN Signal` (dBm)
  * `Status` (online / offline)
- **Schalter:**
  * `Glättungsfilter` (Ein / Aus)

---

## MQTT Topics (Default Prefix: `camper/waterlevel`)

| Topic                | Typ     | Beschreibung                                    |
| -------------------- | ------- | ----------------------------------------------- |
| `.../voltage`        | Sensor  | Aktuelle gefilterte Spannung in Volt            |
| `.../liters`         | Sensor  | Berechneter Tankinhalt in Litern                |
| `.../percent`        | Sensor  | Füllstand in Prozent (0–100 %)                  |
| `.../rssi`           | Sensor  | WLAN Signalstärke (dBm)                         |
| `.../status`         | LWT     | Status des Moduls (`online` / `offline`)        |
| `.../filter_active`  | State   | Status des Moving-Average-Filters (`ON` / `OFF`) |
| `.../cmd/filter/set` | Command | Filter umschalten (`ON` / `OFF` / `toggle`)     |

---

## Erstbenutzung

1. **Passwörter anpassen:** In `config.h` `DEFAULT_AP_PASSWORD` und `DEFAULT_OTA_PASSWORD` vor dem Flashen ändern.
2. **Flashen:** Code (`camper.ino`) sowie den `data/`-Ordner (LittleFS-Upload für die `index.html`) auf den ESP32 flashen.
3. **Erstverbindung:** Wenn noch kein WLAN konfiguriert ist, startet der ESP32 den Access Point `Camper-Waterlevel-AP` (Standard-Passwort: `camperlevel`).
4. **Konfiguration:** Verbinde dich mit dem Access Point, öffne `http://192.168.4.1` im Browser und trage im Tab **📶 WiFi** deine WLAN-Zugangsdaten sowie unter **⚙️ MQTT** deinen MQTT-Broker ein.
5. **Speichern & Neustart:** Nach dem Speichern startet das Modul neu und verbindet sich automatisch mit deinem Netzwerk.

> Gespeicherte Passwörter werden nie an den Browser zurückgesendet. Ein leeres Passwortfeld bedeutet „unverändert lassen“.

---

## Serielle Befehle (115200 Baud)

| Befehl   | Funktion                          |
| -------- | --------------------------------- |
| `read`   | Aktuelle Messwerte ausgeben       |
| `filter` | Glättungsfilter umschalten        |
| `help`   | Befehlsliste                      |

---

## Projektstruktur

```
.
├── data/
│   └── index.html         – Responsive Web-Oberfläche mit Matrix-Editor (LittleFS)
├── config.h               – Systemkonfiguration & Datenstrukturen
├── mqtt_ha.h / .cpp       – MQTT Client & Home Assistant Auto-Discovery
├── sensor.h / .cpp        – ADC Spannungsmessung & dynamische Interpolation
├── webserver.h / .cpp     – Webserver, REST-API, WebSocket Broadcast & Config-Speicher
├── wifi_manager.h / .cpp  – WLAN-Verbindungssteuerung & AP-Fallback
└── camper.ino             – Hauptsketch (Setup & Hauptschleife)
```
