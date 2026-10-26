// ============================================================================
//  BEDIENTEIL (ESP32-S3 Zero) – Golftrolley-Steuerung am Lenker
//  Board in der Arduino IDE: "ESP32S3 Dev Module", dazu einstellen:
//    USB CDC On Boot: Enabled
//    Flash Size:      4MB (32Mb)
//    Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
//    PSRAM:           QSPI PSRAM
// ----------------------------------------------------------------------------
//  Letzte Änderung: 26.10.2026 (b)
//
//  Änderungsverlauf:
//  26.10.2026b Gasgriff am S3 Zero neu eingemessen (gemessen: los 804–825,
//              voll 2747–2846): POTI_START 1000 -> 880, POTI_ENDE 2850 -> 2700,
//              TEMPOMAT_ABBRUCH_ROH 1650 -> 1520 (gleicher Anteil ~35 %).
//  26.10.2026  Umzug vom ESP32-S3 N16R8 auf den ESP32-S3 Zero (Waveshare).
//              Alle Pins liegen an den Seitenleisten (GPIO 1–13):
//              - Taste Rot   15 -> 10
//              - Taste Silber 16 -> 11
//              - Kabel RX    17 -> 12 (WEISS, vom Motorteil)
//              - Kabel TX    18 -> 13 (GELB,  zum Motorteil)
//              Alles andere bleibt: Gasgriff 1, Wippe 4/5, Schwarz 6,
//              Grün 7, Display SDA 8 / SCL 9.
//              - Status-LED (GPIO 21) entfällt, das Display zeigt
//                "Kabel:ERR" bzw. "ERR". GPIO 21 ist beim Zero die
//                eingebaute bunte LED, sie wird beim Start ausgeschaltet.
//              - Die alten JDY-40-Funkeingänge 38–42 sind entfernt,
//                die Fernsteuerung kommt nur noch per ESP-NOW.
//              - Motorteil bleibt unverändert.
//  25.10.2026  Versionsanzeige:
//              - Beim Start zeigt das Display 2 s lang "Version 25.10.2026".
//              - Serieller Monitor (115200 Baud): beim Start
//                "Bedienteil Version ...", jederzeit "v" senden -> Version von
//                Bedienteil UND Motorteil.
//              - Das Motorteil (ab Stand 25.10.2026) meldet seine Version alle
//                2 s mit einem Extra-Paket <V,...>. Sie steht auch oben auf der
//                Protokoll-Webseite. Ältere Motorteile: "unbekannt".
//  25.10.2026  Fernsteuerung fest koppeln: Nach dem Einschalten des Systems hört
//              das Bedienteil nur noch auf die erste Fernsteuerung, die sich
//              meldet (Hardware-Adresse). Andere Sender werden bis zum
//              nächsten Einschalten ignoriert. Ersatz-Fernsteuerung: System
//              aus- und wieder einschalten.
//  25.10.2026  Mit DEBUG_FUNK 1 zeigt der Serielle Monitor alle 2 s die
//              Signalstärke der Fernsteuerung (dBm) und die Anzahl Pakete –
//              zum Vergleich Platinen- gegen externe Antenne.
//  24.10.2026  Fernsteuerung per ESP-NOW (Sender: ESP32-C3 SuperMini mit
//              Fernsteuerung_C3_ESPNOW.ino). Das Bedienteil empfängt die Tasten
//              direkt mit seinem eingebauten Funk, ein JDY-40 ist nicht mehr
//              nötig. Die Eingänge 38–42 bleiben zusätzlich aktiv (JDY-40
//              geht weiterhin, unbeschaltet stören sie nicht).
//              - Taste gilt als losgelassen, wenn 200 ms kein Paket kommt.
//              - Kopplung automatisch über KANAL und FUNK_KENNUNG (gleich wie
//                in der Fernsteuerung).
//              - Steuer-Task läuft jetzt auf Core 1 (der Funk arbeitet auf
//                Core 0 und kann die Steuerung so nicht aufhalten).
//              - Protokoll-WLAN (silberne Taste) läuft auf demselben Kanal,
//                die Fernsteuerung bleibt dabei empfangsbereit.
//  22.10.2026  Protokoll (LOG) im Flash, bleibt auch ohne Strom erhalten:
//              Neustarts mit Grund (z. B. Spannungseinbruch, Absturz), Ein/Aus,
//              Kabel getrennt/wieder da, Neustart des Motorteils, Funk-
//              Sicherheitsstopp, Funkfehler, Akku niedrig.
//              Auslesen über USB, Serieller Monitor 115200 Baud: "l" = anzeigen,
//              "c" = löschen. Nach einem ungewollten Neustart zeigt das Display
//              beim Einschalten 8 s lang den Grund (z. B. "Neust:Spg").
//              Motorteil meldet jetzt zusätzlich Laufzeit und Neustart-Grund.
//  22.10.2026  Protokoll per Handy lesen: Bei AUSGESCHALTETEM System die silberne
//              Taste 3 s halten -> eigenes WLAN "Trolley-Log" (Passwort golf1234),
//              im Browser 192.168.4.1 öffnen. Display zeigt die Zugangsdaten.
//              WLAN geht nach 10 min oder beim Einschalten des Systems wieder aus;
//              im normalen Fahrbetrieb bleibt das WLAN immer aus.
//  22.10.2026  Funk-Eingänge auf die andere Stiftleiste verlegt (alle nebeneinander):
//              Vor 10->38, Zurück 11->39, Links 12->40, Rechts 13->41, Stop 14->42.
//              (35–37 sind beim N16R8 vom PSRAM belegt und dürfen nicht benutzt werden.)
//  14.10.2026  Funk auf JDY-40 umgestellt (die JDY-41 antworteten nicht).
//              Empfänger-Modus C4: Ausgang in Ruhe LOW, HIGH solange die Taste
//              am Sender gedrückt ist -> FUNK_AKTIV_PEGEL = HIGH, Eingänge mit
//              Pull-down (Empfänger stromlos / Kabelbruch = "nicht gedrückt").
//  13.10.2026  Kabel zum Motorteil im Test bestätigt: RX = 17, TX = 18.
//              Am Bedienteil: GPIO 17 = weiß (vom Motorteil), GPIO 18 = gelb (zum Motorteil).
//              Bunte Board-LED (GPIO 48) beim Start ausschalten.
//  13.10.2026  Gasgriff am S3 neu eingemessen (gemessen: los 929–946, voll 2900–3089):
//              POTI_START 1360 -> 1000, POTI_ENDE 3470 -> 2850,
//              TEMPOMAT_ABBRUCH_ROH 2100 -> 1650 (gleicher Anteil ~35 % wie vorher).
//              DEBUG_POTI wieder aus.
//  12.10.2026  Variablen timerStart/timerAktiv umbenannt in zehnSekStart/
//              zehnSekAktiv: "timerStart" ist im ESP32-Paket 3.x/4.x bereits
//              eine Funktion -> Kompilierfehler "redeclared".
//  10.10.2026  Datei umbenannt in Bedien_S3_MotorS3 (vorher Bedienteil_S3_optimiert).
//  10.10.2026  Fernsteuerung rastend statt "nur solange gedrückt":
//              - Vor/Zurück: fährt im Wohlfühltempo los, bis Stop gedrückt wird
//              - gleiche Richtung nochmal drücken = Sicherheitszeit verlängern,
//                Gegenrichtung drücken = Stopp
//              - Links/Rechts während der Fahrt: jeweiliges Rad 3 s langsamer,
//                nochmal drücken = Kurve verlängern
//              - Sicherheit: Stopp nach FUNK_MAX_FAHRZEIT_MS ohne erneuten
//                Tastendruck (Funkabriss/leere Batterie im Sender), außerdem
//                Stopp bei Gasgriff, Wippe nicht auf STOP oder Kabelfehler
//  09.10.2026  Funk an JDY-41-Datenblatt angepasst: Empfänger-Ausgang ist
//              bei gedrückter Taste LOW (Modus C2/C5) -> FUNK_AKTIV_PEGEL LOW,
//              Eingänge mit Pull-up (Kabelbruch = nicht gedrückt).
//              Alle 5 Eingänge gleichzeitig LOW = Empfänger stromlos/defekt:
//              Funk wird dann komplett ignoriert, Lenker bleibt bedienbar.
//  09.10.2026  Aufteilung auf beide Kerne:
//              - Steuerung + Kommunikation in eigenem Task auf Core 0,
//                hohe Priorität, exakter 40-ms-Takt (vTaskDelayUntil)
//              - Display in loop() auf Core 1; die Übertragung zum Display
//                (ca. 25–30 ms) verzögert den Sendetakt nicht mehr
//              - Anzeigewerte werden als Momentaufnahme unter Sperre übergeben
//  08.10.2026  Umzug von ESP32-C3 Zero auf ESP32-S3, neue Pinbelegung
//              Funk-Fernsteuerung über JDY-41 (IO-Modus, 5 Tasten):
//              - nur aktiv, wenn der Richtungsschalter auf Stop steht
//              - fahren nur solange Taste gedrückt, feste langsame Tempi
//              - Halt-Taste wirkt immer (auch gegen Lenker, Tempomat, Timer)
//              - maximale Fahrzeit am Stück, danach erst loslassen
//              - widersprüchliche Tasten (vor+zurück, links+rechts) = Stopp
//              - neues Paket <2,richtungL,prozentL,richtungR,prozentR>
//  08.10.2026  Überarbeitung auf Basis der Version vom 05.10.2026:
//              - Timer-Abbruch repariert (setzte falsche Variable, sendete
//                bei Rückwärts trotzdem vorwärts)
//              - Tempomat aus bei Neutral, Richtungswechsel, Verbindungsverlust
//              - Silber-Taste unterbricht die Fahrpakete nicht mehr
//              - Rote Taste schaltet nur einmal pro Tastendruck um
//              - kein delay(), keine String-Objekte, Poti-Mittelung
//  05.10.2026  Ausgangsversion (ESP32-C3 Zero)
// ============================================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Preferences.h>
#include <WebServer.h>
#include <esp_system.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ============================================================================
//  VERSION  (bei jeder Änderung hier UND oben im Änderungsverlauf eintragen)
// ============================================================================
constexpr char PROGRAMM_VERSION[] = "26.10.2026b";
char motorVersion[12] = "";   // vom Motorteil gemeldet, leer = noch unbekannt

