/* 
Dependencies:
  ArduinoJson by Benoit Blanchon [7.2.1]
  FastLED by Daniel Garcia [3.7.0]
  HttpClient by Adrian McEwen [2.2.0]
  LEDMatrix by Aaron Liddiment (https://github.com/AaronLiddiment/LEDMatrix)
*/
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "piece_data.h"
#include "image_matrices.h"
#include <ArduinoJson.hpp>
#include <HttpClient.h>
#include <WiFi.h>
#include <Wire.h>
//LEDS
#include <FastLED.h>
#include <LEDMatrix.h>

// Change the next 6 defines to match your matrix type and size
//LEDS
#define LED_PIN 10
#define COLOR_ORDER RGB
#define CHIPSET WS2812B

#define MATRIX_WIDTH 32                       // Set this negative if physical led 0 is opposite to where you want logical 0
#define MATRIX_HEIGHT 18                      // Set this negative if physical led 0 is opposite to where you want logical 0
#define MATRIX_TYPE HORIZONTAL_ZIGZAG_MATRIX  // See top of LEDMatrix.h for matrix wiring types

#define HEIGTH 20 + 2  //+2 para n foder a rotação do I piece no inicio
#define WIDTH 10
#define FRAMES_PER_SECOND 60

#define SINGLE 40
#define DOUBLE 100
#define TRIPLE 300
#define TETRIS 1200

#define DEBUG_LED_PIN 2
#define BUTTON_NUMBER 11

//COLORS
#define CYAN_I 0x00FFFF
#define YELLOW_O 0xFFFF00
#define PURPLE_T 0x800080
#define GREEN_S 0x00FF00
#define RED_Z 0xFF0000
#define BLUE_J 0x0000FF
#define ORANGE_L 0xFF7F00

//SERIAL
#define RX_PIN 0  // Pino RX do Arduino
#define TX_PIN 1  // Pino TX do Arduino

SoftwareSerial PS4Serial(RX_PIN, TX_PIN);  // Cria um objeto SoftwareSerial

//WIFI
#define SSID "NEEC_2G"
#define PASS "neecfct!"

#define SCOREBOARD_URL "https://keepthescore.com/api/zfhvdblbcjlme/player/"

bool prevButtonState[BUTTON_NUMBER] = { 0 };

const unsigned long MICROS_PER_FRAME = 1000000.0 / FRAMES_PER_SECOND;
const int DROP_RATE_LEVELS[15] = { 48, 43, 38, 33, 28, 23, 18, 13, 8, 6, 5, 4, 3, 2, 1 };

unsigned char tetrisBoard[HEIGTH][WIDTH] = { 0 };
unsigned long frameTimer;

//Piece visuals
char pieceLetter;
int currentPieceShape[4][2];
int currentPositionX = 5;
int currentPositionY = 3;

int currentRotation = 0;

//Controller logic
bool downIsPressed = false;
bool leftIsPressed = false;
bool rightIsPressed = false;

bool gameIsPaused = false;

//Movement logic
int dasCounter = 0;
int gravityCounter = 0;

int lockMaxFrames = 15;
int lockCounter = 0;

//Level logic
int level = 0;
int linesCleared = 0;
int linesToLevelUp = 10;

char holdPiece = 0;
bool canHoldPiece = true;

unsigned int score = 0;

unsigned long debugTimer = 0;

void (*resetFunc)(void) = 0;

//MESA DE LEDS
cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;

void SetupSerial() {
  PS4Serial.begin(115200);  // Inicia a comunicação serial com o ESP32
  //Serial.begin(115200);     // Inicia a comunicação serial com o monitor serial para depuração
}

void SetupFastLeds() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds[0], 576);
  FastLED.addLeds<CHIPSET, LED_PIN, GRB>(leds[0], 13);

  FastLED.setBrightness(127);
  FastLED.clear(true);
}

void ResetLedMatrix() {
  for (int x = 0; x < MATRIX_WIDTH; ++x) {
    for (int y = 0; y < MATRIX_HEIGHT; ++y) {
      leds(x, y) = 0;
    }
  }
}

