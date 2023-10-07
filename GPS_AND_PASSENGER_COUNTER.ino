#include <WiFi.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define API_KEY "AIzaSyBUiTk7FNOFO7uBf3Pcr2F6n6dFHdIyKuQ"
#define DATABASE_URL "https://track-ce9f2-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "ro3BH2BL2IzBeVjNMFNgoFgZMIwsssQHVFJPMdvB"

#define WIFI_SSID "Test"
#define WIFI_PASSWORD "12345678"

#define ENTRY_SENSOR_TRIGGER_PIN 21
#define ENTRY_SENSOR_ECHO_PIN 19

#define EXIT_SENSOR_TRIGGER_PIN 5
#define EXIT_SENSOR_ECHO_PIN 18

#define DISTANCE_THRESHOLD 25 // In centimeters

TinyGPSPlus gps;
SoftwareSerial GPSSerial(16, 17);
FirebaseData firebaseData;

int passenger_count = 25;

// Initialize variables for storing sensor readings
long entry_duration, exit_duration;
int entry_distance, exit_distance;

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
bool signupOK = false;

unsigned long previousLatLongMillis = 0;
const unsigned long latLongInterval = 5000; // 5 seconds

void count() {
  // Trigger entry sensor and calculate duration
  digitalWrite(ENTRY_SENSOR_TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ENTRY_SENSOR_TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ENTRY_SENSOR_TRIGGER_PIN, LOW);
  entry_duration = pulseIn(ENTRY_SENSOR_ECHO_PIN, HIGH);

  // Trigger exit sensor and calculate duration
  digitalWrite(EXIT_SENSOR_TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(EXIT_SENSOR_TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(EXIT_SENSOR_TRIGGER_PIN, LOW);
  exit_duration = pulseIn(EXIT_SENSOR_ECHO_PIN, HIGH);

  // Calculate distances from durations
  entry_distance = (entry_duration * 0.0343) / 2;
  exit_distance = (exit_duration * 0.0343) / 2;

  // Check if a person has entered
  if (entry_distance < DISTANCE_THRESHOLD && exit_distance > DISTANCE_THRESHOLD && entry_distance > 0) {
    if (passenger_count <= 0) {
      passenger_count = 0;
      Firebase.RTDB.setFloat(&fbdo, "Bus1/Passenger/counter/", passenger_count);
      Serial.print("Person exited. Passenger count: ");
      Serial.println(passenger_count);
    } else {
      passenger_count--;
      Serial.print("Person entered. Passenger count: ");
      Serial.println(passenger_count);
      Firebase.RTDB.setFloat(&fbdo, "Bus1/Passenger/counter/", passenger_count);
      delay(1000); // Wait to avoid counting the same person multiple times
    }
  }

  // Check if a person has exited
  if (exit_distance < DISTANCE_THRESHOLD && entry_distance > DISTANCE_THRESHOLD && exit_distance > 0) {
    if (passenger_count >= 25) {
      passenger_count = 25;
      Firebase.RTDB.setFloat(&fbdo, "Bus1/Passenger/counter/", passenger_count);
      Serial.print("Person exited. Passenger count: ");
      Serial.println(passenger_count);
    } else {
      passenger_count++;
      Serial.print("Person exited. Passenger count: ");
      Serial.println(passenger_count);
      Firebase.RTDB.setFloat(&fbdo, "Bus1/Passenger/counter/", passenger_count);
      delay(1000); // Wait to avoid counting the same person multiple times
    }
  }
}
void latlong() {
  if (millis() - previousLatLongMillis >= latLongInterval) {
    previousLatLongMillis = millis();
  
    while (Serial2.available() > 0) {
      gps.encode(Serial2.read());
    }
    if (gps.location.isValid()) {
      Firebase.RTDB.setFloat(&fbdo, "Bus1/latitude", gps.location.lat());
      Firebase.RTDB.setFloat(&fbdo, "Bus1/longitude", gps.location.lng());
      Serial.print("Latitude: ");
      Serial.println(gps.location.lat(), 6);
      Serial.print("Longitude: ");
      Serial.println(gps.location.lng(), 6);
    } else {
      Firebase.RTDB.setString(&fbdo, "Bus1/nosignal", "Location not available");
      Serial.println("Location not available");
    }
  }
}

void setup() {
  Serial.begin(115200);
  GPSSerial.begin(9600);
  Serial2.begin(9600);
  pinMode(ENTRY_SENSOR_TRIGGER_PIN, OUTPUT);
  pinMode(EXIT_SENSOR_TRIGGER_PIN, OUTPUT);
  pinMode(ENTRY_SENSOR_ECHO_PIN, INPUT);
  pinMode(EXIT_SENSOR_ECHO_PIN, INPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;

  config.database_url = DATABASE_URL;


  if (Firebase.signUp(&config,&auth,"","")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

}


void loop() {
  count();
  latlong();
}
