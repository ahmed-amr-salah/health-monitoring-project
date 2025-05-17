String btInput = "";    // Incoming message from Bluetooth
String serialInput = ""; // Incoming message from Serial Monitor

void setup() {
  Serial.begin(9600);      // Serial Monitor
  Serial1.begin(9600);     // HC-05 Bluetooth module

  Serial.println("Ready! Type in Serial Monitor to send to phone.");
}

void loop() {
  // ----- Read full message from Bluetooth (Serial1) -----
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      Serial.print("Bluetooth → Arduino: ");
      Serial.println(btInput);
      btInput = ""; // Clear buffer
    } else {
      btInput += c;
    }
  }

  // ----- Read full message from Serial Monitor (Serial) -----
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      Serial1.println(serialInput); // Send full string to phone
      Serial.print("Arduino → Bluetooth: ");
      Serial.println(serialInput);
      serialInput = ""; // Clear buffer
    } else {
      serialInput += c;
    }
  }
}
