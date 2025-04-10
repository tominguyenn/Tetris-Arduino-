
#include <LedControl.h>

// LED matrix control: DIN, CLK, CS
LedControl lc = LedControl(11, 13, 10, 1);

// Joystick
const int VRx = A0;
const int VRy = A1;
const int SW = 2;

// Game state
bool board[8][8] = {false};
int blockX = 3, blockY = 0;

unsigned long lastFall = 0;
const int fallDelay = 600;

bool isGameOver = false;
bool hasFlashed = false;

// Shape pointer and size
byte* currentShape;
int shapeWidth;
int shapeHeight;
int shapeIndex = 0;

// Block types (with rotation sets)
const byte shapes[4][4][4][4] = {
  { // O block (no rotation needed)
    {
      {1, 1},
      {1, 1}
    }
  },
  { // L block (4 rotations)
    {
      {1, 0},
      {1, 0},
      {1, 1}
    },
    {
      {1, 1, 1},
      {1, 0, 0}
    },
    {
      {1, 1},
      {0, 1},
      {0, 1}
    },
    {
      {0, 0, 1},
      {1, 1, 1}
    }
  },
  { // T block (4 rotations)
    {
      {1, 1, 1},
      {0, 1, 0}
    },
    {
      {1, 0},
      {1, 1},
      {1, 0}
    },
    {
      {0, 1, 0},
      {1, 1, 1}
    },
    {
      {0, 1},
      {1, 1},
      {0, 1}
    }
  },
  { // I block (2 rotations)
    {
      {1},
      {1},
      {1},
      {1}
    },
    {
      {1, 1, 1, 1}
    }
  }
};

const int shapeRotations[4] = {1, 4, 4, 2};
int currentShapeType = 0;
int currentRotation = 0;

void setup() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 3);
  lc.clearDisplay(0);

  pinMode(VRx, INPUT);
  pinMode(VRy, INPUT);
  pinMode(SW, INPUT_PULLUP);

  pickNewBlock();
}

void loop() {
  lc.clearDisplay(0);

  if (isGameOver) {
    if (!hasFlashed) {
      for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
          lc.setLed(0, y, x, true);
        }
      }
      delay(400);
      lc.clearDisplay(0);
      hasFlashed = true;
    }

    if (digitalRead(SW) == LOW) {
      resetGame();
      delay(300);
    }
    return;
  }

  drawBoard();
  handleInput();
  drawCurrentShape();

  if (millis() - lastFall > fallDelay) {
    blockY++;
    if (checkCollision(blockX, blockY)) {
      placeBlock();
      clearFullLines();
      pickNewBlock();
    }
    lastFall = millis();
  }

  delay(30);
}

void pickNewBlock() {
  currentShapeType = random(4);
  currentRotation = 0;
  updateShapeSize();

  blockX = (8 - shapeWidth) / 2;
  blockY = 0;

  if (checkCollision(blockX, blockY)) {
    isGameOver = true;
    hasFlashed = false;
  }
}

void drawCurrentShape() {
  for (int y = 0; y < shapeHeight; y++) {
    for (int x = 0; x < shapeWidth; x++) {
      byte val = shapes[currentShapeType][currentRotation][y][x];
      if (val == 1) {
        int drawX = blockX + x;
        int drawY = blockY + y;
        if (drawX >= 0 && drawX < 8 && drawY >= 0 && drawY < 8)
          lc.setLed(0, drawY, drawX, true);
      }
    }
  }
}

void drawBoard() {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (board[y][x]) {
        lc.setLed(0, y, x, true);
      }
    }
  }
}

void placeBlock() {
  for (int y = 0; y < shapeHeight; y++) {
    for (int x = 0; x < shapeWidth; x++) {
      byte val = shapes[currentShapeType][currentRotation][y][x];
      if (val == 1) {
        int px = blockX + x;
        int py = blockY + y - 1;
        if (px >= 0 && px < 8 && py >= 0 && py < 8) {
          board[py][px] = true;
        }
      }
    }
  }
}

void clearFullLines() {
  for (int y = 7; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < 8; x++) {
      if (!board[y][x]) {
        full = false;
        break;
      }
    }
    if (full) {
      for (int row = y; row > 0; row--) {
        for (int col = 0; col < 8; col++) {
          board[row][col] = board[row - 1][col];
        }
      }
      for (int col = 0; col < 8; col++) {
        board[0][col] = false;
      }
      y++; // re-check same row after shift
    }
  }
}

bool checkCollision(int xPos, int yPos) {
  for (int y = 0; y < shapeHeight; y++) {
    for (int x = 0; x < shapeWidth; x++) {
      byte val = shapes[currentShapeType][currentRotation][y][x];
      if (val == 1) {
        int tx = xPos + x;
        int ty = yPos + y;
        if (ty >= 8 || (ty >= 0 && tx >= 0 && tx < 8 && board[ty][tx])) {
          return true;
        }
      }
    }
  }
  return false;
}

void handleInput() {
  int xVal = analogRead(VRx);

  if (xVal < 400) {
    blockX--;
    if (blockX < 0) blockX = 0;
    if (checkCollision(blockX, blockY)) blockX++;
    delay(150);
  }

  if (xVal > 600) {
    blockX++;
    if (blockX > 8 - shapeWidth) blockX = 8 - shapeWidth;
    if (checkCollision(blockX, blockY)) blockX--;
    delay(150);
  }

  if (digitalRead(SW) == LOW) {
    rotateBlock();
    delay(150);
  }
}

void rotateBlock() {
  int nextRotation = (currentRotation + 1) % shapeRotations[currentShapeType];
  int tempWidth = 0, tempHeight = 0;

  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (shapes[currentShapeType][nextRotation][y][x] == 1) {
        if (x + 1 > tempWidth) tempWidth = x + 1;
        if (y + 1 > tempHeight) tempHeight = y + 1;
      }
    }
  }

  if (blockX + tempWidth <= 8 && blockY + tempHeight <= 8) {
    int oldRotation = currentRotation;
    currentRotation = nextRotation;
    shapeWidth = tempWidth;
    shapeHeight = tempHeight;

    if (checkCollision(blockX, blockY)) {
      currentRotation = oldRotation;
      updateShapeSize();
    }
  }
}

void updateShapeSize() {
  shapeWidth = 0;
  shapeHeight = 0;
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      if (shapes[currentShapeType][currentRotation][y][x] == 1) {
        if (x + 1 > shapeWidth) shapeWidth = x + 1;
        if (y + 1 > shapeHeight) shapeHeight = y + 1;
      }
    }
  }
}

void resetGame() {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      board[y][x] = false;
    }
  }

  isGameOver = false;
  hasFlashed = false;
  lc.clearDisplay(0);
  pickNewBlock();
}
