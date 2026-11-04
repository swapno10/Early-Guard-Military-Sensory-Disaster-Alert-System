/*
  EarlyGuard - ESP32 Node 1: Sensor Node
  Hardware: ADXL345, HC-SR04, rain sensor, soil moisture sensor
  Network: painlessMesh

  IMPORTANT:
  - Pin assignments below are proposed defaults. Match them to your wiring.
  - Calibrate all sensor thresholds for your actual modules and environment.
  - This is an educational prototype, NOT a certified disaster warning device.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include "painlessMesh.h"

// ---------- Mesh configuration ----------
#define MESH_PREFIX   "EarlyGuardMesh"
#define MESH_PASSWORD "EarlyGuard123"
#define MESH_PORT     5555

Scheduler userScheduler;
painlessMesh mesh;

// ---------- Proposed ESP32 pin assignments ----------
constexpr int PIN_RAIN_ANALOG = 34;  // ADC1 input
constexpr int PIN_SOIL_ANALOG = 35;  // ADC1 input
constexpr int PIN_TRIG        = 26;
constexpr int PIN_ECHO        = 27;  // MUST receive 3.3V max; use voltage divider
constexpr int PIN_I2C_SDA     = 21;
constexpr int PIN_I2C_SCL     = 22;

// ---------- Calibration settings: adjust after testing ----------
constexpr float EARTHQUAKE_DEVIATION_MPS2 = 3.0f;
constexpr float FLOOD_DISTANCE_CM         = 15.0f;
// Many common analog rain/soil modules give LOWER ADC readings when wet.
// Verify this behavior on your modules before relying on these thresholds.
constexpr int RAIN_WET_RAW_THRESHOLD      = 1800;
constexpr int SOIL_WET_RAW_THRESHOLD      = 1800;

constexpr unsigned long SEND_INTERVAL_MS = 2000;
constexpr unsigned long ULTRASONIC_TIMEOUT_US = 30000UL;

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
bool accelerometerReady = false;
unsigned long lastSendMs = 0;

float readWaterDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(3);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  unsigned long duration = pulseIn(PIN_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
  if (duration == 0) return -1.0f; // no echo / out of range
  return (duration * 0.0343f) / 2.0f;
}

void setupMesh() {
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onNewConnection([](uint32_t nodeId) {
    Serial.printf("[MESH] New connection: %u\n", nodeId);
  });
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_RAIN_ANALOG, INPUT);
  pinMode(PIN_SOIL_ANALOG, INPUT);

  analogReadResolution(12); // ESP32 ADC range: 0-4095

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  accelerometerReady = accel.begin(0x53);
  if (accelerometerReady) {
    accel.setRange(ADXL345_RANGE_16_G);
    Serial.println("[SENSOR] ADXL345 detected.");
  } else {
    Serial.println("[WARNING] ADXL345 not detected. Check I2C wiring/address.");
  }

  setupMesh();
  Serial.println("EarlyGuard Node 1 - Sensor node started.");
}

void loop() {
  mesh.update();

  if (millis() - lastSendMs < SEND_INTERVAL_MS) return;
  lastSendMs = millis();

  float ax = 0, ay = 0, az = 0;
  float vibrationDeviation = -1.0f;

  if (accelerometerReady) {
    sensors_event_t event;
    accel.getEvent(&event);
    ax = event.acceleration.x;
    ay = event.acceleration.y;
    az = event.acceleration.z;

    float magnitude = sqrtf(ax * ax + ay * ay + az * az);
    vibrationDeviation = fabsf(magnitude - 9.81f);
  }

  float waterCm = readWaterDistanceCm();
  int rainRaw = analogRead(PIN_RAIN_ANALOG);
  int soilRaw = analogRead(PIN_SOIL_ANALOG);

  // Prototype threshold flags. Validate and calibrate before any field use.
  bool earthquakeAlert =
      accelerometerReady && vibrationDeviation >= EARTHQUAKE_DEVIATION_MPS2;
  bool floodAlert =
      waterCm >= 0.0f && waterCm <= FLOOD_DISTANCE_CM;
  bool rainAlert = rainRaw <= RAIN_WET_RAW_THRESHOLD;
  bool soilAlert = soilRaw <= SOIL_WET_RAW_THRESHOLD;

  bool alert = earthquakeAlert || floodAlert || rainAlert || soilAlert;

  char message[420];
  snprintf(message, sizeof(message),
    "{\"type\":\"EARLYGUARD\",\"node\":1,"
    "\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,"
    "\"vibration\":%.2f,\"water_cm\":%.2f,"
    "\"rain_raw\":%d,\"soil_raw\":%d,"
    "\"earthquake\":%s,\"flood\":%s,\"rain\":%s,"
    "\"soil\":%s,\"alert\":%s}",
    ax, ay, az, vibrationDeviation, waterCm, rainRaw, soilRaw,
    earthquakeAlert ? "true" : "false",
    floodAlert ? "true" : "false",
    rainAlert ? "true" : "false",
    soilAlert ? "true" : "false",
    alert ? "true" : "false"
  );

  mesh.sendBroadcast(String(message));

  Serial.println("\n--- EarlyGuard Sensor Readings ---");
  Serial.printf("Acceleration (m/s^2): X=%.2f Y=%.2f Z=%.2f\n", ax, ay, az);
  Serial.printf("Vibration deviation: %.2f m/s^2\n", vibrationDeviation);
  Serial.printf("Water distance: %.2f cm\n", waterCm);
  Serial.printf("Rain ADC: %d | Soil ADC: %d\n", rainRaw, soilRaw);
  Serial.printf("Flags: earthquake=%d flood=%d rain=%d soil=%d\n",
                earthquakeAlert, floodAlert, rainAlert, soilAlert);
  Serial.printf("Broadcast alert: %s\n", alert ? "YES" : "NO");
}
