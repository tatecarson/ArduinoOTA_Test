#include <WiFiS3.h>
#include <ArduinoOTA.h>
#include "Display.h"
#include "Sync.h"
#include "Web.h"

static char ssid[] = "Lucy";
static char pass[] = "sweetsweet2017";

static bool otaReady = false;
static unsigned long lastLedTick = 0;
static bool ledState = LOW;

static bool ipIsUnset(IPAddress ip) {
  return ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
}

static void onOtaStart() {
  otaInProgress = true;
  digitalWrite(LED_BUILTIN, HIGH);
  showUpload();
  Serial.println("OTA upload started.");
}

static void onOtaApply() {
  Serial.println("OTA upload complete. Applying update...");
}

static void onOtaError(int code, const char* message) {
  otaError = true;
  otaInProgress = false;
  showError();
  Serial.print("OTA error ");
  Serial.print(code);
  Serial.print(": ");
  Serial.println(message);
}

static void printWifiStatus() {
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

static bool waitForWifiAndIp(unsigned long timeoutMs) {
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
  displayBegin();
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
    webBegin();
    syncBegin();
    otaReady = true;
    setMatrixMessage(currentMessage);
    showReady();
    Serial.println("OTA ready.");
    Serial.print("Node id: ");
    Serial.println(syncNodeId());
    Serial.print("Sync UDP port: ");
    Serial.println(SYNC_PORT);
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
  webPoll();
  if (otaReady) {
    syncPoll();
  }

  unsigned long now = millis();
  unsigned long blinkInterval = otaError ? 150 : 1000;

  if (otaReady && !otaInProgress && now - lastLedTick >= blinkInterval) {
    lastLedTick = now;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
  }
}