// ============================================================================
//  DEBUG
// ============================================================================
#define DEBUG_POTI 0   // 1 = Potiwerte alle 40 ms auf den USB-Seriellmonitor
#define DEBUG_FUNK 1   // 1 = Funk-Eingänge bei jeder Änderung ausgeben

// ============================================================================
//  DISPLAY
// ============================================================================
constexpr uint8_t SCREEN_WIDTH  = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ============================================================================
//  PIN-MATRIX ESP32-S3 Zero (nur Seitenleisten, GPIO 1–13)
//  Gemieden: 0 (BOOT), 3 (Startpin), 43/44 (Startmeldung beim Booten),
//            21 (eingebaute RGB-LED). Frei als Reserve: 2, 43, 44 und die
//            Pads auf der Unterseite (14–18, 38–42; 45 meiden).
// ============================================================================
constexpr uint8_t POTI_PIN     = 1;    // Gasgriff (ADC1)
constexpr uint8_t SW_FORWARD   = 4;
constexpr uint8_t SW_BACKWARD  = 5;

constexpr uint8_t BTN_SCHWARZ  = 6;    // Tempomat
constexpr uint8_t BTN_GRUEN    = 7;    // 10-s-Timer
constexpr uint8_t BTN_ROT      = 10;   // Ein/Aus (5 s halten)
constexpr uint8_t BTN_SILBER   = 11;   // Uhr Start/Stop, 3 s = Reset

constexpr uint8_t I2C_SDA      = 8;
constexpr uint8_t I2C_SCL      = 9;

constexpr uint8_t MOTOR_RX     = 12;   // WEISS – Kabel vom Motorteil (dessen TX)
constexpr uint8_t MOTOR_TX     = 13;   // GELB  – Kabel zum Motorteil (dessen RX)

// ============================================================================
//  ZEITEN & PARAMETER  (alles an einer Stelle einstellbar)
// ============================================================================
constexpr uint32_t SENDE_TAKT_MS          = 40;
constexpr uint32_t ANZEIGE_TAKT_MS        = 500;
constexpr uint32_t AUS_SENDE_TAKT_MS      = 100;
constexpr uint32_t VERBINDUNG_TIMEOUT_MS  = 500;
constexpr uint32_t POWER_HALTEZEIT_MS     = 5000;
constexpr uint32_t SILBER_LANG_MS         = 3000;
constexpr uint32_t TEMPOMAT_SPERRZEIT_MS  = 3000;  // so lange kann der Griff den Tempomat nicht beenden
constexpr uint32_t TIMER_DAUER_MS         = 10000;
constexpr uint32_t GELOESCHT_ANZEIGE_MS   = 1500;

constexpr int   POTI_START             = 880;    // Rohwert = 0 %   (S3 Zero: losgelassen ca. 805–825)
constexpr int   POTI_ENDE              = 2700;   // Rohwert = 100 % (S3 Zero: voll ca. 2750–2850)
constexpr float POTI_FILTER            = 0.05f;  // Glättung (kleiner = träger)
constexpr int   POTI_MITTELUNG         = 4;      // ADC-Messungen pro Takt
constexpr int   TEMPOMAT_ABBRUCH_ROH   = 1520;   // Griff über diesem Rohwert beendet Tempomat
constexpr int   TEMPOMAT_MIN_PROZENT   = 5;
constexpr int   TIMER_MAX_POTI_PROZENT = 15;     // Start nur unter diesem Wert, darüber Abbruch
constexpr int   TIMER_GESCHWINDIGKEIT  = 30;
constexpr int   GRIFF_FREI_PROZENT     = 3;      // Griff gilt als losgelassen

// --- Funk-Fernsteuerung (nur ESP-NOW) ---
constexpr int      FUNK_VOR_PROZENT        = 25;     // Wohlfühltempo vorwärts
constexpr int      FUNK_ZURUECK_PROZENT    = 20;     // Tempo rückwärts
constexpr int      FUNK_KURVE_INNEN_ANTEIL = 40;     // Kurvenrad: % der Fahrgeschwindigkeit
constexpr uint32_t FUNK_KURVE_MS           = 3000;   // so lange dreht das Rad langsamer
// SICHERHEIT: Der JDY-40 im Fernbedienungs-Modus meldet keinen Funkabriss. Kommt "Stop"
// nicht an (zu weit weg, Senderbatterie leer), hält der Trolley spätestens
// nach dieser Zeit an. Vor/Zurück erneut drücken startet die Zeit neu.
constexpr uint32_t FUNK_MAX_FAHRZEIT_MS    = 30000;
constexpr uint32_t FUNK_ZEIT_ANZEIGE_MS    = 3000;   // "F-Zeit" so lange anzeigen

// ============================================================================
//  TASTER MIT FLANKENERKENNUNG
//  (wird nur im 40-ms-Takt gelesen -> wirkt gleichzeitig als Entprellung)
// ============================================================================
struct Taster {
  uint8_t pin;
  bool aktuell = false;
  bool vorher  = false;

  void lesen()           { vorher = aktuell; aktuell = (digitalRead(pin) == LOW); }
  void synchronisieren() { aktuell = vorher = (digitalRead(pin) == LOW); }
  bool gedrueckt()    const { return aktuell; }
  bool neuGedrueckt() const { return aktuell && !vorher; }
  bool losgelassen()  const { return !aktuell && vorher; }
};

Taster tasterSchwarz{BTN_SCHWARZ};
Taster tasterGruen{BTN_GRUEN};
Taster tasterSilber{BTN_SILBER};

// ============================================================================
//  FUNK PER ESP-NOW (Fernsteuerung mit ESP32-C3)
//  Der Empfang läuft im Funk-Task (Core 0); hier wird nur der letzte
//  Tastenzustand mit Uhrzeit abgelegt. Die Steuerung liest ihn im 40-ms-Takt.
// ============================================================================
constexpr uint8_t  FUNK_KANAL      = 1;        // gleich wie in der Fernsteuerung!
constexpr uint16_t FUNK_KENNUNG    = 0x4754;   // gleich wie in der Fernsteuerung!
constexpr uint32_t FUNK_HALTE_MS   = 200;      // so lange gilt ein Tastenpaket

