/*
  Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  Permission is hereby granted, free of charge, to any person obtaining a copy of this
  software and associated documentation files (the "Software"), to deal in the Software
  without restriction, including without limitation the rights to use, copy, modify,
  merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
  PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "secrets.h"       // Contains AWS_IOT_ENDPOINT, THINGNAME, WIFI_SSID, WIFI_PASSWORD, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>

// MQTT topics
#define AWS_IOT_PUBLISH_TOPIC   "devices/dispositivo-da-palhoca-93/status/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "devices/dispositivo-da-palhoca-93/status/sub"

// Define the built-in LED pin. For many ESP32 boards, the built-in LED is on GPIO 2.
// Update if your board uses a different pin.
#define LED_PIN 2

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

void messageHandler(String &topic, String &payload) {
  Serial.println("Incoming message on topic: " + topic);
  Serial.println("Payload: " + payload);

  // Parse JSON message. Expecting a structure like: {"command": "on"}
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  const char* command = doc["command"];
  if (command != nullptr) {
    if (strcmp(command, "on") == 0) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Built-in LED turned ON");
    } else if (strcmp(command, "off") == 0) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("Built-in LED turned OFF");
    } else {
      Serial.println("Unknown command received");
    }
  }
}

void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected");

  // Configure secure client with AWS IoT credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to AWS IoT MQTT broker
  client.begin(AWS_IOT_ENDPOINT, 8883, net);
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IoT as ");
  Serial.println(THINGNAME);
  while (!client.connect(THINGNAME)) {
    Serial.println("AWS IoT connecting...");
    delay(100);
  }

  if (!client.connected()){
    Serial.println("AWS IoT Connection Timeout!");
    return;
  }

  // Subscribe to the topic for commands
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected and subscribed to topic!");
}

void publishMessage() {
  // Example publishing sensor data (optional)
  Serial.println("Publishing sensor data");
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["sensor_a0"] = analogRead(0);
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  bool sent = client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
  Serial.print("Message sent? ");
  Serial.println(sent ? "Yes" : "No");
}

void setup() {
  Serial.begin(9600);

  // Initialize the built-in LED pin as an output.
  pinMode(LED_PIN, OUTPUT);
  // Turn LED off initially.
  digitalWrite(LED_PIN, LOW);

  connectAWS();
}

void loop() {
  // Publish sensor data periodically (every second) if needed.
  publishMessage();
  client.loop();
  delay(200);
}