void receiveEvent(int howMany) {
  while (Wire.available()) {
    int c = Wire.read();  // receive byte as a int
    handleButtonPressed(c);
  }
}

void FramePassed() {
  //Check sideways movement
  if (rightIsPressed || leftIsPressed)
    dasCounter++;
  if (dasCounter >= 16) {
    if (rightIsPressed) {
      MovePiece(3);
      dasCounter = 10;
    } else if (leftIsPressed) {
      MovePiece(2);
      dasCounter = 10;
    } else {
      dasCounter = 0;
    }
  }
  //Check down movement
  if (downIsPressed) {
    gravityCounter++;
    if (gravityCounter >= 2) {
      MovePiece(1);
      gravityCounter = 0;
    }
  } else {
    gravityCounter++;
    if (gravityCounter >= DROP_RATE_LEVELS[level]) {
      MovePiece(1);
      gravityCounter = 0;
    }
  }
  PutPieceMatrix(currentPositionX, currentPositionY, false);
  if (hasCollidedWithBottom(currentPositionX, currentPositionY + 1) || hasCollidedWithPiece(currentPositionX, currentPositionY + 1)) {
    lockCounter++;
    if (lockCounter >= lockMaxFrames) {
      LockPiece();
      lockCounter = 0;
    }
  } else {
    lockCounter = 0;
  }
  PutPieceMatrix(currentPositionX, currentPositionY, true);
  DrawBoard();
}

void SpawnPiece(int pieceNumber) {
  currentPositionX = 5;
  currentPositionY = 3;
  currentRotation = 0;

  pieceLetter = tetrominoes[pieceNumber];
  memcpy(currentPieceShape, tetrominoes_shapes[pieceNumber], sizeof(tetrominoes_shapes[pieceNumber]));

  if (hasCollidedWithScreen(currentPositionX, currentPositionY)) {
    return;
  }
  if (hasCollidedWithPiece(currentPositionX, currentPositionY)) {
    //GAME OVER
    GameOver();
    return;
  }

  PutPieceMatrix(currentPositionX, currentPositionY, true);
}

bool hasCollidedWithBottom(int positionX, int positionY) {
  int i, xOffset, yOffset;
  for (i = 0; i < 4; i++) {
    xOffset = currentPieceShape[i][0];
    yOffset = currentPieceShape[i][1];
    if (positionY + yOffset >= HEIGTH) {
      return true;
    }
  }
  return false;
}

bool hasCollidedWithScreen(int positionX, int positionY) {
  int i, xOffset, yOffset;
  for (i = 0; i < 4; i++) {
    xOffset = currentPieceShape[i][0];
    yOffset = currentPieceShape[i][1];
    if (positionX + xOffset < 0 || positionY + yOffset < 0 || positionX + xOffset >= WIDTH || positionY + yOffset >= HEIGTH) {
      return true;
    }
  }
  return false;
}

bool hasCollidedWithPiece(int positionX, int positionY) {
  int i, xOffset, yOffset;
  for (i = 0; i < 4; i++) {
    xOffset = currentPieceShape[i][0];
    yOffset = currentPieceShape[i][1];
    if (tetrisBoard[yOffset + positionY][xOffset + positionX]) {
      return true;
    }
  }
  return false;
}

void PutPieceMatrix(int positionX, int positionY, bool drawNotErase) {
  int i, xOffset, yOffset;
  for (i = 0; i < 4; i++) {
    xOffset = currentPieceShape[i][0];
    yOffset = currentPieceShape[i][1];
    if (drawNotErase) {
      tetrisBoard[yOffset + positionY][xOffset + positionX] = pieceLetter;
    } else {
      tetrisBoard[yOffset + positionY][xOffset + positionX] = 0;
    }
  }
}

