# nrf52-pca10059-zephyr

## Description

## Requirements

### Hardware

- nRF52-Dongle
- MCP9701-E/TO

### Software

## Build and Flash

```
cd %userprofile%\zephyrproject\zephyr\samples\bluetooth
git clone https://github.com/fprotopapa/nrf52-pca10059-zephyr.git

cd ..\..
west build -p auto -b nrf52840dongle_nrf52840 samples/bluetooth/nrf52-pca10059-zephyr
nrfutil pkg generate --hw-version 52 --sd-req=0x00 --application build/zephyr/zephyr.hex --application-version 1 nrf52-pca10059-zephyr.zip
nrfutil dfu usb-serial -pkg nrf52-pca10059-zephyr.zip -p COM<XX>
```

## Circuit

![Circuit](img/dongle_circuit.png)
