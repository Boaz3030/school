// ESP8266 lamp, bediend via internet, met wifi instellen zonder opnieuw uploaden.
//
// Werking:
//  1. Bij het opstarten probeert de ESP het laatst gebruikte wifinetwerk.
//  2. Lukt dat niet binnen 15 seconden, dan maakt hij zelf een wifinetwerk
//     "ESP-Lamp-Setup" (wachtwoord lamp1234). Verbind je gsm daarmee:
//     er opent vanzelf een pagina (anders: http://192.168.4.1).
//     Kies daar je wifi, typ het wachtwoord en tik op Verbinden.
//     Een laadicoon toont of hij nog zoekt, verbonden is of het mislukt is.
//  3. De ESP onthoudt dat netwerk, ook zonder stroom.
//  4. De lamp bedien je op https://boaz3030.github.io/school/
//     Daar zie je ook of de ESP online is, en met "Wifi wijzigen"
//     open je de instelpagina opnieuw.
//
// Nodig: bibliotheek "PubSubClient" van Nick O'Leary (Bibliotheekbeheer).

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>

// ---------- Instellingen ----------
const char* SETUP_NAAM       = "ESP-Lamp-Setup";
const char* SETUP_WACHTWOORD = "lamp1234";       // minstens 8 tekens

// Moet gelijk zijn aan CODE in index.html
const char* ONDERWERP_ZET    = "esplamp/78lb4mti/zet";
const char* ONDERWERP_STATUS = "esplamp/78lb4mti/status";
const char* ONDERWERP_ONLINE = "esplamp/78lb4mti/online";

const int  LED_PIN         = 5;      // 5 = pin D1 op het bord. D1 -> 220 ohm -> LED -> GND
const bool LED_ACTIEF_LAAG = false;  // true voor de ingebouwde LED (LED_PIN = 2, dat is D4)
// ----------------------------------

const char* BROKER = "broker.hivemq.com";

ESP8266WebServer server(80);
DNSServer dns;
WiFiClient net;
PubSubClient mqtt(net);

bool lampAan = false;
bool portaalActief = false;
unsigned long portaalStart = 0;
unsigned long wifiWegSinds = 0;
unsigned long laatsteMqttPoging = 0;

// Toestand van een verbindingspoging vanuit de instelpagina
enum Toestand { WACHT, ZOEKEN, VERBONDEN, MISLUKT };
Toestand toestand = WACHT;
unsigned long zoekStart = 0;
unsigned long verbondenSinds = 0;
String gekozenNaam = "";

// ================= Lamp =================

void zetLamp(bool aan) {
  lampAan = aan;
  bool niveau = LED_ACTIEF_LAAG ? !aan : aan;
  digitalWrite(LED_PIN, niveau ? HIGH : LOW);
  if (mqtt.connected()) mqtt.publish(ONDERWERP_STATUS, aan ? "aan" : "uit", true);
  Serial.println(aan ? "Lamp aan" : "Lamp uit");
}

// ================= Instelpagina (op de ESP zelf) =================

