/*
CPC357 Smart Plant Nurturing System 
*/
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "VOneMqttClient.h"
#include "connection.h"
#include "environment.h"

// Constant for Moisture Sensor (Lower value -> More Moisture)
int minMoistVal = 4095;
int maxMoistVal = 100;
int minMoistPer = 0;
int maxMoistPer = 100;
int moistPer = 0;

// Constant for Water Level Sensor (Lower value -> Less Water Immersed)
int minDepthValue = 0;
int maxDepthValue = 2400;
int minDepth = 0;
int maxDepth = 100;
int depth = 0;

// Constant for LDR (Lower Value -> Higher Light Intensity)
int minLDRVal = 4095;
int maxLDRVal = 100;
int minLDR = 0;
int maxLDR = 100;
int lightIntensity = 0;

// Global flag control
bool isWaterLow = false;
bool isPumpOn = false;

// Input Pin Configuration
const int DHT_PIN = 42; // Right side Maker Port
const int DHT_TYPE = DHT11;
const int MOIST_PIN = A4; // Left side Maker Port
const int LDR_PIN = A2; 
const int WATER_PIN = A3;

// Output Pin Configuration
const int RELAY_PIN = A1;
const int LED_PIN = 21;

DHT dht(DHT_PIN, DHT_TYPE);

// Create an instance of VOneMqttClient
VOneMqttClient voneClient;

// // Cloud Implementation
// WiFiClient espClient;
// PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println("Connecting to WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(11, OUTPUT);
  pinMode(MOIST_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode (LED_PIN, OUTPUT);
  Serial.begin(115200); 
  dht.begin();
  setup_wifi();
  voneClient.setup();
  voneClient.registerActuatorCallback(triggerActuator_callback);

  // client.setServer(MQTT_SERVER, MQTT_PORT);
}

// void reconnect() {
//   while (!client.connected()) {
//     Serial.println("Attempting MQTT connection...");
//     if (client.connect("ESP32Client")) {
//       Serial.println("Connected to MQTT server");
//     } else {
//       Serial.print("Failed, rc=");
//       Serial.print(client.state());
//       Serial.println(" Retrying in 5 seconds...");
//       delay(5000);
//     }
//   }
// }

void loop() {
  // if (!client.connected()) {
  //   reconnect();
  // }
  // client.loop();

  if (!voneClient.connected()) {
    voneClient.reconnect();
    String errorMsg = "Sensor Fail";
    voneClient.publishDeviceStatusEvent(Dht11_module, true);
    voneClient.publishDeviceStatusEvent(Soil_module, true);
    voneClient.publishDeviceStatusEvent(Water_module, true);
    voneClient.publishDeviceStatusEvent(GL55_module, true);
  }
  voneClient.loop();

  // Set Peripheral on
  digitalWrite(11, HIGH);
  delay(2000);
  
  // Call function
  monitorWaterTank();
  if (!isWaterLow) {
    controlAutoWatering();
  }
  controlAutoLightening();

  delay(5000);
  digitalWrite(11, LOW);

  // // Set size
  // char payload[300];

  // //Publish
  // sprintf(payload, "Temperature(c):  %.2f Temperature(F): %.2f Humidity: %.2f Heat Index(c): %.2f Heat Index(F): %.2f Mositure: %.2f  Mositure Percentage:  %.2f", temperatureC, temperatureF, humidity, hic, hif, moistVal, moistPer);
  // client.publish(MQTT_TOPIC, payload);
}