int MovePiece(int action) {  //UP = 0, DOWN=1, LEFT=2, RIGHT=3
  if (gameIsPaused) {
    return -1;
  }
  int newPositionX = currentPositionX;
  int newPositionY = currentPositionY;
  switch (action) {
    case 0:
      newPositionY -= 1;
      break;
    case 1:
      newPositionY += 1;
      break;
    case 2:
      newPositionX -= 1;
      break;
    case 3:
      newPositionX += 1;
      break;
    default:
      Serial.println("[ERROR] - Action not found while moving piece");
      return -1;
  }
  PutPieceMatrix(currentPositionX, currentPositionY, false);
  if (hasCollidedWithScreen(newPositionX, newPositionY)) {
    PutPieceMatrix(currentPositionX, currentPositionY, true);
    return 0;
  }
  if (hasCollidedWithPiece(newPositionX, newPositionY)) {
    PutPieceMatrix(currentPositionX, currentPositionY, true);
    return 0;
  }
  currentPositionX = newPositionX;
  currentPositionY = newPositionY;
  PutPieceMatrix(currentPositionX, currentPositionY, true);
  return 1;
}

void HardDrop() {
  while (MovePiece(1)) {
  }
  LockPiece();
}

void LockPiece() {
  if (gameIsPaused)
    return;
  PutPieceMatrix(currentPositionX, currentPositionY, true);
  CheckForClearedLines();
  int pieceNumber = random(7);
  SpawnPiece(pieceNumber);
  canHoldPiece = true;
}

void handleButtonPressed(int buttonNumberPressed) {
  //Serial.println(buttonNumberPressed);
  if (gameIsPaused) {
    switch (buttonNumberPressed) {
      // Share button pressed
      case 9:
        //Arduino Reset
        resetFunc();
        break;
      // Options button pressed
      case 10:
        gameIsPaused = false;
        break;
    }
    return;
  }
  if (buttonNumberPressed > 128)
    buttonNumberPressed = buttonNumberPressed - 256;

  switch (buttonNumberPressed) {
    // RIGHT button pressed
    case 1:
      rightIsPressed = true;
      MovePiece(2);
      dasCounter = 0;
      break;
    // RIGHT button released
    case -1:
      rightIsPressed = false;
      break;
    // DOWN button pressed
    case 2:
      downIsPressed = true;
      MovePiece(1);
      gravityCounter = 0;
      break;
    // DOWN button released
    case -2:
      downIsPressed = false;
      break;
    // UP button pressed
    case 3:
      HardDrop();
      break;
    // UP button released
    case -3:
      break;
    // LEFT button pressed
    case 4:
      leftIsPressed = true;
      MovePiece(3);
      dasCounter = 0;
      break;
    // LEFT button released
    case -4:
      leftIsPressed = false;
      break;
    // Cross button pressed
    case 5:
      SuperRotationSystem(0);
      break;
    // Cross button released
    case -5:
      break;
    // Circle button pressed
    case 6:
      SuperRotationSystem(2);
      break;
    // Circle button released
    case -6:
      break;
    // L1 button pressed
    case 7:
      HoldPiece();
      break;
    // L1 button released
    case -7:
      break;
    // R1 button pressed
    case 8:
      HoldPiece();
      break;
    // R1 button released
    case -8:
      break;
    // Share button pressed
    case 9:
      //ESP Reset
      //ESP.restart();
      //Arduino Reset
      resetFunc();
      break;
    // Share button released
    case -9:
      break;
    // Options button pressed
    case 10:
      gameIsPaused = true;
      break;
    // Options button released
    case -10:
      break;
    // PS button pressed
    case 11:
      break;
    // PS button released
    case -11:
      break;
  }
}
void SuperRotationSystem(int rotateAction) {
  int oldCurrentRotation = currentRotation;
  int oldPieceShape[4][2];
  memcpy(oldPieceShape, currentPieceShape, sizeof(currentPieceShape));

  PutPieceMatrix(currentPositionX, currentPositionY, false);

  //Serial.printf("Posicao inicial:\n%d %d\n", currentPositionX, currentPositionY);

  if (!RotatePiece(rotateAction)) {
    //Serial.println("Failed default");
    if (!RotateTest2(oldCurrentRotation, currentRotation)) {
      //Serial.println("Failed 2");
      if (!RotateTest3(oldCurrentRotation, currentRotation)) {
        //Serial.println("Failed 3");
        if (!RotateTest4(oldCurrentRotation, currentRotation)) {
          //Serial.println("Failed 4");
          if (!RotateTest5(oldCurrentRotation, currentRotation)) {
            //Serial.println("Failed 5");
            currentRotation = oldCurrentRotation;
            memcpy(currentPieceShape, oldPieceShape, sizeof(oldPieceShape));
            PutPieceMatrix(currentPositionX, currentPositionY, true);
          }
        }
      }
    }
  }
}

