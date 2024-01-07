# esphome-somfy
Esphome configuration for Somfy RTS Blinds

:warning: **This repository will not work with Somfy IO devices, only with RTS devices.** :warning:

<img alt="Contributors" src="https://img.shields.io/github/contributors/Viproz/esphome-somfy"> <img alt="GitHub last commit" src="https://img.shields.io/github/last-commit/Viproz/esphome-somfy">



__Materials:__
* 1 - [Wemos Mini D1](https://www.aliexpress.com/item/1005005972627549.html);
* 1 - [CC1101 Module](https://www.aliexpress.com/item/1005006005197368.html);

__Software:__
* Running Home Assistant [https://www.home-assistant.io/]
* ESPhome component [https://esphome.io/]

Using some cables or a PCB board connect the CC1101 to the ESP32 with the following pins:
| CC1101 Pin | ESP32 Pin | Name   |
|------------|-----------|--------|
| 1          | GND       | Ground |
| 2          | 3.3V      | Power  |
| 3          | IO2       | GD00   |
| 4          | IO5       | CSN    |
| 5          | IO18      | SCK    |
| 6          | IO23      | MOSI   |
| 7          | IO19      | MISO   |
| 8          | IO4       | GD02   |

## Usage:

In your ESPHome installation create a new device, configure the different modules as you ike and add the following lines at the end:

````
external_components:
  - source:
      type: git
      url: https://github.com/Viproz/esphome-somfy
      ref: dev
    components: [ somfy ]
    refresh: 1d

cover:
  - platform: somfy
    name: "Living room shutter"
    RemoteID: 1
    device_class: shutter
  - platform: somfy
    name: "Kitchen shutter"
    RemoteID: 2
    device_class: shutter
````
You can add as many cover elemnts as you would like, make sure all the RemoteID elements are unique.

Install the configuration onto your ESP32 board.

## How to configure

Discover the new esp32 board in Home Assistant and insert your new entities in your home assistant's dashboard.

      They should have names similar to these:
      * cover.living_room_shutter
      * cover.kitchen_shutter

![Blind Card](/img/Blind%20card.png)

## How to program

1. Open one of the entity in Home Assistant.


    __Blind control interface__

![Blind control interface](/img/Blind%20control.png)

3. If this if the first cover you are programming, slide the bar that controls the tilt position to the value 61, this will prepare the SPIFSS memory.
4. Put your blind in programming mode. If necessary, consult the blind manual or the manufacturer, it usually consist in pressing the button on the back of the existing remote or power cycling the device if no remote exists.
5. Slide the bar that controls the tilt position to the value 11, this makes the virtual remote enter programming mode.

   a) If the programming succeeds without problems, your blind will move immediately.
   
   b) In case of problems, check your device's log.

## Blind configuration commands

Some commands were created, accessed by tilting the blind to try to facilitate debugging and configuration.

```
// cmd 11 - program mode
// cmd 16 - program mode for grail curtains
// cmd 21 - delete rolling code file
// cmd 61 - Format filesystem and test.
// cmd 90 - Re-run the setup member
// cmd 97 - Set the CC1101 module to TX mode
// cmd 98 - Set the CC1101 module to idle
// cmd 99 - Set the transmit pin to HIGH
// cmd 100 - Set the transmit pin to LOW
```

## Credits
This project is based on [Nickduino implementation of the Somfy RTS remote](https://github.com/Nickduino/Somfy_Remote) and was first adapter to work with ESPHome by Daniel Stein.

The code was then readapted by Vincent MALZIEU to make it work with CC1101 modules and the newer external component feature in Home Assistant.
