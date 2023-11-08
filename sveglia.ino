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
#include "menu.h"

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

// Menu
MenuItem* currentMenu = nullptr;
byte isMenuOpen = false;

void BackCallback();

MenuItem mainMenu[] = {
  MenuItem("Back", BackCallback), MenuItem("Setup alarm"), MenuItem("Modify tmrw's alarm"), MenuItem("Change alarm sound"), MenuItem("Change LED settings"), MenuItem("Update time")
};
int menuOption = 0;

// Custom chars
byte downArrow[] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B11111,
  B01110,
  B00100
};

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

void BackCallback(){
  isMenuOpen = false;
  drawMainScreen();
}

void updateNTPTime() {
  Serial.println("Updating time...");
  Serial.printf("Was %02d:%02d:%02d", hours, minutes, seconds);

  connectWifi();
  timeClient.begin();
  do{
    timeClient.update();
    seconds = timeClient.getSeconds();
    minutes = timeClient.getMinutes();
    hours = timeClient.getHours();
    day = timeClient.getDay();
    Serial.printf("Is now %02d:%02d:%02d", hours, minutes, seconds);
  }while(hours == 1 && seconds == 6); // When the update fails it sets hours = 1 and seconds = 6 ???? I should change it later...


  timeClient.end();
  WiFi.disconnect();
}

void renderMenu(MenuItem* menu, int firstOption) {
  lcd.clear();
  for (int i = firstOption; i < firstOption + 4; i++) {
    lcd.setCursor(0, i - firstOption);
    lcd.print(mainMenu[i].getText().c_str());
    if (i == firstOption + 3) {
      lcd.setCursor(columns - 1, i - firstOption);
      lcd.write(0);
    }
  }
  lcd.cursor();
  lcd.setCursor(0, 0);
}

void drawMainScreen(){
  lcd.clear();
  lcd.noCursor();

  // The length of the time is always 8 chars with seconds
  lcd.setCursor(calculateCenterTextColumnStart(8), 0);
  lcd.printf("%02d:%02d:%02d", hours, minutes, seconds);
}

void setup() {
  Serial.begin(115200);  // Start serial communication at 115200 baud

  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(CLK, INPUT);
  pinMode(SW, INPUT_PULLUP);
  pinMode(DT, INPUT);

  // Pull-up
  digitalWrite(CLK, HIGH);
  digitalWrite(DT, HIGH);

  lcd.init();
  lcd.createChar(0, downArrow);
  toggleBacklight();
  centerPrint("Connecting...", 1);

  Serial.println("\nStart\n\n");
  Serial.println("Ciao");

  // Encoder
  attachInterrupt(digitalPinToInterrupt(14), encoderRotateInterrupt, FALLING);
}

ICACHE_RAM_ATTR void encoderRotateInterrupt() {
  if(isMenuOpen){
    if(digitalRead(DT)){
      if (menuOption > 0) menuOption--;
    }else{
      if(menuOption < 4) menuOption++;
    }
    lcd.setCursor(0, menuOption);
    delay(100);
  }
}


const int NTPUpdateMillis = 1000 * 60 * 15;  // Update every 15 minutes

long long int prev = 0;
long long int lastTimeUpdate = -NTPUpdateMillis;
long long int backlightTimer = millis();

void loop() {
  if(!isMenuOpen){
    // Turn off backlight after 10 seconds
    if (isBacklightOn && millis() - backlightTimer > 10000) {
      toggleBacklight();
    }
    // Time logic
    if (millis() - lastTimeUpdate > NTPUpdateMillis) {
      updateNTPTime();
      lastTimeUpdate = millis();
    }
  }else{
    backlightTimer = millis();
  }
  if (millis() - prev > 1000) {
    // One second has passed!
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

    if(!isMenuOpen){
      drawMainScreen();
    }
    prev = millis();
  }

  // Menu logic

  // Encoder press
  if (!digitalRead(SW)) {
    if (!isBacklightOn) {
      toggleBacklight();
      backlightTimer = millis();
    }else{
      if(!isMenuOpen){
        currentMenu = mainMenu;
        isMenuOpen = true;
        menuOption = 0;
        renderMenu(currentMenu, 0);
      }else{
        currentMenu[menuOption].executeCallback();
      }
    }
    delay(200);
  }
}