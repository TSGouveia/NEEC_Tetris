#include <PS4Controller.h>

#define BUTTON_NUMBER 11
#define DEBUG_LED_PIN 2

// Variável para armazenar o estado anterior de cada botão
bool prevButtonState[BUTTON_NUMBER] = { 0 };

void setup() {
  Serial.begin(115200);  // Inicializa a comunicação serial para o computador
  PS4.begin("A4:CA:A0:1C:92:85"); // Substitua pelo MAC Address do control e PS4

  pinMode(DEBUG_LED_PIN, OUTPUT);
  digitalWrite(DEBUG_LED_PIN, HIGH); // LED de depuração aceso inicialmente
}

void loop() {
  if (PS4.isConnected()) {
    digitalWrite(DEBUG_LED_PIN, LOW); // LED apagado quando conectado

    if (PS4.PSButton() && PS4.Options()) {
      ESP.restart(); // Se PS e Options pressionados, reinicia o ESP32
    }

    // Verifica o estado de cada botão
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

    // Envia os dados dos botões via UART
    for (int i = 0; i < BUTTON_NUMBER; i++) {
      if (buttonState[i] && !prevButtonState[i]) {
        // Botão foi pressionado
        sendUART(i + 1);  // Envia o valor positivo
      } else if (!buttonState[i] && prevButtonState[i]) {
        // Botão foi solto
        sendUART(-(i + 1)); // Envia o valor negativo
      }
      // Atualiza o estado anterior do botão
      prevButtonState[i] = buttonState[i];
    }

    delay(10); // Delay para não sobrecarregar a UART
  }
}

// Função para enviar dados via UART (Serial)
void sendUART(int data) {
  Serial.println(data);  // Envia o dado via UART (pinos 1 e 3 do ESP32)
}