struct __attribute__((packed)) FunkPaket {
  uint8_t  m1, m2;          // 'G', 'T'
  uint16_t kennung;
  uint8_t  tasten;          // Bit 0 Vor, 1 Zurück, 2 Links, 3 Rechts, 4 Stop; 0xFF = Antwort
  uint8_t  zaehler;
};

portMUX_TYPE      espMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint8_t  espTasten = 0;
volatile uint32_t espZeit   = 0;
volatile bool     espAntworten = false;   // Fernsteuerung sucht -> antworten
uint8_t           espMac[6];
volatile int8_t   espRssi   = 0;        // Signalstärke des letzten Pakets (dBm)
volatile uint16_t espPakete = 0;        // Zähler für die Anzeige
bool              espGebunden = false;    // true = nur noch diese Fernsteuerung
uint8_t           espGebundenMac[6];

// Die Funktionen dazu stehen weiter unten (vor HILFSFUNKTIONEN), damit die
// Arduino-IDE ihre automatischen Prototypen an der richtigen Stelle einfügt.
bool espGedrueckt(uint8_t bit);

// ============================================================================
//  FUNK-EINGANG
//  Ein Zustand gilt erst, wenn er zwei Takte (80 ms) gleich gelesen wurde.
//  Quelle ist nur noch das ESP-NOW-Paket der Fernsteuerung.
// ============================================================================
struct FunkEingang {
  uint8_t bit;               // Bitnummer im ESP-NOW-Paket
  bool roh    = false;
  bool stabil = false;
  bool vorher = false;

  void lesen() {
    vorher = stabil;
    const bool neu = espGedrueckt(bit);
    if (neu == roh) stabil = neu;
    roh = neu;
  }
  bool neuGedrueckt() const { return stabil && !vorher; }
  void zuruecksetzen() { roh = stabil = vorher = false; }
};

FunkEingang funkVor{0};
FunkEingang funkZurueck{1};
FunkEingang funkLinks{2};
FunkEingang funkRechts{3};
FunkEingang funkHalt{4};

struct RadBefehl {
  uint8_t richtung;   // 0 = steht, 1 = vorwärts, 2 = rückwärts
  int     prozent;
};

// ============================================================================
//  ZUSTAND
// ============================================================================
bool systemAktiv = false;

// Empfangene Werte
int      motorIstProzent = 0;
int      batterieProzent = 0;
long     gelaufeneMeter  = 0;
float    realeVoltzahl   = 0.0f;
bool     verbindungOk    = false;
uint32_t letztesSignal   = 0;

// Fahrlogik Lenker
uint8_t  aktuelleRichtung        = 0;   // Stellung des Richtungsschalters
float    potiGeglaettet          = POTI_START;
bool     tempomatAktiv           = false;
int      tempomatGeschwindigkeit = 0;
uint8_t  tempomatRichtung        = 0;
uint32_t tempomatStart           = 0;
bool     zehnSekAktiv              = false;
uint32_t zehnSekStart              = 0;

// Funk
bool        haltAktiv      = false;   // Halt gedrückt oder Sperre danach
enum FunkModus : uint8_t { FUNK_AUS, FUNK_VOR_FAHRT, FUNK_ZURUECK_FAHRT };
FunkModus   funkModus      = FUNK_AUS;
bool        funkGesperrt   = false;   // nach dem Einschalten: erst alle Tasten loslassen
uint32_t    funkFahrStart  = 0;
int8_t      funkKurve      = 0;       // -1 = links langsamer, +1 = rechts langsamer
uint32_t    funkKurveEnde  = 0;
uint32_t    funkZeitAnzeigeBis = 0;
const char* funkAnzeige    = nullptr; // Statustext für das Display

// Stoppuhr
bool     uhrLaeuft            = false;
uint32_t uhrGesamtSekunden    = 0;
uint32_t uhrLetzteSekunde     = 0;
uint32_t silberGedruecktSeit  = 0;
bool     silberLangAusgeloest = false;
uint32_t geloeschtTextStart   = 0;
bool     geloeschtTextAktiv   = false;

// Taktgeber
uint32_t letztesAusPaket  = 0;

// ============================================================================
//  ÜBERGABE STEUER-TASK (Core 0) -> ANZEIGE (Core 1)
//  Nur der Steuer-Task schreibt, nur loop() liest – immer komplett unter Sperre.
// ============================================================================
struct AnzeigeDaten {
  bool        systemAktiv;
  bool        verbindungOk;
  float       volt;
  int         motorProzent;
  long        meter;
  uint8_t     richtung;
  bool        tempomat;
  bool        timer;
  const char* funkText;        // zeigt immer auf feste Texte -> sicher kopierbar
  bool        uhrLaeuft;
  uint32_t    uhrSekunden;
  bool        geloeschtText;
};

portMUX_TYPE anzeigeMux = portMUX_INITIALIZER_UNLOCKED;
AnzeigeDaten anzeige    = {};

constexpr uint32_t   STEUER_STACK     = 6144;
constexpr UBaseType_t STEUER_PRIO     = 3;    // höher als loop() (Priorität 1)
constexpr BaseType_t STEUER_KERN      = 1;    // 24.10.2026: Core 1, Funk läuft auf Core 0
TaskHandle_t         SteuerTaskHandle = nullptr;

// ============================================================================
//  PROTOKOLL (LOG) – dauerhaft im Flash (NVS), überlebt das Ausschalten
//  Der Steuer-Task (Core 0) legt Ereignisse nur in eine kleine Warteschlange;
//  geschrieben wird in loop() (Core 1), damit der 40-ms-Takt nicht wartet.
//  Zeit = Sekunden seit dem Start des Bedienteils in der jeweiligen Sitzung.
// ============================================================================
enum LogTyp : uint8_t { LOG_START = 1, LOG_EIN, LOG_AUS, LOG_KABEL_WEG, LOG_KABEL_OK,
                        LOG_MOTOR_NEUSTART, LOG_FUNK_ZEIT, LOG_FUNK_FEHL, LOG_AKKU_NIEDRIG };
struct LogEintrag {
  uint16_t sitzung;
  uint16_t wert;
  uint32_t sekunde;
  uint8_t  typ;
  uint8_t  frei[3];
};
constexpr int   LOG_GROESSE      = 120;     // Einträge im Flash (Ringpuffer)
constexpr int   LOG_WARTE        = 16;      // Warteschlange RAM
constexpr int   AKKU_WARN_ZEHNTEL = 235;    // unter 23,5 V einmal "Akku niedrig" protokollieren

Preferences  speicher;
LogEintrag   logPuffer[LOG_GROESSE];       // nur loop()/setup() greifen zu
uint16_t     logPos = 0, logAnzahl = 0;
uint16_t     sitzung = 0;
LogEintrag   logWarte[LOG_WARTE];
uint8_t      logKopf = 0, logEnde = 0;
portMUX_TYPE logMux = portMUX_INITIALIZER_UNLOCKED;

// Neustart-Hinweis fürs Display (nur loop() greift zu)
const char*  startHinweis      = nullptr;
uint32_t     startHinweisBis   = 0;
bool         startHinweisGezeigt = false;

// Motorteil-Überwachung (nur Steuer-Task)
bool          motorErstesPaket = true;
unsigned long motorLaufzeitAlt = 0;
bool          kabelWegProtokolliert = false;
bool          akkuWarnungProtokolliert = false;

void logEreignis(uint8_t typ, uint16_t wert = 0) {
  LogEintrag e = {sitzung, wert, millis() / 1000, typ, {0, 0, 0}};
  portENTER_CRITICAL(&logMux);
  const uint8_t naechster = (logEnde + 1) % LOG_WARTE;
  if (naechster != logKopf) { logWarte[logEnde] = e; logEnde = naechster; }   // voll: verwerfen
  portEXIT_CRITICAL(&logMux);
}

