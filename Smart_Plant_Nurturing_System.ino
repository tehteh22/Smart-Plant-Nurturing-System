/*
CPC357 Smart Plant Nurturing System 
*/
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "connection.h"

// Constant for Moisture Sensor (Lower value -> More Moisture)
int minMoistVal = 4095;
int maxMoistVal = 600;
int minMoistPer = 0;
int maxMoistPer = 100;
int moistPer = 0;

// Constant for Water Level Sensor (Lower value -> Less Water Immersed)
int minDepthValue = 0;
int maxDepthValue = 1800;
int minDepth = 0;
int maxDepth = 100;
int depth = 0;

// Constant for LDR (Lower Value -> Higher Light Intensity)
int minLDRVal = 4095;
int maxLDRVal = 500;
int minLDR = 0;
int maxLDR = 100;
int lightIntensity = 0;

// Global flag control
bool isWaterLow = false;

// Input Pin Configuration
const int DHT_PIN = 42;
const int DHT_TYPE = DHT11;
const int MOIST_PIN = A4;
const int LDR_PIN = A2; 
const int WATER_PIN = A3;

// Output Pin Configuration
const int RELAY_PIN = A1;
const int LED_PIN = 21;

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient client(espClient);

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
  client.setServer(MQTT_SERVER, MQTT_PORT);
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT server");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

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

// Function to monitor the water level in water tank
void monitorWaterTank() {
  int waterVal = analogRead(WATER_PIN);
  int depth = map(waterVal, minDepthValue, maxDepthValue, minDepth, maxDepth);

  Serial.print("Water Sensor Value: ");
  Serial.print(waterVal);
  Serial.print(" Water Depth: ");
  Serial.println(depth);

  // Set size
  char payload[200];

  //Publish
  sprintf(payload, "Water Value: %.2i Depth: %.2i ", waterVal,depth);
  client.publish(MQTT_TOPIC_WT, payload);

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

    // Set size
  char payload[300];

  //Publish
  sprintf(payload, "Temperature(c):  %.2f Temperature(F): %.2f Humidity: %.2f Heat Index(c): %.2f Heat Index(F): %.2f Mositure: %.2i  Mositure Percentage:  %.2i", temperatureC, temperatureF, humidity, hic, hif, moistVal, moistPer);
  client.publish(MQTT_TOPIC_AW, payload);

  // High temperature (Frequent Watering -> Longer Watering Duration)
  if (temperatureC > 30) {
    // Low Humidity & Low Moisture
    if (humidity < 60 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH); // turn on water pump
      delay(20000); // pump runs for 20 seconds
      digitalWrite(RELAY_PIN, LOW); // turn off water pump
    } // Low Humidty & Medium Moisture 
    else if (humidity < 60 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(15000); // pump runs for 15 seconds
      digitalWrite(RELAY_PIN, LOW); 
    } // Low Humidity & High Moisture
     else if (humidity < 60 && moistPer > 70) {
      Serial.println("High Moisture Detected, Temporary No Watering for Plant!");
    } // Medium Humidty & Low Moisture 
    else if (humidity >= 60 && humidity < 80 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(15000); // pump runs for 15 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // Medium Humidity & Medium Moisture
    else if (humidity >= 60 && humidity < 80 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(12000); // pump runs for 12 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // Medium Humidity & High Moisture
     else if (humidity >= 60 && humidity < 80 && moistPer > 70) {
      Serial.println("High Moisture Detected, Temporary No Watering for Plant!");
    } // High Humidity & Low Moisture
    else if (humidity > 80 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(10000); // pump runs for 10 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // High Humidity & Medium Moisture 
     else if (humidity > 80 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(8000); // pump runs for 8 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // High Humidity & High Moisture (No action)
  }
  // Low temperature (Normal Watering)
  else {
    // Low Humidity & Low Moisture
    if (humidity < 60 && moistPer < 30) {
      digitalWrite(RELAY_PIN, HIGH); 
      delay(15000); // pump runs for 15 seconds
      digitalWrite(RELAY_PIN, LOW);
    } // Low Humidty & Medium Moisture 
    else if (humidity < 60 && moistPer >= 30 && moistPer < 70) {
      digitalWrite(RELAY_PIN, HIGH);
      delay(10000); // pump runs for 10 seconds
      digitalWrite(RELAY_PIN, LOW); 
    }  // Low Humidity & High Moisture
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

  // Set size
  char payload[200];

  //Publish
  sprintf(payload, "Light Value: %.2i  Light Intensity:  %.2i", lightVal, lightIntensity);
  client.publish(MQTT_TOPIC_AL, payload);

  if (lightIntensity < 40) { // adjust the threshold as needed
    digitalWrite(LED_PIN, HIGH); // turn on LED
  } else {
    digitalWrite(LED_PIN, LOW); // turn off LED
  }
}

