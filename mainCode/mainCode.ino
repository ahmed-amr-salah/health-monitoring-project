// #define USE_ARDUINO_INTERRUPTS true

// #include <Wire.h>
// #include "Protocentral_MAX30205.h"
// #include <PulseSensorPlayground.h>

// MAX30205 tempSensor;
// const bool fahrenheittemp = false;

// const int PULSE_SENSOR_PIN = 0;
// const int LED_PIN = 13;
// const int THRESHOLD = 550;
// const int BUZZER_PIN = 7;

// PulseSensorPlayground pulseSensor;

// float tempReadings[10];
// int bpmReadings[10];
// int readingIndex = 0;
// int validCount = 0;

// void setup() {
//   Serial.begin(9600);
//   Wire.begin();
//   delay(100);

//   pinMode(BUZZER_PIN, OUTPUT);
//   digitalWrite(BUZZER_PIN, LOW); // buzzer off by default

//   tempSensor.sensorAddress = 0x4C;
//   tempSensor.begin();

//   float temp = tempSensor.getTemperature();
//   if (temp == 0.0) {
//     Serial.println("WARNING: Sensor reading 0.0°C. Check wiring.");
//   } else {
//     Serial.println("Temperature sensor initialized.");
//   }

//   pulseSensor.analogInput(PULSE_SENSOR_PIN);
//   pulseSensor.blinkOnPulse(LED_PIN);
//   pulseSensor.setThreshold(THRESHOLD);

//   if (pulseSensor.begin()) {
//     Serial.println("PulseSensor initialized.");
//   } else {
//     Serial.println("PulseSensor failed to start.");
//   }
// }

// void loop() {
//   float temp = tempSensor.getTemperature();
//   if (fahrenheittemp) {
//     temp = (temp * 1.8) + 32;
//   }

//   int bpm = 0;
//   if (pulseSensor.sawStartOfBeat()) {
//     bpm = pulseSensor.getBeatsPerMinute();
//   }

//   tempReadings[readingIndex] =  temp;
//   bpmReadings[readingIndex] =  bpm;

//   readingIndex = (readingIndex + 1) % 10;
//   if (validCount < 10) validCount++;

//   if (validCount == 10) 
//   {
//     float tempMedian = medianWithoutOutliers(tempReadings, 10);
//     int bpmMedian = (int)medianWithoutOutliersInt(bpmReadings, 10);
//     //tempMedian > 0 && bpmMedian > 0
//     if ( true) 
//     {
//       Serial.print("Temp: ");
//       Serial.print(tempMedian, 2);
//       Serial.print(fahrenheittemp ? " °F" : " °C");
//       Serial.print(" | BPM: ");
//       Serial.println(bpmMedian);

//       // Buzzer warning logic
//       if (tempMedian > 37.5 || bpmMedian > 90 || bpmMedian < 55) 
//       {
//         digitalWrite(BUZZER_PIN, HIGH);
//       } else 
//       {
//         digitalWrite(BUZZER_PIN, LOW);
//       }
//     } 
//   } 
//   else 
//   {
//     Serial.println("Collecting readings...");
//   }

//   delay(1000);
// }

// // Sort float array copy and return median after removing 2 outliers
// float medianWithoutOutliers(float *arr, int size) {
//   float copy[10];
//   int valid = 0;

//   for (int i = 0; i < size; i++) {
//     if (arr[i] > 0) copy[valid++] = arr[i];
//   }

//   if (valid < 5) return 0; // not enough data

//   // Simple bubble sort (since array is small)
//   for (int i = 0; i < valid - 1; i++) {
//     for (int j = 0; j < valid - i - 1; j++) {
//       if (copy[j] > copy[j + 1]) {
//         float t = copy[j]; copy[j] = copy[j + 1]; copy[j + 1] = t;
//       }
//     }
//   }

//   // Remove min and max
//   int start = 1;
//   int end = valid - 2;
//   int medianIndex = (start + end) / 2;
//   return copy[medianIndex];
// }

// int medianWithoutOutliersInt(int *arr, int size) {
//   int copy[10];
//   int valid = 0;

//   for (int i = 0; i < size; i++) {
//     if (arr[i] > 0) copy[valid++] = arr[i];
//   }

//   if (valid < 5) return 0;

//   for (int i = 0; i < valid - 1; i++) {
//     for (int j = 0; j < valid - i - 1; j++) {
//       if (copy[j] > copy[j + 1]) {
//         int t = copy[j]; copy[j] = copy[j + 1]; copy[j + 1] = t;
//       }
//     }
//   }

//   int start = 1;
//   int end = valid - 2;
//   int medianIndex = (start + end) / 2;
//   return copy[medianIndex];
// }

#define USE_ARDUINO_INTERRUPTS true

#include <Wire.h>
#include <SoftwareSerial.h>
#include "Protocentral_MAX30205.h"
#include <PulseSensorPlayground.h>

// -------- Sensor Objects --------
MAX30205 tempSensor;
PulseSensorPlayground pulseSensor;

// -------- Settings --------
const bool fahrenheittemp = false;
const int PULSE_SENSOR_PIN = 0;
const int LED_PIN = 13;
const int THRESHOLD = 550;
const int BUZZER_PIN = 7;