bool RotatePiece(int rotateAction) {
  if (pieceLetter == 'O')
    return true;

  PutPieceMatrix(currentPositionX, currentPositionY, false);
  //4x4 piece
  if (pieceLetter == 'I') {
    //offsets todos manhosos para o I,
    //eu só dei hard code, vamos rezar para dar certo
    switch (rotateAction) {
      case 0:
        currentRotation--;
        break;
      case 1:
        currentRotation += 2;
        break;
      case 2:
        currentRotation++;
        break;
    }
    if (currentRotation > 3)
      currentRotation -= 4;
    else if (currentRotation < 0)
      currentRotation += 4;

    memcpy(currentPieceShape, i_rotations[currentRotation], sizeof(i_rotations[currentRotation]));
  } else {

    //3x3 pieces
    switch (rotateAction) {
      //90
      case 0:
        for (int i = 0; i < 4; ++i) {
          int x = currentPieceShape[i][0];
          int y = currentPieceShape[i][1];

          currentPieceShape[i][0] = y;
          currentPieceShape[i][1] = -x;
        }
        currentRotation--;
        break;
      //180
      case 1:
        for (int i = 0; i < 4; ++i) {
          int x = currentPieceShape[i][0];
          int y = currentPieceShape[i][1];

          currentPieceShape[i][0] = -y;
          currentPieceShape[i][1] = -x;
        }
        currentRotation += 2;
        break;
      //270
      case 2:
        for (int i = 0; i < 4; ++i) {
          int x = currentPieceShape[i][0];
          int y = currentPieceShape[i][1];

          currentPieceShape[i][0] = -y;
          currentPieceShape[i][1] = x;
        }
        currentRotation++;
        break;
    }
    if (currentRotation > 3)
      currentRotation -= 4;
    else if (currentRotation < 0)
      currentRotation += 4;
  }

  if (hasCollidedWithScreen(currentPositionX, currentPositionY) || hasCollidedWithPiece(currentPositionX, currentPositionY)) {
    return false;
  }
  return true;
}
bool RotateTest2(int oldRotation, int newRotation) {
  int offsetX = 0, offsetY = 0;
  if (pieceLetter == 'I') {

    if ((oldRotation == 0 && newRotation == 1) || (oldRotation == 3 && newRotation == 2)) {
      offsetX = -2;
      offsetY = 0;
    } else if ((oldRotation == 1 && newRotation == 0) || (oldRotation == 2 && newRotation == 3)) {
      offsetX = 2;
      offsetY = 0;
    } else if ((oldRotation == 1 && newRotation == 2) || (oldRotation == 0 && newRotation == 3)) {
      offsetX = -1;
      offsetY = 0;
    } else if ((oldRotation == 2 && newRotation == 1) || (oldRotation == 3 && newRotation == 0)) {
      offsetX = 1;
      offsetY = 0;
    }
  } else {
    if ((oldRotation == 0 && newRotation == 1) || (oldRotation == 2 && newRotation == 1) || (oldRotation == 3 && newRotation == 2) || (oldRotation == 3 && newRotation == 0)) {
      offsetX = -1;
      offsetY = 0;
    } else if ((oldRotation == 1 && newRotation == 0) || (oldRotation == 1 && newRotation == 2) || (oldRotation == 2 && newRotation == 3) || (oldRotation == 0 && newRotation == 3)) {
      offsetX = 1;
      offsetY = 0;
    }
  }
  //Serial.printf("2: %d %d | %d %d", currentPositionX, offsetX, currentPositionY, offsetY);
  if (hasCollidedWithScreen(currentPositionX + offsetX, currentPositionY - offsetY) || hasCollidedWithPiece(currentPositionX + offsetX, currentPositionY - offsetY)) {
    return false;
  }
  currentPositionX += offsetX;
  currentPositionY -= offsetY;
  PutPieceMatrix(currentPositionX, currentPositionY, true);
  return true;
}
bool RotateTest3(int oldRotation, int newRotation) {
  int offsetX = 0, offsetY = 0;
  if (pieceLetter == 'I') {
    if ((oldRotation == 0 && newRotation == 1) || (oldRotation == 3 && newRotation == 2)) {
      offsetX = 1;
      offsetY = 0;
    } else if ((oldRotation == 1 && newRotation == 0) || (oldRotation == 2 && newRotation == 3)) {
      offsetX = -1;
      offsetY = 0;
    } else if ((oldRotation == 1 && newRotation == 2) || (oldRotation == 0 && newRotation == 3)) {
      offsetX = 2;
      offsetY = 0;
    } else if ((oldRotation == 2 && newRotation == 1) || (oldRotation == 3 && newRotation == 0)) {
      offsetX = -2;
      offsetY = 0;
    }
  } else {
    if ((oldRotation == 0 && newRotation == 1) || (oldRotation == 2 && newRotation == 1)) {
      offsetX = -1;
      offsetY = 1;
    } else if ((oldRotation == 1 && newRotation == 0) || (oldRotation == 1 && newRotation == 2)) {
      offsetX = 1;
      offsetY = -1;
    } else if ((oldRotation == 2 && newRotation == 3) || (oldRotation == 0 && newRotation == 3)) {
      offsetX = 1;
      offsetY = 1;
    } else if ((oldRotation == 3 && newRotation == 2) || (oldRotation == 3 && newRotation == 0)) {
      offsetX = -1;
      offsetY = -1;
    }
  }
  //Serial.printf("3: %d %d | %d %d", currentPositionX, offsetX, currentPositionY, offsetY);
  if (hasCollidedWithScreen(currentPositionX + offsetX, currentPositionY - offsetY) || hasCollidedWithPiece(currentPositionX + offsetX, currentPositionY - offsetY)) {
    return false;
  }
  currentPositionX += offsetX;
  currentPositionY -= offsetY;

  PutPieceMatrix(currentPositionX, currentPositionY, true);
  return true;
}
bool RotateTest4(int oldRotation, int newRotation) {
  int offsetX = 0, offsetY = 0;
  if (pieceLetter == 'I') {
    if ((oldRotation == 0 && newRotation == 1) || (oldRotation == 3 && newRotation == 2)) {
      offsetX = -2;
      offsetY = -1;
    } else if ((oldRotation == 1 && newRotation == 0) || (oldRotation == 2 && newRotation == 3)) {
      offsetX = 2;
      offsetY = 1;
    } else if ((oldRotation == 1 && newRotation == 2) || (oldRotation == 0 && newRotation == 3)) {
      offsetX = -1;
      offsetY = 2;
    } else if ((oldRotation == 2 && newRotation == 1) || (oldRotation == 3 && newRotation == 0)) {
      offsetX = 1;
      offsetY = -2;
    }
  } else {
    if ((oldRotation == 0 && newRotation == 1) || (oldRotation == 2 && newRotation == 1) || (oldRotation == 2 && newRotation == 3) || (oldRotation == 0 && newRotation == 3)) {
      offsetX = -2;
      offsetY = 0;
    } else if ((oldRotation == 1 && newRotation == 0) || (oldRotation == 1 && newRotation == 2) || (oldRotation == 3 && newRotation == 2) || (oldRotation == 3 && newRotation == 0)) {
      offsetX = 2;
      offsetY = 0;
    }
  }
  //Serial.printf("4: %d %d | %d %d", currentPositionX, offsetX, currentPositionY, offsetY);
  if (hasCollidedWithScreen(currentPositionX + offsetX, currentPositionY - offsetY) || hasCollidedWithPiece(currentPositionX + offsetX, currentPositionY - offsetY)) {
    return false;
  }
  currentPositionX += offsetX;
  currentPositionY -= offsetY;
  PutPieceMatrix(currentPositionX, currentPositionY, true);
  return true;
}
bool RotateTest5(int oldRotation, int newRotation) {
  int offsetX = 0, offsetY = 0;
  if (pieceLetter == 'I') {
    if ((oldRotation == 0 && newRotation == 1) || (oldRotation == 3 && newRotation == 2)) {
      offsetX = 1;
      offsetY = 2;
    } else if ((oldRotation == 1 && newRotation == 0) || (oldRotation == 2 && newRotation == 3)) {
      offsetX = -1;
      offsetY = -2;
    } else if ((oldRotation == 1 && newRotation == 2) || (oldRotation == 0 && newRotation == 3)) {
      offsetX = 2;
      offsetY = -1;
    } else if ((oldRotation == 2 && newRotation == 1) || (oldRotation == 3 && newRotation == 0)) {
      offsetX = -2;
      offsetY = 1;
    }
  } else {
    if ((oldRotation == 0 && newRotation == 1) || (oldRotation == 2 && newRotation == 1)) {
      offsetX = -1;
      offsetY = -2;
    } else if ((oldRotation == 1 && newRotation == 0) || (oldRotation == 1 && newRotation == 2)) {
      offsetX = 1;
      offsetY = 2;
    } else if ((oldRotation == 2 && newRotation == 3) || (oldRotation == 0 && newRotation == 3)) {
      offsetX = 1;
      offsetY = -2;
    } else if ((oldRotation == 3 && newRotation == 2) || (oldRotation == 3 && newRotation == 0)) {
      offsetX = -1;
      offsetY = 2;
    }
  }
  //Serial.printf("5: %d %d | %d %d", currentPositionX, offsetX, currentPositionY, offsetY);
  if (hasCollidedWithScreen(currentPositionX + offsetX, currentPositionY - offsetY) || hasCollidedWithPiece(currentPositionX + offsetX, currentPositionY - offsetY)) {
    return false;
  }
  currentPositionX += offsetX;
  currentPositionY -= offsetY;
  PutPieceMatrix(currentPositionX, currentPositionY, true);
  return true;
}

