
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
bool defaultMode = false;
unsigned long previousMillis = 0;

static time_t now;
static time_t sunsetTime;
static time_t turnoffTime;

String host = "https://api.ipgeolocation.io/";
StaticJsonDocument<1024> sunsetResponse;

void vShowTime()
{
  now = time(nullptr);
  Serial.printf("The current timestamp: %ju\n", (uintmax_t)now);
  // human readable
  Serial.println(ctime(&now));
}

void vBlinkLed()
{
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 1000)
  {
    previousMillis = currentMillis;

    ledState = !ledState;
    digitalWrite(LED_BUILTIN, !ledState);
  }
}

void vSetupWifi()
{
  uint8_t retries = 0;
  Serial.println("Connneting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  // Wait for connection
  // TODO non-blocking plz?
  while (WiFi.status() != WL_CONNECTED && retries <= WiFiRetries)
  {
    delay(WiFiRetryDelay);
    Serial.print(".");
    retries++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.print("Connected, ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    defaultMode = false;
  }
  else
  {
    Serial.println("");
    Serial.print("Could not connect to WiFi.");
  }
}

void vTurnOffWifi()
{
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  printf("Wifi turned off\n");
}

void vSetupTime()
{
  uint8_t retries = 0;
  // TODO dynamic timezone
  configTime(MYTZ, "pool.ntp.org");
  // Serial.printf("timezone:  %s\n", getenv("TZ") ?: "(none)");
  // wait for sntp to sync
  while ((int)time(nullptr) < 702243420 && retries <= 10)
  {
    if (retries == 0) Serial.println("Waiting for time sync");
    else Serial.print(".");
    delay(100);
    retries++;
  }
}

// sunset time in format H:M
time_t tConvertHourToTimestamp(const char *sunsetTime, bool forceNextDay)
{
  int hour = atoi(sunsetTime);
  int minute = atoi(sunsetTime + 3);
  struct tm tm;
  now = time(nullptr);
  tm = *localtime(&now);
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = 0;
  // between those hours we mean the next day for sure
  if (forceNextDay || (hour >= 0 && hour < 12))
  {
    tm.tm_mday += 1;
  }
  return mktime(&tm);
}

void vGetSunsetData(bool nextDay)
{
  Serial.println("Retrieving sunset time...");
  HTTPClient http;
  WiFiClientSecure client;
  DeserializationError error;

  client.setInsecure();
  client.connect(host, 443);

  if (nextDay)
  {
    char nextDayDate[11];
    struct tm nextDayT;
    nextDayT = *localtime(&now);
    nextDayT.tm_mday += 1;
    strftime(nextDayDate, sizeof(nextDayDate), "%Y-%m-%d", &nextDayT);
    http.begin(client, (host + SUNSET_API + "&date=" + nextDayDate).c_str());
  }
  else
  {
    http.begin(client, (host + SUNSET_API).c_str());
  }

  int httpResponseCode = http.GET();

  if (httpResponseCode > 0)
  {
    String payload = http.getString();
    error = deserializeJson(sunsetResponse, payload);
    const char *sunset = sunsetResponse["sunset"];
    if (nextDay)
    {
      Serial.printf("Sunset tommorow: %s\n", sunset);
    }
    else
    {
      Serial.printf("Sunset today: %s \n", sunset);
    }
    // print converted sunset time
    sunsetTime = tConvertHourToTimestamp(sunset, nextDay);
    Serial.printf("Sunset timestamp: %lu \n", (long unsigned int)sunsetTime);
    vShowTime();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  // set the next turn off time
  turnoffTime = tConvertHourToTimestamp(turnoffTimeStr, false);
  Serial.printf("Turnoff timestamp: %lu \n", (long unsigned int)turnoffTime);
  now = time(nullptr);
}

void vToggleLed()
{
  if (now >= sunsetTime && now <= turnoffTime)
  {
    Serial.printf("%s Turning ON leds\n", ctime(&now));
    digitalWrite(D3, HIGH);
  }
  else if(!defaultMode)
  {
    Serial.printf("%s Turning OFF leds\n", ctime(&now));
    digitalWrite(D3, LOW);
  }
}

void vCheckNextToggleTime()
{
  now = time(nullptr);
  if (now >= turnoffTime)
  {
    vToggleLed();
    vSetupWifi();
    if (WiFi.status() == WL_CONNECTED)
    {
      // TODO it will work only if turnOff time is next day, TODO checking if it is next day
      vGetSunsetData(false);
      vTurnOffWifi();
    }
  } else if (now >= sunsetTime) {
    vToggleLed();
    vSetupWifi();
    if (WiFi.status() == WL_CONNECTED)
    {
      vGetSunsetData(true);
      vTurnOffWifi();
    }
  }
  
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D3, OUTPUT);

  Serial.begin(74880);
  Serial.println("Sunset WiFi Switch Init");

  vSetupWifi();
  if (WiFi.status() == WL_CONNECTED)
  {
    vSetupTime();
    vGetSunsetData(false);
    vToggleLed();
  }
  else
  {
    Serial.println("Enabling default mode (ON).");
    defaultMode = true;
    digitalWrite(D3, HIGH);
  }
  vTurnOffWifi();
}

void loop()
{
  vBlinkLed();
  vCheckNextToggleTime();
}
