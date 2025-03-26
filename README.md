# esphome-somfy
Esphome configuration for Somfy RTS Blinds

:warning: **This repository will not work with Somfy IO devices, only with RTS devices.** :warning:

<img alt="Contributors" src="https://img.shields.io/github/contributors/Viproz/esphome-somfy"> <img alt="GitHub last commit" src="https://img.shields.io/github/last-commit/Viproz/esphome-somfy">

[**This product is available all assembled on Tindie!**](https://www.tindie.com/products/domotech/somfy-rts-remote-for-home-assistant-and-esphome/)

__Materials:__
* 1 - [Wemos Mini D1](https://s.click.aliexpress.com/e/_DCIQhtB);
* 1 - [CC1101 Module](https://s.click.aliexpress.com/e/_DkjwEXf);

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

In your ESPHome installation create a new device, configure the different modules as you like and add the following lines at the end:

````
external_components:
  - source:
      type: git
      url: https://github.com/Viproz/esphome-somfy
    components: [ somfy ]
    refresh: 1d
preferences:
  flash_write_interval: 0s

cover:
  - platform: somfy
    name: "Living room shutter"
    RemoteID: 11234
    device_class: shutter
  - platform: somfy
    name: "Kitchen shutter"
    RemoteID: 2896
    device_class: shutter
````
You can add as many cover elemnts as you would like, make sure all the RemoteID elements are unique.
For additionnal security it is recommended to use random numbers for the remote IDs between 1 and 16777214, [you can generate them online](https://www.random.org/integers/?num=10&min=1&max=16777214&col=1&base=10&format=html&rnd=new).

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

2. Put your blind in programming mode. If necessary, consult the blind manual or the manufacturer, it usually consist in pressing the button on the back of the existing remote a few seconds or power cycling the device if no remote exists.
3. Slide the bar that controls the tilt position to the value 11, this makes the virtual remote enter programming mode.

   a) If the programming succeeds without problems, your blind will move immediately.
   
   b) In case of problems, check your device's log.

## Blind configuration commands

Some commands were created, accessed by tilting the blind to try to facilitate debugging and configuration.

```
// cmd 0 - Prints the current rolling code
// cmd 11 - Program mode
// cmd 16 - Program mode for grail curtains
// cmd 21 - Delete rolling code file
// cmd 61 - Clears all Preferences set
// cmd 90 - Re-run the setup member
// cmd 97 - Set the CC1101 module to TX mode
// cmd 98 - Set the CC1101 module to idle
// cmd 99 - Set the transmit pin to HIGH
// cmd 100 - Set the transmit pin to LOW
```

## Credits
This project is based on [Nickduino implementation of the Somfy RTS remote](https://github.com/Nickduino/Somfy_Remote) and was first adapted to work with ESPHome by Daniel Stein.

The code was then readapted by Vincent MALZIEU to make it work with CC1101 modules and the newer external component feature in ESPHome.
