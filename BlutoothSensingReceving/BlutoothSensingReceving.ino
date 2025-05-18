#include <SoftwareSerial.h>

// RX = pin 10 (Arduino receives data here from BT TX)
// TX = pin 11 (Arduino sends data here to BT RX)
SoftwareSerial bluetooth(12, 13);

String btInput = "";     // Buffer for incoming data from BT
String serialInput = ""; // Buffer for input from Serial Monitor

void setup() {
  Serial.begin(9600);       // Serial Monitor
  bluetooth.begin(9600);    // Bluetooth module baud rate (usually 9600 for HC-05/HC-06)

  Serial.println("Bluetooth Test Started.");
  Serial.println("Type in Serial Monitor to send to Bluetooth.");
}

void loop() {
  // Read from Bluetooth
  while (bluetooth.available()) 
  {
    char c = bluetooth.read();
    Serial.print("BT → Arduino: ");
    Serial.print(c);
  }
    // if (c == '\n') {
    //   Serial.print("BT → Arduino: ");
    //   Serial.println(btInput);
    //   btInput = "";
    // } else {
    //   btInput += c;
    // }


  // Read from Serial Monitor
  while (Serial.available()) {
    char c = Serial.read();
    Serial.print("Arduino → BT: ");
    Serial.println(serialInput);
    
    if (c == '\n') {
      bluetooth.println(serialInput);  // Send to Bluetooth
      Serial.print("Arduino → BT: ");
      Serial.println(serialInput);
      serialInput = "";
    } else {
      serialInput += c;
    }
  }
}
