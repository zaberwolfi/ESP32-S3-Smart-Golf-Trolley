# ? OpenSource Smart-Golf-Trolley Upgrade (2x 200W)

Dieses Projekt beschreibt den vollständigen High-Power-Umbau eines herkömmlichen Golftrolleys zu einem intelligenten Transport-System. Das System nutzt ein duales ESP32-S3 Setup, eine dedizierte Signalaufteilung auf die Prozessorkerne (FreeRTOS-Multitasking), eine kabellose ESP-NOW Fernsteuerung sowie ein integriertes On-Board-Diagnosesystem (OBD) mit einem zukunftssicheren WLAN-Webinterface.

> **Status des Projekts (Herbst/Winter 2026):** Die gesamte Elektronik, Sensorik und Software wurde erfolgreich auf dem Breadboard aufgebaut und im Zusammenspiel ausgiebig getestet. Der mechanische Einbau in das Trolley-Chassis und die Montage der 3D-Druck-Gehäuse erfolgen im Laufe des Winters nach Saisonende.

---

## ? System-Features

* **Dual-Core Motorregelung (Unteres Motorteil - ESP32-S3):** 
  * **Core 0:** Läuft im strikten 20-ms-Takt exklusiv für die latenzfreie Motoransteuerung (PWM & Richtung) und den seriellen UART-Datenstrom.
  * **Core 1:** Übernimmt die präzise Hall-Sensor-Odometrie, führt die Batterieüberwachung durch und steuert die RGB-Status-LED.
* **Asynchrones Cockpit-Management (Oberes Bedienteil - ESP32-S3 Zero):**
  * **Core 0:** Verarbeitet exklusiv den hardwarenahen Wi-Fi/ESP-NOW Funkverkehr der Fernbedienung.
  * **Core 1 (Steuer-Task):** Arbeitet im deterministischen 40-ms-Takt, um Taster zu entprellen, den Gasgriff zu filtern, die Fahrlogik zu berechnen und UART-Pakete zu senden. Die trägere Display-Ausgabe läuft entkoppelt in der Hintergrund-Schleife, um den Sendetakt niemals zu verzögern.
* **Duales Bedienkonzept:** Intuitive analoge Fahrtenregelung über einen **Hominas 20X E-Bike-Drehgasgriff** am Lenker (direkt mit sicheren 3,3V betrieben) sowie eine kabellose, rastende Funk-Fernbedienung via **ESP-NOW** (Sender: Seeed Studio XIAO ESP32-C3 oder C3 SuperMini).
* **Zentrales Cockpit:** Kontrastreiches **2,42" SSD1309 OLED-Display** (über I2C mit beschleunigten 400 kHz) zur Anzeige von Akkuspannung, Motorleistung (Grafischer Balken), Modus und Runden-Strecke.
* **Komfort-Modi:** 4 physische Taster für Power (5s halten), 10-Sekunden-Timer (fährt eigenständig mit 30% Leistung vorwärts), Tempomat (Cruise Control friert aktuelle Gasgriff-Geschwindigkeit ein) und Fahrtrichtungsumschaltung.
* **On-Board-Diagnose (OBD) & Blackbox:** Permanente Fehlerüberwachung (UART-Kabelbrüche, System-Abstürze, Motor-Neustarts, Funk-Sicherheitsstopps und Unterspannungen). Ereignisse werden flüchtig im RAM gesammelt und im Hintergrund im nicht-flüchtigen Flash-Speicher (NVS Ringpuffer) abgelegt.
* **Drahtloses Log-Webinterface:** Bei ausgeschaltetem System lässt sich durch 3-sekündiges Drücken der silbernen Taste ein eigenständiges Wi-Fi-Netz (`Trolley-Log`) starten. Über die IP-Adresse `192.168.4.1` kann das vollständige Fehlerprotokoll plattformunabhängig direkt im Browser des Smartphones ausgelesen und gelöscht werden.

---

## ?? Technische Details & Pin-Belegung

### 1. Motorteil (Unten am Chassis) – ESP32-S3 (z. B. Waveshare S3 Zero)
Steuert zwei **Cytron MD13S** Treiber im Sign-Magnitude-Verfahren an, betrieben an einem **24V (7S5P) Akku**.
* **Motor 1 (LINKS):** PWM = `GPIO 6` | DIR = `GPIO 5`
* **Motor 2 (RECHTS):** PWM = `GPIO 8` | DIR = `GPIO 7`
* **Hall-Sensor Odometer (A3144):** `GPIO 3` (Nutzt internen `INPUT_PULLUP`)
* **Batterie-Messung (ADC):** `GPIO 4` (Spannungsteiler 100k / 10k)
* **Kabelverbindung nach oben:** `RX = GPIO 1` (Gelb, kommt von oben TX) | `TX = GPIO 2` (Weiß, geht nach oben RX)
* **On-Board Status-LED (WS2812):** `GPIO 21` (Blau = Warte auf Daten, Gelb = Ungültige Zeichen, Weiß = Datenstrom OK)
* **Fahrparameter:** Lautlose 20 kHz PWM-Frequenz (8-Bit). Sanftanlauf-Rampe (0 auf 100% in ~2,6s), Brems-Rampe (100% auf 0 in ~0,85s).

