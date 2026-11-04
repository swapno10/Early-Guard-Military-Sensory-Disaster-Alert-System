# EarlyGuard – ESP32 Pin Configuration

## Node 1 – Sensor and Alert Node

| Component | ESP32 Pin |
|---|---|
| ADXL345 SDA | GPIO 21 |
| ADXL345 SCL | GPIO 22 |
| HC-SR04 TRIG | GPIO 26 |
| HC-SR04 ECHO | GPIO 27 |
| Rain Sensor | GPIO 34 |
| Soil Moisture Sensor | GPIO 35 |
| DHT11 Data | GPIO 4 |
| Siren Driver | GPIO 25 |
| Alert LED | GPIO 18 |

## Nodes 2 and 3 – Receiver Nodes

| Component | ESP32 Pin |
|---|---|
| Siren Driver | GPIO 25 |
| Alert LED | GPIO 18 |

## Mesh Configuration

- Mesh Name: EarlyGuardMesh
- Mesh Password: EarlyGuard123
- Mesh Port: 5555
- Communication Library: painlessMesh

## Electrical Safety

- Never connect a 5V siren directly to an ESP32 GPIO.
- Use an appropriate MOSFET or transistor driver.
- Use a suitable external 5V siren supply.
- Connect the external supply ground to ESP32 ground.
- Use a voltage divider on the HC-SR04 Echo output before connecting it to GPIO 27.
- ESP32 GPIO pins are not 5V tolerant.

**Note:** Verify all connections against the actual assembled hardware before using this document as a wiring reference.

