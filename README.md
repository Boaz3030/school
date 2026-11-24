# ESP8266 lamp

Eén pagina, op de ESP zelf. Je gsm op hetzelfde wifi als de ESP, en je bedient de lamp. Internet is niet nodig.

## 👉 [Open de lamp: http://esp-lamp.local](http://esp-lamp.local/)

## 👉 [Script voor Arduino IDE](esp_lamp_online.ino)

Klik in het script rechtsboven op **Copy raw file** en plak alles in Arduino IDE.

## Stappen

1. Sluit de LED aan: pin **D1** → weerstand 220 Ω → **lange poot** LED, **korte poot** → **GND**.
2. Arduino IDE: bordpakket **esp8266** geïnstalleerd, bord en poort gekozen onder Tools. Geen extra bibliotheek nodig.
3. Plak het script en upload. Je hoeft geen wifi in te vullen.
4. Verbind je gsm met wifi **ESP-Lamp** (wachtwoord **lamp1234**) en open **http://esp-lamp.local** (of http://192.168.4.1).
5. Druk op **AAN** en **UIT** om te testen.
6. Tik rechtsboven op het **wifi icoon**, kies een netwerk, typ het wachtwoord en tik op **Verbinden**.
   Een laadicoon draait tot er een groen ✓ of een rood ✗ verschijnt.
7. Zet je gsm op datzelfde netwerk en open weer **http://esp-lamp.local**.

## Goed om te weten

- De ESP onthoudt zijn netwerk, ook zonder stroom. Opnieuw uploaden is niet nodig om van wifi te wisselen.
- Zijn eigen wifi **ESP-Lamp** blijft altijd aan. Zo kom je er altijd bij.
- Werkt `esp-lamp.local` niet op je gsm, gebruik dan het IP adres. Dat staat op de pagina en in de Seriële monitor.
- Met **Alleen ESP netwerk** in het wifipaneel vergeet de ESP zijn netwerk.

## Seriële monitor (115200 baud)

Toont de eigen wifi, welk netwerk hij probeert, of het lukt, het IP adres en elke keer dat de lamp aan of uit gaat.

## Onderhoud

De pagina staat in `onderhoud/pagina.html`, de rest van het script in `onderhoud/sketch_bron.ino`.
Na een wijziging: `python3 onderhoud/maak_sketch.py` maakt `esp_lamp_online.ino` opnieuw.
