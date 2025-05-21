Arduino Healthcare Monitoring System — Code Documentation
Libraries and Sensor Setup
Wire.h: I2C protocol for MAX30205 temperature sensor.


SoftwareSerial.h: Software serial port for Bluetooth (pins 2 RX, 4 TX).


PulseSensorPlayground.h: Pulse sensor interface, BPM detection.


LiquidCrystal_I2C.h: 16x2 LCD display over I2C (address 0x27).



Pin & Constants
const int PULSE_SENSOR_PIN = 0;     // Analog input A0 for pulse sensor
const int LED_PIN = 13;             // Built-in LED for heartbeat blink
const int THRESHOLD = 700;          // Pulse detection threshold
const int BUZZER_PIN = 7;           // Buzzer output pin

#define MAX30205_ADDRESS 0x4C       // MAX30205 I2C address
const unsigned long BT_INTERVAL = 1000;  // Bluetooth send interval (1 sec)


Global Buffers & Variables
float tempReadings[10];     // Circular buffer for temperature smoothing
int bpmReadings[10];        // Circular buffer for BPM smoothing
int readingIndex = 0;       // Current buffer index
int validCount = 0;         // Number of valid buffered readings
int buzzerMode = 1;         // Buzzer modes: 0=OFF, 1=Temp alert, 2=BPM alert
unsigned long lastBTReport = 0; // Timestamp of last Bluetooth data sent


readTemperatureLowLevel()
Direct AVR TWI register-level I2C communication to read raw temperature bytes from MAX30205.


Combines MSB and LSB, converts to Celsius with calibration.


Avoids library overhead for more precise control.



Median Calculation Functions
float medianWithoutOutliers(float *arr, int size);
int medianWithoutOutliersInt(int *arr, int size);

Remove invalid (≤0) readings.


Sort remaining values.


Discard min and max as outliers.


Return median of trimmed dataset for noise-resistant smoothing.



setup()
Initialize Serial, Bluetooth (9600 baud).


Begin I2C.


Configure MAX30205 for continuous conversion.


Set buzzer pin as output (initially LOW).


Initialize pulse sensor with threshold, LED feedback.


Initialize LCD, display startup message.



loop() Workflow
Bluetooth Command Parsing:


Read incoming chars.


'0' disables buzzer, '1' enables temp alert, '2' enables BPM alert.


Display mode on LCD & Serial; unknown commands are reported.


Sensor Reading:


Temperature via low-level I2C read.


Convert to Fahrenheit if selected.


BPM from pulse sensor if heartbeat detected.


Buffer Update:


Store latest temp & bpm in rolling arrays.


Increment valid count up to buffer size (10).


Median Calculation (if buffer full):


Calculate median temp and bpm excluding outliers.


BPM median adjusted: minimum 40, smoothed by division by 3.


Buzzer Control (by mode):


OFF: buzzer off.


TEMP ONLY: buzzer if temp > 37.5°C.


BPM ONLY: buzzer if bpm < 55 or > 90.


Direct PORTD bit manipulation used to optimize pin toggling.


Bluetooth Reporting & LCD Update:


Every 1 sec, send formatted temp and bpm string over Bluetooth.


Update LCD with latest median readings.


Delay 1 second to control sampling rate.



Optimization Notes
Uses direct register manipulation (e.g., TWCR, PORTD) for precise and fast control.


Median filtering reduces sensor noise and transient spikes.


Circular buffers ensure continuous smoothing without dynamic memory.


Modular design separates hardware interface (temp read, pulse sensor) from processing and output logic.

