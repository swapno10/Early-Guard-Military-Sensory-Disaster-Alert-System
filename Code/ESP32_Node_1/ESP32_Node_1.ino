/*
  EarlyGuard - ESP32 Node 1: Sensor + Mesh + Local Warning
  Sensors: DHT11, ADXL345, rain module, soil-moisture module, HC-SR04
  Outputs: status LED and 5V siren through an external MOSFET driver

  Original mesh settings and original Node 1 sensor pin assignments preserved.
  Prototype only: calibrate thresholds; not a certified life-safety system.
*/

#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <ArduinoJson.h>
#include "painlessMesh.h"

// Original mesh configuration
#define MESH_PREFIX   "EarlyGuardMesh"
#define MESH_PASSWORD "EarlyGuard123"
#define MESH_PORT     5555

// Node 1 pins (existing sensor pins retained)
constexpr int PIN_DHT           = 4;
constexpr int PIN_RAIN_ANALOG   = 34;
constexpr int PIN_SOIL_ANALOG   = 35;
constexpr int PIN_TRIG          = 26; // retained from your uploaded Node 1 code
constexpr int PIN_ECHO          = 27; // retained; use 2k/1k voltage divider
constexpr int PIN_I2C_SDA       = 21;
constexpr int PIN_I2C_SCL       = 22;
constexpr int PIN_SIREN_DRIVER  = 25; // MOSFET gate/driver input
constexpr int PIN_ALERT_LED     = 18;

#define DHT_TYPE DHT11

// Starting values only. Calibrate against your own modules.
constexpr float EARTHQUAKE_DEVIATION_MPS2 = 3.0f;
constexpr float FLOOD_DISTANCE_CM = 15.0f;
constexpr int RAIN_WET_RAW_THRESHOLD = 1800; // assumes lower = wetter
constexpr int SOIL_WET_RAW_THRESHOLD = 1800;  // assumes lower = wetter

constexpr unsigned long SEND_INTERVAL_MS = 2000UL;
constexpr unsigned long ACCEL_SAMPLE_INTERVAL_MS = 50UL;
constexpr unsigned long ULTRASONIC_TIMEOUT_US = 30000UL;
constexpr unsigned long SIREN_ON_MS = 1200UL;
constexpr unsigned long SIREN_COOLDOWN_MS = 5000UL;
constexpr bool SIREN_ACTIVE_HIGH = true;

DHT dht(PIN_DHT, DHT_TYPE);
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
Scheduler userScheduler;
painlessMesh mesh;

bool accelerometerReady = false;
bool alarmActive = false;
unsigned long lastSendMs = 0;
unsigned long lastAccelSampleMs = 0;
unsigned long sirenOffAt = 0;
unsigned long nextSirenAt = 0;
float peakVibrationDeviation = 0.0f;
uint32_t sequenceNumber = 0;

void setSiren(bool enabled) {
  bool high = SIREN_ACTIVE_HIGH ? enabled : !enabled;
  digitalWrite(PIN_SIREN_DRIVER, high ? HIGH : LOW);
}

void triggerSirenPulse() {
  unsigned long now = millis();
  if ((long)(now - nextSirenAt) < 0) return;
  setSiren(true);
  alarmActive = true;
  sirenOffAt = now + SIREN_ON_MS;
  nextSirenAt = now + SIREN_COOLDOWN_MS;
}

void updateSiren() {
  if (alarmActive && (long)(millis() - sirenOffAt) >= 0) {
    setSiren(false);
    alarmActive = false;
  }
}

void sampleAccelerometer() {
  if (!accelerometerReady) return;
  unsigned long now = millis();
  if (now - lastAccelSampleMs < ACCEL_SAMPLE_INTERVAL_MS) return;
  lastAccelSampleMs = now;

  sensors_event_t event;
  accel.getEvent(&event);
  float magnitude = sqrtf(event.acceleration.x * event.acceleration.x +
                          event.acceleration.y * event.acceleration.y +
                          event.acceleration.z * event.acceleration.z);
  float deviation = fabsf(magnitude - 9.80665f);
  if (deviation > peakVibrationDeviation) {
    peakVibrationDeviation = deviation;
  }
}

float readWaterDistanceCm() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(3);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long duration = pulseIn(PIN_ECHO, HIGH, ULTRASONIC_TIMEOUT_US);
  if (duration == 0) return -1.0f;
  return (duration * 0.0343f) / 2.0f;
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("[MESH RX] from %u: %s\n", from, msg.c_str());
}

