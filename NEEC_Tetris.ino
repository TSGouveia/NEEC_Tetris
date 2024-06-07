#include "piece_data.h"

#define HEIGTH 20 + 2  //+2 para n foder a rotação do I piece no inicio
#define WIDTH 10
#define FRAMES_PER_SECOND 60
const unsigned long MICROS_PER_FRAME = 1000000.0 / FRAMES_PER_SECOND;

unsigned char tetrisBoard[HEIGTH][WIDTH] = { 0 };
unsigned long frameTimer;

void setup() {
  Serial.begin(115200);

  int pieceNumber = random(7);
  SpawnPiece(pieceNumber);
}

void loop() {
  frameTimer = micros();
  while (micros() - frameTimer < MICROS_PER_FRAME) {
    //26000 iterações cá dentro, deve ser suficiente os controlos responderem naturalmente
  }
  //FramePassed();
}

void FramePassed() {
  ShowBoard();
}

void SpawnPiece(int pieceNumber) {
  char pieceLetter = tetrominoes[pieceNumber];
  char pieceColor[3];
  char pieceShape[4][4];

  memcpy(pieceColor, tetrominoes_colors[pieceNumber], sizeof(tetrominoes_colors[pieceNumber]));
  memcpy(pieceShape, tetrominoes_shapes[pieceNumber], sizeof(tetrominoes_shapes[pieceNumber]));

  Serial.printf("Piece is %c\n", pieceLetter);
  Serial.printf("Its color is (%d,%d,%d)\n", pieceColor[0], pieceColor[1], pieceColor[2]);
  Serial.println("and shape is:");
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      Serial.print(pieceShape[i][j]);
    }
    Serial.println();
  }
  Serial.println();
}


void ShowBoard() {
  Serial.print("\n\n\n\n\n\n\n");
  for (int i = 0; i < HEIGTH; i++) {
    for (int j = 0; j < WIDTH; j++) {
      Serial.print(tetrisBoard[i][j]);
    }
    Serial.println();
  }
}
