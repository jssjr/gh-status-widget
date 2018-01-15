#include "GithubStatus.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#ifndef DEBUG
//#define DEBUG 1
#endif

bool GithubStatus::update() {
  WiFiClientSecure client;

#ifdef DEBUG
  const char* host = "192.168.1.35";
  const int httpsPort = 8443;
  const char* fingerprint = "DF 8A 60 EF 8B 2A 99 84 1A B4 29 FF A1 64 1A E2 8A 3F AA 7F";
#else
  const char* host = "status.github.com";
  const int httpsPort = 443;
  const char* fingerprint = "54 16 0F E9 6D DD E4 A8 87 49 57 99 BB 01 22 B4 82 BA CA FA";
#endif

  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return false;
  }

#ifndef DEBUG
  if (client.verify(fingerprint, host)) {
    Serial.println("certificate matches");
  } else {
    Serial.println("certificate does not match");
    return false;
  }
#endif

  String url = "/api/last-message.json";
  client.print(
      String("GET ") + url + " HTTP/1.1\r\n" +
      "Host: " + host + "\r\n" +
      "User-Agent: gh-status-gadget\r\n" +
      "Connection: close\r\n\r\n");
  Serial.println("request sent");

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }

  String line;
  client.setTimeout(500);

  while (client.available()) {
    line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers recieved");
      break;
    }
  }

  line = "";
  if (client.available()) {
    line += client.readStringUntil('\r');
  } else {
    line = "------------------------------";
  }
  Serial.println("reply was:");
  Serial.println("==========");
  Serial.println(line);
  Serial.println("==========");
  Serial.println("closing connection");

  parseJSON(line);
  return true;
}

void GithubStatus::parseJSON(String data) {
  ArduinoJson::DynamicJsonBuffer jsonBuffer;
  ArduinoJson::JsonObject& root = jsonBuffer.parseObject(data);

  status = root[String("status")].as<String>();
  return;

  if (data[12] == 'o') {
    status = String("good");
  } else if (data[12] == 'i') {
    status = String("minor");
  } else if (data[12] == 'a') {
    status = String("major");
  } else {
    status = String("unknown");
  }
  return;
}

String GithubStatus::getStatus() {
  return status;
}