// V-one actuator triggering function
void triggerActuator_callback(const char* actuatorDeviceId, const char* actuatorCommand)
{
  Serial.print("Main received callback : ");
  Serial.print(actuatorDeviceId);
  Serial.print(" : ");
  Serial.println(actuatorCommand);

  String errorMsg = "";

  JSONVar commandObjct = JSON.parse(actuatorCommand);
  JSONVar keys = commandObjct.keys();

  if (String(actuatorDeviceId) == Relay_module)
  {
    String key = "";
    String commandValue = "";
    for (int i = 0; i < keys.length(); i++) {
      key = (const char* )keys[i];
      commandValue = (const char* ) commandObjct[keys[i]];

    }
    Serial.print("Key : ");
    Serial.println(key.c_str());
    Serial.print("value : ");
    Serial.println(commandValue);

    if(commandValue == "OPEN" && !isPumpOn) {
      Serial.println("Relay ON");
      digitalWrite(RELAY_PIN, HIGH);
      isPumpOn = true;
    } 
    else if(commandValue == "CLOSE" && isPumpOn) {
      Serial.println("Relay OFF");
      digitalWrite(RELAY_PIN, LOW);
      isPumpOn = false;
    }
    voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, errorMsg.c_str(), true);//publish actuator status
  }

  if(String(actuatorDeviceId) == LED_module) {
    String key = "";
    String commandValue = "";
    for (int i = 0; i < keys.length(); i++) {
      key = (const char* )keys[i];
      commandValue = (const char* )commandObjct[keys[i]];

    }
    Serial.print("Key : ");
    Serial.println(key.c_str());
    Serial.print("value : ");
    Serial.println(commandValue);

    if(commandValue == "ON") {
      Serial.println("LED ON");
      digitalWrite(LED_PIN, HIGH);
    } else if(commandValue == "OFF"){
      Serial.println("LED OFF");
      digitalWrite(LED_PIN, LOW);
    }
    voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, errorMsg.c_str(), true);//publish actuator status
  }
  // else
  // {
  //   Serial.print(" No actuator found : ");
  //   Serial.println(actuatorDeviceId);
  //   errorMsg = "No actuator found";
  //   voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, errorMsg.c_str(), false);//publish actuator status
  // }
}


// Function to monitor the water level in water tank
void monitorWaterTank() {
  int waterVal = analogRead(WATER_PIN);
  int depth = map(waterVal, minDepthValue, maxDepthValue, minDepth, maxDepth);

  Serial.print("Water Sensor Value: ");
  Serial.print(waterVal);
  Serial.print(" Water Depth: ");
  Serial.println(depth);

  // // Set size
  // char payload[200];

  // //Publish
  // sprintf(payload, "Water Value: %.2i Depth: %.2i ", waterVal,depth);
  // client.publish(MQTT_TOPIC_WT, payload);

  // Publish telemtry data
  voneClient.publishTelemetryData(Water_module, "Depth", depth);

  if (depth < 30) { 
    Serial.println("Water level is low. Please refill the tank.");
    isWaterLow = true;
  } 
  else {
    isWaterLow = false;
  }
}

