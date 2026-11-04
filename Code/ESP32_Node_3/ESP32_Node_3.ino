/*
  EarlyGuard - ESP32 Node 3: Siren Alert Node

  Receives EarlyGuard JSON broadcasts and activates a siren driver when
  the incoming "alert" field is true.

  IMPORTANT:
  - Do NOT power a siren directly from an ESP32 GPIO.
  - Use a suitable transistor/MOSFET or relay driver and separate supply.
  - Add a flyback diode for a suitable inductive DC load.
  - Confirm your driver input polarity before connecting the siren.
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include "painlessMesh.h"

#define MESH_PREFIX   "EarlyGuardMesh"
#define MESH_PASSWORD "EarlyGuard123"
#define MESH_PORT     5555

Scheduler userScheduler;
painlessMesh mesh;

constexpr int PIN_SIREN_DRIVER = 25; // Proposed GPIO; change to match wiring
constexpr bool SIREN_ACTIVE_HIGH = true;
constexpr unsigned long MESSAGE_TIMEOUT_MS = 10000UL;

bool alarmActive = false;
unsigned long lastValidMessageMs = 0;

void setSiren(bool enabled) {
  alarmActive = enabled;
  bool outputHigh = SIREN_ACTIVE_HIGH ? enabled : !enabled;
  digitalWrite(PIN_SIREN_DRIVER, outputHigh ? HIGH : LOW);
}

void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (error) {
    Serial.printf("[JSON] Invalid message from %u: %s\n",
                  from, error.c_str());
    return;
  }

  const char *type = doc["type"] | "";
  if (strcmp(type, "EARLYGUARD") != 0) return;

  lastValidMessageMs = millis();

  bool alert = doc["alert"] | false;
  bool earthquake = doc["earthquake"] | false;
  bool flood = doc["flood"] | false;
  bool rain = doc["rain"] | false;
  bool soil = doc["soil"] | false;

  Serial.printf("\n[ALERT NODE] Message from %u\n", from);
  Serial.printf("Earthquake=%d Flood=%d Rain=%d Soil=%d\n",
                earthquake, flood, rain, soil);
  Serial.printf("Overall alert: %s\n", alert ? "ACTIVE" : "CLEAR");

  setSiren(alert);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[MESH] New connection: %u\n", nodeId);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PIN_SIREN_DRIVER, OUTPUT);
  setSiren(false);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);

  Serial.println("EarlyGuard Node 3 - Siren alert node started.");
}

void loop() {
  mesh.update();

  // Fail-safe behavior: stop the siren if sensor broadcasts disappear.
  // This timeout is not a substitute for a real communications watchdog.
  if (alarmActive &&
      lastValidMessageMs != 0 &&
      millis() - lastValidMessageMs > MESSAGE_TIMEOUT_MS) {
    Serial.println("[WARNING] Sensor messages timed out; siren switched OFF.");
    setSiren(false);
  }
}
