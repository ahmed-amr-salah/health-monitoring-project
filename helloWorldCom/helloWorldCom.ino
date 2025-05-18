void setup() {
  Serial.begin(9600);           // Start serial communication at 9600 baud
  Serial.println("Hello, World!");  // Print once at startup
}

void loop() {
  Serial.println("Looping..."); // Print repeatedly
  delay(1000);                  // Wait 1 second
}
