# EarlyGuard – Testing and Results

## 1. Project Testing

The EarlyGuard prototype consists of three ESP32 nodes connected through a wireless mesh network.

## 2. Testing Areas

| Test Area | Description |
|---|---|
| Sensor Monitoring | Reading vibration, rainfall, soil moisture, water distance, temperature and humidity |
| Mesh Communication | Checking wireless communication between ESP32 nodes |
| Alert Transmission | Sending sensor alert messages from Node 1 |
| Receiver Operation | Receiving and processing alerts at Nodes 2 and 3 |
| Warning Outputs | Activating local LEDs and sirens |

## 3. System Operation

- Node 1 monitors the connected sensors.
- Sensor readings are evaluated against configured thresholds.
- Node 1 broadcasts alert information through painlessMesh.
- Nodes 2 and 3 receive the messages.
- Receiver nodes activate their local LED and siren when an alert is received.

## 4. Results

The project implements a three-node ESP32 mesh-based disaster alert prototype with sensor monitoring and local warning outputs.

Detailed quantitative performance results and field validation are not included in this document.

## 5. Limitations

- Sensor thresholds require calibration.
- Extended-range communication requires further testing.
- The prototype is not a certified disaster prediction or life-safety warning system.
- Field validation is required before any real-world deployment.

## 6. Future Improvements

- Improve sensor accuracy and calibration.
- Conduct extended mesh-network testing.
- Optimize power consumption.
- Expand sensor coverage and warning nodes.

