// ESP8266 lamp, bediend via internet
// De ESP luistert naar een openbare MQTT server (broker.hivemq.com).
// De webpagina stuurt daar "aan" of "uit" naartoe.
// Nodig: bibliotheek "PubSubClient" van Nick O'Leary (Bibliotheekbeheer).

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ---------- Instellingen ----------
const char* WIFI_NAAM       = "jouw_wifi";
const char* WIFI_WACHTWOORD = "jouw_wachtwoord";

// Moet exact gelijk zijn aan CODE in lamp.html
const char* ONDERWERP_ZET    = "esplamp/78lb4mti/zet";
const char* ONDERWERP_STATUS = "esplamp/78lb4mti/status";

const int  LED_PIN         = D1;     // D1 -> 220 ohm -> LED -> GND
const bool LED_ACTIEF_LAAG = false;  // true voor de ingebouwde LED (LED_PIN = D4)
// ----------------------------------

const char* BROKER = "broker.hivemq.com";

WiFiClient wifi;
PubSubClient mqtt(wifi);
bool lampAan = false;

void zetLamp(bool aan) {
  lampAan = aan;
  bool niveau = LED_ACTIEF_LAAG ? !aan : aan;
  digitalWrite(LED_PIN, niveau ? HIGH : LOW);
  mqtt.publish(ONDERWERP_STATUS, aan ? "aan" : "uit", true);  // true = onthouden
  Serial.println(aan ? "Lamp aan" : "Lamp uit");
}

void bericht(char* onderwerp, byte* data, unsigned int lengte) {
  String tekst;
  for (unsigned int i = 0; i < lengte; i++) tekst += (char)data[i];
  if (tekst == "aan") zetLamp(true);
  else if (tekst == "uit") zetLamp(false);
}

void verbindMqtt() {
  while (!mqtt.connected()) {
    Serial.print("Verbinden met MQTT...");
    String id = "esplamp-" + String(ESP.getChipId(), HEX);
    if (mqtt.connect(id.c_str())) {
      Serial.println(" gelukt");
      mqtt.subscribe(ONDERWERP_ZET);
      mqtt.publish(ONDERWERP_STATUS, lampAan ? "aan" : "uit", true);
    } else {
      Serial.print(" mislukt, code ");
      Serial.println(mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_ACTIEF_LAAG ? HIGH : LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NAAM, WIFI_WACHTWOORD);
  Serial.print("\nVerbinden met wifi");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println(" gelukt");

  mqtt.setServer(BROKER, 1883);
  mqtt.setCallback(bericht);
}

void loop() {
  if (!mqtt.connected()) verbindMqtt();
  mqtt.loop();
}
