// ESP8266 lamp met één bedieningspagina.
//
// Werking:
//  - De ESP maakt altijd zijn eigen wifi "ESP-Lamp" (wachtwoord lamp1234).
//  - Open de pagina op hetzelfde wifi als de ESP:  http://esp-lamp.local
//    (lukt dat niet: het IP adres uit de Seriële monitor; op ESP-Lamp ook http://192.168.4.1)
//  - Rechtsboven op de pagina zit de wifiknop: kies daar een netwerk voor de ESP,
//    of "Alleen ESP netwerk". De ESP onthoudt het netwerk, ook zonder stroom.
//  - Zit je gsm op hetzelfde netwerk als de ESP, dan bedien je de lamp. Internet is niet nodig.
//
// Geen extra bibliotheken nodig: alles zit in het esp8266 bordpakket.
//
// LET OP: esp_lamp_online.ino wordt gemaakt door onderhoud/maak_sketch.py uit
// onderhoud/sketch_bron.ino en onderhoud/pagina.html. Pas die aan, niet de kopie.

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>

// ---------- Instellingen ----------
const char* ESP_NAAM       = "ESP-Lamp";
const char* ESP_WACHTWOORD = "lamp1234";   // minstens 8 tekens
const char* ADRES_NAAM     = "esp-lamp";   // geeft http://esp-lamp.local

const int  LED_PIN         = 5;      // 5 = pin D1 op het bord. D1 -> 220 ohm -> LED -> GND
const bool LED_ACTIEF_LAAG = false;  // true voor de ingebouwde LED (LED_PIN = 2, dat is D4)
// ----------------------------------

ESP8266WebServer server(80);
DNSServer dns;

bool lampAan = false;

// Opnieuw proberen van het opgeslagen netwerk, met pauzes zodat de eigen wifi stabiel blijft
unsigned long probeerStart = 0;
bool pauze = false;
unsigned long pauzeStart = 0;

// Toestand van een verbindingspoging vanuit de pagina
enum Toestand { WACHT, ZOEKEN, VERBONDEN, MISLUKT };
Toestand toestand = WACHT;
unsigned long zoekStart = 0;
String gekozenNaam = "";

// ================= Lamp =================

void zetLamp(bool aan) {
  lampAan = aan;
  bool niveau = LED_ACTIEF_LAAG ? !aan : aan;
  digitalWrite(LED_PIN, niveau ? HIGH : LOW);
  Serial.println(aan ? "Lamp aan" : "Lamp uit");
}

// ================= Pagina =================

const char PAGINA[] PROGMEM = R"rawliteral(
@@PAGINA@@
)rawliteral";

String jsonTekst(const String& s) {
  String r;
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { r += '\\'; r += c; }
    else if ((uint8_t)c >= 0x20) r += c;
  }
  return r;
}

bool wifiVerbonden() { return WiFi.status() == WL_CONNECTED; }

void paginaStatus() {
  String j = "{\"aan\":" + String(lampAan ? "true" : "false") +
             ",\"netwerk\":\"" + jsonTekst(WiFi.SSID()) + "\"" +
             ",\"verbonden\":" + String(wifiVerbonden() ? "true" : "false") +
             ",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  server.send(200, "application/json", j);
}

void paginaLamp() {
  if (server.hasArg("aan")) zetLamp(server.arg("aan") == "1");
  paginaStatus();
}

void paginaScan() {
  int n = WiFi.scanNetworks();
  String j = "[";
  bool eerste = true;
  for (int i = 0; i < n; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue;
    bool dubbel = false;
    for (int k = 0; k < i; k++) if (WiFi.SSID(k) == s) { dubbel = true; break; }
    if (dubbel) continue;
    if (!eerste) j += ",";
    eerste = false;
    j += "{\"naam\":\"" + jsonTekst(s) + "\",\"rssi\":" + String(WiFi.RSSI(i)) +
         ",\"slot\":" + String(WiFi.encryptionType(i) != ENC_TYPE_NONE ? "true" : "false") + "}";
  }
  j += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", j);
}

void paginaVerbind() {
  gekozenNaam = server.arg("naam");
  String wachtwoord = server.arg("wachtwoord");
  Serial.print("Nieuw netwerk gekozen: ");
  Serial.println(gekozenNaam);
  WiFi.begin(gekozenNaam.c_str(), wachtwoord.c_str());   // wordt ook onthouden
  toestand = ZOEKEN;
  zoekStart = millis();
  pauze = false;
  probeerStart = millis();
  server.send(200, "application/json", "{\"ok\":true}");
}