void sendSensorData() {
  int rainRaw = analogRead(PIN_RAIN_ANALOG);
  int soilRaw = analogRead(PIN_SOIL_ANALOG);
  float waterCm = readWaterDistanceCm();

  float temperatureC = dht.readTemperature();
  float humidityPct = dht.readHumidity();
  bool dhtValid = !isnan(temperatureC) && !isnan(humidityPct);

  bool earthquakeAlert = accelerometerReady &&
      peakVibrationDeviation >= EARTHQUAKE_DEVIATION_MPS2;
  bool floodAlert = waterCm >= 0.0f && waterCm <= FLOOD_DISTANCE_CM;
  bool rainAlert = rainRaw <= RAIN_WET_RAW_THRESHOLD;
  bool soilWet = soilRaw <= SOIL_WET_RAW_THRESHOLD;

  // Combined heavy-rain + wet-soil condition is only a landslide-risk indicator.
  bool landslideRiskIndicator = rainAlert && soilWet;
  bool alert = earthquakeAlert || floodAlert || rainAlert || landslideRiskIndicator;

  String events;
  if (earthquakeAlert) events += "VIBRATION,";
  if (floodAlert) events += "HIGH_WATER_LEVEL,";
  if (rainAlert) events += "HEAVY_RAIN,";
  if (landslideRiskIndicator) events += "LANDSLIDE_RISK_INDICATOR,";
  if (events.length() == 0) events = "NORMAL";
  else events.remove(events.length() - 1);

  digitalWrite(PIN_ALERT_LED, alert ? HIGH : LOW);
  if (alert) triggerSirenPulse();

  StaticJsonDocument<512> doc;
  doc["type"] = "EARLYGUARD";
  doc["node"] = 1;
  doc["sequence"] = ++sequenceNumber;
  doc["temperature_c"] = dhtValid ? temperatureC : -999.0;
  doc["humidity_pct"] = dhtValid ? humidityPct : -1.0;
  doc["dht_valid"] = dhtValid;
  doc["ax"] = 0.0; // peak vibration is transmitted below
  doc["vibration"] = peakVibrationDeviation;
  doc["water_cm"] = waterCm;
  doc["rain_raw"] = rainRaw;
  doc["soil_raw"] = soilRaw;
  doc["earthquake"] = earthquakeAlert;
  doc["flood"] = floodAlert;
  doc["rain"] = rainAlert;
  doc["soil"] = soilWet;
  doc["landslide_risk_indicator"] = landslideRiskIndicator;
  doc["alert"] = alert;
  doc["events"] = events;

  String message;
  serializeJson(doc, message);
  mesh.sendBroadcast(message);

  Serial.println("\n--- EarlyGuard Node 1 ---");
  Serial.printf("DHT11: %s", dhtValid ? "" : "invalid reading | ");
  if (dhtValid) Serial.printf("%.1f C, %.1f %%RH\n", temperatureC, humidityPct);
  Serial.printf("ADXL345 peak deviation: %.2f m/s^2\n", peakVibrationDeviation);
  Serial.printf("Water distance: %.2f cm | Rain ADC: %d | Soil ADC: %d\n",
                waterCm, rainRaw, soilRaw);
  Serial.printf("Alert: %s | Events: %s\n", alert ? "YES" : "NO", events.c_str());
  Serial.println("Broadcast: " + message);

  peakVibrationDeviation = 0.0f;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_RAIN_ANALOG, INPUT);
  pinMode(PIN_SOIL_ANALOG, INPUT);
  pinMode(PIN_SIREN_DRIVER, OUTPUT);
  pinMode(PIN_ALERT_LED, OUTPUT);
  setSiren(false);
  digitalWrite(PIN_ALERT_LED, LOW);

  analogReadResolution(12);
  dht.begin();

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  accelerometerReady = accel.begin(0x53);
  if (accelerometerReady) {
    accel.setRange(ADXL345_RANGE_16_G);
    accel.setDataRate(ADXL345_DATARATE_100_HZ);
    Serial.println("[SENSOR] ADXL345 detected.");
  } else {
    Serial.println("[WARNING] ADXL345 not detected. Check I2C wiring/address.");
  }

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);

  lastSendMs = millis();
  Serial.println("EarlyGuard Node 1 started.");
}

void loop() {
  mesh.update();
  sampleAccelerometer();
  updateSiren();

  if (millis() - lastSendMs >= SEND_INTERVAL_MS) {
    lastSendMs = millis();
    sendSensorData();
  }
}
