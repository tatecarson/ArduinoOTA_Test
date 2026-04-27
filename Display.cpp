#include "Display.h"
#include "Arduino_LED_Matrix.h"

char currentMessage[5] = "TATE";
bool otaInProgress = false;
bool otaError = false;

static ArduinoLEDMatrix matrix;
static uint8_t messageFrame[8][12];

static uint8_t readyFrame[8][12] = {
  { 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1 },
  { 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

static uint8_t uploadFrame[8][12] = {
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }
};

static uint8_t errorFrame[8][12] = {
  { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
  { 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0 },
  { 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0 },
  { 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0 },
  { 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0 },
  { 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0 }
};

char cleanDisplayChar(char c) {
  if (c >= 'a' && c <= 'z') return c - 32;
  if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return c;
  return ' ';
}

static uint8_t glyphRow(char c, int row) {
  static const uint8_t digits[10][5] = {
    { 7, 5, 5, 5, 7 }, { 2, 6, 2, 2, 7 }, { 7, 1, 7, 4, 7 }, { 7, 1, 7, 1, 7 }, { 5, 5, 7, 1, 1 },
    { 7, 4, 7, 1, 7 }, { 7, 4, 7, 5, 7 }, { 7, 1, 2, 4, 4 }, { 7, 5, 7, 5, 7 }, { 7, 5, 7, 1, 7 }
  };

  if (c >= '0' && c <= '9') return digits[c - '0'][row];

  switch (c) {
    case 'A': { static const uint8_t p[5] = { 7, 5, 7, 5, 5 }; return p[row]; }
    case 'B': { static const uint8_t p[5] = { 6, 5, 6, 5, 6 }; return p[row]; }
    case 'C': { static const uint8_t p[5] = { 7, 4, 4, 4, 7 }; return p[row]; }
    case 'D': { static const uint8_t p[5] = { 6, 5, 5, 5, 6 }; return p[row]; }
    case 'E': { static const uint8_t p[5] = { 7, 4, 6, 4, 7 }; return p[row]; }
    case 'F': { static const uint8_t p[5] = { 7, 4, 6, 4, 4 }; return p[row]; }
    case 'G': { static const uint8_t p[5] = { 7, 4, 5, 5, 7 }; return p[row]; }
    case 'H': { static const uint8_t p[5] = { 5, 5, 7, 5, 5 }; return p[row]; }
    case 'I': { static const uint8_t p[5] = { 7, 2, 2, 2, 7 }; return p[row]; }
    case 'J': { static const uint8_t p[5] = { 1, 1, 1, 5, 7 }; return p[row]; }
    case 'K': { static const uint8_t p[5] = { 5, 5, 6, 5, 5 }; return p[row]; }
    case 'L': { static const uint8_t p[5] = { 4, 4, 4, 4, 7 }; return p[row]; }
    case 'M': { static const uint8_t p[5] = { 5, 7, 7, 5, 5 }; return p[row]; }
    case 'N': { static const uint8_t p[5] = { 5, 7, 7, 7, 5 }; return p[row]; }
    case 'O': { static const uint8_t p[5] = { 7, 5, 5, 5, 7 }; return p[row]; }
    case 'P': { static const uint8_t p[5] = { 7, 5, 7, 4, 4 }; return p[row]; }
    case 'Q': { static const uint8_t p[5] = { 7, 5, 5, 7, 1 }; return p[row]; }
    case 'R': { static const uint8_t p[5] = { 7, 5, 7, 6, 5 }; return p[row]; }
    case 'S': { static const uint8_t p[5] = { 7, 4, 7, 1, 7 }; return p[row]; }
    case 'T': { static const uint8_t p[5] = { 7, 2, 2, 2, 2 }; return p[row]; }
    case 'U': { static const uint8_t p[5] = { 5, 5, 5, 5, 7 }; return p[row]; }
    case 'V': { static const uint8_t p[5] = { 5, 5, 5, 5, 2 }; return p[row]; }
    case 'W': { static const uint8_t p[5] = { 5, 5, 7, 7, 5 }; return p[row]; }
    case 'X': { static const uint8_t p[5] = { 5, 5, 2, 5, 5 }; return p[row]; }
    case 'Y': { static const uint8_t p[5] = { 5, 5, 2, 2, 2 }; return p[row]; }
    case 'Z': { static const uint8_t p[5] = { 7, 1, 2, 4, 7 }; return p[row]; }
    default: return 0;
  }
}

void displayBegin() {
  matrix.begin();
}

void renderMessage(const char* text) {
  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 12; col++) {
      messageFrame[row][col] = 0;
    }
  }

  for (int i = 0; i < 4 && text[i] != '\0'; i++) {
    for (int row = 0; row < 5; row++) {
      uint8_t bits = glyphRow(text[i], row);
      for (int col = 0; col < 3; col++) {
        if (bits & (1 << (2 - col))) {
          messageFrame[row + 1][i * 3 + col] = 1;
        }
      }
    }
  }

  matrix.renderBitmap(messageFrame, 8, 12);
}

static void normalizeInto(const char* text, char* out) {
  int writeIndex = 0;
  for (int i = 0; text[i] != '\0' && writeIndex < 4; i++) {
    char c = cleanDisplayChar(text[i]);
    if (c != ' ') {
      out[writeIndex++] = c;
    }
  }
  out[writeIndex] = '\0';
  if (writeIndex == 0) {
    strcpy(out, "TATE");
  }
}

void setMatrixMessage(const char* text) {
  normalizeInto(text, currentMessage);
  if (!otaInProgress) {
    renderMessage(currentMessage);
  }
}

void applyRemoteMessage(const char* text) {
  normalizeInto(text, currentMessage);
  if (!otaInProgress) {
    renderMessage(currentMessage);
  }
}

void showReady() {
  renderMessage(currentMessage);
}

void showUpload() {
  matrix.renderBitmap(uploadFrame, 8, 12);
}

void showError() {
  matrix.renderBitmap(errorFrame, 8, 12);
}