void CheckForClearedLines() {
  int numberOfLinesCleared = 0;
  for (int row = 0; row < HEIGTH; row++) {
    bool shouldClear = true;
    for (int col = 0; col < WIDTH; col++) {
      if (tetrisBoard[row][col] == 0) {
        shouldClear = 0;
        break;
      }
    }
    if (shouldClear) {
      ShiftLinesDown(row);
      numberOfLinesCleared++;
      row--;  // Check the same row again as it has new content after shifting
    }
  }
  switch (numberOfLinesCleared) {
    case 0: break;
    case 1:
      score += SINGLE * (level + 1);
      break;
    case 2:
      score += DOUBLE * (level + 1);
      break;
    case 3:
      score += TRIPLE * (level + 1);
      break;
    case 4:
      score += TETRIS * (level + 1);
      break;
    default:
      Serial.println("[ERROR] - Couldn't count rows cleared while calculating score");
      break;
  }
}

void ShiftLinesDown(int clearedRow) {
  for (int row = clearedRow; row > 0; row--) {
    memcpy(tetrisBoard[row], tetrisBoard[row - 1], WIDTH * sizeof(unsigned char));
  }
  // Clear the top line after shifting
  memset(tetrisBoard[0], 0, WIDTH * sizeof(unsigned char));

  linesCleared++;

  if (linesCleared >= linesToLevelUp) {
    level++;
    if (level >= 9 && level <= 15 || level >= 25) {
      // Bug do tetris a calcular linhas, não faz nada
    } else {
      linesToLevelUp += 10;
    }
  }
}

