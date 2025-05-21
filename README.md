# Arduino Healthcare Monitoring System — Code Documentation

## 1. Libraries and Sensor Setup

This project utilizes the following libraries:

- **`Wire.h`**  
  Used for I2C communication with the MAX30205 temperature sensor.

- **`SoftwareSerial.h`**  
  Provides software-based serial communication for Bluetooth (RX on pin 2, TX on pin 4).

- **`PulseSensorPlayground.h`**  
  Interface for the pulse sensor, used for heartbeat detection and BPM calculation.

- **`LiquidCrystal_I2C.h`**  
  Controls a 16x2 I2C LCD display. Default I2C address: `0x27`.

---

## 2. Pin Configuration and Constants

The following constants are used for hardware configuration:

```cpp
const int PULSE_SENSOR_PIN = 0;     // Analog pin A0 for pulse sensor
const int LED_PIN = 13;             // Built-in LED for heartbeat visual feedback
const int THRESHOLD = 700;          // Threshold for detecting pulse
const int BUZZER_PIN = 7;           // Buzzer output pin

#define MAX30205_ADDRESS 0x4C       // I2C address for MAX30205 temperature sensor
const unsigned long BT_INTERVAL = 1000;  // Bluetooth send interval in milliseconds
```

## 3. Global Buffers and Variables

Global buffers and variables used for processing sensor data and maintaining system state:

```cpp
float tempReadings[10];             // Circular buffer for temperature readings
int bpmReadings[10];                // Circular buffer for BPM readings
int readingIndex = 0;               // Current index in the circular buffers
int validCount = 0;                 // Number of valid readings collected (up to 10)
int buzzerMode = 1;                 // Buzzer mode: 0 = OFF, 1 = Temp alert, 2 = BPM alert
unsigned long lastBTReport = 0;     // Timestamp of the last Bluetooth report
```

## 4. Function: `readTemperatureLowLevel()`

This function retrieves temperature data from the MAX30205 sensor using low-level AVR TWI (I2C) register communication, bypassing the standard `Wire` library.

### Features:
- Uses direct register-level access (`TWCR`, `TWDR`, etc.).
- Combines MSB and LSB from the sensor’s output.
- Converts raw data into Celsius using the MAX30205 calibration.
- Minimizes library overhead for faster, precise control.

---

## 5. Median Filtering Functions

Custom functions used to remove noise and smooth readings from the temperature and pulse sensors.

```cpp
float medianWithoutOutliers(float *arr, int size);
int medianWithoutOutliersInt(int *arr, int size);
```


## How It Works

- Discards invalid values (≤ 0).
- Sorts the valid subset.
- Removes the minimum and maximum values as outliers.
- Returns the median of the remaining values.

This process ensures that transient spikes don't distort the system's readings.


## 6. `setup()` Function

Prepares all components and peripherals for operation at startup.

### Steps:
- Initialize Serial and Bluetooth communication at 9600 baud.
- Start I2C communication for the MAX30205.
- Configure the temperature sensor for continuous mode.
- Set the buzzer pin (`BUZZER_PIN`) as an output and set it LOW.
- Initialize the pulse sensor:
  - Set the analog threshold (`THRESHOLD`).
  - Enable LED feedback using `LED_PIN`.
- Set up the LCD and display a welcome/startup message.


## 7. `loop()` Function

Runs continuously, managing inputs, sensor readings, alerts, and output.

### 7.1 Bluetooth Command Parsing
- Checks for incoming characters:
  - `'0'`: Disable buzzer.
  - `'1'`: Enable temperature alert only.
  - `'2'`: Enable BPM alert only.
- Updates current mode on both Serial and LCD.
- Prints warnings for unknown commands.

### 7.2 Sensor Reading
- **Temperature**: 
  - Read using `readTemperatureLowLevel()`.
  - Optional Fahrenheit conversion.
- **Heart Rate**:
  - Read BPM via the pulse sensor if a heartbeat is detected.

### 7.3 Buffer Update
- Store the latest temperature and BPM in circular buffers:
  - `tempReadings[]`
  - `bpmReadings[]`
- Update `readingIndex` circularly (wrap around after 10).
- Increment `validCount` until 10 (buffer full).

### 7.4 Median Calculation (When Buffer is Full)
- Use:
  - `medianWithoutOutliers()` for temperature.
  - `medianWithoutOutliersInt()` for BPM.
- BPM adjustment:
  - Minimum value: 40
  - Divide by 3 to smooth spikes.

### 7.5 Buzzer Control Logic
- **Mode 0 (OFF)**: Buzzer always off.
- **Mode 1 (TEMP alert)**: Buzzer if temp > 37.5°C.
- **Mode 2 (BPM alert)**: Buzzer if BPM < 55 or BPM > 90.
- Uses `PORTD` bit manipulation instead of `digitalWrite()` for faster response.

### 7.6 Bluetooth and LCD Output
- Every 1 second:
  - Format and send a Bluetooth string:
    - `"Temp: XX.XX C, BPM: YY"`
  - Display median temp and BPM on the LCD.
- Include `delay(1000)` to regulate the reporting/sampling interval.

---

## 8. Optimization Notes

- **Direct Register Control**: 
  - Improves performance and timing precision.
  - Especially useful for time-sensitive I2C and output pin toggling.
- **Median Filtering**: 
  - Reduces false positives/negatives caused by noise or spikes.
- **Circular Buffers**:
  - Prevent memory fragmentation.
  - Allow ongoing smoothing without dynamic memory.
- **Modular Design**:
  - Separation of hardware reading, signal processing, and user feedback for maintainability.



