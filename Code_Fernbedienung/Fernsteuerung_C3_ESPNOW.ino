// ============================================================================
//  FERNSTEUERUNG (ESP32-C3 SuperMini oder XIAO ESP32C3) – Golftrolley, ESP-NOW
//  Board in der Arduino IDE: "ESP32C3 Dev Module", USB CDC On Boot: Enabled,
//  Flash Size 4MB, Partition Scheme "Default 4MB with spiffs"
// ----------------------------------------------------------------------------
//  Letzte Änderung: 25.10.2026, Version 25.10.2026b (Akkuwarnung mit Duo-LED)
//
//  Änderungsverlauf:
//  25.10.2026  Akkuwarnung (nur XIAO): Akkuspannung wird über einen Spannungs-
//              teiler (2x 220 kOhm) an D1 gemessen, alle 5 s, gemittelt.
//              Duo-LED rot/grün statt der gelben LED:
//                grün an        = verbunden, Akku gut
//                grün blinkt    = keine Verbindung, Akku gut
//                rot an/blinkt  = Akku unter 3,5 V -> bald laden
//                rot schnell    = Akku unter 3,3 V -> jetzt laden
//              Wieder grün erst über 3,6 V (nach dem Laden).
//              Dafür neue Pins: Taste Vor D1 -> D7, rote LED an D6.
//              "v" im Seriellen Monitor zeigt jetzt auch die Akkuspannung.
//              Version "25.10.2026b".
//  25.10.2026  Versionsanzeige: Beim Start steht im Seriellen Monitor
//              (115200 Baud) "Fernsteuerung Version 25.10.2026". Später
//              jederzeit "v" senden -> Version wird nochmal ausgegeben.
//  24.10.2026  Auch für Seeed XIAO ESP32C3 (Akku an BAT+/BAT-, Antenne steckt an):
//              unten BOARD_XIAO auf 1 stellen. Dann Tasten an D1–D5, LED an D10,
//              volle Sendeleistung (externe Antenne).
//  24.10.2026  Erstfassung. Ersetzt den JDY-40-Sender: Der C3 schickt die
//              Tasten direkt per ESP-NOW an das Bedienteil (ESP32-S3), dort
//              ist kein Empfänger-Modul mehr nötig.
//              - Kopplung automatisch: Die Fernsteuerung ruft zuerst "an alle",
//                das Bedienteil mit gleicher KENNUNG antwortet; danach wird nur
//                noch direkt an dieses Bedienteil gesendet (mit Empfangs-
//                bestätigung durch die Funkhardware).
//              - Solange eine Taste gedrückt ist: alle 50 ms ein Paket.
//                Das Bedienteil wertet eine Taste als losgelassen, wenn 200 ms
//                lang nichts kommt -> Funkabriss = Taste los = sicher.
//              - LED (optional, GPIO 6): leuchtet = Bedienteil bestätigt,
//                blinkt = keine Verbindung.
//              - Nach 5 min ohne Tastendruck Stromsparschlaf; jede Taste weckt.
// ----------------------------------------------------------------------------
//  VERDRAHTUNG XIAO ESP32C3 (BOARD_XIAO 1)
//    Vor D7 | Zurück D2 | Links D3 | Rechts D4 | Stop D5   (Tasten gegen GND)
//    Duo-LED rot/grün, gemeinsame Kathode (mittleres, längstes Bein):
//      D10 -> 2,2 kOhm -> grün      D6 -> 2,2 kOhm -> rot      Kathode -> GND
//    Akku: rot -> Schiebeschalter -> BAT+ (Unterseite), schwarz -> BAT-
//    Akkumessung: Schalter-Ausgang (= BAT+) -> 220 kOhm -> D1 -> 220 kOhm -> GND,
//                 dazu 100 nF von D1 nach GND
//    Geladen wird über die USB-C-Buchse des XIAO (Schalter dabei auf EIN).
//    D0, D8, D9 frei lassen (Startpins).
//
//  VERDRAHTUNG C3 SuperMini (BOARD_XIAO 0)
//    Tasten (gegen GND, wie bisher):
//      Vor     (gelb)    -> GPIO 0
//      Zurück  (orange)  -> GPIO 1
//      Links   (blau)    -> GPIO 3
//      Rechts  (grün)    -> GPIO 4
//      Stop    (braun)   -> GPIO 5
//    LED gelb: GPIO 6 -> LED Anode (langes Bein), Kathode -> 2,2 kOhm -> GND
//    Strom: Batterie-Plus (über Schiebeschalter) -> 3V3-Pin, Minus -> GND
//           (NICHT an 5V!). 470 µF zwischen 3V3 und GND (Plus an 3V3).
//    GPIO 2, 8, 9 frei lassen (Startpins).
// ============================================================================

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <esp_idf_version.h>
#include <driver/gpio.h>

