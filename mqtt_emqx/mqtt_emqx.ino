#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// WiFi settings
const char *ssid = "Lanterns";             // Replace with your WiFi name
const char *password = "88888888";   // Replace with your WiFi password

// MQTT Broker settings
const char *mqtt_broker = "ubfdec17.ala.cn-shenzhen.emqxsl.cn";  // EMQX broker hostname
const char *mqtt_topic = "testtopic";     // MQTT topic
const char *mqtt_username = "doorClient";  // MQTT username for authentication
const char *mqtt_password = "333666999";  // MQTT password for authentication
const int mqtt_port = 8883;  // MQTT port (TLS)

bool ledState = LOW;  // Variable to hold the state of the LED

WiFiClientSecure espClient;
PubSubClient mqtt_client(espClient);

void connectToWiFi();

void connectToMQTTBroker();

void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);  // Initialize the built-in LED pin as an output
    digitalWrite(LED_BUILTIN, ledState);  // Set the initial state of the LED
    Serial.begin(9600);
    connectToWiFi();
    espClient.setInsecure();
    mqtt_client.setServer(mqtt_broker, mqtt_port);
    mqtt_client.setCallback(mqttCallback);
    connectToMQTTBroker();
}

void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to the WiFi network");
}

void connectToMQTTBroker() {
    while (!mqtt_client.connected()) {
        String client_id = "esp8266-client-" + String(WiFi.macAddress());
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Connected to MQTT broker");
            mqtt_client.subscribe(mqtt_topic);
            // Publish message upon successful connection
            mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 ^^");
        } else {
            Serial.print("Failed to connect to MQTT broker, rc=");
            Serial.print(mqtt_client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message received on topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();   
    Serial.println("-----------------------");
}

void loop() {
    if (!mqtt_client.connected()) {
        connectToMQTTBroker();
    }
    mqtt_client.loop();
}