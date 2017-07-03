#include <vector>

#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <PubSubClient.h>

const int relayPin = D1; // The Shield uses pin 1 for the relay
int powerStatus = LOW;

const char* MQTT_HOST = "openhab";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_COMMAND_QUEUE = "wohnung/lichterkette/command";
const char* MQTT_STATUS_QUEUE = "wohnung/lichterkette/status";

const char MQTT_ON_CHAR = '0';
const char MQTT_OFF_CHAR = '1';

const char* MQTT_ON_RESPONSE = "On";
const char* MQTT_OFF_RESPONSE = "Off";

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void turn_light_on() {
    digitalWrite(relayPin, LOW);
    mqtt_client.publish(MQTT_STATUS_QUEUE, MQTT_ON_RESPONSE);
}

void turn_light_off() {
    digitalWrite(relayPin, HIGH);
    mqtt_client.publish(MQTT_STATUS_QUEUE, MQTT_OFF_RESPONSE);
}

void mqtt_on_message(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    char receivedChar = (char)payload[i];

    if (receivedChar == MQTT_ON_CHAR) {
      turn_light_on();
    } else if (receivedChar == MQTT_OFF_CHAR) {
      turn_light_off();
    }
  }
}

void mqtt_reconnect() {
  while (!mqtt_client.connected()) {
    if (mqtt_client.connect("ESP8266 Client")) {
      mqtt_client.subscribe(MQTT_COMMAND_QUEUE);
      printf("Subscribed to %s", MQTT_COMMAND_QUEUE);
    } else {
      Serial.println("No connection to MQTT. Waiting 5 seconds and then trying again.");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // WiFiManager
  WiFiManager wifiManager;
  wifiManager.autoConnect("Lichterkette");

  // Initialize Relay-Shield
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Connect to MQTT
  mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
  mqtt_client.setCallback(mqtt_on_message);

  printf("Setup done");
}

void loop() {
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }

  mqtt_client.loop();
}
