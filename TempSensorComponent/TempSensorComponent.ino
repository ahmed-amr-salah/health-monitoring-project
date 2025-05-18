#include <Wire.h>

#define MAX30205_ADDRESS 0x48  // Default address if A0–A2 are GND

void setup() {
  Serial.begin(9600);
  Wire.begin();  // SDA = 20, SCL = 21 on Mega
  Serial.println("MAX30205 Initialized...");
}

void loop() {
  float temperature = readTemperature();

  if (temperature > -50.0) { // skip error value
    Serial.print("Body Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
  }

  delay(1000);
}

float readTemperature() {
  Wire.beginTransmission(MAX30205_ADDRESS);
  Wire.write(0x00);  // Temperature register
  if (Wire.endTransmission(false) != 0) {
    Serial.println("I2C write failed");
    return -100.0;
  }

  Wire.requestFrom(MAX30205_ADDRESS, 2);
  if (Wire.available() < 2) {
    Serial.println("I2C read failed");
    return -100.0;
  }

  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();

  int16_t raw = (msb << 8) | lsb;
  raw >>= 3;  // 13-bit data

  return raw * 0.0625; // Convert to Celsius
}
