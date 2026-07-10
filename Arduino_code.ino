//code for arduino 

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Arduino.h>

// Replace with your network credentials
#define WIFI_SSID "WIFI"
#define WIFI_PASSWORD "WIFI24!"

// Replace with your Firebase project credentials
#define FIREBASE_HOST "leefyweb-d0e86-default-rtdb.asia-southeast1.firebasedatabase.app/" // Your Firebase Database URL
#define FIREBASE_AUTH "AIzaSyBXyNY3RHIRTWBvk-Jl6KU869ZP1urWonQ" // Replace with your actual Firebase Auth token

// Define the pins
const int moisturePin = 32;   // Analog pin connected to the moisture sensor (ESP32)
const int relayPin = 23;     // Digital pin connected to the relay module (ESP32)
const int tempPin = 34;      // Analog pin connected to the NTC thermistor (ESP32)

// Calibration values for the moisture sensor (adjust these based on your sensor)
const int dryValue = 0;   // Analog reading when the sensor is completely dry
const int wetValue = 4095;  // Analog reading when the sensor is completely wet (ESP32)

// Threshold percentages
const int moistureThresholdOn = 40;  // Percentage to turn the relay ON
const int moistureThresholdOff = 60; // Percentage to turn the relay OFF

// NTC Thermistor parameters
const float r0 = 10000;  // Resistance of the thermistor at 25°C
const float beta = 3950;    // Beta coefficient of the thermistor

// Variables
int moistureReading;
float moisturePercentage;
bool relayState = true; // Initially, the relay is OFF
int tempReading;
float temperature;

FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  pinMode(moisturePin, INPUT);
  pinMode(tempPin, INPUT);
  
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Set Firebase configuration
  firebaseConfig.database_url = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH; // Use the legacy token for authentication

  // Initialize Firebase
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);
  
  Serial.println("Firebase initialized successfully");

  // Ask for username
  String username = askForUsername();
  
  // Send sensor data to Firebase under the user's path
  if (sendSensorDataToFirebase(username)) {
    Serial.println("Sensor data sent to Firebase successfully.");
  } else {
    Serial.println("Failed to send sensor data to Firebase.");
  }
}

void loop() {
  // Moisture Sensor Reading
  moistureReading = analogRead(moisturePin);
  moisturePercentage = map(moistureReading, dryValue, wetValue, 100, 0);
  moisturePercentage = constrain(moisturePercentage, 0, 100);

  Serial.print("Moisture: ");
  Serial.print(moisturePercentage);
  Serial.print("%  ");

  // Relay Control
  if (moisturePercentage <= moistureThresholdOn && !relayState) {
    digitalWrite(relayPin, LOW);
    relayState = true;
    Serial.print("Relay ON  ");
  } else if (moisturePercentage >= moistureThresholdOff && relayState) {
    digitalWrite(relayPin, HIGH);
    relayState = false;
    Serial.print("Relay OFF ");
  }

  int adcValue = analogRead(tempPin); // Read the analog value
  float voltage = adcValue * (3.3 / 4095.0); // Convert to voltage
  float resistance = (3.3 * r0) / voltage - r0; // Calculate resistance

  // Calculate temperature using the Beta value
  temperature = 1 / (1 / 298.15 + (1 / beta) * log(resistance / r0)) - 273.15;

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C");

  // Send sensor data to Firebase every loop iteration
  String username = askForUsername(); // You might want to store this instead of asking every loop
  sendSensorDataToFirebase(username);

  delay(500);
}

String askForUsername() {
  String username;
  Serial.println("Please enter your username:");
  
  while (Serial.available() == 0) {
    // Wait for user input
  }
  
  // Read the username from Serial
  username = Serial.readStringUntil('\n');
  username.trim(); // Remove any whitespace
  return username;
}

bool sendSensorDataToFirebase(String username) {
  // Create a path in the database to check if the username exists
  String userPath = "users/" + username;

  // Check if the username exists
  if (Firebase.getString(firebaseData, userPath)) {
    // If the username exists, create new paths for the sensor data
    String moisturePath = userPath + "/PlantData/moisture"; // New hierarchy under the username
    String temperaturePath = userPath + "/PlantData/temperature"; // New hierarchy under the username

    // Set moisture and temperature data in the database
    if (Firebase.setFloat(firebaseData, moisturePath, moisturePercentage) &&
        Firebase.setFloat(firebaseData, temperaturePath, temperature)) {
      return true;
    } else {
      Serial.print("Error sending data: ");
      Serial.println(firebaseData.errorReason());
      return false;
    }
  } else {
    Serial.println("Username does not exist. Data not sent.");
    return false; // Username does not exist, do not send data
  }
}
