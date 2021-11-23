# nrf52-pca10059-zephyr

Clone repository to %userprofile%\zephyrproject\zephyr\samples\bluetooth

```
cd cd %userprofile%\zephyrproject\zephyr
west build -p auto -b nrf52840dongle_nrf52840 samples/bluetooth/nrf52-pca10059-zephyr
nrfutil pkg generate --hw-version 52 --sd-req=0x00 --application build/zephyr/zephyr.hex --application-version 1 nrf52-pca10059-zephyr.zip
nrfutil dfu usb-serial -pkg peripheral_temp.zip -p COM<XX>
```