const char* grundText(int r) {
  switch (r) {
    case ESP_RST_POWERON:   return "normal eingeschaltet";
    case ESP_RST_EXT:       return "Reset-Taste";
    case ESP_RST_SW:        return "Neustart durch Programm";
    case ESP_RST_PANIC:     return "ABSTURZ (Programmfehler)";
    case ESP_RST_INT_WDT:   return "HAENGER (Interrupt-Watchdog)";
    case ESP_RST_TASK_WDT:  return "HAENGER (Task-Watchdog)";
    case ESP_RST_WDT:       return "HAENGER (Watchdog)";
    case ESP_RST_DEEPSLEEP: return "aus Tiefschlaf";
    case ESP_RST_BROWNOUT:  return "SPANNUNGSEINBRUCH (Brownout)";
    default:                return "unbekannt";
  }
}

const char* logText(uint8_t typ) {
  switch (typ) {
    case LOG_START:          return "Bedienteil gestartet:";
    case LOG_EIN:            return "System EIN";
    case LOG_AUS:            return "System AUS";
    case LOG_KABEL_WEG:      return "Kabel zum Motorteil: keine Antwort mehr";
    case LOG_KABEL_OK:       return "Kabel zum Motorteil: wieder da";
    case LOG_MOTOR_NEUSTART: return "Motorteil neu gestartet:";
    case LOG_FUNK_ZEIT:      return "Funk-Sicherheitsstopp (Zeit abgelaufen)";
    case LOG_FUNK_FEHL:      return "Funkfehler (alle Eingaenge aktiv)";
    case LOG_AKKU_NIEDRIG:   return "Akku niedrig, Volt x10:";
    default:                 return "?";
  }
}

void logSpeichern() {
  speicher.putBytes("e", logPuffer, sizeof(logPuffer));
  speicher.putUShort("pos", logPos);
  speicher.putUShort("anz", logAnzahl);
}

// in loop(): wartende Ereignisse in den Flash schreiben
void logVerarbeiten() {
  bool geaendert = false;
  for (;;) {
    LogEintrag e;
    bool da = false;
    portENTER_CRITICAL(&logMux);
    if (logKopf != logEnde) { e = logWarte[logKopf]; logKopf = (logKopf + 1) % LOG_WARTE; da = true; }
    portEXIT_CRITICAL(&logMux);
    if (!da) break;
    logPuffer[logPos] = e;
    logPos = (logPos + 1) % LOG_GROESSE;
    if (logAnzahl < LOG_GROESSE) logAnzahl++;
    geaendert = true;
  }
  if (geaendert) logSpeichern();
}

void logAusgeben() {
  Serial.printf("\n=== Protokoll: %u Eintraege (aktuelle Sitzung %u) ===\n", logAnzahl, sitzung);
  for (uint16_t i = 0; i < logAnzahl; i++) {
    const LogEintrag& e = logPuffer[(logPos + LOG_GROESSE - logAnzahl + i) % LOG_GROESSE];
    Serial.printf("Sitzung %4u  %02lu:%02lu:%02lu  %s", e.sitzung,
                  (unsigned long)(e.sekunde / 3600), (unsigned long)(e.sekunde / 60 % 60),
                  (unsigned long)(e.sekunde % 60), logText(e.typ));
    if (e.typ == LOG_START || e.typ == LOG_MOTOR_NEUSTART) Serial.printf(" %s", grundText(e.wert));
    else if (e.typ == LOG_AKKU_NIEDRIG) Serial.printf(" %u", e.wert);
    Serial.println();
  }
  Serial.println("=== Ende ('c' = loeschen) ===");
}

// ============================================================================
//  PROTOKOLL PER HANDY: kleines eigenes WLAN mit Webseite (nur bei System AUS)
// ============================================================================
const char*        WLAN_NAME     = "Trolley-Log";
const char*        WLAN_PASSWORT = "golf1234";       // mindestens 8 Zeichen
constexpr uint32_t WLAN_DAUER_MS = 10UL * 60UL * 1000UL;
WebServer webServer(80);
bool      wlanAktiv = false;
uint32_t  wlanStartZeit = 0;

void seiteProtokoll() {
  String h;
  h.reserve(12000);
  h += F("<!doctype html><html><head><meta charset='utf-8'>"
         "<meta name='viewport' content='width=device-width,initial-scale=1'>"
         "<title>Trolley-Protokoll</title><style>"
         "body{font-family:sans-serif;margin:12px;background:#fff;color:#111}"
         "h1{font-size:20px;color:#1f5e3a}table{border-collapse:collapse;width:100%;font-size:14px}"
         "td,th{border-bottom:1px solid #ccc;padding:5px 4px;text-align:left;vertical-align:top}"
         "th{background:#1f5e3a;color:#fff}.w{color:#b00000;font-weight:bold}"
         "a.k{display:inline-block;margin:12px 8px 0 0;padding:10px 14px;background:#1f5e3a;color:#fff;"
         "border-radius:6px;text-decoration:none}</style></head><body><h1>Golftrolley – Protokoll</h1>");
  h += "<p>Version Bedienteil: " + String(PROGRAMM_VERSION) + " &middot; Motorteil: " +
       (motorVersion[0] ? String(motorVersion) : String("unbekannt")) + "</p>";
  h += "<p>Aktuelle Sitzung: " + String(sitzung) + " &middot; Einträge: " + String(logAnzahl) +
       " &middot; neueste oben</p><table><tr><th>Sitzung</th><th>Zeit</th><th>Ereignis</th></tr>";
  for (int i = logAnzahl - 1; i >= 0; i--) {
    const LogEintrag& e = logPuffer[(logPos + LOG_GROESSE - logAnzahl + i) % LOG_GROESSE];
    char zeit[12];
    snprintf(zeit, sizeof(zeit), "%02lu:%02lu:%02lu", (unsigned long)(e.sekunde / 3600),
             (unsigned long)(e.sekunde / 60 % 60), (unsigned long)(e.sekunde % 60));
    String text = logText(e.typ);
    bool warnung = (e.typ == LOG_KABEL_WEG || e.typ == LOG_FUNK_FEHL || e.typ == LOG_AKKU_NIEDRIG);
    if (e.typ == LOG_START || e.typ == LOG_MOTOR_NEUSTART) {
      text += " "; text += grundText(e.wert);
      warnung = (e.wert != ESP_RST_POWERON && e.wert != ESP_RST_SW && e.wert != ESP_RST_EXT);
    } else if (e.typ == LOG_AKKU_NIEDRIG) {
      text += " " + String(e.wert / 10) + "," + String(e.wert % 10) + " V";
    }
    h += "<tr><td>" + String(e.sitzung) + "</td><td>" + zeit + "</td><td" +
         (warnung ? " class='w'>" : ">") + text + "</td></tr>";
  }
  h += F("</table><a class='k' href='/'>Aktualisieren</a>"
         "<a class='k' href='/loeschen' onclick=\"return confirm('Protokoll wirklich löschen?')\">Löschen</a>"
         "<p style='font-size:12px;color:#666'>Zeit = Stunden:Minuten:Sekunden seit dem Start des Bedienteils.</p>"
         "</body></html>");
  webServer.send(200, "text/html; charset=utf-8", h);
}

void seiteLoeschen() {
  logPos = logAnzahl = 0;
  memset(logPuffer, 0, sizeof(logPuffer));
  logSpeichern();
  webServer.sendHeader("Location", "/");
  webServer.send(303);
}

void wlanStarten() {
  WiFi.mode(WIFI_AP_STA);                                // Fernsteuerung bleibt empfangsbereit
  WiFi.softAP(WLAN_NAME, WLAN_PASSWORT, FUNK_KANAL);
  espStarten();
  webServer.on("/", seiteProtokoll);
  webServer.on("/loeschen", seiteLoeschen);
  webServer.begin();
  wlanAktiv = true;
  wlanStartZeit = millis();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 0);  display.print("Protokoll per WLAN");
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
  display.setCursor(0, 16); display.print("Netz: ");     display.print(WLAN_NAME);
  display.setCursor(0, 28); display.print("Passw: ");    display.print(WLAN_PASSWORT);
  display.setCursor(0, 40); display.print("Browser: 192.168.4.1");
  display.setCursor(0, 54); display.print("Ende: rote Taste 5s");
  display.display();
}