### 2. Bedienteil (Oben am Lenker) – ESP32-S3 Zero
* **Hominas Gasgriff (Analog-In):** `GPIO 1` (Eingemessen: Ruhebereich/Start = 880 | Vollgas = 2700)
* **Wippschalter (Richtung):** Vorwärts = `GPIO 4` | Rückwärts = `GPIO 5`
* **Taster Cockpit:** Tempomat (Schwarz) = `GPIO 6` | 10s-Timer (Grün) = `GPIO 7` | Power (Rot) = `GPIO 10` | Uhr/Log (Silber) = `GPIO 11`
* **SSD1309 OLED (I2C):** SDA = `GPIO 8` | SCL = `GPIO 9`
* **Kabelverbindung nach unten:** `RX = GPIO 12` (Weiß, kommt von unten TX) | `TX = GPIO 13` (Gelb, geht nach unten RX)

### 3. Fernbedienung (Sender) – ESP32-C3 Hardware-Optionen
Das System unterstützt per Software-Switch (`BOARD_XIAO`) zwei kompakte Formfaktoren. Die Tasten schalten direkt gegen GND.

#### Option A: Seeed Studio XIAO ESP32-C3 (`BOARD_XIAO = 1`)
* **Tasten D1–D5:** Vor = `GPIO 3` | Zurück = `GPIO 4` | Links = `GPIO 5` | Rechts = `GPIO 6` | Stop = `GPIO 7`
* **Status-LED:** `GPIO 10` (D10) über 2,2 kO gegen GND.
* **Sendeleistung:** Auf maximale 19,5 dBm kalibriert (optimiert für die externe IPEX-Antenne).
* **Power:** Direktbetrieb über LiPo-Akku an den rückseitigen `BAT+` / `BAT-` Pads inklusive USB-C Ladeschaltung.

#### Option B: ESP32-C3 SuperMini (`BOARD_XIAO = 0`)
* **Tasten:** Vor = `GPIO 0` | Zurück = `GPIO 1` | Links = `GPIO 3` | Rechts = `GPIO 4` | Stop = `GPIO 5`
* **Status-LED:** `GPIO 6` über 2,2 kO gegen GND. On-Board-LED = `GPIO 8`.
* **Sendeleistung:** Gedrosselt auf 8,5 dBm (verhindert HF-Einstreuungen in den Quarz nahe der internen Antenne; Reichweite dennoch über 50 Meter).
* **Power:** Versorgung über 3V3-Pin (470 µF Stützkondensator zwingend erforderlich).

---

## ?? Kommunikations- & Sicherheitsprotokoll

### 1. Datenstrom Kabel (UART @ 19200 Baud)
Die Übertragung erfolgt über ein ASCII-Protokoll, eingefasst in `<` und `>`:
* **Fahrbefehle nach unten:** `<0>` (System Aus) | `<1,richtung,prozent>` (Lenkerbetrieb synchron) | `<2,richtL,prozL,richtR,prozR>` (Funkbetrieb getrennt für Wenden auf der Stelle mit 40% Kurveninnen-Absenkung).
* **Telemetrie nach oben:** `<motor_ist%, akku_prozent, gelaufene_meter, volt_zehntel, laufzeit_sekunden, neustart_grund>`
* **Watchdog-Schutz:** Reißt die serielle Verbindung für mehr als 500 ms ab, lösen beide Funktionseinheiten sofort einen harten Not-Stopp ohne Rampe aus.

### 2. Funk-Sicherheit (ESP-NOW)
* **Auto-Discovery & Feste Bindung:** Beim Start sucht die Fernbedienung per Broadcast nach dem System. Sobald das Cockpit antwortet, wird die MAC-Adresse fixiert. Signale von fremden Trolleys werden blockiert.
* **Timeout-Halt:** Bleibt ein zyklisches Signal (Takt 50 ms bei Tastendruck) für mehr als 200 ms aus (z. B. Funkabriss), stoppt die Bewegung. Nach maximal 30 Sekunden ununterbrochener Funkfahrt greift ein automatischer Sicherheits-Fahrzeitstopp.
* **Echtzeit-Priorität:** Manuelle Bewegungen des Gasgriffs am Lenker brechen jeglichen Funkbetrieb oder Tempomaten augenblicklich ab.
* **Deep-Sleep:** Nach genau 5 Minuten ohne Tastendruck fällt die Fernbedienung in einen stromsparenden Light-Sleep und wacht bei jedem beliebigen Tastendruck verzögerungsfrei wieder auf.

---

## ?? Ordnerstruktur im Repository

* `/Code_Motorteil`: Quellcode für den unteren ESP32-S3 (Motorregelung & Core 0 FreeRTOS-Task).
* `/Code_Displayteil`: Quellcode für den ESP32-S3 Zero am Lenker (OLED, Webserver, Log-Verarbeitung, ESP-NOW Empfang).
* `/Code_Fernbedienung`: Quellcode für den XIAO ESP32-C3 / SuperMini Sender.
* `/Schaltplaene`: Technische Verdrahtungspläne inklusive Masseführung (GND-Kopplung) und Pegelprüfung.
* `/3D_Modelle_PETG`: STL-Druckdateien für die wetterfesten, vibrationsstabilen Gehäusekomponenten.
* `/Anleitungen`: Stückliste (BOM), Bedienungsanleitung sowie eine detaillierte Wartungs- und Reparaturanleitung.

---

## ?? Credits & Danksagung
* **Konzept, Systemarchitektur & Hardware-Design:** Komplett in Eigenregie entworfen und auf dem Breadboard validiert.
* **Software-Support:** Bei der strukturellen Code-Implementierung für die ESP32-S3/C3 Multi-Core-Prozesse, der Interrupt-Filterung und dem Daten-Handling stand **Claude (Anthropic AI)** als digitaler Entwicklungs-Assistent unterstützend zur Seite.