const char PORTAAL[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Lamp wifi instellen</title>
<style>
  * { box-sizing: border-box; }
  body { margin: 0; min-height: 100vh; display: flex; align-items: center; justify-content: center;
    font-family: system-ui, -apple-system, sans-serif; color: #eef2ff;
    background: radial-gradient(circle at 30% 20%, #2b3a67, #0b1020 70%); padding: 16px; }
  .kaart { width: min(380px, 100%); padding: 26px 20px; border-radius: 28px;
    background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.18);
    box-shadow: 0 0 30px rgba(120,160,255,0.25); }
  h1 { margin: 0 0 6px; font-size: 21px; text-align: center; }
  .uitleg { margin: 0 0 18px; font-size: 14px; opacity: .7; text-align: center; }
  .lijst { max-height: 230px; overflow-y: auto; margin-bottom: 14px; }
  .net { display: flex; justify-content: space-between; width: 100%; padding: 12px 14px; margin-bottom: 8px;
    border-radius: 14px; border: 1px solid rgba(255,255,255,0.2); background: rgba(255,255,255,0.06);
    color: #fff; font-size: 15px; cursor: pointer; text-align: left; }
  .net.gekozen { border-color: #4da3ff; box-shadow: 0 0 12px #4da3ff; }
  .net span:last-child { opacity: .6; font-size: 13px; }
  input { width: 100%; padding: 12px 14px; margin-bottom: 10px; border-radius: 14px; font-size: 16px;
    border: 1px solid rgba(255,255,255,0.25); background: rgba(0,0,0,0.25); color: #fff; }
  button.hoofd { width: 100%; padding: 14px; border: 0; border-radius: 999px; font-size: 17px; font-weight: 700;
    color: #fff; background: #2f80ed; box-shadow: 0 0 18px #2f80ed; cursor: pointer; }
  button.hoofd:disabled { opacity: .4; }
  .klein { background: none; border: 0; color: #9cc3ff; font-size: 14px; cursor: pointer; padding: 4px; }
  .status { display: flex; align-items: center; gap: 12px; margin-top: 18px; min-height: 28px; font-size: 15px; }
  .draai { width: 24px; height: 24px; flex: none; border-radius: 50%;
    border: 3px solid rgba(255,255,255,0.2); border-top-color: #4da3ff; animation: draai 0.9s linear infinite; }
  @keyframes draai { to { transform: rotate(360deg); } }
  .ok { color: #2ecc71; } .fout { color: #ff6b6b; }
  .icoon { font-size: 22px; flex: none; }
</style>
</head>
<body>
<div class="kaart">
  <h1>Wifi instellen</h1>
  <p class="uitleg">Kies het netwerk waarmee de lamp moet verbinden.</p>
  <div style="text-align:right"><button class="klein" id="opnieuw">Opnieuw zoeken</button></div>
  <div class="lijst" id="lijst"></div>
  <input id="naam" placeholder="Netwerknaam" autocomplete="off" autocapitalize="off">
  <input id="ww" type="password" placeholder="Wachtwoord">
  <button class="hoofd" id="verbind">Verbinden</button>
  <div class="status" id="status"></div>
</div>
<script>
  const lijst = document.getElementById('lijst');
  const naam = document.getElementById('naam');
  const ww = document.getElementById('ww');
  const knop = document.getElementById('verbind');
  const status = document.getElementById('status');
  let volgen = null;

  function toonStatus(soort, tekst) {
    status.innerHTML = '';
    const links = document.createElement('div');
    if (soort === 'zoeken') links.className = 'draai';
    else { links.className = 'icoon ' + (soort === 'ok' ? 'ok' : 'fout'); links.textContent = soort === 'ok' ? '✓' : '✗'; }
    const t = document.createElement('div');
    t.textContent = tekst;
    if (soort === 'ok') t.className = 'ok';
    if (soort === 'fout') t.className = 'fout';
    status.append(links, t);
  }

  function zoekNetwerken() {
    lijst.innerHTML = '';
    toonStatus('zoeken', 'Netwerken zoeken...');
    fetch('/scan').then(r => r.json()).then(netten => {
      status.innerHTML = '';
      netten.sort((a, b) => b.rssi - a.rssi);
      if (!netten.length) lijst.textContent = 'Geen netwerken gevonden.';
      netten.forEach(n => {
        const b = document.createElement('button');
        b.className = 'net';
        const s1 = document.createElement('span'); s1.textContent = n.naam;
        const s2 = document.createElement('span');
        s2.textContent = (n.rssi > -60 ? 'sterk' : n.rssi > -75 ? 'goed' : 'zwak') + (n.slot ? ' 🔒' : '');
        b.append(s1, s2);
        b.onclick = () => {
          document.querySelectorAll('.net').forEach(x => x.classList.remove('gekozen'));
          b.classList.add('gekozen');
          naam.value = n.naam;
          ww.focus();
        };
        lijst.append(b);
      });
    }).catch(() => toonStatus('fout', 'Zoeken mislukt. Probeer opnieuw.'));
  }

  function volgToestand() {
    fetch('/toestand').then(r => r.json()).then(d => {
      if (d.t === 'zoeken') toonStatus('zoeken', 'Verbinden met ' + d.naam + '...');
      else if (d.t === 'verbonden') {
        clearInterval(volgen);
        toonStatus('ok', 'Verbonden met ' + d.naam + '. Bedien de lamp nu op boaz3030.github.io/school. Dit setup netwerk sluit zo vanzelf.');
        knop.disabled = false;
      } else if (d.t === 'mislukt') {
        clearInterval(volgen);
        toonStatus('fout', 'Verbinden met ' + d.naam + ' lukt niet. Controleer naam en wachtwoord.');
        knop.disabled = false;
      }
    }).catch(() => {
      clearInterval(volgen);
      toonStatus('fout', 'Contact met de lamp verloren. Kijk op boaz3030.github.io/school of hij online is.');
      knop.disabled = false;
    });
  }

  knop.onclick = () => {
    if (!naam.value) { toonStatus('fout', 'Kies eerst een netwerk.'); return; }
    knop.disabled = true;
    toonStatus('zoeken', 'Verbinden met ' + naam.value + '...');
    fetch('/verbind', { method: 'POST', body: new URLSearchParams({ naam: naam.value, wachtwoord: ww.value }) })
      .then(() => { clearInterval(volgen); volgen = setInterval(volgToestand, 1000); })
      .catch(() => { toonStatus('fout', 'Versturen mislukt.'); knop.disabled = false; });
  };

  document.getElementById('opnieuw').onclick = zoekNetwerken;
  zoekNetwerken();
</script>
</body>
</html>
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
  server.send(200, "application/json", "{\"ok\":true}");
}

void paginaToestand() {
  const char* t = toestand == ZOEKEN ? "zoeken" : toestand == VERBONDEN ? "verbonden"
                : toestand == MISLUKT ? "mislukt" : "wacht";
  String j = "{\"t\":\"" + String(t) + "\",\"naam\":\"" + jsonTekst(gekozenNaam) + "\"}";
  server.send(200, "application/json", j);
}

void paginaOnbekend() {
  // Stuurt elke onbekende vraag naar de instelpagina, zodat de gsm ze vanzelf opent
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

void startPortaal() {
  if (portaalActief) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(SETUP_NAAM, SETUP_WACHTWOORD);
  dns.start(53, "*", WiFi.softAPIP());
  portaalActief = true;
  portaalStart = millis();
  toestand = WACHT;
  Serial.println();
  Serial.println("=== Instelmodus ===");
  Serial.print("Verbind je gsm met wifi '");
  Serial.print(SETUP_NAAM);
  Serial.print("' (wachtwoord ");
  Serial.print(SETUP_WACHTWOORD);
  Serial.print(") en open http://");
  Serial.println(WiFi.softAPIP());
}

void stopPortaal() {
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  portaalActief = false;
  toestand = WACHT;
  Serial.println("Instelmodus gesloten");
}

void beheerPortaal() {
  dns.processNextRequest();
  server.handleClient();

  if (toestand == ZOEKEN) {
    wl_status_t s = WiFi.status();
    // Eerst 1 s wachten, zodat een oude verbinding niet als geslaagd telt
    if (s == WL_CONNECTED && millis() - zoekStart > 1000 && WiFi.SSID() == gekozenNaam) {
      toestand = VERBONDEN;
      verbondenSinds = millis();
      Serial.print("Wifi verbonden, IP ");
      Serial.println(WiFi.localIP());
    } else if (s == WL_CONNECT_FAILED || millis() - zoekStart > 20000) {
      toestand = MISLUKT;
      Serial.println("Wifi verbinden mislukt");
    }
  }

  // Na een geslaagde verbinding: nog 15 s open, zodat de gsm het vinkje ziet
  if (toestand == VERBONDEN && millis() - verbondenSinds > 15000) stopPortaal();

  // Geopend via "Wifi wijzigen" maar niets gekozen: na 5 minuten weer dicht
  if (toestand == WACHT && WiFi.status() == WL_CONNECTED && millis() - portaalStart > 300000) stopPortaal();
}

// ================= MQTT =================

void bericht(char* onderwerp, byte* data, unsigned int lengte) {
  String tekst;
  for (unsigned int i = 0; i < lengte; i++) tekst += (char)data[i];
  if (tekst == "aan") zetLamp(true);
  else if (tekst == "uit") zetLamp(false);
  else if (tekst == "wifi") startPortaal();
}

void probeerMqtt() {
  if (millis() - laatsteMqttPoging < 5000) return;
  laatsteMqttPoging = millis();
  Serial.print("Verbinden met MQTT...");
  String id = "esplamp-" + String(ESP.getChipId(), HEX);
  // Laatste wil: als de ESP wegvalt, meldt de server zelf "offline"
  if (mqtt.connect(id.c_str(), ONDERWERP_ONLINE, 0, true, "offline")) {
    Serial.println(" gelukt");
    mqtt.subscribe(ONDERWERP_ZET);
    mqtt.publish(ONDERWERP_ONLINE, "online", true);
    mqtt.publish(ONDERWERP_STATUS, lampAan ? "aan" : "uit", true);
  } else {
    Serial.print(" mislukt, code ");
    Serial.println(mqtt.state());
  }
}

// ================= Start en lus =================

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_ACTIEF_LAAG ? HIGH : LOW);

  server.on("/", []() { server.send_P(200, "text/html", PORTAAL); });
  server.on("/scan", paginaScan);
  server.on("/verbind", HTTP_POST, paginaVerbind);
  server.on("/toestand", paginaToestand);
  server.onNotFound(paginaOnbekend);
  server.begin();

  mqtt.setServer(BROKER, 1883);
  mqtt.setCallback(bericht);

  WiFi.mode(WIFI_STA);
  WiFi.begin();   // laatst onthouden netwerk
  Serial.println();
  Serial.print("Verbinden met opgeslagen wifi '");
  Serial.print(WiFi.SSID());
  Serial.print("'");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(" gelukt, IP ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" niet gevonden");
    startPortaal();
  }
}

void loop() {
  if (portaalActief) beheerPortaal();

  if (WiFi.status() == WL_CONNECTED) {
    wifiWegSinds = 0;
    if (!mqtt.connected()) probeerMqtt();
    else mqtt.loop();
  } else if (!portaalActief) {
    // Wifi 30 s weg? Dan instelmodus openen
    if (wifiWegSinds == 0) wifiWegSinds = millis();
    else if (millis() - wifiWegSinds > 30000) startPortaal();
  }
}