void paginaToestand() {
  const char* t = toestand == ZOEKEN ? "zoeken" : toestand == VERBONDEN ? "verbonden"
                : toestand == MISLUKT ? "mislukt" : "wacht";
  String j = "{\"t\":\"" + String(t) + "\",\"naam\":\"" + jsonTekst(gekozenNaam) +
             "\",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  server.send(200, "application/json", j);
}

void paginaLokaal() {
  server.send(200, "application/json", "{\"ok\":true}");
  Serial.println("Alleen ESP netwerk");
  WiFi.disconnect();      // verlaat het netwerk en vergeet het
  toestand = WACHT;
  gekozenNaam = "";
  pauze = false;
  probeerStart = 0;
}

void paginaOnbekend() {
  // Stuurt elke onbekende vraag naar de bedieningspagina, zodat de gsm ze vanzelf opent
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

// ================= Wifi =================

void beheerWifi() {
  bool verbonden = wifiVerbonden();

  if (toestand == ZOEKEN) {
    // Eerst 1 s wachten, zodat een oude verbinding niet als geslaagd telt
    if (verbonden && millis() - zoekStart > 1000 && WiFi.SSID() == gekozenNaam) {
      toestand = VERBONDEN;
    } else if (WiFi.status() == WL_CONNECT_FAILED || millis() - zoekStart > 20000) {
      toestand = MISLUKT;
      Serial.println("Wifi verbinden mislukt");
    }
    return;
  }

  if (verbonden) { probeerStart = 0; pauze = false; return; }
  if (WiFi.SSID().length() == 0) return;   // geen netwerk opgeslagen: alleen ESP-Lamp

  // Opgeslagen netwerk niet gevonden: 20 s proberen, 60 s pauze, opnieuw
  if (!pauze) {
    if (probeerStart == 0) probeerStart = millis();
    else if (millis() - probeerStart > 20000) {
      Serial.print("Netwerk '");
      Serial.print(WiFi.SSID());
      Serial.println("' niet gevonden, opnieuw over 60 s");
      wifi_station_disconnect();   // stopt alleen het zoeken, vergeet niets
      pauze = true;
      pauzeStart = millis();
    }
  } else if (millis() - pauzeStart > 60000) {
    pauze = false;
    probeerStart = millis();
    WiFi.begin();
  }
}

// ================= Start en lus =================

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_ACTIEF_LAAG ? HIGH : LOW);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ESP_NAAM, ESP_WACHTWOORD);
  dns.start(53, "*", WiFi.softAPIP());

  server.on("/", []() { server.send_P(200, "text/html", PAGINA); });
  server.on("/status", paginaStatus);
  server.on("/lamp", paginaLamp);
  server.on("/scan", paginaScan);
  server.on("/verbind", HTTP_POST, paginaVerbind);
  server.on("/toestand", paginaToestand);
  server.on("/lokaal", HTTP_POST, paginaLokaal);
  server.onNotFound(paginaOnbekend);
  server.begin();

  if (MDNS.begin(ADRES_NAAM)) MDNS.addService("http", "tcp", 80);

  Serial.println();
  Serial.print("Eigen wifi: '");
  Serial.print(ESP_NAAM);
  Serial.print("' (wachtwoord ");
  Serial.print(ESP_WACHTWOORD);
  Serial.println("), pagina op http://esp-lamp.local of http://192.168.4.1");

  if (WiFi.SSID().length() > 0) {
    Serial.print("Verbinden met opgeslagen wifi '");
    Serial.print(WiFi.SSID());
    Serial.println("'...");
    WiFi.begin();
    probeerStart = millis();
  } else {
    Serial.println("Geen wifi opgeslagen: alleen ESP-Lamp");
  }
}

void loop() {
  dns.processNextRequest();
  server.handleClient();
  MDNS.update();
  beheerWifi();

  static bool wasVerbonden = false;
  bool verbonden = wifiVerbonden();
  if (verbonden && !wasVerbonden) {
    Serial.print("Wifi verbonden met '");
    Serial.print(WiFi.SSID());
    Serial.print("'. Pagina op dat netwerk: http://esp-lamp.local of http://");
    Serial.println(WiFi.localIP());
  }
  wasVerbonden = verbonden;
}