void GameOver() {
  gameIsPaused = true;
  Serial.print("\n\n\n\n\n\n\n");
  Serial.println("=[Game Over]=");
  Serial.print("Score: ");
  Serial.println(score);
  Serial.print("Lines cleared: ");
  Serial.println(linesCleared);
  Serial.print("Level reached: ");
  Serial.println(level);
  //SendValueToScoreboard("testWIP", score);
}

void HoldPiece() {
  if (!canHoldPiece)
    return;
  char aux = holdPiece;
  holdPiece = pieceLetter;
  if (aux == 0) {
    PutPieceMatrix(currentPositionX, currentPositionY, false);
    int pieceNumber = random(7);
    SpawnPiece(pieceNumber);
    return;
  } else {
    pieceLetter = aux;
    PutPieceMatrix(currentPositionX, currentPositionY, false);
    for (int i = 0; i < 7; i++) {
      if (pieceLetter == tetrominoes[i]) {
        SpawnPiece(i);
        break;
      }
    }
  }
  canHoldPiece = false;
}

void SendScore(char* name, int score) {
  digitalWrite(DEBUG_LED_PIN, HIGH);
  // Crie um objeto JSON com os valores que deseja enviar
  StaticJsonDocument<256> docJson;
  docJson["name"] = name;
  docJson["score"] = score;

  // Serializa o JSON em uma string
  char json[256];
  serializeJson(docJson, json);

  digitalWrite(DEBUG_LED_PIN, LOW);
}