// -------- Bluetooth (TX->11, RX->10) --------
SoftwareSerial bluetooth(10, 11); // RX, TX

// -------- Data Buffers --------
float tempReadings[10];
int bpmReadings[10];
int readingIndex = 0;
int validCount = 0;

// -------- Buzzer Control Mode --------
int buzzerMode = 1;  // Default to "temp critical"
unsigned long lastBTReport = 0;

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);

  Wire.begin();
  delay(100);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  tempSensor.sensorAddress = 0x4C;
  tempSensor.begin();

  float temp = tempSensor.getTemperature();
  Serial.println((temp == 0.0) ? "WARNING: Sensor reading 0.0°C." : "Temp sensor initialized.");

  pulseSensor.analogInput(PULSE_SENSOR_PIN);
  pulseSensor.blinkOnPulse(LED_PIN);
  pulseSensor.setThreshold(THRESHOLD);

  Serial.println(pulseSensor.begin() ? "PulseSensor initialized." : "PulseSensor failed.");
}

void loop() {
  // --- Read Temperature ---
  float temp = tempSensor.getTemperature();
  if (fahrenheittemp) temp = (temp * 1.8) + 32;

  // --- Read BPM ---
  int bpm = 0;
  if (pulseSensor.sawStartOfBeat()) {
    bpm = pulseSensor.getBeatsPerMinute();
  }

  tempReadings[readingIndex] = temp;
  bpmReadings[readingIndex] = bpm;

  readingIndex = (readingIndex + 1) % 10;
  if (validCount < 10) validCount++;

  // --- Bluetooth Command Handling ---
  if (bluetooth.available()) {
    char incoming = bluetooth.read();
    Serial.println(incoming);
    if (incoming == '0') {
      buzzerMode = 0;
      Serial.println("Buzzer mode: OFF");
    } else if (incoming == '1') {
      buzzerMode = 1;
      Serial.println("Buzzer mode: TEMP ONLY");
    } else if (incoming == '2') {
      buzzerMode = 2;
      Serial.println("Buzzer mode: BPM ONLY");
    }
  
  }


  // --- Once 10 readings are collected ---
  if (validCount == 10) {
    float tempMedian = medianWithoutOutliers(tempReadings, 10);
    int bpmMedian = medianWithoutOutliersInt(bpmReadings, 10);

    Serial.print("Temp: ");
    Serial.print(tempMedian, 2);
    Serial.print(fahrenheittemp ? " °F" : " °C");
    Serial.print(" | BPM: ");
    Serial.println(bpmMedian);

    // --- Buzzer Control Logic ---
    bool tempAlert = (tempMedian > 37.5);
    bool bpmAlert = (bpmMedian > 90 || bpmMedian < 55);

    switch (buzzerMode) {
      case 0:
        digitalWrite(BUZZER_PIN, LOW);
        break;
      case 1:
        digitalWrite(BUZZER_PIN, tempAlert ? HIGH : LOW);
        break;
      case 2:
        digitalWrite(BUZZER_PIN, bpmAlert ? HIGH : LOW);
        break;
    }

    // --- Send Data to Bluetooth every 20 seconds ---
    if (millis() - lastBTReport > 20000) {
      bluetooth.print("Temp: ");
      bluetooth.print(tempMedian, 2);
      bluetooth.print(fahrenheittemp ? " °F" : " °C");
      bluetooth.print(" | BPM: ");
      bluetooth.println(bpmMedian);
      lastBTReport = millis();
    }
  } else {
    Serial.println("Collecting readings...");
  }

  delay(1000);
}

// -------- Helper Functions --------
float medianWithoutOutliers(float *arr, int size) {
  float copy[10];
  int valid = 0;
  for (int i = 0; i < size; i++) {
    if (arr[i] > 20.0 && arr[i] < 45.0) copy[valid++] = arr[i];
  }

  if (valid < 5) return 0.0;

  // Simple bubble sort
  for (int i = 0; i < valid - 1; i++) {
    for (int j = i + 1; j < valid; j++) {
      if (copy[i] > copy[j]) {
        float t = copy[i];
        copy[i] = copy[j];
        copy[j] = t;
      }
    }
  }

  int mid = valid / 2;
  return (valid % 2 == 0) ? (copy[mid - 1] + copy[mid]) / 2.0 : copy[mid];
}

int medianWithoutOutliersInt(int *arr, int size) {
  int copy[10];
  int valid = 0;
  for (int i = 0; i < size; i++) {
    if (arr[i] > 40 && arr[i] < 180) copy[valid++] = arr[i];
  }

  if (valid < 5) return 0;

  // Simple bubble sort
  for (int i = 0; i < valid - 1; i++) {
    for (int j = i + 1; j < valid; j++) {
      if (copy[i] > copy[j]) {
        int t = copy[i];
        copy[i] = copy[j];
        copy[j] = t;
      }
    }
  }

  int mid = valid / 2;
  return (valid % 2 == 0) ? (copy[mid - 1] + copy[mid]) / 2 : copy[mid];
}

