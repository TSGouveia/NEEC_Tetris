#include "piece_data.h"
#include <Wire.h>

#define HEIGTH 20 + 2  //+2 para n foder a rotação do I piece no inicio
#define WIDTH 10
#define FRAMES_PER_SECOND 60

const unsigned long MICROS_PER_FRAME = 1000000.0 / FRAMES_PER_SECOND;
const int DROP_RATE_LEVELS[15] = { 48, 43, 38, 33, 28, 23, 18, 13, 8, 6, 5, 4, 3, 2, 1 };

unsigned char tetrisBoard[HEIGTH][WIDTH] = { 0 };
unsigned long frameTimer;

//Piece visuals
char pieceLetter;
int currentPieceShape[4][2];
int currentPositionX = 5;
int currentPositionY = 2;

//Controller logic
bool downIsPressed = false;
bool leftIsPressed = false;
bool rightIsPressed = false;

//Tetris logic

//Movement logic
int dasCounter = 0;
int gravityCounter = 0;

//Level logic
int level = 1;

unsigned long debugTimer = 0;

void setup() {
  Serial.begin(115200);
  SetupPS4I2C();

  int pieceNumber = random(7);
  SpawnPiece(pieceNumber);

  debugTimer = micros();
}

void loop() {
  frameTimer = micros();
  while (micros() - frameTimer < MICROS_PER_FRAME) {
    //26000 iterações cá dentro, deve ser suficiente para os controlos responderem naturalmente
  }
  FramePassed();
}

void SetupPS4I2C() {
  Wire.begin(8);                 // join i2c bus with address #8
  Wire.onReceive(receiveEvent);  // register event
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
    if (gravityCounter >= DROP_RATE_LEVELS[level - 1]) {
      MovePiece(1);
      gravityCounter = 0;
    }
  }
  ShowBoard();
}

void SpawnPiece(int pieceNumber) {
  //char pieceColor[3];
  //memcpy(pieceColor, tetrominoes_colors[pieceNumber], sizeof(tetrominoes_colors[pieceNumber]));
  pieceLetter = tetrominoes[pieceNumber];
  memcpy(currentPieceShape, tetrominoes_shapes[pieceNumber], sizeof(tetrominoes_shapes[pieceNumber]));

  if (hasCollidedWithScreen(currentPositionX, currentPositionY)) {
    return;
  }
  if (hasCollidedWithPiece(currentPositionX, currentPositionY)) {
    return;
  }

  PutPieceMatrix(currentPositionX, currentPositionY, true);
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

void MovePiece(int action) {  //UP = 0, DOWN=1, LEFT=2, RIGHT=3
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
      return;
  }
  PutPieceMatrix(currentPositionX, currentPositionY, false);
  if (hasCollidedWithScreen(newPositionX, newPositionY)) {
    PutPieceMatrix(currentPositionX, currentPositionY, true);
    return;
  }
  if (hasCollidedWithPiece(newPositionX, newPositionY)) {
    PutPieceMatrix(currentPositionX, currentPositionY, true);
    return;
  }
  currentPositionX = newPositionX;
  currentPositionY = newPositionY;
  PutPieceMatrix(currentPositionX, currentPositionY, true);
}



void receiveEvent(int howMany) {
  while (Wire.available()) {
    int c = Wire.read();  // receive byte as a int
    handleButtonPressed(c);
  }
}

void handleButtonPressed(int buttonNumberPressed) {
  switch (buttonNumberPressed) {
    // RIGHT button pressed
    case 1:
      rightIsPressed = true;
      MovePiece(3);
      dasCounter = 0;
      break;
    // RIGHT button released
    case -1:
      rightIsPressed = false;
      break;
    // DOWN button pressed
    case 2:
      downIsPressed = true;
      gravityCounter = 0;
      break;
    // DOWN button released
    case -2:
      downIsPressed = false;
      break;
    // UP button pressed
    case 3:
      break;
    // UP button released
    case -3:
      break;
    // LEFT button pressed
    case 4:
      leftIsPressed = true;
      MovePiece(2);
      dasCounter = 0;
      break;
    // LEFT button released
    case -4:
      leftIsPressed = false;
      break;
    // Cross button pressed
    case 5:
      break;
    // Cross button released
    case -5:
      break;
    // Circle button pressed
    case 6:
      break;
    // Circle button released
    case -6:
      break;
    // L1 button pressed
    case 7:
      break;
    // L1 button released
    case -7:
      break;
    // R1 button pressed
    case 8:
      break;
    // R1 button released
    case -8:
      break;
    // Share button pressed
    case 9:
      break;
    // Share button released
    case -9:
      break;
    // Options button pressed
    case 10:
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

//DEBUG
void ShowBoard() {
  Serial.print("\n\n\n\n\n\n\n");
  for (int i = 0; i < HEIGTH; i++) {
    for (int j = 0; j < WIDTH; j++) {
      Serial.printf("%c", tetrisBoard[i][j]);
    }
    Serial.println();
  }
}