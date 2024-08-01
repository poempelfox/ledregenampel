
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


## Hardware Beschreibung Grob-Uebersicht

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
  mit einem dedizierten I/O-Pin pro Farbe. Die Helligkeit kann dabei
  fuer jede Farbe einzeln ueber PWM mit 4096 Stufen geregelt werden.
  Das OLED wird ueber I2C angesprochen.
* momentaner Status: Hardware wurde geliefert und funktioniert zumindest
  teilweise.


## Hardware-Beschreibung Detail

Die Platine wurde in KiCad entworfen.
Die KiCad-Dateien finden sich im Unterverzeichnis [kicad/](kicad/).

Um die Platine nachbauen zu lassen, braucht man moeglicherweise
nichtmal KiCad zu oeffnen: Denn im Unterverzeichnis
[kicad/ledregenampel/jlcpcb/production_files/](kicad/ledregenampel/jlcpcb/production_files/)
finden sich die drei Dateien, die JLCPCB benoetigt um die Platine
zu fertigen: Das gerber-.zip, die BOM (Bill Of Materials), und das
CPL (Component PLacement). Verwendet wurde das, was im Juli 2024
dort auf Lager war. Solltet ihr zu einem spaeteren Zeitpunkt versuchen
das auch fertigen zu lassen, koennte es natuerlich sein dass Teile
nicht auf Lager sind und ersetzt werden muessen. Anders formuliert: YMMV.

Neben der gefertigten Platine sind nur zwei zusaetzliche Teile
notwendig:

* WaveShare ESP32-C3-Zero. Ein Microcontroller mit castellated mounting
  holes, so dass man ihn ohne Gefummel mit Buchsen- und Stiftleisten
  einfach auf die Rueckseite der Platine loeten kann. Der C3 ist zum
  Zeitpunkt dieses Projekts einer der kleinsten (und billigsten)
  Microcontroller die Espressif anbietet, fuer die ihm hier zugedachten
  Aufgaben aber immer noch deutlich ueberdimensioniert. Auf der
  Waveshare-Platine befindet sich u.a. eine USB-C-Buchse und ein
  Spannungsregler (Linearregler :-/) fuer die 3.3V.
* [Ein von TinyTronics bezogenes 2.08 inch OLED-Display mit 256x64 Pixeln](https://www.tinytronics.nl/en/displays/oled/2.08-inch-oled-display-256*64-pixels-white-i2c). Ich hatte
  ernsthafte Probleme, ein meinen Anforderungen nach "nicht winzig, I2C,
  extra-wide, 3.3V" entsprechendes Display zu finden, das war eines der
  wenigen passenden Angebote. Es handelt sich um ein Display
  des zumindest mir voellig unbekannten Herstellers "ZHONGJINGYUAN"
  mit einem SHT1122 Controller. Das Display wird ueber I2C
  angesprochen. Es wird auf die Vorderseite der LED-Regenampel-
  Platine geloetet.

Die USB-C-Buchse auf dem ESP32-Board ist der normale Weg, um
das Projekt mit Strom zu versorgen. USB-Power und "5V" Schiene
auf der Platine sind dabei einfach durchverbunden, d.h. es darf
falls auf andere Weise 5V eingespeist werden KEINESFALLS
gleichzeitig USB-C verbunden werden. Es ist ausserdem zu
beachten, dass der erwartete Stromverbrauch einer einzigen
Ampelfarbe bei 100% Helligkeit um 1A liegt. Es empfiehlt sich
also nicht, mehrere Ampelfarben gleichzeitig mit hoher
Helligkeit leuchten zu lassen.

Uebrigens sind 100% Helligkeit generell nicht der intendierte
Betriebsmodus. Mal abgesehen von dem geringfuegigen Problem,
dass das vermutlich den Betrachter der Ampel blind machen
wuerde, ist auch die dabei entstehende Abwaerme enorm -
die Platine wuerde wahrscheinlich recht schnell ueberhitzen.
"Normal" ist PWM mit unter 20% duty cycle...


## Hardware - known bugs

* Der ESP32-C3 wurde bei ersten Vorabtests alleine unangenehm
  heiss. Das ist laut "Internet" normal, und laut Specs darf
  der Chip auch bis zu 105 Grad. Nach dem aufloeten der Waveshare-
  Platine auf die grosse Platine scheint jedoch die
  Waerme sehr gut an die grosse Platine abgegeben zu werden,
  es ist kein nennenswerter Waermeherd mehr feststellbar.
* Die Loecher fuer das I2C-Display sind etwas arg klein geraten.
  Es passt gerade noch so, hat aber merklichen Widerstand beim
  einstecken der Display-Pins, und ist unschoen zu loeten.
  Dieser Fehler wurde im KiCad-.pcb-File bereits behoben.


## Firmware

* basiert auf der foxesptemp-Firmware.
* hat ein Admin-Webinterface, ueber das der angezeigte Ort (bzw. Weg)
  und die Helligkeit der Ampelfarben eingestellt werden kann.
* Hinweis: Der C3 wird anders programmiert als die meisten
  ESPs, es gibt naemlich keinen USB-seriell-Adapter auf der
  winzigen Platine. Stattdessen muss man: USB mit dem Rechner
  verbinden, dann den "Boot"-Knopf gedrueckt halten, dann
  kurz "Reset" druecken, dann kann man nach ein paar
  Sekunden "Boot" wieder loslassen. Dadurch geht der Chip
  in den Programmiermodus, der Bootloader emuliert einen
  USB-Seriell-Adapter, und man kann ihn wie gewohnt
  ueber idf.py flashen. Auch den Reset am Ende des Flashens
  muss man allerdings manuell machen, wieder ueber
  Knopfdruck. Gluecklicherweise muss man dieses komplexe
  Procedere nur ein mal zur Erstbefuellung durchfuehren:
  Die ledregenampel-firmware unterstuetzt Online-Firmware-
  Update ueber ihr Webinterface...

