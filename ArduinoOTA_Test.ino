#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include "Arduino_LED_Matrix.h"

char ssid[] = "Lucy";
char pass[] = "sweetsweet2017";

ArduinoLEDMatrix matrix;

bool otaReady = false;
bool otaInProgress = false;
bool otaError = false;
unsigned long lastLedTick = 0;
bool ledState = LOW;

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

void showReady() {
  matrix.renderBitmap(readyFrame, 8, 12);
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
    otaReady = true;
    showReady();
    Serial.println("OTA ready.");
  } else {
    Serial.println("Still connected without a usable DHCP lease.");
    showError();
  }
  printWifiStatus();
}


void loop() {
  ArduinoOTA.poll();

  unsigned long now = millis();
  unsigned long blinkInterval = otaError ? 150 : 1000;

  if (otaReady && !otaInProgress && now - lastLedTick >= blinkInterval) {
    lastLedTick = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}
