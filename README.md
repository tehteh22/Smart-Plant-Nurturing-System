This is an IoT project about Smart Plant Nurturing System using Maker Feather AIoT S3 microcontroller.

Sensors:
DHT11, Soil Moisture, Water Level Sensor, GL55 LDR, Relay Switch

Actuators:
Water Pump, LED 

Modules:
1. Water Tank Monitoring: Consists of Water Level Sensor and Water Pump
2. Auto Watering: Consists of DHT11 and Soil Moisture Sensor
3. Auto Lightening: Consits of GL55 LDR and LED

Objective:
Offering a comprehensive solution for automated monitoring and care of plants, which includes watering and supplying light for photosynthesis, while ensuring an adequate water supply in the tank. 

Features:
- In GCP, creating a VM instance for receiving the real-time data from sensor using Mosquitto(Publish-Subscirbe MQTT protocol).
- Creating MongoDB on cloud to store the data for further data analysis.
- Integrating with V-One Software in creating a monitoring dashboard that enable the user to interact and manipulate the control of actuators remotely.
- Creating 3 workflows in V-One which inclduing training model, predicting data (Quality of Environment Growth) and alert plan (Notify user when water level in tank is low).
