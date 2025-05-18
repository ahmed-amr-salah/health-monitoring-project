#include <Wire.h>
#include "Protocentral_MAX30205.h"

MAX30205 tempSensor;
const bool fahrenheittemp = false; // Set to false for Celsius

void setup() {
  Serial.begin(9600);
  Wire.begin();
  delay(100);

  // Set the sensor's I2C address
  tempSensor.sensorAddress = 0x4C;
  tempSensor.begin();

  // Try reading temperature to check if sensor works
  float temp = tempSensor.getTemperature();
  if (temp == 0.0) {
    Serial.println("WARNING: Sensor reading 0.0°C. Check if temperature is realistic.");
  } else {
    Serial.println("Sensor initialized and responding.");
  }
}

void loop() {
  float temp = tempSensor.getTemperature();

  if (temp == 0.0) {
    Serial.println("Sensor read failed or temperature is 0.0°C.");
  } else {
    if (fahrenheittemp) {
      temp = (temp * 1.8) + 32;
      Serial.print(temp, 2);
      Serial.println(" °F");
    } else {
      Serial.print(temp, 2);
      Serial.println(" °C");
    }
  }

  delay(1000);
}


