
# LED-Regen-Ampel

## Intro

This is a small project that contains hardware design and
firmware for a small traffic-light-style display that shows
whether you can leave the house without getting wet. As it
utilizes regenampel.de to get the prediction, this is pretty
much useless outside of germany, which is why the rest of
the description is in german.

Haupt-Ziel dieses Projekts war es, mal eine Platine mit
richtig vielen LEDs drauf zu designen, um sie dann von einem
guenstigen Chinesischen Prototypenfertiger (JLCPCB) anfertigen
zu lassen.
Da ich ein Fan der [Regenampel](https://regenampel.de)
bin, kam die Idee auf, eine aus ganz vielen LEDs bestehende
Regenampel zu konstruieren, in einer Groesse die man bequem
neben die Wohnungstuer oder auch die Tuer des [ZAM](https://zam.haus)
haengen kann.


## Hardware-Beschreibung

* ca. 21 x 7,5 cm grosse Platine
* Rot / Gelb / Gruen bestehend aus je 51 in konzentrischen Kreisen
  angeordneten LEDs (keine superhellen, generische in 0805 Gehaeuse),
  Durchmesser 5 cm.
* kleines OLED-Display unter der Ampel, um den Zusatztext den die
  Regenampel in manchen Situationen liefert (Beispiel: "Noch 20 Minuten
  starker Niederschlag") anzeigen zu koennen.
* Ein WaveShare ESP32-C3-Zero dient als zentrale Steuerung. Er wird
  auf die Rueckseite geloetet.
* Der ESP32-C3 steuert die LEDs ueber je einen MOSFET pro Farbe
  mit einem dedizierten I/O-Pin pro Farbe. Das OLED wird ueber I2C
  angesprochen.
* momentaner Status: Hardware wurde geliefert und funktioniert zumindest
  teilweise.


## Firmware

* ist in Arbeit (derzeit hauptsaechlich eine Kopie der foxesptemp-firmware)
* geplant ist ein Webinterface auf dem ESP, ueber das man den
  angezeigten Ort (bzw. Weg) einstellen kann.

