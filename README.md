# OpenHAB2-Relay

- MQTT-Binding must be installed
- copy the files in folder conf to your openhab2 conf-folder
- import the project in your platformio-ide
- then compile and upload onto the wemos d1 mini

To enable OTA issue this command in the same network:
```
mosquitto_pub -t ota/enable -m 1
```
