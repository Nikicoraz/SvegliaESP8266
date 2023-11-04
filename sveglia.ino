/*

  Author: Nicola Corato
  Email: nicolacorato05@gmail.com

*/
#define ledPin D8
#define buzzerPin D7
#define SW D3
#define DT D6
#define CLK D5

#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include "secrets.h"
#include <WiFiUdp.h>

const char* SSID = SECRET_SSID;
const char* PASSWD = SECRET_PASSWD;

// Time zone offset
const long utcOffsetInSeconds = 3600;

// Global time variables
byte seconds = 0;
byte minutes = 0;
byte hours = 0;
byte day = 0;

char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);

// LCD setup
const byte columns = 20;
LiquidCrystal_I2C lcd(0x27, columns, 4);
byte isBacklightOn = 0;

int calculateCenterTextColumnStart(int length) {
  return (columns - length) / 2;
}

int calculateCenterTextColumnStart(String text) {
  return calculateCenterTextColumnStart(text.length());
}


void centerPrint(String text, int row = 0) {
  lcd.setCursor(calculateCenterTextColumnStart(text), row);
  lcd.print(text);
}

void toggleBacklight() {
  isBacklightOn ? lcd.noBacklight() : lcd.backlight();
  isBacklightOn = !isBacklightOn;
}

void connectWifi() {
  WiFi.begin(SSID, PASSWD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Connected to ");
  Serial.println(SSID);
}

void updateNTPTime() {
  Serial.println("Updating time...");
  Serial.printf("Was %02d:%02d:%02d", hours, minutes, seconds);

  connectWifi();
  timeClient.begin();
  timeClient.update();
  seconds = timeClient.getSeconds();
  minutes = timeClient.getMinutes();
  hours = timeClient.getHours();
  day = timeClient.getDay();

  Serial.printf("Is now %02d:%02d:%02d", hours, minutes, seconds);

  timeClient.end();
  WiFi.disconnect();
}

void setup() {
  Serial.begin(115200);  // Start serial communication at 115200 baud

  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(CLK, INPUT);
  digitalWrite(5, HIGH);
  pinMode(SW, INPUT_PULLUP);
  pinMode(DT, INPUT);
  digitalWrite(6, HIGH);

  attachInterrupt(digitalPinToInterrupt(14), encoderRotateInterrupt, FALLING);

  lcd.init();
  toggleBacklight();
  centerPrint("Connecting...", 1);

  Serial.println("\nStart\n\n");
  Serial.println("Ciao");
}

bool fired = false;
ICACHE_RAM_ATTR void encoderRotateInterrupt() {
  fired = true;
}


const int NTPUpdateMillis = 1000 * 60 * 15;  // Update every 15 minutes

long long int prev = 0;
long long int lastTimeUpdate = -NTPUpdateMillis;
long long int backlightTimer = millis();

void loop() {
  // Turn off backlight after 10 seconds
  if (isBacklightOn && millis() - backlightTimer > 10000) {
    toggleBacklight();
  }
  // Time logic
  if (millis() - lastTimeUpdate > NTPUpdateMillis) {
    updateNTPTime();
    lastTimeUpdate = millis();
  }
  if (millis() - prev > 1000) {
    // One second has passed

    digitalWrite(ledPin, !digitalRead(ledPin));
    seconds += 1;
    if (seconds == 60) {
      seconds = 0;
      minutes += 1;
      if (minutes == 60) {
        minutes = 0;
        hours += 1;
        if (hours == 24) {
          hours = 0;
          day = (day + 1) % 7;
        }
      }
    }

    lcd.clear();

    // The length of the time is always 8 chars with seconds
    lcd.setCursor(calculateCenterTextColumnStart(8), 0);
    lcd.printf("%02d:%02d:%02d", hours, minutes, seconds);
    prev = millis();
  }

  // Menu logic

  // Encoder press
  if (!digitalRead(SW)) {
    if (!isBacklightOn) {
      toggleBacklight();
    }
    backlightTimer = millis();
  }

  if (fired) {
    Serial.print("CLK: ");
    Serial.println(digitalRead(CLK));
    Serial.print("DT: ");
    Serial.println(digitalRead(DT));
    fired = false;
  }
}