void wlanBeenden() {
  webServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  espStarten();
  wlanAktiv = false;
  display.clearDisplay();
  display.display();
}

// in loop(): bei System AUS silberne Taste 3 s -> WLAN an; Ende nach 10 min oder bei System EIN
void wlanPruefen(bool systemAn, uint32_t jetzt) {
  static uint32_t silberSeit = 0;
  static bool     silberGemeldet = false;
  if (wlanAktiv) {
    webServer.handleClient();
    if (systemAn || jetzt - wlanStartZeit >= WLAN_DAUER_MS) wlanBeenden();
    return;
  }
  if (systemAn) { silberSeit = 0; silberGemeldet = false; return; }
  if (digitalRead(BTN_SILBER) == LOW) {
    if (silberSeit == 0) silberSeit = jetzt;
    if (!silberGemeldet && jetzt - silberSeit >= SILBER_LANG_MS) {
      silberGemeldet = true;
      logVerarbeiten();          // alles Wartende noch speichern
      wlanStarten();
    }
  } else {
    silberSeit = 0;
    silberGemeldet = false;
  }
}

void zeigeVersion() {
  Serial.print("Bedienteil Version ");
  Serial.println(PROGRAMM_VERSION);
  Serial.print("Motorteil  Version ");
  Serial.println(motorVersion[0] ? motorVersion : "unbekannt (keine Meldung, Stand vor 25.10.2026 oder Kabel fehlt)");
}

void logBefehle() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == 'l' || c == 'L') logAusgeben();
    else if (c == 'v' || c == 'V') zeigeVersion();
    else if (c == 'c' || c == 'C') {
      logPos = logAnzahl = 0;
      memset(logPuffer, 0, sizeof(logPuffer));
      logSpeichern();
      Serial.println("Protokoll geloescht.");
    }
  }
}

// ============================================================================
//  FUNK PER ESP-NOW – Funktionen
// ============================================================================
void espEmpfangen(const esp_now_recv_info_t* info, const uint8_t* daten, int len) {
  if (len != sizeof(FunkPaket)) return;
  FunkPaket p; memcpy(&p, daten, sizeof(p));
  if (p.m1 != 'G' || p.m2 != 'T' || p.kennung != FUNK_KENNUNG || p.tasten == 0xFF) return;
  const bool anAlle = info->des_addr && info->des_addr[0] == 0xFF && info->des_addr[1] == 0xFF;
  portENTER_CRITICAL(&espMux);
  if (espGebunden && memcmp(espGebundenMac, info->src_addr, 6) != 0) {   // fremder Sender
    portEXIT_CRITICAL(&espMux);
    return;
  }
  if (!espGebunden) { memcpy(espGebundenMac, info->src_addr, 6); espGebunden = true; }
  espTasten = p.tasten;
  espZeit   = millis();
  if (info->rx_ctrl) espRssi = info->rx_ctrl->rssi;
  espPakete = espPakete + 1;
  if (anAlle) { memcpy(espMac, info->src_addr, 6); espAntworten = true; }
  portEXIT_CRITICAL(&espMux);
}

bool espGedrueckt(uint8_t bit) {
  portENTER_CRITICAL(&espMux);
  const uint8_t  t = espTasten;
  const uint32_t z = espZeit;
  portEXIT_CRITICAL(&espMux);
  return (millis() - z < FUNK_HALTE_MS) && (t & (1 << bit));
}

// im Steuer-Task: der suchenden Fernsteuerung antworten ("hier bin ich")
void espKopplung() {
  if (!espAntworten) return;
  uint8_t mac[6];
  portENTER_CRITICAL(&espMux);
  memcpy(mac, espMac, 6); espAntworten = false;
  portEXIT_CRITICAL(&espMux);
  if (!esp_now_is_peer_exist(mac)) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;              // aktueller Kanal
    peer.ifidx   = WIFI_IF_STA;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
  }
  FunkPaket a = {'G', 'T', FUNK_KENNUNG, 0xFF, 0};
  esp_now_send(mac, (const uint8_t*)&a, sizeof(a));
}

// Funk einschalten (beim Start und nach dem Protokoll-WLAN)
void espStarten() {
  esp_now_deinit();
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(FUNK_KANAL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() == ESP_OK) esp_now_register_recv_cb(espEmpfangen);
  else Serial.println("ESP-NOW Start fehlgeschlagen");
}

// ============================================================================
//  HILFSFUNKTIONEN
// ============================================================================
int lesePotiRoh() {
  uint32_t summe = 0;
  for (int i = 0; i < POTI_MITTELUNG; i++) summe += analogRead(POTI_PIN);
  return summe / POTI_MITTELUNG;
}

int rohZuProzent(int roh) {
  if (roh <= POTI_START) return 0;
  if (roh >= POTI_ENDE)  return 100;
  return map(roh, POTI_START, POTI_ENDE, 0, 100);
}

uint8_t leseRichtung() {
  bool fwd = (digitalRead(SW_FORWARD)  == LOW);
  bool bwd = (digitalRead(SW_BACKWARD) == LOW);
  if (fwd && !bwd) return 1;
  if (bwd && !fwd) return 2;
  return 0;
}

void setzeSystemAktiv(bool aktiv, uint32_t jetzt) {
  systemAktiv   = aktiv;
  tempomatAktiv = false;
  zehnSekAktiv    = false;
  funkModus     = FUNK_AUS;
  funkKurve     = 0;
  funkGesperrt  = false;
  haltAktiv     = false;
  funkAnzeige   = nullptr;

  logEreignis(aktiv ? LOG_EIN : LOG_AUS);
  if (aktiv) {                      // Kopplung lösen: die nächste Fernsteuerung wird gemerkt
    portENTER_CRITICAL(&espMux);
    espGebunden = false;
    espTasten   = 0;
    portEXIT_CRITICAL(&espMux);
  }
  kabelWegProtokolliert    = false;
  akkuWarnungProtokolliert = false;

  if (aktiv) {
    // Alte Daten verwerfen und Tasterzustände übernehmen,
    // damit beim Einschalten keine falschen Flanken entstehen
    while (Serial1.available()) Serial1.read();
    tasterSchwarz.synchronisieren();
    tasterGruen.synchronisieren();
    tasterSilber.synchronisieren();
    silberLangAusgeloest = tasterSilber.gedrueckt();
    funkVor.zuruecksetzen();
    funkZurueck.zuruecksetzen();
    funkLinks.zuruecksetzen();
    funkRechts.zuruecksetzen();
    funkHalt.zuruecksetzen();
    // Funk erst freigeben, wenn nach dem Einschalten alle Tasten losgelassen sind
    funkGesperrt   = true;
    potiGeglaettet = lesePotiRoh();
    verbindungOk   = false;
  } else {
    uhrLaeuft = false;
    // Display wird von loop() gelöscht, sobald es "System aus" sieht
  }
}

// ============================================================================
//  ROTE TASTE: 5 s HALTEN = EIN/AUS  (nicht blockierend)
// ============================================================================
void pruefePowerTaste(uint32_t jetzt) {
  static bool     war        = false;
  static bool     ausgeloest = false;
  static uint32_t seit       = 0;

  bool gedrueckt = (digitalRead(BTN_ROT) == LOW);
  if (gedrueckt) {
    if (!war) { war = true; seit = jetzt; ausgeloest = false; }
    // Nur einmal pro Tastendruck umschalten – erst loslassen, dann wieder möglich
    if (!ausgeloest && jetzt - seit >= POWER_HALTEZEIT_MS) {
      ausgeloest = true;
      setzeSystemAktiv(!systemAktiv, jetzt);
    }
  } else {
    war = false;
  }
}

