# SqueezUINO
SqueezUINO brings the idea of TonyUINO to a Squeezebox system.

It enables your Squeezeboxes to play a song, a playlist or favorite by passing a RFID card in front of a reader.

## Prerequisite
You need:
* an Openhab installation with 
  * Squeezebox binding https://www.openhab.org/addons/bindings/squeezebox/
  * MQTT 
* a RFID card set. I like this one: https://www.az-delivery.de/products/rfid-set
* an Arduino - of course ;)

## Setup
* configure your Openhab installation with to be able to connect to your Squeezebox AND be able to read MQTT messages. You can use the openhab files of this repo as an example
* Hardware: setup your RFID reader with your Arudino -> https://microcontrollerslab.com/rc522-rfid-reader-pinout-arduino-interfacing-examples-features/
* install Arduino sketch
* identify a RFID tag. The sketch will print the tag ID to serial. To connect and listen for the first time. 
* tweak your openhab config using transform/squeezuino.map to map RFID tag to a song, playlist or favorite.
 