#include <Wire.h>
#include <PS4Controller.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);
  //MAC ADDRESS DO MOVEL DO GOUVEIA
  PS4.begin("B0:4A:6A:56:FD:B5");
  Serial.println("Ready.");
}

void loop() {
  // Below has all accessible outputs from the controller
  if (PS4.isConnected()) {
    if (PS4.Right()) Wire.write("Right Button");
    if (PS4.Down()) Wire.write("Down Button");
    if (PS4.Up()) Wire.write("Up Button");
    if (PS4.Left()) Wire.write("Left Button");

    if (PS4.Square()) Wire.write("Square Button");
    if (PS4.Cross()) Wire.write("Cross Button");
    if (PS4.Circle()) Wire.write("Circle Button");
    if (PS4.Triangle()) Wire.write("Triangle Button");

    if (PS4.UpRight()) Wire.write("Up Right");
    if (PS4.DownRight()) Wire.write("Down Right");
    if (PS4.UpLeft()) Wire.write("Up Left");
    if (PS4.DownLeft()) Wire.write("Down Left");

    if (PS4.L1()) Wire.write("L1 Button");
    if (PS4.R1()) Wire.write("R1 Button");

    if (PS4.Share()) Wire.write("Share Button");
    if (PS4.Options()) Wire.write("Options Button");
    if (PS4.L3()) Wire.write("L3 Button");
    if (PS4.R3()) Wire.write("R3 Button");

    if (PS4.PSButton()) Wire.write("PS Button");
    if (PS4.Touchpad()) Wire.write("Touch Pad Button");

    if (PS4.L2()) {
      Serial.printf("L2 button at %d\n", PS4.L2Value());
    }
    if (PS4.R2()) {
      Serial.printf("R2 button at %d\n", PS4.R2Value());
    }

    if (PS4.LStickX()) {
      Serial.printf("Left Stick x at %d\n", PS4.LStickX());
    }
    if (PS4.LStickY()) {
      Serial.printf("Left Stick y at %d\n", PS4.LStickY());
    }
    if (PS4.RStickX()) {
      Serial.printf("Right Stick x at %d\n", PS4.RStickX());
    }
    if (PS4.RStickY()) {
      Serial.printf("Right Stick y at %d\n", PS4.RStickY());
    }

    if (PS4.Charging())  Serial.println("The controller is charging");
    if (PS4.Audio())  Serial.println("The controller has headphones attached");
    if (PS4.Mic())  Serial.println("The controller has a mic attached");

    Serial.printf("Battery Level : %d\n", PS4.Battery());

    Serial.println();
    // This delay is to make the output more human readable
    // Remove it when you're not trying to see the output
    delay(1000);
  }
}