// ============================================================================
//  40-ms-TAKT: EINGÄNGE AUSWERTEN UND SENDEN
// ============================================================================
void sendeTakt(uint32_t jetzt) {
  tasterSchwarz.lesen();
  tasterGruen.lesen();
  tasterSilber.lesen();
  funkVor.lesen();
  funkZurueck.lesen();
  funkLinks.lesen();
  funkRechts.lesen();
  funkHalt.lesen();

  // Alle 5 gleichzeitig "gedrückt" ist mit einer Hand unmöglich ->
  // Empfänger stromlos oder defekt: Funk komplett ignorieren
  const bool funkFehler = funkVor.stabil && funkZurueck.stabil && funkLinks.stabil &&
                          funkRechts.stabil && funkHalt.stabil;
  static bool funkFehlerVorher = false;
  if (funkFehler && !funkFehlerVorher) logEreignis(LOG_FUNK_FEHL);
  funkFehlerVorher = funkFehler;
  const bool fVor     = !funkFehler && funkVor.stabil;
  const bool fZurueck = !funkFehler && funkZurueck.stabil;
  const bool fLinks   = !funkFehler && funkLinks.stabil;
  const bool fRechts  = !funkFehler && funkRechts.stabil;
  const bool fHalt    = !funkFehler && funkHalt.stabil;

  uint8_t richtung = leseRichtung();
  aktuelleRichtung = richtung;

  int potiRoh = lesePotiRoh();
  potiGeglaettet += (potiRoh - potiGeglaettet) * POTI_FILTER;
  int potiProzent       = rohZuProzent((int)potiGeglaettet);  // geglättet -> zum Senden
  int potiProzentDirekt = rohZuProzent(potiRoh);              // schnell -> für Abbrüche

#if DEBUG_POTI
  Serial.printf("Poti: %d | Prozent: %d\n", potiRoh, potiProzent);
#endif

  // ==========================================================================
  //  FUNK-HALT: wirkt IMMER, auch gegen Lenker, Tempomat und Timer.
  //  Nach dem Loslassen bleibt der Halt, bis der Gasgriff auf 0 ist –
  //  sonst würde der Trolley mit noch gedrücktem Griff sofort losfahren.
  // ==========================================================================
  if (fHalt) {
    haltAktiv     = true;
    tempomatAktiv = false;
    zehnSekAktiv    = false;
  } else if (haltAktiv && potiProzentDirekt <= GRIFF_FREI_PROZENT) {
    haltAktiv = false;
  }

  // ==========================================================================
  //  LENKER-LOGIK (unverändert)
  // ==========================================================================
  // --- Schwarz: Tempomat ein/aus ---
  if (tasterSchwarz.neuGedrueckt()) {
    if (!tempomatAktiv && !haltAktiv && richtung > 0 && potiProzent > TEMPOMAT_MIN_PROZENT) {
      tempomatAktiv           = true;
      tempomatGeschwindigkeit = potiProzent;
      tempomatRichtung        = richtung;
      tempomatStart           = jetzt;
      zehnSekAktiv              = false;
    } else {
      tempomatAktiv = false;
    }
  }

  // --- Grün: 10-s-Timer ein/aus ---
  if (tasterGruen.neuGedrueckt()) {
    if (!zehnSekAktiv && !haltAktiv && richtung == 1 && potiProzent < TIMER_MAX_POTI_PROZENT) {
      zehnSekAktiv    = true;
      zehnSekStart    = jetzt;
      tempomatAktiv = false;
    } else {
      zehnSekAktiv = false;
    }
  }

  // --- Abbruchbedingungen ---
  if (tempomatAktiv) {
    if (richtung != tempomatRichtung || !verbindungOk) {
      tempomatAktiv = false;   // Neutral, Richtungswechsel oder Verbindung weg: sofort aus
    } else if (jetzt - tempomatStart > TEMPOMAT_SPERRZEIT_MS && potiRoh > TEMPOMAT_ABBRUCH_ROH) {
      tempomatAktiv = false;   // Gasgriff übersteuert
    }
  }

  if (zehnSekAktiv) {
    if (jetzt - zehnSekStart >= TIMER_DAUER_MS ||
        richtung != 1 ||
        !verbindungOk ||
        potiProzentDirekt > TIMER_MAX_POTI_PROZENT) {
      zehnSekAktiv = false;
    }
  }

  int sendeWert = tempomatAktiv ? tempomatGeschwindigkeit
                : zehnSekAktiv    ? TIMER_GESCHWINDIGKEIT
                :                 potiProzent;

  // ==========================================================================
  //  FUNK-FAHRT (rastend)
  //  Vor/Zurück: im Wohlfühltempo losfahren, weiterfahren bis Stop.
  //  Gleiche Richtung nochmal = Sicherheitszeit neu, Gegenrichtung = Stopp.
  //  Links/Rechts während der Fahrt: jeweiliges Rad FUNK_KURVE_MS langsamer.
  //  Nur aktiv bei Wippe auf STOP, Kabel ok, Gasgriff losgelassen, kein Halt.
  // ==========================================================================
  const bool funkTaste = fVor || fZurueck || fLinks || fRechts || fHalt;
  if (!funkTaste) funkGesperrt = false;   // nach dem Einschalten erst alles loslassen

  const bool funkErlaubt = (richtung == 0) && verbindungOk && !haltAktiv &&
                           potiProzentDirekt <= GRIFF_FREI_PROZENT;
  const bool flankeOk = !funkGesperrt && !funkFehler;
  const bool nVor     = flankeOk && funkVor.neuGedrueckt();
  const bool nZurueck = flankeOk && funkZurueck.neuGedrueckt();
  const bool nLinks   = flankeOk && funkLinks.neuGedrueckt();
  const bool nRechts  = flankeOk && funkRechts.neuGedrueckt();

  if (!funkErlaubt) {
    funkModus = FUNK_AUS;
    funkKurve = 0;
  } else {
    if (nVor) {
      if (funkModus == FUNK_ZURUECK_FAHRT) funkModus = FUNK_AUS;            // Gegenrichtung = Stopp
      else { funkModus = FUNK_VOR_FAHRT; funkFahrStart = jetzt; }            // starten / verlängern
      funkKurve = 0;
    } else if (nZurueck) {
      if (funkModus == FUNK_VOR_FAHRT) funkModus = FUNK_AUS;
      else { funkModus = FUNK_ZURUECK_FAHRT; funkFahrStart = jetzt; }
      funkKurve = 0;
    }
    if (funkModus != FUNK_AUS && (nLinks || nRechts)) {
      funkKurve     = nLinks ? -1 : 1;
      funkKurveEnde = jetzt + FUNK_KURVE_MS;
    }
    if (funkModus != FUNK_AUS && jetzt - funkFahrStart >= FUNK_MAX_FAHRZEIT_MS) {
      funkModus = FUNK_AUS;                                                   // Sicherheitsstopp
      funkKurve = 0;
      funkZeitAnzeigeBis = jetzt + FUNK_ZEIT_ANZEIGE_MS;
      logEreignis(LOG_FUNK_ZEIT);
    }
  }
  if (funkKurve != 0 && (int32_t)(jetzt - funkKurveEnde) >= 0) funkKurve = 0;

  RadBefehl radL = {0, 0}, radR = {0, 0};
  bool funkPaket = false;
  funkAnzeige = nullptr;

  if (funkModus != FUNK_AUS) {
    const bool    vor   = (funkModus == FUNK_VOR_FAHRT);
    const uint8_t dir   = vor ? 1 : 2;
    const int     voll  = vor ? FUNK_VOR_PROZENT : FUNK_ZURUECK_PROZENT;
    const int     innen = voll * FUNK_KURVE_INNEN_ANTEIL / 100;
    radL = {dir, funkKurve < 0 ? innen : voll};
    radR = {dir, funkKurve > 0 ? innen : voll};
    funkPaket = true;
    if (vor) funkAnzeige = funkKurve < 0 ? "F-VorL" : funkKurve > 0 ? "F-VorR" : "F-Vor";
    else     funkAnzeige = funkKurve < 0 ? "F-ZurL" : funkKurve > 0 ? "F-ZurR" : "F-Zur";
  }

  if (haltAktiv)                                                      funkAnzeige = "Halt";
  else if (richtung == 0 && (int32_t)(jetzt - funkZeitAnzeigeBis) < 0) funkAnzeige = "F-Zeit";
  else if (funkFehler && richtung == 0)                               funkAnzeige = "F-Fehl";

#if DEBUG_FUNK
  static uint8_t letzteMaske = 0xFF;
  const uint8_t maske = funkVor.stabil | funkZurueck.stabil << 1 | funkLinks.stabil << 2 |
                        funkRechts.stabil << 3 | funkHalt.stabil << 4;
  if (maske != letzteMaske) {
    letzteMaske = maske;
    Serial.printf("Funk: V%d Z%d L%d R%d H%d\n", funkVor.stabil, funkZurueck.stabil,
                  funkLinks.stabil, funkRechts.stabil, funkHalt.stabil);
  }
  static uint32_t rssiZeit = 0;
  if (jetzt - rssiZeit >= 2000) {
    rssiZeit = jetzt;
    portENTER_CRITICAL(&espMux);
    const int8_t r = espRssi; const uint16_t n = espPakete; espPakete = 0;
    portEXIT_CRITICAL(&espMux);
    if (n) Serial.printf("Funk-Signal: %d dBm, %u Pakete in 2 s\n", r, n);
    else   Serial.println("Funk-Signal: keine Pakete");
  }
#endif

  // ==========================================================================
  //  SILBER: kurz = Uhr Start/Stop, 3 s = Reset
  // ==========================================================================
  bool sonderPaketGesendet = false;

  if (tasterSilber.neuGedrueckt()) {
    silberGedruecktSeit  = jetzt;
    silberLangAusgeloest = false;
  }
  if (tasterSilber.gedrueckt() && !silberLangAusgeloest &&
      jetzt - silberGedruecktSeit >= SILBER_LANG_MS) {
    silberLangAusgeloest = true;
    uhrGesamtSekunden    = 0;
    uhrLaeuft            = false;
    gelaufeneMeter       = 0;
    geloeschtTextStart   = jetzt;
    geloeschtTextAktiv   = true;
    Serial1.print("<1,8,0>");
    sonderPaketGesendet = true;
  }
  if (tasterSilber.losgelassen() && !silberLangAusgeloest) {
    uhrLaeuft = !uhrLaeuft;
    if (uhrLaeuft) uhrLetzteSekunde = jetzt;
    Serial1.print("<1,9,0>");
    sonderPaketGesendet = true;
  }

  // ==========================================================================
  //  FAHRPAKET SENDEN
  //  Halt hat Vorrang vor allem, dann Funk, dann Lenker.
  // ==========================================================================
  // Ein Sonderpaket (8/9) ersetzt höchstens einen Takt; das Motorteil
  // behält so lange den letzten Fahrbefehl bei.
  if (sonderPaketGesendet) return;

  char paket[32];
  if (haltAktiv) {
    snprintf(paket, sizeof(paket), "<1,0,0>");
  } else if (funkPaket) {
    snprintf(paket, sizeof(paket), "<2,%u,%d,%u,%d>",
             radL.richtung, radL.prozent, radR.richtung, radR.prozent);
  } else {
    snprintf(paket, sizeof(paket), "<1,%u,%d>", richtung, sendeWert);
  }
  Serial1.print(paket);
}

