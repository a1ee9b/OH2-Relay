#include <vector>

#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <PubSubClient.h>

#include <ArduinoOTA.h>


const char* DEVICE_NAME = "Lichterkette";

const char* MQTT_HOST = "openhab";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_COMMAND_QUEUE = "wohnung/lichterkette/command";
const char* MQTT_STATUS_QUEUE = "wohnung/lichterkette/status";

const char MQTT_ON_CHAR = '1';
const char MQTT_OFF_CHAR = '0';

const char* MQTT_ON_RESPONSE = "On";
const char* MQTT_OFF_RESPONSE = "Off";

const char* MQTT_OTA_QUEUE = "ota/enable";
const char MQTT_OTA_CHAR = '1';


const int relayPin = D1; // The Shield uses pin 1 for the relay
int powerStatus = LOW;
bool ota_flag = false;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

void turn_light_on() {
    digitalWrite(relayPin, HIGH);
    mqtt_client.publish(MQTT_STATUS_QUEUE, MQTT_ON_RESPONSE);
}

void turn_light_off() {
    digitalWrite(relayPin, LOW);
    mqtt_client.publish(MQTT_STATUS_QUEUE, MQTT_OFF_RESPONSE);
}

void handle_light_command(char receivedChar) {
  Serial.printf("Handling light command. Received char: %c\n", receivedChar);

  if (receivedChar == MQTT_ON_CHAR) {
    turn_light_on();
  } else if (receivedChar == MQTT_OFF_CHAR) {
    turn_light_off();
  }
}

void handle_ota_command(char receivedChar) {
  Serial.printf("Handling ota command. Received char: %c\n", receivedChar);

  if (receivedChar == MQTT_OTA_CHAR) {
    Serial.println("Starting OTA handling.");
    Serial.println(WiFi.localIP());
    ota_flag = true;
  }
}

void mqtt_on_message(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Got topic: %s\n", topic);

  for (int i = 0; i < length; i++) {
    char receivedChar = (char)payload[i];
    if (strcmp(topic, MQTT_COMMAND_QUEUE) == 0) {
      handle_light_command(receivedChar);
    }

    if (strcmp(topic, MQTT_OTA_QUEUE) == 0) {
      handle_ota_command(receivedChar);
    }

  }
}

void mqtt_reconnect() {
  while (!mqtt_client.connected()) {
    if (mqtt_client.connect(DEVICE_NAME)) {
      mqtt_client.subscribe(MQTT_COMMAND_QUEUE);
      mqtt_client.subscribe(MQTT_OTA_QUEUE);

      Serial.printf("Subscribed to %s\n", MQTT_COMMAND_QUEUE);
      Serial.printf("Subscribed to %s\n", MQTT_OTA_QUEUE);
    } else {
      Serial.println("No connection to MQTT. Waiting 5 seconds and then trying again.");
      delay(5000);
    }
  }
}

void initialize_ota() {
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(DEVICE_NAME);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"123"); // password

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup starting");

  // WiFiManager
  WiFiManager wifiManager;
  wifiManager.autoConnect(DEVICE_NAME);

  // Initialize Relay-Shield
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Connect to MQTT
  mqtt_client.setServer(MQTT_HOST, MQTT_PORT);
  mqtt_client.setCallback(mqtt_on_message);

  initialize_ota();

  Serial.println("Setup done");
}

void loop() {
  if (!mqtt_client.connected()) {
    mqtt_reconnect();
  }

  mqtt_client.loop();

  if (ota_flag) {
    Serial.println("OTA Flag is true.");
    ArduinoOTA.handle();
  }
}
