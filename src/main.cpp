
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
#include "main.h"

#define MYTZ TZ_Europe_Warsaw
#define WiFiRetries 20
#define WiFiRetryDelay 500
#define turnoffTimeStr "00:05"

bool ledState = false;
unsigned long previousMillis = 0;  

static time_t now;
static time_t sunsetTime;
static time_t turnoffTime;

String host = "https://api.ipgeolocation.io/";
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
  Serial.println("Connneting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
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
  // between those hours we mean the next day for sure
  if (hour >= 0 && hour < 12) {
    tm.tm_mday += 1;
  }
  return mktime(&tm);
}

void vGetSunsetData() {
  Serial.println("Retrieving sunset time...");
  HTTPClient http;
  WiFiClientSecure client;
  DeserializationError error;

  client.setInsecure();
  client.connect(host, 443);

  http.begin(client, (host + SUNSET_API).c_str());
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

void vProcessLedStatus() {
  vSetupWifi();
  if (WiFi.status() == WL_CONNECTED) {
    vSetupTime();
    vGetSunsetData();
    // set the next turn off time
    turnoffTime = tConvertHourToTimestamp(turnoffTimeStr);
    Serial.printf("Turnoff timestamp: %lu \n", (long unsigned int)turnoffTime);
    now = time(nullptr);

    if (now >= sunsetTime) {
      Serial.printf("[%s] Turning on leds\n", ctime(&now));
      digitalWrite(D3, HIGH);
    } else {
      Serial.printf("[%s] Turning off leds\n", ctime(&now));
      digitalWrite(D3, LOW);
    }

  } else {
    Serial.println("Enabling default mode (ON).");
    digitalWrite(D3, HIGH);
  }
  vTurnOffWifi();
}

void vWaitForTurnOffTime() {
  now = time(nullptr);
  if (now > turnoffTime || now >= sunsetTime) {
    vProcessLedStatus();
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D3, OUTPUT);

  Serial.begin(74880);
  Serial.println("Sunset WiFi Switch Init");

  vProcessLedStatus();
  
}

void loop() {
  vBlinkLed();
  vWaitForTurnOffTime();
}