// ============================================================================
//  EMPFANG  <motor,batterie,meter,voltZehntel,laufzeitSek,grund>  und  <V,version>
// ============================================================================
void werteEmpfangAus(const char* daten, uint32_t jetzt) {
  int  motor, batterie, vZehntel, motorGrund = 0;
  long meter;
  unsigned long motorLaufzeit = 0;
  const int n = sscanf(daten, "%d,%d,%ld,%d,%lu,%d", &motor, &batterie, &meter, &vZehntel,
                       &motorLaufzeit, &motorGrund);
  if (n < 4) return;

  // Neustart des Motorteils erkennen: Laufzeit springt zurück, oder beim ersten
  // Paket nach dem Start ein ungewöhnlicher Neustart-Grund
  if (n == 6) {
    if (!motorErstesPaket && motorLaufzeit + 2 < motorLaufzeitAlt) {
      logEreignis(LOG_MOTOR_NEUSTART, motorGrund);
    } else if (motorErstesPaket && motorGrund != ESP_RST_POWERON && motorLaufzeit < 30) {
      logEreignis(LOG_MOTOR_NEUSTART, motorGrund);
    }
    motorErstesPaket = false;
    motorLaufzeitAlt = motorLaufzeit;
  }
  if (!verbindungOk && kabelWegProtokolliert) {
    logEreignis(LOG_KABEL_OK);
    kabelWegProtokolliert = false;
  }
  if (vZehntel > 0 && vZehntel < AKKU_WARN_ZEHNTEL && !akkuWarnungProtokolliert) {
    logEreignis(LOG_AKKU_NIEDRIG, vZehntel);
    akkuWarnungProtokolliert = true;
  }

  motorIstProzent = constrain(motor, 0, 100);
  batterieProzent = batterie;
  gelaufeneMeter  = meter;
  if (vZehntel > 0) realeVoltzahl = vZehntel / 10.0f;
  letztesSignal = jetzt;
  verbindungOk  = true;
}

// Versionspaket vom Motorteil <V,25.10.2026> – Zugriff nur aus dem Steuer-Task,
// gelesen wird motorVersion nur zum Anzeigen (kurzer Text, schlimmstenfalls
// einmal halb alt angezeigt – unkritisch)
bool versionPaket(const char* daten) {
  if (daten[0] != 'V' || daten[1] != ',') return false;
  strlcpy(motorVersion, daten + 2, sizeof(motorVersion));
  return true;
}

// nurVersion = true: System ist AUS, nur das Versionspaket auswerten
void empfangeDaten(uint32_t jetzt, bool nurVersion = false) {
  static char    puffer[48];
  static uint8_t laenge  = 0;
  static bool    imPaket = false;

  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '<') {
      laenge  = 0;
      imPaket = true;
    } else if (!imPaket) {
      // Zeichen außerhalb eines Pakets ignorieren
    } else if (c == '>') {
      puffer[laenge] = '\0';
      imPaket = false;
      if (!versionPaket(puffer) && !nurVersion) werteEmpfangAus(puffer, jetzt);
    } else if (laenge < sizeof(puffer) - 1) {
      puffer[laenge++] = c;
    } else {
      imPaket = false;   // Überlauf: Paket verwerfen
    }
  }
  if (nurVersion) return;

  if (verbindungOk && jetzt - letztesSignal > VERBINDUNG_TIMEOUT_MS) {
    verbindungOk    = false;
    motorIstProzent = 0;
    if (!kabelWegProtokolliert) { logEreignis(LOG_KABEL_WEG); kabelWegProtokolliert = true; }
  }
}

// ============================================================================
//  ANZEIGE  (läuft in loop() auf Core 1, arbeitet nur mit der Momentaufnahme)
// ============================================================================
void zeichneStatus(const AnzeigeDaten& a) {
  display.setCursor(0, 52);
  if (startHinweis && (int32_t)(millis() - startHinweisBis) < 0) { display.print(startHinweis); return; }
  if      (!a.verbindungOk)                         display.print("Not-Aus");
  else if (a.funkText)                              display.print(a.funkText);
  else if (a.tempomat)                              display.print("Tempo");
  else if (a.timer)                                 display.print("10s Tim");
  else if (a.motorProzent > 0 && a.richtung == 0)   display.print("Rampe");
  else if (a.richtung == 1) { display.print("Vorw"); display.write(132); display.print("rts"); }
  else if (a.richtung == 2) { display.print("R"); display.write(129); display.print("ckw"); display.write(132); display.print("rts"); }
  else                                              display.print("Stop");
}

void zeichneUhr(const AnzeigeDaten& a) {
  display.setCursor(56, 52);

  if (a.geloeschtText) {
    display.print("  Gel"); display.write(148); display.print("scht");
    return;
  }

  display.print(" Zeit ");
  if (!a.uhrLaeuft && a.uhrSekunden > 0) display.print("P");

  uint32_t h = a.uhrSekunden / 3600;
  uint32_t m = (a.uhrSekunden % 3600) / 60;
  uint32_t s = a.uhrSekunden % 60;
  char text[16];
  if (h == 0) snprintf(text, sizeof(text), "%02lu:%02lu",  (unsigned long)m, (unsigned long)s);
  else        snprintf(text, sizeof(text), "%02luh%02lu", (unsigned long)h, (unsigned long)m);
  display.print(text);
}

