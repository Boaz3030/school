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
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Mijn Lamp</title>
<style>
  :root { --aan: #ffd34d; --uit: #3a4256; --blauw: #4da3ff; }
  * { box-sizing: border-box; }
  body { margin: 0; min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 16px;
    font-family: system-ui, -apple-system, sans-serif; color: #eef2ff;
    background: radial-gradient(circle at 30% 20%, #2b3a67, #0b1020 70%); }
  .kaart { position: relative; width: min(360px, 100%); padding: 32px 24px; text-align: center; border-radius: 28px;
    background: rgba(255,255,255,0.08); backdrop-filter: blur(18px);
    border: 1px solid rgba(255,255,255,0.18); box-shadow: 0 0 30px rgba(120,160,255,0.25); }
  h1 { margin: 0 0 18px; font-size: 22px; font-weight: 600; }
  .wifiknop { position: absolute; top: 16px; right: 16px; width: 44px; height: 44px; border-radius: 50%; cursor: pointer;
    display: flex; align-items: center; justify-content: center; color: #cfe0ff;
    background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.25); }
  .wifiknop:hover { box-shadow: 0 0 14px var(--blauw); }
  .wifiknop svg { width: 22px; height: 22px; }
  .esp { display: flex; align-items: center; justify-content: center; gap: 10px; margin-bottom: 22px; font-size: 15px; min-height: 22px; }
  .draai { width: 18px; height: 18px; flex: none; border-radius: 50%; border: 3px solid rgba(255,255,255,0.2);
    border-top-color: var(--blauw); animation: draai 0.9s linear infinite; }
  @keyframes draai { to { transform: rotate(360deg); } }
  .stip { width: 12px; height: 12px; flex: none; border-radius: 50%; background: #2ecc71; box-shadow: 0 0 10px #2ecc71; }
  .lamp { width: 120px; height: 120px; margin: 0 auto 28px; border-radius: 50%; background: var(--uit); transition: all .35s; }
  .aan .lamp { background: var(--aan); box-shadow: 0 0 60px 20px rgba(255,211,77,.55); }
  .knoppen { display: flex; gap: 14px; }
  .knop { flex: 1; padding: 16px 0; font-size: 18px; font-weight: 700; letter-spacing: 1px; color: #fff; cursor: pointer;
    border-radius: 999px; background: rgba(255,255,255,0.12); border: 1px solid rgba(255,255,255,0.3); transition: .2s; }
  .knop:disabled { opacity: .4; cursor: default; }
  .aan .aanknop { background: #2ecc71; box-shadow: 0 0 18px #2ecc71; }
  .kaart:not(.aan) .uitknop:not(:disabled) { background: #e74c3c; box-shadow: 0 0 18px #e74c3c; }
  .tekst { margin-top: 18px; font-size: 18px; opacity: .85; }
  .verbinding { margin-top: 10px; font-size: 13px; opacity: .6; line-height: 1.5; }

  .achter { position: fixed; inset: 0; display: none; align-items: center; justify-content: center; padding: 16px;
    background: rgba(5,8,20,0.6); backdrop-filter: blur(6px); }
  .achter.open { display: flex; }
  .paneel { width: min(380px, 100%); max-height: calc(100vh - 32px); overflow-y: auto; padding: 22px 20px; border-radius: 26px;
    background: rgba(30,40,70,0.92); border: 1px solid rgba(255,255,255,0.2); box-shadow: 0 0 30px rgba(77,163,255,0.3); }
  .kop { display: flex; align-items: center; justify-content: space-between; margin-bottom: 12px; }
  .kop h2 { margin: 0; font-size: 20px; }
  .sluit { width: 36px; height: 36px; border-radius: 50%; border: 0; font-size: 18px; color: #fff; cursor: pointer; background: rgba(255,255,255,0.1); }
  .huidig { padding: 12px 14px; margin-bottom: 14px; border-radius: 14px; font-size: 14px; line-height: 1.5; background: rgba(255,255,255,0.06); }
  .rij { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; font-size: 14px; opacity: .85; }
  .klein { background: none; border: 0; color: #9cc3ff; font-size: 14px; cursor: pointer; padding: 4px; }
  .lijst { max-height: 220px; overflow-y: auto; margin-bottom: 12px; }
  .net { display: flex; justify-content: space-between; width: 100%; padding: 12px 14px; margin-bottom: 8px; cursor: pointer;
    border-radius: 14px; border: 1px solid rgba(255,255,255,0.2); background: rgba(255,255,255,0.06); color: #fff; font-size: 15px; text-align: left; }
  .net.gekozen { border-color: var(--blauw); box-shadow: 0 0 12px var(--blauw); }
  .net span:last-child { opacity: .6; font-size: 13px; }
  input { width: 100%; padding: 12px 14px; margin-bottom: 10px; border-radius: 14px; font-size: 16px;
    border: 1px solid rgba(255,255,255,0.25); background: rgba(0,0,0,0.25); color: #fff; }
  .hoofd, .tweede { width: 100%; padding: 14px; border-radius: 999px; font-size: 16px; font-weight: 700; color: #fff; cursor: pointer; }
  .hoofd { border: 0; background: #2f80ed; box-shadow: 0 0 18px #2f80ed; }
  .tweede { margin-top: 12px; background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.3); }
  .hoofd:disabled, .tweede:disabled { opacity: .4; }
  .status { display: flex; align-items: flex-start; gap: 12px; margin-top: 16px; min-height: 24px; font-size: 15px; text-align: left; line-height: 1.45; }
  .ok { color: #2ecc71; } .fout { color: #ff6b6b; }
  .icoon { font-size: 22px; flex: none; line-height: 1; }
</style>
</head>
<body>
  <div class="kaart" id="kaart">
    <button class="wifiknop" id="wifiknop" title="Wifi" aria-label="Wifi instellen">
      <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round">
        <path d="M2 8.5a15 15 0 0 1 20 0"/><path d="M5 12a10.5 10.5 0 0 1 14 0"/><path d="M8.5 15.5a5.5 5.5 0 0 1 7 0"/>
        <circle cx="12" cy="19" r="1.2" fill="currentColor"/></svg>
    </button>
    <h1>Mijn Lamp</h1>
    <div class="esp" id="esp"><div class="draai"></div><span>ESP zoeken...</span></div>
    <div class="lamp"></div>
    <div class="knoppen">
      <button id="aan" class="knop aanknop" disabled>AAN</button>
      <button id="uit" class="knop uitknop" disabled>UIT</button>
    </div>
    <div class="tekst" id="tekst">Wachten op lamp...</div>
    <div class="verbinding" id="verbinding"></div>
  </div>

  <div class="achter" id="achter">
    <div class="paneel">
      <div class="kop"><h2>Wifi</h2><button class="sluit" id="sluit" aria-label="Sluiten">✕</button></div>
      <div class="huidig" id="huidig">Netwerk: onbekend</div>
      <div class="rij"><span>Kies een netwerk voor de ESP</span><button class="klein" id="opnieuw">Opnieuw zoeken</button></div>
      <div class="lijst" id="lijst"></div>
      <input id="naam" placeholder="Netwerknaam" autocomplete="off" autocapitalize="off">
      <input id="ww" type="password" placeholder="Wachtwoord">
      <button class="hoofd" id="verbind">Verbinden</button>
      <button class="tweede" id="lokaal">Alleen ESP netwerk (ESP-Lamp)</button>
      <div class="status" id="status"></div>
    </div>
  </div>

<script>
  // Deze pagina komt van de ESP zelf. Open haar op hetzelfde wifi als de ESP:
  // http://esp-lamp.local (of het IP adres). Op wifi ESP-Lamp: ook http://192.168.4.1
  const $ = id => document.getElementById(id);
  const kaart = $('kaart'), knopAan = $('aan'), knopUit = $('uit');
  let volgen = null;

  function toonLamp(aan) {
    kaart.classList.toggle('aan', aan);
    $('tekst').textContent = aan ? 'Lamp aan' : 'Lamp uit';
  }
  function toonEsp(ok, tekst) {
    const e = $('esp'); e.innerHTML = '';
    const a = document.createElement('div'); a.className = ok ? 'stip' : 'draai';
    const b = document.createElement('span'); b.textContent = tekst;
    e.append(a, b);
    knopAan.disabled = !ok; knopUit.disabled = !ok;
  }
  function toonStatus(soort, tekst) {
    const s = $('status'); s.innerHTML = '';
    if (!soort) return;
    const a = document.createElement('div');
    if (soort === 'zoeken') a.className = 'draai';
    else { a.className = 'icoon ' + soort; a.textContent = soort === 'ok' ? '✓' : '✗'; }
    const b = document.createElement('div'); b.textContent = tekst;
    if (soort !== 'zoeken') b.className = soort;
    s.append(a, b);
  }

  function vraagStatus() {
    fetch('/status').then(r => r.json()).then(d => {
      toonEsp(true, 'Verbonden met ESP');
      toonLamp(d.aan);
      $('huidig').textContent = d.netwerk
        ? 'ESP netwerk: ' + d.netwerk + (d.verbonden ? ' (verbonden, adres http://esp-lamp.local of http://' + d.ip + ')' : ' (zoeken...)')
        : 'ESP netwerk: geen, alleen ESP-Lamp';
      $('verbinding').textContent = d.verbonden
        ? 'ESP op ' + d.netwerk + ': http://esp-lamp.local of http://' + d.ip
        : 'ESP op eigen wifi ESP-Lamp';
    }).catch(() => toonEsp(false, 'ESP niet bereikbaar. Zit je op hetzelfde wifi?'));
  }

  function zoekNetwerken() {
    const lijst = $('lijst'); lijst.innerHTML = '';
    toonStatus('zoeken', 'Netwerken zoeken...');
    fetch('/scan').then(r => r.json()).then(netten => {
      toonStatus();
      netten.sort((a, b) => b.rssi - a.rssi);
      if (!netten.length) lijst.textContent = 'Geen netwerken gevonden.';
      netten.forEach(n => {
        const b = document.createElement('button'); b.className = 'net';
        const s1 = document.createElement('span'); s1.textContent = n.naam;
        const s2 = document.createElement('span');
        s2.textContent = (n.rssi > -60 ? 'sterk' : n.rssi > -75 ? 'goed' : 'zwak') + (n.slot ? ' 🔒' : '');
        b.append(s1, s2);
        b.onclick = () => {
          document.querySelectorAll('.net').forEach(x => x.classList.remove('gekozen'));
          b.classList.add('gekozen'); $('naam').value = n.naam; $('ww').focus();
        };
        lijst.append(b);
      });
    }).catch(() => toonStatus('fout', 'Zoeken mislukt. Probeer opnieuw.'));
  }

  function volgToestand() {
    fetch('/toestand').then(r => r.json()).then(d => {
      if (d.t === 'zoeken') toonStatus('zoeken', 'Verbinden met ' + d.naam + '...');
      else if (d.t === 'verbonden') { clearInterval(volgen); $('verbind').disabled = false;
        toonStatus('ok', 'Verbonden met ' + d.naam + '. Zet je gsm op ' + d.naam +
          ' en open http://esp-lamp.local (of http://' + d.ip + ') om de lamp daar te bedienen.'); }
      else if (d.t === 'mislukt') { clearInterval(volgen); $('verbind').disabled = false;
        toonStatus('fout', 'Verbinden met ' + d.naam + ' lukt niet. Controleer naam en wachtwoord.'); }
    }).catch(() => { clearInterval(volgen); $('verbind').disabled = false;
      toonStatus('fout', 'Contact met de ESP even kwijt. Verbind opnieuw met wifi ESP-Lamp en kijk hier of het gelukt is.'); });
  }

  $('wifiknop').onclick = () => { $('achter').classList.add('open'); toonStatus(); zoekNetwerken(); };
  $('sluit').onclick = () => $('achter').classList.remove('open');
  $('achter').onclick = e => { if (e.target.id === 'achter') $('achter').classList.remove('open'); };
  $('opnieuw').onclick = zoekNetwerken;
  knopAan.onclick = () => fetch('/lamp?aan=1').then(vraagStatus);
  knopUit.onclick = () => fetch('/lamp?aan=0').then(vraagStatus);

  $('verbind').onclick = () => {
    const naam = $('naam').value;
    if (!naam) { toonStatus('fout', 'Kies eerst een netwerk.'); return; }
    $('verbind').disabled = true;
    toonStatus('zoeken', 'Verbinden met ' + naam + '...');
    fetch('/verbind', { method: 'POST', body: new URLSearchParams({ naam, wachtwoord: $('ww').value }) })
      .then(() => { clearInterval(volgen); volgen = setInterval(volgToestand, 1000); })
      .catch(() => { toonStatus('fout', 'Versturen mislukt.'); $('verbind').disabled = false; });
  };
  $('lokaal').onclick = () => {
    if (!confirm('De ESP verlaat zijn wifinetwerk en vergeet het. Hij is daarna alleen te bedienen via wifi ESP-Lamp. Doorgaan?')) return;
    fetch('/lokaal', { method: 'POST' }).then(() => { toonStatus('ok', 'De ESP gebruikt nu alleen zijn eigen wifi ESP-Lamp.'); vraagStatus(); });
  };

  vraagStatus();
  setInterval(vraagStatus, 2000);
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
