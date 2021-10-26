
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <time.h>
#include <sys/time.h>
#include "time.h"
#include <TZ.h>
#include "ArduinoJson.h"

#define MYTZ TZ_Europe_Warsaw
#define WiFiRetries 20

bool ledState = false;
unsigned long previousMillis = 0;  

static timeval tv;
static time_t now;

String host = "https://api.ipgeolocation.io/";
String sunsetApi = "astronomy?apiKey=xxx";
StaticJsonDocument<1024> sunsetResponse;

void vShowTime() {
  gettimeofday(&tv, nullptr);
  now = time(nullptr);
  Serial.printf("The current timestamp: %ju\n", (uintmax_t)now);
  // human readable
  Serial.println(ctime(&now));
}

void vBlinkLed() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;

    ledState = !ledState;
    digitalWrite(LED_BUILTIN, !ledState);
    digitalWrite(D3, ledState);
    vShowTime();
  }
}

void vSetupWifi() {
  uint8_t retries = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin("xxx", "xxx");
  // Wait for connection
  // TODO non-blocking plz
  while (WiFi.status() != WL_CONNECTED && retries <= WiFiRetries) {
    delay(500);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected, ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
     Serial.println("");
     Serial.print("Could not connect to WiFi.");
  }
}

void vSetupTime() {
  // TODO dynamic timezone
  configTime(MYTZ, "pool.ntp.org");
  Serial.printf("timezone:  %s\n", getenv("TZ") ? : "(none)");
  vShowTime();
}

void vGetSunsetData() {
  HTTPClient http;
  WiFiClientSecure client;
  DeserializationError error;

  client.setInsecure();
  client.connect(host, 443);

  http.begin(client, (host + sunsetApi).c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        error = deserializeJson(sunsetResponse, payload);
        const char* sunset = sunsetResponse["sunset"];
        Serial.printf("Sunset today: %s \n" , sunset);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      http.end();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D3, OUTPUT);

  Serial.begin(74880);
  Serial.println("Sunset WiFi Switch Init");

  vSetupWifi();
  if (WiFi.status() == WL_CONNECTED) {
    vSetupTime();
    vGetSunsetData();
  } else {
    Serial.println("Enabling default mode.");
    // TODO just turn on the switch
  }
}

void loop() {
  vBlinkLed();
}
