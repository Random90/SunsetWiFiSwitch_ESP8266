
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <time.h>
#include <sys/time.h>
#include "time.h"
#include <TZ.h>
#include "ArduinoJson.h"
#include <time.h>

#define MYTZ TZ_Europe_Warsaw
#define WiFiRetries 20
#define WiFiRetryDelay 500
#define turnoffTime "00:05"

bool ledState = false;
unsigned long previousMillis = 0;  

static time_t now;
static time_t sunsetTime;

String host = "https://api.ipgeolocation.io/";
String sunsetApi = "astronomy?apiKey=xxx";
StaticJsonDocument<1024> sunsetResponse;

void vShowTime() {
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
  }
}

void vSetupWifi() {
  uint8_t retries = 0;
  WiFi.mode(WIFI_STA);
  WiFi.begin("xxx", "xxx");
  // Wait for connection
  // TODO non-blocking plz
  while (WiFi.status() != WL_CONNECTED && retries <= WiFiRetries) {
    delay(WiFiRetryDelay);
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

void vTurnOffWifi() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  printf("Wifi turned off\n");
}

void vSetupTime() {
  // TODO dynamic timezone
  configTime(MYTZ, "pool.ntp.org");
  Serial.printf("timezone:  %s\n", getenv("TZ") ? : "(none)");
}

// sunset time in format H:M
time_t tConvertHourToTimestamp(const char* sunsetTime) {
  int hour = atoi(sunsetTime);
  int minute = atoi(sunsetTime + 3);
  struct tm tm;
  now = time(nullptr);
  tm = *localtime(&now);
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = 0;
  return mktime(&tm);
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
        // print converted sunset time
        sunsetTime = tConvertHourToTimestamp(sunset);
        Serial.printf("Sunset timestamp: %lu \n", (long unsigned int)sunsetTime);
        vShowTime();
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }

      http.end();
}

void vToggleOutput() {
  now = time(nullptr);
  if (now >= sunsetTime) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(D3, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(D3, LOW);
  }
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
    vToggleOutput();
  } else {
    Serial.println("Enabling default mode.");
    digitalWrite(D3, HIGH);
  }
  vTurnOffWifi();
}

void loop() {
  vBlinkLed();
}


