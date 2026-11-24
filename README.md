# ESP8266 lamp via internet

Een LED op een ESP8266 die je aan en uit zet met twee knoppen op een webpagina.

## Bestanden

- `esp_lamp_online.ino`: de sketch voor Arduino IDE.
- `index.html`: de webpagina met de knoppen AAN en UIT.

## Stappen

1. Sluit de LED aan: pin D1 → weerstand 220 Ω → LED (lange poot) → korte poot naar GND.
2. Installeer in Arduino IDE het bordpakket **esp8266** en de bibliotheek **PubSubClient** (Nick O'Leary).
3. Open `esp_lamp_online.ino`, vul `WIFI_NAAM` en `WIFI_WACHTWOORD` in en upload.
4. Seriële monitor (115200 baud) moet tonen: `Verbinden met MQTT... gelukt`.
5. Open de pagina op gsm of computer: **https://boaz3030.github.io/school/**

## Hoe het werkt

De ESP en de webpagina verbinden allebei met de openbare MQTT server `broker.hivemq.com`.
De pagina stuurt `aan` of `uit` naar `esplamp/78lb4mti/zet`.
De ESP schakelt de LED en meldt de stand terug op `esplamp/78lb4mti/status`.