// ---------------- Version (bei jeder Änderung hier UND oben im Änderungsverlauf eintragen)
constexpr char PROGRAMM_VERSION[] = "25.10.2026b";

#define BOARD_XIAO 1   // 1 = Seeed XIAO ESP32C3, 0 = ESP32-C3 SuperMini

// ---------------- Einstellungen (KANAL und KENNUNG im Bedienteil gleich!)
constexpr uint8_t  KANAL          = 1;        // WLAN-Kanal 1..13
constexpr uint16_t KENNUNG        = 0x4754;   // eigene Nummer, frei wählbar
constexpr uint32_t SENDE_TAKT_MS  = 50;       // bei gedrückter Taste
constexpr uint32_t HERZSCHLAG_MS  = 1000;     // ohne Taste: Lebenszeichen
constexpr uint32_t SCHLAF_NACH_MS = 5UL * 60UL * 1000UL;
// Viele C3 SuperMini funken bei voller Leistung unsauber (Antenne zu nah am
// Quarz). 8,5 dBm ist dort erprobt und reicht für 50 m+.
#if BOARD_XIAO
constexpr wifi_power_t SENDELEISTUNG = WIFI_POWER_19_5dBm;   // externe Antenne
#else
constexpr wifi_power_t SENDELEISTUNG = WIFI_POWER_8_5dBm;
#endif

// ---------------- Pins
#if BOARD_XIAO
constexpr uint8_t TASTEN_PINS[5] = {20, 4, 5, 6, 7};  // D7, D2..D5: Vor, Zurück, Links, Rechts, Stop
constexpr int8_t  LED_PIN        = 10;                // D10 grün, -1 = keine LED
constexpr int8_t  LED_ROT        = 21;                // D6 rot (Duo-LED), -1 = keine
constexpr int8_t  AKKU_PIN       = 3;                 // D1 über Spannungsteiler, -1 = keine Messung
constexpr int8_t  BOARD_LED      = -1;                // XIAO hat keine eigene LED
#else
constexpr uint8_t TASTEN_PINS[5] = {0, 1, 3, 4, 5};   // Vor, Zurück, Links, Rechts, Stop
constexpr int8_t  LED_PIN        = 6;                 // -1 = keine LED
constexpr int8_t  LED_ROT        = -1;
constexpr int8_t  AKKU_PIN       = -1;
constexpr int8_t  BOARD_LED      = 8;                 // blaue LED auf dem SuperMini (LOW = an)
#endif

// ---------------- Akku (LiPo 1 Zelle)
constexpr float    AKKU_TEILER      = 2.0f;    // 220k/220k -> Pin sieht die halbe Spannung
constexpr float    AKKU_KORREKTUR   = 1.00f;   // mit Multimeter abgleichen (z. B. 1.03)
constexpr uint32_t AKKU_TAKT_MS     = 5000;    // so oft messen
constexpr int      AKKU_NIEDRIG_MV  = 3500;    // darunter: rot = bald laden
constexpr int      AKKU_LEER_MV     = 3300;    // darunter: rot schnell blinkend = jetzt laden
constexpr int      AKKU_WIEDER_OK_MV = 3600;   // erst darüber wieder grün (Hysterese)

int  akkuMillivolt = 0;       // 0 = noch nicht gemessen / keine Messung
bool akkuNiedrig   = false;
bool akkuLeer      = false;

