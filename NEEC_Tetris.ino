#include "piece_data.h"

#define HEIGTH 20 + 2  //+2 para n foder a rotação do I piece no inicio
#define WIDTH 10
#define FRAMES_PER_SECOND 60
#define OFFSET_LIMIT 3

const unsigned long MICROS_PER_FRAME = 1000000.0 / FRAMES_PER_SECOND;

unsigned char tetrisBoard[HEIGTH][WIDTH] = { 0 };
unsigned long frameTimer;

bool pieceIsSpawned = false;
char currentPieceShape[4][4];
int currentOffsetX = 3;
int currentOffsetY = 1;

void setup() {
  Serial.begin(115200);

  int pieceNumber = random(7);
  SpawnPiece(pieceNumber);
}

void loop() {
  /*frameTimer = micros();
  while (micros() - frameTimer < MICROS_PER_FRAME) {
    //26000 iterações cá dentro, deve ser suficiente para os controlos responderem naturalmente
  }*/
  //FramePassed();

  MovePiece(1);
  FramePassed();
  delay(1000);
}

void FramePassed() {
  ShowBoard();
}

void SpawnPiece(int pieceNumber) {
  char pieceLetter = tetrominoes[pieceNumber];
  //char pieceColor[3];
  //memcpy(pieceColor, tetrominoes_colors[pieceNumber], sizeof(tetrominoes_colors[pieceNumber]));
  memcpy(currentPieceShape, tetrominoes_shapes[pieceNumber], sizeof(tetrominoes_shapes[pieceNumber]));

  SpawnPiece();

  pieceIsSpawned = true;
}

void SpawnPiece() {
  currentOffsetX = 3;
  currentOffsetY = 1;
  UpdatePiece(currentOffsetX, currentOffsetY, true);
}

void MovePiece(int action) {  //UP = 0, DOWN=1, LEFT=2, RIGHT=3
  int newOffsetX = currentOffsetX;
  int newOffsetY = currentOffsetY;
  switch (action) {
    case 0:
      newOffsetY -= 1;
      break;
    case 1:
      newOffsetY += 1;
      break;
    case 2:
      newOffsetX -= 1;
      break;
    case 3:
      newOffsetX += 1;
      break;
    default:
      Serial.println("[ERROR] - Action not found while moving piece");
      return;
  }
  if (newOffsetX < 0 || newOffsetX > WIDTH - OFFSET_LIMIT || newOffsetY < 0 || newOffsetY > HEIGTH - OFFSET_LIMIT) {
    Serial.println("[ERROR] - Moved piece out of bounds");
    return;
  }
  UpdatePiece(currentOffsetX, currentOffsetY, false);
  currentOffsetX = newOffsetX;
  currentOffsetY = newOffsetY;
  UpdatePiece(currentOffsetX, currentOffsetY, true);
}
void UpdatePiece(int horizontalOffset, int verticalOffset, bool drawNotErase) {
  if (horizontalOffset + OFFSET_LIMIT > WIDTH || verticalOffset + OFFSET_LIMIT > HEIGTH) {
    Serial.println("[ERROR] - Drawing piece out of bounds");
    return;
  }
  int hMatrix, vMatrix, vShape, hShape;
  for (hMatrix = horizontalOffset; hMatrix < horizontalOffset + 4; hMatrix++) {
    for (vMatrix = verticalOffset; vMatrix < verticalOffset + 4; vMatrix++) {
      hShape = hMatrix - horizontalOffset;
      vShape = vMatrix - verticalOffset;

      if (currentPieceShape[vShape][hShape]) {
        if (drawNotErase) {
          tetrisBoard[vMatrix][hMatrix] = currentPieceShape[vShape][hShape];
        } else {
          tetrisBoard[vMatrix][hMatrix] = 0;
        }
      }
    }
  }
}

void ShowBoard() {
  Serial.print("\n\n\n\n\n\n\n");
  for (int i = 2; i < HEIGTH; i++) {
    for (int j = 0; j < WIDTH; j++) {
      Serial.printf("%c", tetrisBoard[i][j]);
    }
    Serial.println();
  }
}