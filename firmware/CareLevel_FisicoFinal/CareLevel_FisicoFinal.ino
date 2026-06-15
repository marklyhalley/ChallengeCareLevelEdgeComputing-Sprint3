#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <DHT.h>
#include "MAX30100_PulseOximeter.h"

// ================= WIFI =================
const char* WIFI_SSID     = "FIAP-IOT";
const char* WIFI_PASSWORD = "F!@p25.IOT";

// ================= MQTT =================
const char* MQTT_BROKER   = "2f9aceebe7a246b1b1bb52306aa12c22.s1.eu.hivemq.cloud";
const int   MQTT_PORT     = 8883;
const char* MQTT_USER     = "carelevel";
const char* MQTT_PASSWORD = "Abcd1234";

// ================= TOPICOS =================
const char* TOPIC_BPM         = "carelevel/saude/bpm";
const char* TOPIC_SPO2        = "carelevel/saude/spo2";
const char* TOPIC_TEMPERATURA = "carelevel/saude/temperatura";
const char* TOPIC_TEMP_VALIDA = "carelevel/saude/temp_valida";

// ================= DHT11 =================
#define DHT_PIN 5
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// ================= MAX30100 =================
PulseOximeter pox;

float bpm = 0;
float spo2 = 0;

#define REPORTING_PERIOD_MS 1000
uint32_t tsLastReport = 0;

// ================= MQTT =================
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_MS = 5000;

// ================= CALLBACK =================
void onBeatDetected() {
  Serial.println("Batimento detectado!");
}

// ================= WIFI =================
void setup_wifi() {
  Serial.print("Conectando WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado!");
  Serial.println(WiFi.localIP());
}

// ================= MQTT =================
void reconnect() {

  while (!client.connected()) {

    Serial.print("Conectando MQTT...");

    String clientId = "ESP32-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(),
                       MQTT_USER,
                       MQTT_PASSWORD)) {

      Serial.println("Conectado!");

    } else {

      Serial.print("Falhou. RC=");
      Serial.println(client.state());

      delay(5000);
    }
  }
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);

  dht.begin();

  Wire.begin(21, 22);

  Serial.println("Inicializando MAX30100...");

  if (!pox.begin()) {

    Serial.println("MAX30100 NÃO ENCONTRADO!");

    while (1);
  }

  Serial.println("MAX30100 OK");

  pox.setOnBeatDetectedCallback(onBeatDetected);

  setup_wifi();

  espClient.setInsecure();

  client.setServer(MQTT_BROKER, MQTT_PORT);
}

// ================= LOOP =================
void loop() {

  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  pox.update();

  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {

    bpm = pox.getHeartRate();
    spo2 = pox.getSpO2();

    tsLastReport = millis();
  }

  if (millis() - ultimoEnvio < INTERVALO_MS) {
    return;
  }

  ultimoEnvio = millis();

  float temperatura = dht.readTemperature();

  bool tempValida = !isnan(temperatura);

  if (!tempValida) {
    temperatura = 36.5;
  }

  char buffer[64];

  snprintf(buffer,
           sizeof(buffer),
           "{\"bpm\":%.1f}",
           bpm);

  client.publish(TOPIC_BPM, buffer);

  snprintf(buffer,
           sizeof(buffer),
           "{\"spo2\":%.1f}",
           spo2);

  client.publish(TOPIC_SPO2, buffer);

  snprintf(buffer,
           sizeof(buffer),
           "{\"temperatura\":%.2f}",
           temperatura);

  client.publish(TOPIC_TEMPERATURA, buffer);

  snprintf(buffer,
           sizeof(buffer),
           "{\"temp_valida\":%s}",
           tempValida ? "true" : "false");

  client.publish(TOPIC_TEMP_VALIDA, buffer);

  Serial.println("================================");

  Serial.print("BPM: ");
  Serial.println(bpm);

  Serial.print("SpO2: ");
  Serial.println(spo2);

  Serial.print("Temperatura: ");
  Serial.println(temperatura);

  Serial.println("================================");
}