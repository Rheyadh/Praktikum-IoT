#include <painlessMesh.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "LabIoTMesh"
#define MESH_PASSWORD "iotmeshpassword"
#define MESH_PORT     5555

const char* nodeName = "Node-2 LDR-Wheww";
const int ldrPin = A0; // Pin ADC A0 NodeMCU

Scheduler userScheduler;
painlessMesh mesh;

void sendMessage();
Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, &sendMessage);

void sendMessage() {
  int nilaiAdc = analogRead(ldrPin); // Membaca nilai cahaya (0 - 1023)

  StaticJsonDocument<200> doc;
  doc["tipe"] = "cahaya_node";
  doc["adc"] = nilaiAdc;

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);

  Serial.print("[KIRIM NODE 2] ADC Cahaya: ");
  Serial.println(nilaiAdc);
}

void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (!error) {
    String tipe = doc["tipe"];
    if (tipe == "suhu_node") {
      float suhu = doc["suhu"];
      float kelembapan = doc["kelembapan"];
      Serial.printf("[TERIMA DARI NODE 1] Suhu: %.2f °C | Kelembapan: %.2f %%\n", suhu, kelembapan);
    }
  }
}

void newConnectionCallback(uint32_t nodeId) {}
void changedConnectionCallback() {}

void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  mesh.update();
}