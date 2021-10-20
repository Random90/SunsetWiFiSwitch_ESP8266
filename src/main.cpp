
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <time.h>
#include <sys/time.h>
#include "time.h"
#include <TZ.h>

#define MYTZ TZ_Europe_Warsaw

bool ledState = false;
unsigned long previousMillis = 0;  

static timeval tv;
static time_t now;

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
  WiFi.mode(WIFI_STA);
  WiFi.begin("xxx", "xxxx");
  // Wait for connection
  // TODO non-blocking plz
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected, ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void vSetupTime() {
  // TODO dynamic timezone
  configTime(MYTZ, "pool.ntp.org");
  Serial.printf("timezone:  %s\n", getenv("TZ") ? : "(none)");
  vShowTime();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D3, OUTPUT);

  Serial.begin(74880);
  Serial.println("Sunset WiFi Switch Init");

  vSetupWifi();
  vSetupTime();
}

void loop() {
  vBlinkLed();
}
