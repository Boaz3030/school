"""Maakt esp_lamp_online.ino uit onderhoud/sketch_bron.ino en index.html.

Zo staat de bedieningspagina maar op een plek: index.html.
Gebruik: python3 onderhoud/maak_sketch.py
"""
from pathlib import Path

hier = Path(__file__).resolve().parent
bron = (hier / "sketch_bron.ino").read_text(encoding="utf-8")
pagina = (hier.parent / "index.html").read_text(encoding="utf-8").strip()

if ")rawliteral" in pagina:
    raise SystemExit("index.html mag de tekst )rawliteral niet bevatten")

(hier.parent / "esp_lamp_online.ino").write_text(bron.replace("@@PAGINA@@", pagina), encoding="utf-8")
print("esp_lamp_online.ino gemaakt")
