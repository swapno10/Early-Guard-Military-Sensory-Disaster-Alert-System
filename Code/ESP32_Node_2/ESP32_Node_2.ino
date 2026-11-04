/*
  EarlyGuard - ESP32 Node 2: Mesh Relay / Network Monitor

  painlessMesh automatically routes mesh messages. This node joins the
  same mesh and monitors received broadcasts; it does not manually rebroadcast
  them, which would create unnecessary duplicate traffic.

  No external sensor is assigned to this node in this proposed architecture.
*/

#include <Arduino.h>
#include "painlessMesh.h"

#define MESH_PREFIX   "EarlyGuardMesh"
#define MESH_PASSWORD "EarlyGuard123"
#define MESH_PORT     5555

Scheduler userScheduler;
painlessMesh mesh;

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("\n[MESH] Message received from node %u:\n", from);
  Serial.println(msg);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[MESH] New connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("[MESH] Connection topology changed. Nodes: %d\n",
                mesh.getNodeList().size());
}

void setup() {
  Serial.begin(115200);
  delay(500);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  Serial.println("EarlyGuard Node 2 - Mesh relay/monitor started.");
  Serial.println("painlessMesh handles routing between connected nodes.");
}

void loop() {
  mesh.update();
}
