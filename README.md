# ESP8266 lamp via internet

Een LED op een ESP8266 die je aan en uit zet met twee knoppen op een webpagina.

## 👉 [Open de interface (knoppen AAN en UIT)](https://boaz3030.github.io/school/)

## 👉 [Open het script voor Arduino IDE](esp_lamp_online.ino)

Klik in het script rechtsboven op het icoon **Copy raw file** en plak alles in Arduino IDE.

## Bestanden

- `esp_lamp_online.ino`: de sketch voor Arduino IDE.
- `index.html`: de webpagina met de knoppen AAN en UIT.

## Stappen

1. Sluit de LED aan: pin D1 → weerstand 220 Ω → LED (lange poot) → korte poot naar GND.
2. Installeer in Arduino IDE het bordpakket **esp8266** en de bibliotheek **PubSubClient** (Nick O'Leary).
3. Kopieer `esp_lamp_online.ino` naar Arduino IDE en upload. Je hoeft geen wifi in te vullen.
4. Verbind je gsm met wifi **ESP-Lamp-Setup** (wachtwoord **lamp1234**). Er opent een pagina; kies daar je wifi en typ het wachtwoord. Een laadicoon toont of de ESP nog zoekt, verbonden is of het mislukt is.
5. Open de interface: **https://boaz3030.github.io/school/**. Bovenaan staat "ESP online" met een groene stip; daarna werken AAN en UIT.

## Ander wifinetwerk

Niet opnieuw uploaden. De ESP onthoudt zijn laatste netwerk, ook zonder stroom.
Vindt hij dat netwerk niet binnen 15 seconden, dan maakt hij zelf **ESP-Lamp-Setup** aan (stap 4).
Is hij nog online, tik dan in de interface op **Wifi wijzigen**.

## Seriële monitor (115200 baud)

Toont welk netwerk hij probeert, of het lukt, het IP adres, de MQTT verbinding en elke keer dat de lamp aan of uit gaat.

## Hoe het werkt

De ESP en de webpagina verbinden allebei met de openbare MQTT server `broker.hivemq.com`.
De pagina stuurt `aan`, `uit` of `wifi` naar `esplamp/78lb4mti/zet`.
De ESP meldt de lamp op `esplamp/78lb4mti/status` en `online` of `offline` op `esplamp/78lb4mti/online`.