//DEBUG
void ShowBoard() {
  Serial.print("\n\n\n\n\n\n\n");
  for (int i = 0; i < HEIGTH; i++) {
    for (int j = 0; j < WIDTH; j++) {
      char showchar;
      tetrisBoard[i][j] == 0 ? showchar = '0' : showchar = tetrisBoard[i][j];
      Serial.print(showchar);
    }
    Serial.println();
  }
}

void DrawBoard() {
  ShowBoard();
  int horizontalOffset = 4;
  int verticalOffset = 12;

  DrawBackground();

  for (int x = 0; x < WIDTH; ++x) {
    for (int y = 0; y < HEIGTH - 2; ++y) {
      switch (tetrisBoard[y + 2][x]) {
        case 'I':
          leds(y + verticalOffset, x + horizontalOffset) = CRGB(0, 255, 255);
          break;
        case 'O':
          leds(y + verticalOffset, x + horizontalOffset) = CRGB(255, 255, 0);
          break;
        case 'T':
          leds(y + verticalOffset, x + horizontalOffset) = CRGB(128, 0, 128);
          break;
        case 'J':
          leds(y + verticalOffset, x + horizontalOffset) = CRGB(0, 255, 0);
          break;
        case 'L':
          leds(y + verticalOffset, x + horizontalOffset) = CRGB(255, 0, 0);
          break;
        case 'S':
          leds(y + verticalOffset, x + horizontalOffset) = CRGB(0, 0, 255);
          break;
        case 'Z':
          leds(y + verticalOffset, x + horizontalOffset) = CRGB(255, 127, 0);
          break;
        default:
          leds(y + verticalOffset, x + horizontalOffset) = 0;
          break;
      }
    }
  }
  FastLED.show();
  Serial.println("Displaying on LEDS");
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

void setup() {
  SetupSerial();

  int pieceNumber = random(7);
  SpawnPiece(pieceNumber);

  debugTimer = micros();

  pinMode(DEBUG_LED_PIN, OUTPUT);

  SetupFastLeds();

  ResetLedMatrix();
  FastLED.show();
}

void loop() {
  int i = 0;
  frameTimer = micros();
  while (micros() - frameTimer < MICROS_PER_FRAME) {
    int receivedData = 0;
    if (PS4Serial.available() > 0) {
      receivedData = PS4Serial.parseInt();  // Lê o dado enviado pelo ESP32
      if (receivedData != 0) {
        handleButtonPressed(receivedData);
      }
    }
    if (!gameIsPaused) {
      FramePassed();
    }
  }
}

void DrawBackground() {
  for (int x = 0; x < MATRIX_WIDTH; ++x) {
    for (int y = 0; y < MATRIX_HEIGHT; ++y) {
      leds(x, y) = TetrisNEEC[x][y];
    }
  }
}

/*void SendPostRequest(String json) {
  HttpClient http;

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

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.disconnect();
  }
  digitalWrite(DEBUG_LED_PIN, LOW);
}*/