#define USE_ARDUINO_INTERRUPTS true

#include <Wire.h>
#include <SoftwareSerial.h>
// #include "Protocentral_MAX30205.h"
#include <PulseSensorPlayground.h>
#include <LiquidCrystal_I2C.h>   // Add LCD library

// -------- Sensor Objects --------
// MAX30205 tempSensor;
PulseSensorPlayground pulseSensor;

// -------- Settings --------
const bool fahrenheittemp = false;
const int PULSE_SENSOR_PIN = 0;
const int LED_PIN = 13;
const int THRESHOLD = 550;
const int BUZZER_PIN = 7;

// -------- Bluetooth (RX->2, TX->4) --------
SoftwareSerial bluetooth(2, 4); // RX, TX

// -------- LCD --------
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD address 0x27, 16 chars, 2 lines

// -------- Data Buffers --------
float tempReadings[10];
int bpmReadings[10];
int readingIndex = 0;
int validCount = 0;

// -------- Buzzer Control Mode --------
int buzzerMode = 1;  // Default to "temp critical"
unsigned long lastBTReport = 0;
// const unsigned long BT_INTERVAL = 20000;  // 20 seconds
const unsigned long BT_INTERVAL = 1000;  // 20 seconds

#define MAX30205_ADDRESS 0x4C  // Sensor I2C address
#define TEMP_REG 0x00          // Temperature register

float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(TEMP_REG);
  Wire.endTransmission(false); // repeated‐start

  Wire.requestFrom(MAX30205_ADDRESS, 2);
  if (Wire.available() < 2) return 0.0;

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  int16_t raw = (msb << 8) | lsb; // Combine bytes into 16-bit signed value
  float reading = raw * 0.00390625; // Correct scaling for normal mode
  reading = (reading - 21.0) * 12.0 / 9.0 + 21.0;
  return reading;
}

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);

  Wire.begin();
  delay(100);

  // ======== MAX30205 configuration ========
  // Write CONFIG register (0x01) = 0x01 to enable continuous conversion
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x01);    // CONFIG register
  Wire.write(0x00);    // Continuous‐conversion bit = 1
  Wire.endTransmission(); 
  delay(50);           // wait for first conversion
  // ======== end MAX30205 init ========

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // tempSensor.sensorAddress = 0x4C;
  // tempSensor.begin();

  // float temp = tempSensor.getTemperature();
  float temp = readTemperature();
  Serial.println((temp == 0.0) ? "WARNING: Sensor reading 0.0°C." : "Temp sensor initialized.");

  pulseSensor.analogInput(PULSE_SENSOR_PIN);
  pulseSensor.blinkOnPulse(LED_PIN);
  pulseSensor.setThreshold(THRESHOLD);

  if (pulseSensor.begin()) {
    Serial.println("PulseSensor initialized.");
  } else {
    Serial.println("PulseSensor failed to start.");
  }

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("System Starting");
}

