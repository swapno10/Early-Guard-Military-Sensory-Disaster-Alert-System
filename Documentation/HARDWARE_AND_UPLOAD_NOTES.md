# EarlyGuard – Hardware and Implementation Notes

## 1. Project Overview

EarlyGuard is a community-driven disaster early warning prototype designed to detect potential environmental hazards and distribute alerts through an ESP32-based wireless mesh network.

The system is intended for areas where reliable internet connectivity may be unavailable.

## 2. Hardware Components

| Component | Quantity | Purpose |
|---|---:|---|
| ESP32 development board | 3 | Mesh communication and processing |
| ADXL345 accelerometer | 1 | Vibration monitoring |
| HC-SR04 ultrasonic sensor | 1 | Water-level distance measurement |
| Rain sensor | 1 | Rainfall detection |
| Soil moisture sensor | 1 | Soil moisture monitoring |
| DHT11 sensor | 1 | Temperature and humidity monitoring |
| LED | 3 | Local visual warning |
| 5V siren | 3 | Local audible warning |
| MOSFET driver circuit | 3 | Siren switching |

## 3. ESP32 Node Organization

- **Node 1:** Sensor monitoring, alert evaluation and mesh broadcasting.
- **Node 2:** Mesh receiver and local warning activation.
- **Node 3:** Mesh receiver and local warning activation.

## 4. Mesh Network Configuration

The firmware uses the following configuration:

- Mesh name: `EarlyGuardMesh`
- Mesh password: `EarlyGuard123`
- Mesh port: `5555`
- Communication library: painlessMesh
- Message format: JSON using ArduinoJson

## 5. Firmware Structure

```text
Code/
├── ESP32_Node_1/
│   └── ESP32_Node_1.ino
├── ESP32_Node_2/
│   └── ESP32_Node_2.ino
└── ESP32_Node_3/
    └── ESP32_Node_3.ino
```

## 6. Important Hardware Safety Notes

- Do not connect a 5V siren directly to an ESP32 GPIO pin.
- Use an appropriate external MOSFET or transistor driver circuit.
- Use a suitable external 5V supply for the sirens.
- Connect the external supply ground and ESP32 ground together.
- The HC-SR04 Echo output is 5V. Use a voltage divider before connecting it to an ESP32 GPIO.
- ESP32 GPIO pins are not 5V tolerant.

## 7. Prototype Limitations

EarlyGuard is an experimental prototype, not a certified life-safety warning system.

The detection thresholds require calibration and field testing. Sensor readings and alert conditions should not be treated as confirmed disaster predictions.

The landslide-related output is a risk indicator based on sensor conditions, not a prediction of an actual landslide.

## 8. Future Improvements

- Field calibration and validation of sensor thresholds.
- Improved alert prioritization and reliability.
- Extended mesh-network testing.
- Power optimization for remote deployment.
- Additional sensor nodes and community warning coverage.
- Enclosure design and environmental protection.