// Function to control auto watering (No action when the plant is in high moistPer)
void controlAutoWatering() {
  // Read temperature in Celecius and Fahrenheit
  float temperatureC = dht.readTemperature();
  float temperatureF = dht.readTemperature(true);
  // Read humidity
  float humidity = dht.readHumidity();
  // Compute Heat Index in Fahrenheit and Celcius
  float hif = dht.computeHeatIndex(temperatureF, humidity);
  float hic = dht.computeHeatIndex(temperatureC, humidity, false);
  // Read humidity and compute in percentage
  int moistVal = analogRead(MOIST_PIN);
  int moistPer = map(moistVal, minMoistVal, maxMoistVal, minMoistPer, maxMoistPer);

  Serial.print("Temperature(c): ");
  Serial.print(temperatureC);
  Serial.print(" Temperature(F): ");
  Serial.print(temperatureF);
  Serial.print(" Humidity: ");
  Serial.print(humidity);
  Serial.print(" Heat Index(c): ");
  Serial.print(hic);
  Serial.print(" Heat Index(F): ");
  Serial.println(hif);
  Serial.print("Moisture: ");
  Serial.print(moistVal);
  Serial.print(" Moisture Percentage: ");
  Serial.println(moistPer);

  //   // Set size
  // char payload[300];

  // //Publish
  // sprintf(payload, "Temperature(c):  %.2f Temperature(F): %.2f Humidity: %.2f Heat Index(c): %.2f Heat Index(F): %.2f Mositure: %.2i  Mositure Percentage:  %.2i", temperatureC, temperatureF, humidity, hic, hif, moistVal, moistPer);
  // client.publish(MQTT_TOPIC_AW, payload);

  // Declare payload object in JSON and publish telemtry data
  JSONVar payloadObject;
  payloadObject["Humidity"] = humidity;
  payloadObject["Temperature"] = temperatureC;
  voneClient.publishTelemetryData(Dht11_module, payloadObject);
  voneClient.publishTelemetryData(Soil_module, "Soil moisture", moistPer);

  // High temperature (Frequent Watering -> Longer Watering Duration)
  if (temperatureC > 30) {
    // Low Humidity & Low Moisture
    if (humidity < 60 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH); // turn on water pump
      delay(15000); // pump runs for 15 seconds
      digitalWrite(RELAY_PIN, LOW); // turn off water pump
    } // Low Humidty & Medium Moisture 
    else if (humidity < 60 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(10000); // pump runs for 10 seconds
      digitalWrite(RELAY_PIN, LOW); 
    } // Low Humidity & High Moisture
     else if (humidity < 60 && moistPer > 70) {
      Serial.println("High Moisture Detected, Temporary No Watering for Plant!");
    } // Medium Humidty & Low Moisture 
    else if (humidity >= 60 && humidity < 80 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(10000); // pump runs for 10 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // Medium Humidity & Medium Moisture
    else if (humidity >= 60 && humidity < 80 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(8000); // pump runs for 8 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // Medium Humidity & High Moisture
     else if (humidity >= 60 && humidity < 80 && moistPer > 70) {
      Serial.println("High Moisture Detected, Temporary No Watering for Plant!");
    } // High Humidity & Low Moisture
    else if (humidity > 80 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(8000); // pump runs for 8 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // High Humidity & Medium Moisture 
     else if (humidity > 80 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(5000); // pump runs for 5 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // High Humidity & High Moisture (No action)
  }
  // Low temperature (Normal Watering)
  else {
    // Low Humidity & Low Moisture
    if (humidity < 60 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH); 
      delay(13000); // pump runs for 13 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // Low Humidty & Medium Moisture 
    else if (humidity < 60 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(8000); // pump runs for 8 seconds
      digitalWrite(RELAY_PIN, LOW); 
    }  // Low Humidity & High Moisture
     else if (humidity < 60 && moistPer > 70) {
      Serial.println("High Moisture Detected, Temporary No Watering for Plant!");
    } // Medium Humidty & Low Moisture 
    else if (humidity >= 60 && humidity < 80 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(8000); // pump runs for 8 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // Medium Humidity & Medium Moisture
    else if (humidity >= 60 && humidity < 80 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(5000); // pump runs for 5 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // Medium Humidity & High Moisture
     else if (humidity >= 60 && humidity < 80 && moistPer > 70) {
      Serial.println("High Moisture Detected, Temporary No Watering for Plant!");
    } // High Humidity & Low Moisture
    else if (humidity > 80 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(5000); // pump runs for 5 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // High Humidity & Medium Moisture 
     else if (humidity > 80 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(3000); // pump runs for 3 seconds
      digitalWrite(RELAY_PIN, LOW);
    }
  }
}

// Function to control auto lightening
void controlAutoLightening() {
  int lightVal = analogRead(LDR_PIN);
  int lightIntensity = map(lightVal, minLDRVal, maxLDRVal, minLDR, maxLDR);

  Serial.print("LDR Value: ");
  Serial.print(lightVal);
  Serial.print(" Light Intensity: ");
  Serial.println(lightIntensity);

  // // Set size
  // char payload[200];

  // //Publish
  // sprintf(payload, "Light Value: %.2i  Light Intensity:  %.2i", lightVal, lightIntensity);
  // client.publish(MQTT_TOPIC_AL, payload);

  // Publish telemtry data
  voneClient.publishTelemetryData(GL55_module, "Light Intensity", lightIntensity);

  if (lightIntensity < 40) { // adjust the threshold as needed
    digitalWrite(LED_PIN, HIGH); // turn on LED
  } else {
    digitalWrite(LED_PIN, LOW); // turn off LED
  }
}

