#include <SPI.h>
#include <WiFiNINA.h>
#include <Wire.h>
#include <BH1750.h>

BH1750 GY30; // instantiate a sensor event object

#include "Secret.h" 
// Please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // Your network SSID (name)
char pass[] = SECRET_PASS;        // Your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;      // The WiFi radio's status
WiFiClient client;

// defines IFTTT event webhook listeners
char   HOST_NAME[] = "maker.ifttt.com";
String LIGHT_PATH   = "/trigger/light-intensity /with/key/pftond4Ov8LNj4gbPPCEi7sTux35xoYOq4jIzNt4S6P"; // change your EVENT-NAME and YOUR-KEY
String MOISTURE_PATH = "/trigger/soil-moisture/with/key/pftond4Ov8LNj4gbPPCEi7sTux35xoYOq4jIzNt4S6P"; // change your EVENT-NAME and YOUR-KEY

// Soil moisture sensor setup
#define SOIL_MOISTURE_PIN A0
int dryValue = 0; // Adjust this value based on your calibration
int wetValue = 600;  // Adjust this value based on your calibration
bool moistureNotified = false; // Flag to track if notification for soil moisture has been sent

void setup() {
  Serial.begin(9600);

  // Attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    // Wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to WiFi");

  Wire.begin(); // Initialize the I2C bus for use by the BH1750 library  
  GY30.begin(); // Initialize the sensor object

  if (client.connect(HOST_NAME, 80)) {
    // if connected:
    Serial.println("Connected to server");
  } else {
    // if not connected:
    Serial.println("connection failed");
  }
}

bool aboveThresholdNotified = false; // Flag to track if notification for lux > 150 has been sent
bool belowThresholdNotified = false; // Flag to track if notification for lux < 150 has been sent

void loop() {
  float lux = GY30.readLightLevel(); // read the light level from the sensor and store it in a variable
  Serial.println(lux); // print the data to the serial monitor
  
  // Read soil moisture
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  float soilMoisturePercentage = map(soilMoistureValue, dryValue, wetValue, 0, 100);
  Serial.print("Soil Moisture: ");
  Serial.print(soilMoisturePercentage);
  Serial.println(" %");

  delay(1000); // Pause for a second before repeating the sensor poll

  // Light intensity notification
  if (lux >= 150.00) { // Send trigger if lux exceeds 150.00
    if (!aboveThresholdNotified) { // Check if notification has not been sent yet
      aboveThresholdNotified = true; // Set the flag to indicate notification sent
      belowThresholdNotified = false; // Reset the flag for below threshold notification

      // Disconnect from the server to allow new trigger
      if (client.connected()) {
        client.stop();
        Serial.println("Disconnected from server");
      }

      // Attempt to reconnect to the server
      if (client.connect(HOST_NAME, 80)) {
        Serial.println("Connected to server");
      } else {
        Serial.println("Connection failed");
      }

      // make a HTTP request for lux > 150 notification:
      // send HTTP header
      client.println("GET " + LIGHT_PATH + "?value1=" + String(lux) + " HTTP/1.1");
      client.println("Host: " + String(HOST_NAME));
      client.println("Connection: close");
      client.println(); // end HTTP header
    }
  } else { // lux < 150
    if (!belowThresholdNotified) { // Check if notification has not been sent yet
      belowThresholdNotified = true; // Set the flag to indicate notification sent
      aboveThresholdNotified = false; // Reset the flag for above threshold notification

      // Disconnect from the server to allow new trigger
      if (client.connected()) {
        client.stop();
        Serial.println("Disconnected from server");
      }

      // Attempt to reconnect to the server
      if (client.connect(HOST_NAME, 80)) {
        Serial.println("Connected to server");
      } else {
        Serial.println("Connection failed");
      }

      // make a HTTP request for lux < 150 notification:
      // send HTTP header
      client.println("GET " + LIGHT_PATH + "?value1=" + String(lux) + " HTTP/1.1");
      client.println("Host: " + String(HOST_NAME));
      client.println("Connection: close");
      client.println(); // end HTTP header
    }
  }

  // Soil moisture notification
  if (soilMoisturePercentage < 50 && !moistureNotified) { // Send trigger if soil moisture is below 50% and not already notified
    moistureNotified = true; // Set the flag to indicate notification sent

    // Disconnect from the server to allow new trigger
    if (client.connected()) {
      client.stop();
      Serial.println("Disconnected from server");
    }

    // Attempt to reconnect to the server
    if (client.connect(HOST_NAME, 80)) {
      Serial.println("Connected to server");
    } else {
      Serial.println("Connection failed");
    }

    // make a HTTP request for soil moisture < 50 notification:
    // send HTTP header
    client.println("GET " + MOISTURE_PATH + "?value1=" + String(soilMoisturePercentage) + " HTTP/1.1");
    client.println("Host: " + String(HOST_NAME));
    client.println("Connection: close");
    client.println(); // end HTTP header
  } else if (soilMoisturePercentage >= 50) {
    moistureNotified = false; // Reset the flag for soil moisture notification
  }
}
