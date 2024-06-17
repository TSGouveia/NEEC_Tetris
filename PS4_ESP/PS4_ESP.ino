#include <Wire.h>
#include <PS4Controller.h>

#define BUTTON_NUMBER 11
#define DEBUG_LED_PIN 2

// Variable to store the previous state of each button
bool prevButtonState[BUTTON_NUMBER] = { 0 };

void setup() {
  Wire.begin();

  // MAC ADDRESS DE UM MOVEL AI
  PS4.begin("22:22:D9:D5:19:E3");
  pinMode(DEBUG_LED_PIN, OUTPUT);
  digitalWrite(DEBUG_LED_PIN, HIGH);
}

void loop() {
  // Below has all accessible outputs from the controller
  if (PS4.isConnected()) {
    digitalWrite(DEBUG_LED_PIN, LOW);
    if (PS4.PSButton() && PS4.Options()) {
      ESP.restart();
    }
    // Check each button state
    bool buttonState[BUTTON_NUMBER] = {
      PS4.Right(),
      PS4.Down(),
      PS4.Up(),
      PS4.Left(),
      PS4.Cross(),
      PS4.Circle(),
      PS4.L1(),
      PS4.R1(),
      PS4.Share(),
      PS4.Options(),
      PS4.PSButton(),
    };

    // Iterate through each button
    for (int i = 0; i < BUTTON_NUMBER; i++) {
      // Check if the button is currently pressed and was not pressed previously
      //BUTTON WAS PRESSED
      if (buttonState[i] && !prevButtonState[i]) {
        // Send data corresponding to the button

        Wire.beginTransmission(8);
        Wire.write(i + 1);
        Wire.endTransmission();
      }
      //BUTTON WAS RELEASED
      if (!buttonState[i] && prevButtonState[i]) {
        // Send data corresponding to the button
        Wire.beginTransmission(8);
        Wire.write(-(i + 1));
        Wire.endTransmission();
      }
      // Update the previous state of the button
      prevButtonState[i] = buttonState[i];
    }

    // Add a small delay to prevent spamming I2C
    delay(10);
  }
}

// ******************************************************
// LEADERBOARD
// ******************************************************

void SetupI2C(){
  Wire.begin(9);                 // join i2c bus with address #8
  Wire.onReceive(receiveEvent);  // register event
}

void receiveEvent(int numBytes) {
  // Tamanho máximo do JSON (ajuste conforme necessário)
  const size_t capacity = JSON_OBJECT_SIZE(2) + 20;
  StaticJsonDocument<capacity> doc;

  // Lê os dados do buffer I2C
  while (Wire.available()) {
    char c = Wire.read();
    // Adiciona o caractere ao JSON
    deserializeJson(doc, c);
  }

  // Extrai os valores do JSON
  const char* name = doc["name"];
  int score = doc["score"];

  // Chama a função SendValueToScoreboard com os valores
  SendValueToScoreboard(name, score);
}

void ConnectToWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(SSID, PASS);
      Serial.println("Connecting to WiFi");
      while (WiFi.status() != WL_CONNECTED) {
        delay(100);
      }
    }
    Serial.println("Connected");
    digitalWrite(DEBUG_LED_PIN, HIGH);
  }
}

String CriaJson(char* playerName, int score) {
  // Tamanho do JSON baseado nos campos fornecidos
  const size_t capacity = JSON_OBJECT_SIZE(8);

  // Criação do objeto JSON
  DynamicJsonDocument doc(capacity);

  // Preenchendo o objeto JSON com os dados
  doc["name"] = playerName;
  doc["score"] = score;
  doc["is_eliminated"] = false;
  doc["goal"] = nullptr;              // Valor null
  doc["text_color"] = nullptr;        // Valor null
  doc["background_color"] = nullptr;  // Valor null
  doc["profile_image"] = nullptr;     // Valor null
  doc["team"] = nullptr;              // Valor null

  // Serializa o objeto JSON para uma string
  String jsonStr;
  serializeJson(doc, jsonStr);

  return jsonStr;
}

void SendPostRequest(String json) {
  HTTPClient http;

  // Configura o URL e o endpoint do servidor
  http.begin(SCOREBOARD_URL);

  // Configura cabeçalho do conteúdo JSON
  http.addHeader("Content-Type", "application/json");
  http.addHeader("accept", "/*");  // Configuração opcional dependendo da API

  Serial.println("A enviar score...");
  // Envie o POST e aguarde a resposta
  int httpResponseCode = http.POST(json);

  // Verifique o código de resposta
  if (httpResponseCode > 0) {
    String resposta = http.getString();
    Serial.println("Score enviado com sucesso");
  } else {
    Serial.print("Erro a enviar score");
    Serial.println(httpResponseCode);
  }

  // Libere os recursos
  http.end();
}

void SendValueToScoreboard(char* name, int score) {
  ConnectToWifi();
  String json = CriaJson(name, score);
  SendPostRequest(json);
  WiFi.disconnect();
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(DEBUG_LED_PIN, LOW);
  }
}

