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
const int THRESHOLD = 700;
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
int bpm_avg = 0;
// -------- Buzzer Control Mode --------
int buzzerMode = 0;  // Default to "temp critical"
unsigned long lastBTReport = 0;
// const unsigned long BT_INTERVAL = 20000;  // 20 seconds
const unsigned long BT_INTERVAL = 1000;  // 20 seconds


#define MAX30205_ADDRESS 0x4C  // Sensor I2C address
#define TEMP_REG 0x00          // Temperature register


int pulse_count = 0;


float readTemperatureLowLevel() {
 #define MAX30205_ADDRESS_WRITE 0x98  // 0x4C << 1 | write (0)
 #define MAX30205_ADDRESS_READ  0x99  // 0x4C << 1 | read (1)
 #define TEMP_REG 0x00


 // Initialize TWI
 TWBR = 72; // 100kHz for 16MHz clock
 TWSR = 0x00;


 // Send START
 TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
 while (!(TWCR & (1 << TWINT)));


 // Send device address (write)
 TWDR = MAX30205_ADDRESS_WRITE;
 TWCR = (1 << TWEN) | (1 << TWINT);
 while (!(TWCR & (1 << TWINT)));


 // Send register pointer (TEMP_REG = 0x00)
 TWDR = TEMP_REG;
 TWCR = (1 << TWEN) | (1 << TWINT);
 while (!(TWCR & (1 << TWINT)));


 // Repeated START
 TWCR = (1 << TWSTA) | (1 << TWEN) | (1 << TWINT);
 while (!(TWCR & (1 << TWINT)));


 // Send device address (read)
 TWDR = MAX30205_ADDRESS_READ;
 TWCR = (1 << TWEN) | (1 << TWINT);
 while (!(TWCR & (1 << TWINT)));


 // Receive MSB (ACK)
 TWCR = (1 << TWEN) | (1 << TWINT) | (1 << TWEA);
 while (!(TWCR & (1 << TWINT)));
 uint8_t msb = TWDR;


 // Receive LSB (NACK)
 TWCR = (1 << TWEN) | (1 << TWINT);  // no TWEA = send NACK
 while (!(TWCR & (1 << TWINT)));
 uint8_t lsb = TWDR;


 // Send STOP
 TWCR = (1 << TWSTO) | (1 << TWEN) | (1 << TWINT);


 // Process temperature
 int16_t raw = (msb << 8) | lsb;
 float reading = raw * 0.00390625;
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


 DDRD |= (1 << PD7);  // Set PD7 (pin 7) as output
 digitalWrite(BUZZER_PIN, LOW);


 // tempSensor.sensorAddress = 0x4C;
 // tempSensor.begin();


 // float temp = tempSensor.getTemperature();
 float temp = readTemperatureLowLevel();
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
   } else if (incoming == '3') {
     buzzerMode = 3;
     Serial.println("Buzzer mode: BPM and Temp");
     lcd.clear();
     lcd.print("Buzzer mode: BPM");
   }
   else {
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
 float temp = readTemperatureLowLevel();
 if (fahrenheittemp) {
   temp = (temp * 1.8) + 32;
 }


 int bpm = 0;
 if (pulseSensor.sawStartOfBeat()) {
   bpm = pulseSensor.getBeatsPerMinute();
   pulse_count++;
 }


 tempReadings[readingIndex] = temp;
 bpmReadings[readingIndex] = bpm;


 readingIndex = (readingIndex + 1) % 10;
 if (validCount < 10) validCount++;


 if (validCount == 10) {
   float tempMedian = medianWithoutOutliers(tempReadings, 10);
   int bpmMedian = (int)medianWithoutOutliersInt(bpmReadings, 10);
   bpmMedian = 40 * (bpmMedian > 0) + bpmMedian / 3;


   // Control buzzer based on mode
   switch (buzzerMode) {
     case 0: // OFF
       PORTD &= ~(1 << PD7);  // Clear PD7 (LOW)
       break;
     case 1: // TEMP ONLY
       digitalWrite(BUZZER_PIN, ((tempMedian > 37.5) || (tempMedian < 36.5)) ? HIGH : LOW);
       break;
     case 2: // BPM ONLY
       digitalWrite(BUZZER_PIN, (bpmMedian > 110 || bpmMedian < 55) ? HIGH : LOW);
       break;
     case 3: // BPM ONLY
       digitalWrite(BUZZER_PIN, (((tempMedian > 37.5) || (tempMedian < 36.5)) || (bpmMedian > 100 || bpmMedian < 55)) ? HIGH : LOW);
       break;
     default:
       PORTD &= ~(1 << PD7);  // Clear PD7 (LOW)
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
   Serial.println("Collecting readings...");


   // float tempMedian = medianWithoutOutliers(tempReadings, 10);
   // int bpmMedian = (int)medianWithoutOutliersInt(bpmReadings, 10);
  
   // // Control buzzer based on mode
   // switch (buzzerMode) {
   //   case 0: // OFF
   //     digitalWrite(BUZZER_PIN, LOW);
   //     break;
   //   case 1: // TEMP ONLY
   //     digitalWrite(BUZZER_PIN, (tempMedian > 37.5) ? HIGH : LOW);
   //     break;
   //   case 2: // BPM ONLY
   //     digitalWrite(BUZZER_PIN, (bpmMedian > 90 || bpmMedian < 55) ? HIGH : LOW);
   //     break;
   //   default:
   //     digitalWrite(BUZZER_PIN, LOW);
   //     break;
   // }


   // // Send data every 20 seconds
   // unsigned long currentMillis = millis();
   // if (currentMillis - lastBTReport >= BT_INTERVAL) {
   //   lastBTReport = currentMillis;


   //   String dataString = "Temp: " + String(tempMedian, 2) + (fahrenheittemp ? " °F" : " °C") + " | BPM: " + String(bpmMedian);
   //   bluetooth.println(dataString);
   //   Serial.println("Sent over BT: " + dataString);


   //   // Update LCD with latest values too
   //   lcd.clear();
   //   lcd.print("T:");
   //   lcd.print(tempMedian, 1);
   //   lcd.print(fahrenheittemp ? "F" : "C");
   //   lcd.setCursor(0, 1);
   //   lcd.print("BPM:");
   //   lcd.print(bpmMedian);
   // }
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
