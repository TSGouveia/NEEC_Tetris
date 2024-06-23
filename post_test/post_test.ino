#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define MQTT_BUFFER_SIZE 8192
#define LED 2

const char* ssid = "Pastel di nata";
const char* password = "OsMaisBilhas";
const char* mqtt_server = "192.168.2.72";  // Replace with your MQTT server IP address
const char* mqtt_topic = "video/rgb";

WiFiClient espClient;
PubSubClient client(espClient);

bool firstMessageReceived = false;
unsigned long firstMessageTime;
int minutos = 3;
int segundos = 39;
int fator_caganso = 5;
unsigned long printMessageTime = minutos * 60000 + (segundos + fator_caganso) * 1000;  // 3 minutes and 39 seconds in milliseconds

int count = 0;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (!firstMessageReceived) {
    Serial.println("Recebi a primeira mensagem");
    digitalWrite(LED, LOW);
    firstMessageReceived = true;
    firstMessageTime = millis();
  }

  count++;
  // Other processing of the MQTT message if needed
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  setup_wifi();

  // Adjust MQTT client internal buffer size
  client.setBufferSize(MQTT_BUFFER_SIZE);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  digitalWrite(LED, HIGH);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (firstMessageReceived) {
    unsigned long currentTime = millis();
    if (currentTime - firstMessageTime >= printMessageTime) {
      Serial.println("Passou o tempo");
      Serial.println(count);
      digitalWrite(LED, HIGH);
      firstMessageReceived = false;
    }
  }
}