void aktualisiereAnzeige(const AnzeigeDaten& a) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setFont();
  display.setTextSize(1);

  // Kopfzeile: Spannung, Motorbalken, Status
  display.setCursor(0, 0);
  if (a.verbindungOk) { display.print(a.volt, 1); display.print("V"); }
  else                { display.print("Kabel:ERR"); }

  display.drawRect(42, 0, 52, 7, SSD1306_WHITE);
  display.fillRect(42, 0, map(a.motorProzent, 0, 100, 0, 52), 7, SSD1306_WHITE);
  display.setCursor(110, 0);
  display.print(a.verbindungOk ? "ON" : "ERR");

  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
  display.setCursor(10, 14);
  display.print("Strecke auf Runde:");

  // Strecke groß
  display.setFont(&FreeSansBold9pt7b);
  display.setCursor(20, 36);
  if (a.meter < 1000) {
    display.print(a.meter);
    display.setFont(); display.print(" m");
  } else {
    display.print(a.meter / 1000.0f, 1);
    display.setFont(); display.print(" km");
  }

  display.setFont();
  display.drawFastHLine(0, 44, 128, SSD1306_WHITE);

  zeichneStatus(a);
  zeichneUhr(a);

  display.display();   // ca. 25–30 ms I2C – blockiert nur Core 1
}

// ============================================================================
//  STEUER-TASK  (Core 0, alle 40 ms)
//  Ein/Aus, Taster, Gasgriff, Tempomat, Timer, Funk, Uhr, Senden, Empfangen, LED
// ============================================================================
void veroeffentlicheAnzeige(uint32_t jetzt) {
  if (geloeschtTextAktiv && jetzt - geloeschtTextStart >= GELOESCHT_ANZEIGE_MS) {
    geloeschtTextAktiv = false;
  }

  AnzeigeDaten a;
  a.systemAktiv   = systemAktiv;
  a.verbindungOk  = verbindungOk;
  a.volt          = realeVoltzahl;
  a.motorProzent  = motorIstProzent;
  a.meter         = gelaufeneMeter;
  a.richtung      = aktuelleRichtung;
  a.tempomat      = tempomatAktiv;
  a.timer         = zehnSekAktiv;
  a.funkText      = funkAnzeige;
  a.uhrLaeuft     = uhrLaeuft;
  a.uhrSekunden   = uhrGesamtSekunden;
  a.geloeschtText = geloeschtTextAktiv;

  portENTER_CRITICAL(&anzeigeMux);
  anzeige = a;
  portEXIT_CRITICAL(&anzeigeMux);
}

void SteuerTask(void* parameter) {
  TickType_t letzterTakt = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&letzterTakt, pdMS_TO_TICKS(SENDE_TAKT_MS));
    const uint32_t jetzt = millis();

    pruefePowerTaste(jetzt);
    espKopplung();             // Fernsteuerung sucht? -> antworten

    if (!systemAktiv) {
      if (jetzt - letztesAusPaket >= AUS_SENDE_TAKT_MS) {
        letztesAusPaket = jetzt;
        Serial1.print("<0,0,0,0,0,0>");
      }
      empfangeDaten(jetzt, true);   // Antworten verwerfen, nur Version merken
      veroeffentlicheAnzeige(jetzt);
      continue;
    }

    // Stoppuhr (holt verpasste Sekunden nach, driftet nicht)
    while (uhrLaeuft && jetzt - uhrLetzteSekunde >= 1000) {
      uhrGesamtSekunden++;
      uhrLetzteSekunde += 1000;
    }

    sendeTakt(jetzt);
    empfangeDaten(jetzt);

    veroeffentlicheAnzeige(jetzt);
  }
}

// ============================================================================
//  SETUP
// ============================================================================
void setup() {
  rgbLedWrite(21, 0, 0, 0);   // eingebaute bunte LED des S3 Zero aus
  Serial.begin(115200);
  Serial.print("Bedienteil Version ");
  Serial.println(PROGRAMM_VERSION);

  // Protokoll laden, Sitzung zählen, Startgrund festhalten
  speicher.begin("trolley", false);
  sitzung   = speicher.getUShort("sitz", 0) + 1;
  speicher.putUShort("sitz", sitzung);
  logPos    = speicher.getUShort("pos", 0);
  logAnzahl = speicher.getUShort("anz", 0);
  if (speicher.getBytes("e", logPuffer, sizeof(logPuffer)) != sizeof(logPuffer) ||
      logPos >= LOG_GROESSE || logAnzahl > LOG_GROESSE) {
    memset(logPuffer, 0, sizeof(logPuffer)); logPos = logAnzahl = 0;
  }
  const esp_reset_reason_t grund = esp_reset_reason();
  logEreignis(LOG_START, grund);
  switch (grund) {
    case ESP_RST_BROWNOUT: startHinweis = "Neust:Spg"; break;
    case ESP_RST_PANIC:    startHinweis = "Neust:Abst"; break;
    case ESP_RST_INT_WDT: case ESP_RST_TASK_WDT: case ESP_RST_WDT: startHinweis = "Neust:Hng"; break;
    default: break;
  }
  WiFi.mode(WIFI_STA);      // Funk für die Fernsteuerung (ESP-NOW), kein WLAN-Netz
  espStarten();
  btStop();
  Serial1.begin(19200, SERIAL_8N1, MOTOR_RX, MOTOR_TX);

  delay(800);   // Display-Hochlaufzeit
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 nicht gefunden!");
  }
  // Version 2 s anzeigen (Motor ist dabei sicher aus, der Steuer-Task läuft noch nicht)
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setFont();
  display.setTextSize(1);
  display.setCursor(31, 16); display.print("Golftrolley");
  display.setCursor(43, 32); display.print("Version");
  display.setCursor(34, 44); display.print(PROGRAMM_VERSION);
  display.display();
  delay(2000);
  display.clearDisplay();
  display.display();

  pinMode(SW_FORWARD,  INPUT_PULLUP);
  pinMode(SW_BACKWARD, INPUT_PULLUP);
  pinMode(BTN_SCHWARZ, INPUT_PULLUP);
  pinMode(BTN_GRUEN,   INPUT_PULLUP);
  pinMode(BTN_ROT,     INPUT_PULLUP);
  pinMode(BTN_SILBER,  INPUT_PULLUP);
  pinMode(POTI_PIN,    INPUT);

  // ADC auf vollen Messbereich (3 = maximale Dämpfung 11/12 dB)
  analogSetAttenuation((adc_attenuation_t)3);
  analogReadResolution(12);

  // Steuerung + Kommunikation auf Core 1 mit hoher Priorität (Funk/ESP-NOW arbeitet auf Core 0)
  xTaskCreatePinnedToCore(SteuerTask, "SteuerTask", STEUER_STACK, nullptr,
                          STEUER_PRIO, &SteuerTaskHandle, STEUER_KERN);
}

// ============================================================================
//  LOOP (Core 1) – nur Anzeige
// ============================================================================
void loop() {
  static bool     warAktiv      = false;
  static uint32_t letzteAnzeige = 0;

  AnzeigeDaten a;
  portENTER_CRITICAL(&anzeigeMux);
  a = anzeige;
  portEXIT_CRITICAL(&anzeigeMux);

  const uint32_t jetzt = millis();

  logVerarbeiten();
  logBefehle();
  wlanPruefen(a.systemAktiv, jetzt);

  if (!a.systemAktiv) {
    if (warAktiv && !wlanAktiv) {            // gerade ausgeschaltet -> Display einmal löschen
      warAktiv = false;
      display.clearDisplay();
      display.display();
    }
  } else if (!warAktiv || jetzt - letzteAnzeige >= ANZEIGE_TAKT_MS) {
    if (!warAktiv && startHinweis && !startHinweisGezeigt) {   // Neustart-Grund 8 s zeigen
      startHinweisBis     = jetzt + 8000;
      startHinweisGezeigt = true;
    }
    warAktiv      = true;      // beim Einschalten sofort zeichnen
    letzteAnzeige = jetzt;
    aktualisiereAnzeige(a);
  }

  delay(10);   // gibt Rechenzeit ab; die Steuerung läuft unabhängig auf Core 0
}
