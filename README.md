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
4. Verbind je gsm met wifi **ESP-Lamp** (wachtwoord **lamp1234**) en open **http://192.168.4.1** (opent vaak vanzelf).
   Daar bedien je de lamp meteen, ook zonder internet.
5. Tik rechtsboven op het **wifi icoon**, kies je netwerk, typ het wachtwoord en tik op **Verbinden**.
   Een laadicoon toont of de ESP nog zoekt; daarna een groen ✓ of een rood ✗.
6. Nu werkt de lamp ook via internet: **https://boaz3030.github.io/school/**
7. Zit je gsm op hetzelfde wifi als de ESP, dan werkt ook **http://esp-lamp.local** (of het IP adres uit de Seriële monitor), zelfs zonder internet.

## Welke link wanneer

| Situatie | Link |
|---|---|
| ESP heeft internet | https://boaz3030.github.io/school/ (van overal) |
| Gsm en ESP op hetzelfde wifi | http://esp-lamp.local of het IP adres |
| ESP heeft geen wifi | verbind met wifi ESP-Lamp, open http://192.168.4.1 |

De GitHub pagina kan de ESP niet rechtstreeks via het eigen wifi bedienen: de browser blokkeert dat voor beveiligde (https) pagina's.
Daarom staan de lokale links in het wifipaneel van de GitHub pagina.

## Wifi knop (rechtsboven)

- **Op de ESP pagina (192.168.4.1):** netwerk kiezen, of **Alleen lokaal ESP netwerk** (de ESP vergeet zijn wifi).
- **Op de GitHub pagina:** toont het huidige netwerk en kan de ESP naar **alleen lokaal** zetten.
  Een nieuw netwerk kies je op de ESP pagina, zodat je wachtwoord niet over het openbare internet gaat.

De ESP onthoudt zijn netwerk, ook zonder stroom. Vindt hij het niet, dan probeert hij elke minuut opnieuw;
zijn eigen wifi **ESP-Lamp** blijft altijd werken.

## Seriële monitor (115200 baud)

Toont de eigen wifi, welk netwerk hij probeert, of het lukt (met IP adres), de MQTT verbinding en elke keer dat de lamp aan of uit gaat.

## Hoe het werkt

De ESP en de GitHub pagina verbinden allebei met de openbare MQTT server `broker.hivemq.com`.
De pagina stuurt `aan`, `uit` of `lokaal` naar `esplamp/78lb4mti/zet`.
De ESP meldt de lamp op `.../status`, `online` of `offline` op `.../online`, en zijn netwerk op `.../netwerk`.

## Onderhoud

De bedieningspagina staat op één plek: `index.html`. De ESP heeft er een kopie van in de sketch.
Na een wijziging: `python3 onderhoud/maak_sketch.py` (maakt `esp_lamp_online.ino` opnieuw uit `onderhoud/sketch_bron.ino`).
