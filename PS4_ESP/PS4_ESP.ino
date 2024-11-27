#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <Wire.h>
#include <PS4Controller.h>

#define BUTTON_NUMBER 11
#define DEBUG_LED_PIN 2
#define ESP_ADDRESS 9
#define ARDUINO_ADDRESS 8

// Variable to store the previous state of each button
bool prevButtonState[BUTTON_NUMBER] = { 0 };

void setup() {
  Wire.begin();
  SetupI2C();
  // MAC ADDRESS DE UM MOVEL AI
  PS4.begin("A4:CA:A0:1C:92:85");
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

        Wire.beginTransmission(ARDUINO_ADDRESS);
        Wire.write(i + 1);
        Wire.endTransmission();
      }
      //BUTTON WAS RELEASED
      if (!buttonState[i] && prevButtonState[i]) {
        // Send data corresponding to the button
        Wire.beginTransmission(ARDUINO_ADDRESS);
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

void SetupI2C() {
  Wire.begin(ESP_ADDRESS);                  
  Wire.onReceive(receiveEvent);  // register event
}

void receiveEvent(int numBytes) {
}