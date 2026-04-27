#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include "Arduino_LED_Matrix.h"

char ssid[] = "Lucy";
char pass[] = "sweetsweet2017";

ArduinoLEDMatrix matrix;
WiFiServer controlServer(80);

bool otaReady = false;
bool otaInProgress = false;
bool otaError = false;
unsigned long lastLedTick = 0;
bool ledState = LOW;
char currentMessage[5] = "TATE";
uint8_t messageFrame[8][12];

uint8_t readyFrame[8][12] = {
  { 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1 },
  { 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1 },
  { 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

uint8_t uploadFrame[8][12] = {
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
  { 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }
};

uint8_t errorFrame[8][12] = {
  { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
  { 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0 },
  { 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0 },
  { 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0 },
  { 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0 },
  { 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0 }
};

bool ipIsUnset(IPAddress ip) {
  return ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
}

char cleanDisplayChar(char c) {
  if (c >= 'a' && c <= 'z') {
    return c - 32;
  }
  if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
    return c;
  }
  return ' ';
}

uint8_t glyphRow(char c, int row) {
  static const uint8_t digits[10][5] = {
    { 7, 5, 5, 5, 7 }, { 2, 6, 2, 2, 7 }, { 7, 1, 7, 4, 7 }, { 7, 1, 7, 1, 7 }, { 5, 5, 7, 1, 1 },
    { 7, 4, 7, 1, 7 }, { 7, 4, 7, 5, 7 }, { 7, 1, 2, 4, 4 }, { 7, 5, 7, 5, 7 }, { 7, 5, 7, 1, 7 }
  };

  if (c >= '0' && c <= '9') {
    return digits[c - '0'][row];
  }

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

void setMatrixMessage(const char* text) {
  int writeIndex = 0;

  for (int readIndex = 0; text[readIndex] != '\0' && writeIndex < 4; readIndex++) {
    char c = cleanDisplayChar(text[readIndex]);
    if (c != ' ') {
      currentMessage[writeIndex++] = c;
    }
  }

  currentMessage[writeIndex] = '\0';
  if (writeIndex == 0) {
    strcpy(currentMessage, "TATE");
  }

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

void onOtaStart() {
  otaInProgress = true;
  digitalWrite(LED_BUILTIN, HIGH);
  showUpload();
  Serial.println("OTA upload started.");
}

void onOtaApply() {
  Serial.println("OTA upload complete. Applying update...");
}

void onOtaError(int code, const char* message) {
  otaError = true;
  otaInProgress = false;
  showError();
  Serial.print("OTA error ");
  Serial.print(code);
  Serial.print(": ");
  Serial.println(message);
}

void printWifiStatus() {
  Serial.print("WiFi status: ");
  Serial.println(WiFi.status());
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("RSSI: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

int hexValue(char c) {
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  return -1;
}

String urlDecode(String value) {
  String decoded;

  for (int i = 0; i < value.length(); i++) {
    char c = value.charAt(i);

    if (c == '+') {
      decoded += ' ';
    } else if (c == '%' && i + 2 < value.length()) {
      int high = hexValue(value.charAt(i + 1));
      int low = hexValue(value.charAt(i + 2));

      if (high >= 0 && low >= 0) {
        decoded += char((high << 4) | low);
        i += 2;
      }
    } else {
      decoded += c;
    }
  }

  return decoded;
}

String getQueryValue(String path, const char* key) {
  String marker = String(key) + "=";
  int start = path.indexOf(marker);

  if (start < 0) {
    return "";
  }

  start += marker.length();
  int end = path.indexOf('&', start);

  if (end < 0) {
    end = path.length();
  }

  return urlDecode(path.substring(start, end));
}

void sendControlResponse(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Connection: close");
  client.println("Content-Type: text/html");
  client.println();
  client.println("<!doctype html><html><body>");
  client.println("<h1>Arduino LED Matrix</h1>");
  client.print("<p>Current message: ");
  client.print(currentMessage);
  client.println("</p>");
  client.println("<form action=\"/msg\" method=\"get\">");
  client.println("<input name=\"text\" maxlength=\"4\" autofocus>");
  client.println("<button type=\"submit\">Send</button>");
  client.println("</form>");
  client.println("<p>Use up to 4 letters or numbers.</p>");
  client.println("</body></html>");
}

void handleControlClient() {
  WiFiClient client = controlServer.available();

  if (!client) {
    return;
  }

  String requestLine = client.readStringUntil('\n');
  requestLine.trim();

  while (client.connected()) {
    String header = client.readStringUntil('\n');
    header.trim();
    if (header.length() == 0) {
      break;
    }
  }

  if (requestLine.startsWith("GET ")) {
    int pathStart = 4;
    int pathEnd = requestLine.indexOf(' ', pathStart);
    String path = requestLine.substring(pathStart, pathEnd);

    if (path.startsWith("/msg?")) {
      String text = getQueryValue(path, "text");
      setMatrixMessage(text.c_str());
      Serial.print("Matrix message changed to: ");
      Serial.println(currentMessage);
    }
  }

  sendControlResponse(client);
  delay(5);
  client.stop();
}

bool waitForWifiAndIp(unsigned long timeoutMs) {
  unsigned long startedAt = millis();

  while (millis() - startedAt < timeoutMs) {
    if (WiFi.status() == WL_CONNECTED && !ipIsUnset(WiFi.localIP())) {
      return true;
    }

    delay(500);
    Serial.print(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  return false;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  matrix.begin();
  delay(2000);

  Serial.println("Starting WiFi test");
  Serial.print("WiFi firmware: ");
  Serial.println(WiFi.firmwareVersion());

  WiFi.begin(ssid, pass);

  if (!waitForWifiAndIp(30000)) {
    Serial.println();
    Serial.println("WiFi did not get a usable IP address. Retrying once...");
    printWifiStatus();

    WiFi.disconnect();
    delay(1000);
    WiFi.begin(ssid, pass);
  }

  Serial.println();
  if (waitForWifiAndIp(30000)) {
    Serial.println("Connected with IP address.");
    ArduinoOTA.onStart(onOtaStart);
    ArduinoOTA.beforeApply(onOtaApply);
    ArduinoOTA.onError(onOtaError);
    ArduinoOTA.begin(WiFi.localIP(), "ArduinoOTA_Test", "password", InternalStorage);
    controlServer.begin();
    otaReady = true;
    setMatrixMessage(currentMessage);
    showReady();
    Serial.println("OTA ready.");
    Serial.print("Message control: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/msg?text=TATE");
  } else {
    Serial.println("Still connected without a usable DHCP lease.");
    showError();
  }
  printWifiStatus();
}


void loop() {
  ArduinoOTA.poll();
  handleControlClient();

  unsigned long now = millis();
  unsigned long blinkInterval = otaError ? 150 : 1000;

  if (otaReady && !otaInProgress && now - lastLedTick >= blinkInterval) {
    lastLedTick = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}
