#include "Web.h"
#include <WiFiS3.h>
#include "Display.h"
#include "Sync.h"

static WiFiServer controlServer(80);

static int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static String urlDecode(String value) {
  String decoded;
  for (int i = 0; i < (int)value.length(); i++) {
    char c = value.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%' && i + 2 < (int)value.length()) {
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

static String getQueryValue(String path, const char* key) {
  String marker = String(key) + "=";
  int start = path.indexOf(marker);
  if (start < 0) return "";
  start += marker.length();
  int end = path.indexOf('&', start);
  if (end < 0) end = path.length();
  return urlDecode(path.substring(start, end));
}

static void sendControlResponse(WiFiClient& client) {
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
  syncWritePeersHtml(client);
  client.println("</body></html>");
}

void webBegin() {
  controlServer.begin();
}

void webPoll() {
  WiFiClient client = controlServer.available();
  if (!client) return;

  String requestLine = client.readStringUntil('\n');
  requestLine.trim();

  while (client.connected()) {
    String header = client.readStringUntil('\n');
    header.trim();
    if (header.length() == 0) break;
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
      broadcastMessage(currentMessage);
    } else if (path == "/peers.json" || path.startsWith("/peers.json?")) {
      // CORS: only allow origins that browsers serialize as "null"
      // (file:// pages, sandboxed iframes). Tightens the previous "*" wildcard
      // so a random website on the LAN cannot fingerprint the cluster.
      // The bundled led_message_controller.html is opened from disk, so its
      // Origin header is "null" and the page works under this policy.
      client.println("HTTP/1.1 200 OK");
      client.println("Connection: close");
      client.println("Content-Type: application/json");
      client.println("Access-Control-Allow-Origin: null");
      client.println();
      syncWritePeersJson(client);
      delay(5);
      client.stop();
      return;
    }
  }

  sendControlResponse(client);
  delay(5);
  client.stop();
}