// ---------------- Funkpaket (gleich im Bedienteil)
struct __attribute__((packed)) FunkPaket {
  uint8_t  m1 = 'G', m2 = 'T';
  uint16_t kennung = KENNUNG;
  uint8_t  tasten = 0;      // Bit 0 Vor, 1 Zurück, 2 Links, 3 Rechts, 4 Stop; 0xFF = Antwort
  uint8_t  zaehler = 0;
};

const uint8_t ALLE[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t  bedienMac[6];
volatile bool     partnerBekannt = false;
volatile bool     neuerPartner   = false;
volatile uint32_t letzteBestaetigung = 0;
uint8_t  neueMac[6];
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

// ---------------- Rückmeldungen der Funkhardware
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
void gesendet(const wifi_tx_info_t*, esp_now_send_status_t status) {
#else
void gesendet(const uint8_t*, esp_now_send_status_t status) {
#endif
  if (status == ESP_NOW_SEND_SUCCESS && partnerBekannt) letzteBestaetigung = millis();
}

void empfangen(const esp_now_recv_info_t* info, const uint8_t* daten, int len) {
  if (len != sizeof(FunkPaket)) return;
  FunkPaket p; memcpy(&p, daten, sizeof(p));
  if (p.m1 != 'G' || p.m2 != 'T' || p.kennung != KENNUNG || p.tasten != 0xFF) return;
  portENTER_CRITICAL(&mux);
  memcpy(neueMac, info->src_addr, 6);
  neuerPartner = true;
  portEXIT_CRITICAL(&mux);
}

void partnerEintragen(const uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = KANAL;
  peer.ifidx   = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

void funkStarten() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(KANAL, WIFI_SECOND_CHAN_NONE);
  WiFi.setTxPower(SENDELEISTUNG);
  if (esp_now_init() != ESP_OK) { Serial.println("ESP-NOW Start fehlgeschlagen"); return; }
  esp_now_register_send_cb(gesendet);
  esp_now_register_recv_cb(empfangen);
  partnerEintragen(ALLE);
  if (partnerBekannt) partnerEintragen(bedienMac);
}

void funkStoppen() {
  esp_now_deinit();
  WiFi.mode(WIFI_OFF);
}

uint8_t tastenLesen() {
  uint8_t b = 0;
  for (int i = 0; i < 5; i++) if (digitalRead(TASTEN_PINS[i]) == LOW) b |= (1 << i);
  return b;
}

void senden(uint8_t tasten) {
  static uint8_t zaehler = 0;
  FunkPaket p; p.tasten = tasten; p.zaehler = zaehler++;
  esp_now_send(partnerBekannt ? bedienMac : ALLE, (const uint8_t*)&p, sizeof(p));
}

void zeigeVersion() {
  Serial.print("Fernsteuerung Version ");
  Serial.print(PROGRAMM_VERSION);
  Serial.println(BOARD_XIAO ? " (XIAO ESP32C3)" : " (C3 SuperMini)");
  if (AKKU_PIN >= 0) {
    Serial.printf("Akku: %d,%02d V%s\n", akkuMillivolt / 1000, (akkuMillivolt % 1000) / 10,
                  akkuLeer ? "  -> JETZT LADEN" : (akkuNiedrig ? "  -> bald laden" : ""));
  }
}

// Akkuspannung messen: 16 Messungen mitteln, alle AKKU_TAKT_MS (sofort, wenn jetzt == 0)
void messeAkku(uint32_t jetzt) {
  static uint32_t letzteMessung = 0;
  if (AKKU_PIN < 0) return;
  if (letzteMessung != 0 && jetzt - letzteMessung < AKKU_TAKT_MS) return;
  letzteMessung = jetzt | 1;

  uint32_t summe = 0;
  for (int i = 0; i < 16; i++) summe += analogReadMilliVolts(AKKU_PIN);
  const int mv = (int)((summe / 16) * AKKU_TEILER * AKKU_KORREKTUR);

  // leicht glätten, damit kurze Einbrüche beim Funken nicht stören
  akkuMillivolt = (akkuMillivolt == 0) ? mv : (akkuMillivolt * 3 + mv) / 4;

  if (akkuMillivolt < AKKU_NIEDRIG_MV) akkuNiedrig = true;
  if (akkuMillivolt < AKKU_LEER_MV)    akkuLeer    = true;
  if (akkuMillivolt > AKKU_WIEDER_OK_MV) akkuNiedrig = akkuLeer = false;
}

void led(bool an) {
  if (LED_PIN >= 0) digitalWrite(LED_PIN, (an && !akkuNiedrig) ? HIGH : LOW);   // grün
  if (LED_ROT >= 0) digitalWrite(LED_ROT, (an &&  akkuNiedrig) ? HIGH : LOW);   // rot
}

// Stromsparschlaf bis zum nächsten Tastendruck
void schlafen() {
  led(false);
  funkStoppen();
  for (uint8_t pin : TASTEN_PINS) {
    gpio_sleep_sel_dis((gpio_num_t)pin);                  // Pull-up bleibt im Schlaf aktiv
    gpio_wakeup_enable((gpio_num_t)pin, GPIO_INTR_LOW_LEVEL);
  }
  esp_sleep_enable_gpio_wakeup();
  Serial.flush();
  esp_light_sleep_start();
  for (uint8_t pin : TASTEN_PINS) gpio_wakeup_disable((gpio_num_t)pin);
  funkStarten();
}

// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1500);     // Zeit, damit der Serielle Monitor nach dem Hochladen mitliest
  zeigeVersion();
  for (uint8_t pin : TASTEN_PINS) pinMode(pin, INPUT_PULLUP);
  if (LED_PIN >= 0) pinMode(LED_PIN, OUTPUT);
  if (LED_ROT >= 0) pinMode(LED_ROT, OUTPUT);
  led(false);
  if (AKKU_PIN >= 0) { pinMode(AKKU_PIN, INPUT); analogSetPinAttenuation(AKKU_PIN, ADC_11db); }
  messeAkku(0);   // sofort einmal messen
  if (BOARD_LED >= 0) { pinMode(BOARD_LED, OUTPUT); digitalWrite(BOARD_LED, HIGH); }   // blaue LED aus
  funkStarten();
  Serial.print("Fernsteuerung bereit, eigene Adresse: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  static uint8_t  tastenAlt     = 0;
  static uint32_t letztesSenden = 0;
  static uint32_t letzteTaste   = 0;
  const uint32_t jetzt = millis();

  // neues Bedienteil gemeldet? -> ab jetzt direkt senden
  if (neuerPartner) {
    portENTER_CRITICAL(&mux);
    memcpy(bedienMac, neueMac, 6); neuerPartner = false;
    portEXIT_CRITICAL(&mux);
    partnerEintragen(bedienMac);
    partnerBekannt = true;
    letzteBestaetigung = jetzt;
    Serial.println("Bedienteil gefunden.");
  }

  // "v" im Seriellen Monitor -> Version anzeigen
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == 'v' || c == 'V') zeigeVersion();
  }

  const uint8_t tasten = tastenLesen();
  if (tasten) letzteTaste = jetzt;

  const bool faellig = (tasten != tastenAlt) ||
                       (tasten && jetzt - letztesSenden >= SENDE_TAKT_MS) ||
                       (jetzt - letztesSenden >= HERZSCHLAG_MS);
  if (faellig) {
    senden(tasten);
    letztesSenden = jetzt;
    tastenAlt = tasten;
  }

  // keine Bestätigung seit 3 s -> neu suchen (z. B. anderes Bedienteil)
  if (partnerBekannt && jetzt - letzteBestaetigung > 3000) partnerBekannt = false;

  messeAkku(jetzt);

  // LED: an = verbunden, blinkt = keine Verbindung; Farbe rot = Akku niedrig
  if (akkuLeer) led(jetzt % 250 < 125);                       // rot schnell = jetzt laden
  else          led(partnerBekannt ? true : (jetzt % 500 < 250));

  if (jetzt - letzteTaste > SCHLAF_NACH_MS) {
    schlafen();
    letzteTaste = millis();
    messeAkku(0);   // nach dem Aufwachen gleich neu messen
  }
  delay(10);
}