void loop() {
  // --- Bluetooth receiving ---
  if (bluetooth.available()) {
    char incoming = bluetooth.read();

    if (incoming == '\r' || incoming == '\n') {
      // Ignore newline or carriage return characters
    } else if (incoming == '0') {
      buzzerMode = 0;
      Serial.println("Buzzer mode: OFF");
      lcd.clear();
      lcd.print("Buzzer mode: OFF");
    } else if (incoming == '1') {
      buzzerMode = 1;
      Serial.println("Buzzer mode: TEMP ONLY");
      lcd.clear();
      lcd.print("Buzzer mode: TEMP");
    } else if (incoming == '2') {
      buzzerMode = 2;
      Serial.println("Buzzer mode: BPM ONLY");
      lcd.clear();
      lcd.print("Buzzer mode: BPM");
    } else {
      Serial.print("Unknown command: ");
      Serial.println(incoming);
      lcd.clear();
      lcd.print("Unknown cmd:");
      lcd.setCursor(0, 1);
      lcd.print(incoming);
    }
  }

  // --- Sensor reading and processing ---
  // float temp = tempSensor.getTemperature();
  float temp = readTemperature();
  if (fahrenheittemp) {
    temp = (temp * 1.8) + 32;
  }

  int bpm = 0;
  if (pulseSensor.sawStartOfBeat()) {
    bpm = pulseSensor.getBeatsPerMinute();
  }

  tempReadings[readingIndex] = temp;
  bpmReadings[readingIndex] = bpm;

  readingIndex = (readingIndex + 1) % 10;
  if (validCount < 10) validCount++;

  if (validCount == 10) {
    float tempMedian = medianWithoutOutliers(tempReadings, 10);
    int bpmMedian = (int)medianWithoutOutliersInt(bpmReadings, 10);

    // Control buzzer based on mode
    switch (buzzerMode) {
      case 0: // OFF
        digitalWrite(BUZZER_PIN, LOW);
        break;
      case 1: // TEMP ONLY
        digitalWrite(BUZZER_PIN, (tempMedian > 37.5) ? HIGH : LOW);
        break;
      case 2: // BPM ONLY
        digitalWrite(BUZZER_PIN, (bpmMedian > 90 || bpmMedian < 55) ? HIGH : LOW);
        break;
      default:
        digitalWrite(BUZZER_PIN, LOW);
        break;
    }

    // Send data every 20 seconds
    unsigned long currentMillis = millis();
    if (currentMillis - lastBTReport >= BT_INTERVAL) {
      lastBTReport = currentMillis;

      String dataString = "Temp: " + String(tempMedian, 2) + (fahrenheittemp ? " °F" : " °C") + " | BPM: " + String(bpmMedian);
      bluetooth.println(dataString);
      Serial.println("Sent over BT: " + dataString);

      // Update LCD with latest values too
      lcd.clear();
      lcd.print("T:");
      lcd.print(tempMedian, 1);
      lcd.print(fahrenheittemp ? "F" : "C");
      lcd.setCursor(0, 1);
      lcd.print("BPM:");
      lcd.print(bpmMedian);
    }
  } else {
    // Serial.println("Collecting readings...");

    float tempMedian = medianWithoutOutliers(tempReadings, 10);
    int bpmMedian = (int)medianWithoutOutliersInt(bpmReadings, 10);

    // Control buzzer based on mode
    switch (buzzerMode) {
      case 0: // OFF
        digitalWrite(BUZZER_PIN, LOW);
        break;
      case 1: // TEMP ONLY
        digitalWrite(BUZZER_PIN, (tempMedian > 37.5) ? HIGH : LOW);
        break;
      case 2: // BPM ONLY
        digitalWrite(BUZZER_PIN, (bpmMedian > 90 || bpmMedian < 55) ? HIGH : LOW);
        break;
      default:
        digitalWrite(BUZZER_PIN, LOW);
        break;
    }

    // Send data every 20 seconds
    unsigned long currentMillis = millis();
    if (currentMillis - lastBTReport >= BT_INTERVAL) {
      lastBTReport = currentMillis;

      String dataString = "Temp: " + String(tempMedian, 2) + (fahrenheittemp ? " °F" : " °C") + " | BPM: " + String(bpmMedian);
      bluetooth.println(dataString);
      Serial.println("Sent over BT: " + dataString);

      // Update LCD with latest values too
      lcd.clear();
      lcd.print("T:");
      lcd.print(tempMedian, 1);
      lcd.print(fahrenheittemp ? "F" : "C");
      lcd.setCursor(0, 1);
      lcd.print("BPM:");
      lcd.print(bpmMedian);
    }
  }

  delay(1000);
}

// Sort float array copy and return median after removing 2 outliers
float medianWithoutOutliers(float *arr, int size) {
  float copy[10];
  int valid = 0;

  for (int i = 0; i < size; i++) {
    if (arr[i] > 0) copy[valid++] = arr[i];
  }

  if (valid < 5) return 0; // not enough data

  // Simple bubble sort (small array)
  for (int i = 0; i < valid - 1; i++) {
    for (int j = 0; j < valid - i - 1; j++) {
      if (copy[j] > copy[j + 1]) {
        float t = copy[j]; copy[j] = copy[j + 1]; copy[j + 1] = t;
      }
    }
  }

  // Remove min and max
  int start = 1;
  int end = valid - 2;
  int medianIndex = (start + end) / 2;
  return copy[medianIndex];
}

int medianWithoutOutliersInt(int *arr, int size) {
  int copy[10];
  int valid = 0;

  for (int i = 0; i < size; i++) {
    if (arr[i] > 0) copy[valid++] = arr[i];
  }

  if (valid < 5) return 0;

  for (int i = 0; i < valid - 1; i++) {
    for (int j = 0; j < valid - i - 1; j++) {
      if (copy[j] > copy[j + 1]) {
        int t = copy[j]; copy[j] = copy[j + 1]; copy[j + 1] = t;
      }
    }
  }

  int start = 1;
  int end = valid - 2;
  int medianIndex = (start + end) / 2;
  return copy[medianIndex];
}
