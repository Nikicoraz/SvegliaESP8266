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
byte alarmTimes[7][2] = { {0,0} };

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);

// LCD setup
const byte columns = 20;
const byte rows = 4;
LiquidCrystal_I2C lcd(0x27, columns, rows);
byte isBacklightOn = 0;

// Menu
MenuItem* currentMenu = nullptr;
int currentMenuLength;
int menuOption = 0;
int firstMenuOption = 0;
byte isMenuOpen = false;
int genericCouter;
bool genericCount = false;
bool genericDelay = false;

//
// --- GENERAL FUNCTIONS ---
//

void millisDelay(int delay){
  unsigned long temp = millis();
  while(temp - millis() < delay) {
  }
  return;
}

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

void closeMenu(){
  isMenuOpen = false;
  drawMainScreen();
}

void updateNTPTime() {
  Serial.println("Updating time...");
  Serial.printf("Was %02d:%02d:%02d", hours, minutes, seconds);

  connectWifi();
timeClient.begin();
  while(!timeClient.update()){} // Retry until the update succeds
  seconds = timeClient.getSeconds();
  minutes = timeClient.getMinutes();
  hours = timeClient.getHours();
  day = timeClient.getDay();
  Serial.printf("Is now %02d:%02d:%02d", hours, minutes, seconds);

  timeClient.end();
  WiFi.disconnect();
}

void renderMenu(MenuItem* menu, int firstOption, bool resetCursor = true) {
  lcd.clear();
  for (int i = firstOption; i < firstOption + 4; i++) {
    lcd.setCursor(0, i - firstOption);
    lcd.print(menu[i].getText().c_str());
    if (i == firstOption + 3) {
      lcd.setCursor(columns - 1, i - firstOption);
      lcd.write(0);
    }
  }
  if(resetCursor){
    lcd.cursor();
    lcd.setCursor(0, 0);
  }
}

void drawMainScreen(){
  lcd.clear();
  lcd.noCursor();

  // The length of the time is always 8 chars with seconds
  lcd.setCursor(calculateCenterTextColumnStart(8), 0);
  lcd.printf("%02d:%02d:%02d", hours, minutes, seconds);
}

void changeMenu(MenuItem* menu, int length){
  currentMenu = menu;
  currentMenuLength = length;
  isMenuOpen = true;
  menuOption = 0;
  renderMenu(currentMenu, 0);
}

ICACHE_RAM_ATTR void encoderRotateInterrupt() {
  if(genericCount){
    if(!genericDelay){    // This is because you can't do delays in functions nested too deep :(
      if(digitalRead(DT)){
        genericCouter--;
      }else{
        genericCouter++;
      }
      genericDelay = true;
    }
  }else if(isMenuOpen){
    // Change the current menu option
    if(digitalRead(DT)){
      if (menuOption > 0) menuOption--;
    }else{
      if(currentMenuLength - menuOption - 1 > 0){
        menuOption++;
      }
    }
    
    // Render menu based on the first menu option
    if(menuOption - firstMenuOption >= rows){
      firstMenuOption++;
      renderMenu(currentMenu, firstMenuOption, false);
    }else if(menuOption < firstMenuOption){
      firstMenuOption--;
      renderMenu(currentMenu, firstMenuOption, false);
    }
    lcd.setCursor(0, menuOption - firstMenuOption);
    delay(100);
  }
}

int abs(int x){
  return x > 0 ? x : -x;
}

// 
//  --- CALLBACKS ---
//

void updateTimeCallback(){
  lcd.clear();
  lcd.noCursor();
  centerPrint("Updating time...", 1);

  updateNTPTime();
  closeMenu();
}


MenuItem alarmMenu[] = {
  MenuItem("Back", alarmMenuBackCallback), MenuItem("Weekdays", setupAlarmDayCallback), MenuItem("Weekend", setupAlarmDayCallback), MenuItem("Monday", setupAlarmDayCallback), 
  MenuItem("Tuesday", setupAlarmDayCallback),  MenuItem("Wednesday", setupAlarmDayCallback), MenuItem("Thursday", setupAlarmDayCallback), 
  MenuItem("Friday", setupAlarmDayCallback), MenuItem("Saturday", setupAlarmDayCallback),   MenuItem("Sunday", setupAlarmDayCallback)
};

void setupAlarmCallback(){
  changeMenu(alarmMenu, 10);
}

void setupAlarmDayCallback(){
  int selectedDay = menuOption;
  genericCount = true;
  genericCouter = 0;
  lcd.clear();
  lcd.noCursor();
  centerPrint("Alarm time");
  delay(200);

  int min = 0, h = 0;
  char buffer[5]; // "0" "0" ":" "0" "0" "\0"
  // Hour
  do{
    if(genericCouter < 0){
      genericCouter = 23;
    }
    h = genericCouter % 24;
    sprintf(buffer, "%02d:%02d", h, min);
    centerPrint(buffer, 1);
    delay(200);
    genericDelay = false;
  }while(digitalRead(SW));
  genericCouter = 0;
  
  // Minutes
  do{
    if(genericCouter < 0){
      genericCouter = 23;
    }
    min = genericCouter % 60;
    sprintf(buffer, "%02d:%02d", h, min);
    centerPrint(buffer, 1);
    delay(200);
    genericDelay = false;
  }while(digitalRead(SW));

  genericCount = false;
  // TODO: Set the alarm and save to flash

  changeMenu(alarmMenu, 10);
}

MenuItem mainMenu[] = {
  MenuItem("Back", closeMenu), MenuItem("Setup alarm", setupAlarmCallback), MenuItem("Modify tmrw's alarm"), MenuItem("Change alarm sound"), MenuItem("Change LED settings"), MenuItem("Update time", updateTimeCallback)
};

void alarmMenuBackCallback(){
  changeMenu(mainMenu, 6);
}

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

const int NTPUpdateMillis = 1000 * 60 * 10;  // Update every 10 minutes

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
        changeMenu(mainMenu, 6);
      }else{
        currentMenu[menuOption].executeCallback();
      }
    }
    delay(200);
  }
}