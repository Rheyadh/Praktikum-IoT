/*
 * PRAKTIKUM 7: SIMPUL SENSOR-AKTUATOR IoT
 * ESP8266 + DHT11 + LED + MQTT Mosquitto
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// 1. Wi-Fi
const char* ssid = "Rheyyy";
const char* wifiPassword = "22229999";

// 2. MQTT Broker di laptop
const char* mqttServer = "172.20.10.2";
const int mqttPort = 1883;
const char* mqttUser = "admin";
const char* mqttPassword = "rahasia123";

// 3. Identitas dan topik
const char* clientId = "ESP8266_Local_Node01";
const char* topicPubTele = "kampus/labiot/telemetri";
const char* topicSubCmd = "kampus/labiot/kendali";

// 4. Pin perangkat
const byte dhtPin = D4;  // GPIO2
const byte ledPin = D1;  // GPIO5

#define DHTTYPE DHT22
// Jika memakai DHT22, ganti menjadi:
// #define DHTTYPE DHT22

DHT dht(dhtPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastTeleTime = 0;
const unsigned long teleInterval = 4000;

// --------------------------------------------------
// Menerima perintah JSON untuk LED melalui MQTT
// --------------------------------------------------
void onMessageCallback(char* topic, byte* payload,
                       unsigned int length) {
  StaticJsonDocument<256> doc;

  DeserializationError err =
      deserializeJson(doc, payload, length);

  if (err) {
    Serial.print("[ERROR] JSON tidak valid: ");
    Serial.println(err.c_str());
    return;
  }

  Serial.print("[MQTT] Pesan dari topik: ");
  Serial.println(topic);

  if (strcmp(topic, topicSubCmd) == 0 && doc.containsKey("led")) {
    int stateLED = doc["led"];

    if (stateLED == 1) {
      digitalWrite(ledPin, HIGH);
      Serial.println("[LED] MENYALA");
    } else if (stateLED == 0) {
      digitalWrite(ledPin, LOW);
      Serial.println("[LED] PADAM");
    } else {
      Serial.println("[ERROR] Nilai led harus 0 atau 1");
    }
  }
}

// --------------------------------------------------
// Menghubungkan ke Wi-Fi
// --------------------------------------------------
void connectToWiFi() {
  Serial.print("[Wi-Fi] Menghubungkan ke ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifiPassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[Wi-Fi] Berhasil terhubung!");
  Serial.print("[Wi-Fi] IP NodeMCU: ");
  Serial.println(WiFi.localIP());
}

// --------------------------------------------------
// Menghubungkan ke MQTT dengan autentikasi
// --------------------------------------------------
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("[MQTT] Menghubungkan ke broker ");
    Serial.print(mqttServer);
    Serial.print("... ");

    if (client.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("BERHASIL!");

      if (client.subscribe(topicSubCmd)) {
        Serial.print("[MQTT] Subscribe: ");
        Serial.println(topicSubCmd);
      } else {
        Serial.println("[MQTT] Subscribe gagal!");
      }
    } else {
      Serial.print("GAGAL, rc=");
      Serial.println(client.state());
      Serial.println("Coba lagi dalam 4 detik...");
      delay(4000);
    }
  }
}

// --------------------------------------------------
// Setup
// --------------------------------------------------
void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  dht.begin();
  connectToWiFi();

  client.setServer(mqttServer, mqttPort);
  client.setCallback(onMessageCallback);
  client.setBufferSize(512);
}

// --------------------------------------------------
// Loop utama: MQTT dan pengiriman sensor
// --------------------------------------------------
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  unsigned long now = millis();

  if (now - lastTeleTime >= teleInterval) {
    lastTeleTime = now;

    float suhu = dht.readTemperature();
    float kelembapan = dht.readHumidity();

    if (isnan(suhu) || isnan(kelembapan)) {
      Serial.println("[ERROR] Gagal membaca DHT!");
      return;
    }

    StaticJsonDocument<256> doc;

    doc["node_id"] = clientId;
    doc["suhu"] = suhu;
    doc["kelembapan"] = kelembapan;
    doc["status_led"] = digitalRead(ledPin);

    char buffer[256];
    serializeJson(doc, buffer, sizeof(buffer));

    if (client.publish(topicPubTele, buffer)) {
      Serial.print("[TELEMETRI TERKIRIM] ");
      Serial.println(buffer);
    } else {
      Serial.println("[ERROR] Gagal publish telemetri!");
    }
  }
}