/*
  EarlyGuard - ESP32 Node 3: Mesh Receiver + Local Warning
  painlessMesh automatically routes mesh messages; this node does not
  manually rebroadcast received broadcasts.

  Outputs: LED and 5V siren through an external MOSFET driver.
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include "painlessMesh.h"

#define MESH_PREFIX   "EarlyGuardMesh"
#define MESH_PASSWORD "EarlyGuard123"
#define MESH_PORT     5555

constexpr int PIN_SIREN_DRIVER = 25; // MOSFET gate/driver input
constexpr int PIN_ALERT_LED = 18;
constexpr bool SIREN_ACTIVE_HIGH = true;

constexpr unsigned long SIREN_ON_MS = 1200UL;
constexpr unsigned long SIREN_COOLDOWN_MS = 5000UL;
constexpr unsigned long LED_HOLD_MS = 3000UL;

Scheduler userScheduler;
painlessMesh mesh;

bool sirenOn = false;
unsigned long sirenOffAt = 0;
unsigned long nextSirenAt = 0;
unsigned long ledOffAt = 0;
uint32_t lastSourceNode = 0;
uint32_t lastSequence = 0;
bool haveLastSequence = false;

void setSiren(bool enabled) {
  bool high = SIREN_ACTIVE_HIGH ? enabled : !enabled;
  digitalWrite(PIN_SIREN_DRIVER, high ? HIGH : LOW);
}

void triggerSirenPulse() {
  unsigned long now = millis();
  if ((long)(now - nextSirenAt) < 0) return;
  setSiren(true);
  sirenOn = true;
  sirenOffAt = now + SIREN_ON_MS;
  nextSirenAt = now + SIREN_COOLDOWN_MS;
}

void updateOutputs() {
  unsigned long now = millis();
  if (sirenOn && (long)(now - sirenOffAt) >= 0) {
    setSiren(false);
    sirenOn = false;
  }
  if (ledOffAt != 0 && (long)(now - ledOffAt) >= 0) {
    digitalWrite(PIN_ALERT_LED, LOW);
    ledOffAt = 0;
  }
}

void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, msg);
  if (error) {
    Serial.printf("[JSON] Invalid message from %u: %s\n", from, error.c_str());
    return;
  }

  const char *type = doc["type"] | "";
  if (strcmp(type, "EARLYGUARD") != 0) return;

  uint32_t sourceNode = doc["node"] | from;
  uint32_t sequence = doc["sequence"] | 0;

  // Suppress duplicate packets from the same originating sensor node.
  if (haveLastSequence && sourceNode == lastSourceNode &&
      sequence == lastSequence) return;
  haveLastSequence = true;
  lastSourceNode = sourceNode;
  lastSequence = sequence;

  bool alert = doc["alert"] | false;
  bool earthquake = doc["earthquake"] | false;
  bool flood = doc["flood"] | false;
  bool rain = doc["rain"] | false;
  bool soil = doc["soil"] | false;
  bool landslide = doc["landslide_risk_indicator"] | false;
  const char *events = doc["events"] | "unspecified";

  Serial.printf("\n[Node 3] Alert from node %u, seq %u: %s\n",
                sourceNode, sequence, alert ? "ACTIVE" : "CLEAR");
  Serial.printf("Earthquake=%d Flood=%d Rain=%d SoilWet=%d LandslideRisk=%d\n",
                earthquake, flood, rain, soil, landslide);
  Serial.printf("Events: %s\n", events);

  if (alert) {
    digitalWrite(PIN_ALERT_LED, HIGH);
    ledOffAt = millis() + LED_HOLD_MS;
    triggerSirenPulse();
  } else {
    digitalWrite(PIN_ALERT_LED, LOW);
    ledOffAt = 0;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_SIREN_DRIVER, OUTPUT);
  pinMode(PIN_ALERT_LED, OUTPUT);
  setSiren(false);
  digitalWrite(PIN_ALERT_LED, LOW);

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);

  Serial.println("EarlyGuard Node 3 receiver started.");
  Serial.println("Waiting for Node 1 sensor broadcasts...");
}

void loop() {
  mesh.update();
  updateOutputs();
}
