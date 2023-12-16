/*

  Author: Nicola Corato
  Email: nicolacorato05@gmail.com

*/
#define buzzerPin D7
#define SW D3
#define DT D6
#define CLK D5

#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include "secrets.h"
#include "menu.h"
#include "alarms.h"

const char* SSID = SECRET_SSID;
const char* PASSWD = SECRET_PASSWD;

// Time zone offset
const long utcOffsetInSeconds = 3600;

// Global time variables
byte seconds = 0;
byte minutes = 0;
byte hours = 0;
byte day = 0;
unsigned long ntpEpochTime;
unsigned long ntpStart;

char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// For unset hour set to 255
byte alarmTimes[7][2] = { {255,255}, {255,255}, {255,255}, {255,255}, {255,255}, {255,255}, {255,255} };
byte nextAlarm[2] = {255, 255};
int nextDay = -1;

bool alarmSounded = false;
int selectedAlarm = 0;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);

// LCD setup
const byte columns = 20;
const byte rows = 4;
LiquidCrystal_I2C lcd(0x27, columns, rows);
byte isBacklightOn = 0;
long long int backlightTimer = millis();

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

bool saveAlarmsToEEPROM(){
  EEPROM.put(0, alarmTimes);
  EEPROM.put(16, nextAlarm);
  EEPROM.put(19, nextDay);
  return EEPROM.commit();
}

void loadAlarmsFromEEPROM(){
  EEPROM.get(0, alarmTimes);
  EEPROM.get(16, nextAlarm);
  EEPROM.get(19, nextDay);
}

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
  if(WiFi.status() != WL_CONNECTED){
    WiFi.begin(SSID, PASSWD);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.print("Connected to ");
    Serial.println(SSID);
  }
}

void closeMenu(){
  isMenuOpen = false;
  if(((ntpEpochTime + ((millis() - ntpStart) / 1000)) / 60 ) % 60 != minutes){
    minutes = ((ntpEpochTime + ((millis() - ntpStart) / 1000)) / 60 ) % 60;
    minuteChange();
  }
  drawMainScreen();
}

void updateNTPTime() {
  Serial.println("Updating time...");
  Serial.printf("Was %02d:%02d:%02d", hours, minutes, seconds);

  connectWifi();
  timeClient.begin();
  while(!timeClient.update()){
    delay(50);
    if(timeClient.forceUpdate()){   // Try to force the update since it only updates every 60 secs
      break;
    }
  } // Retry until the update succeds
  seconds = timeClient.getSeconds();
  minutes = timeClient.getMinutes();
  hours = timeClient.getHours();
  day = timeClient.getDay();
  ntpEpochTime = timeClient.getEpochTime();
  ntpStart = millis();
  Serial.printf("Is now %02d:%02d:%02d", hours, minutes, seconds);

  timeClient.end();
}

void renderMenu(MenuItem* menu, int firstOption, bool resetCursor = true) {
  lcd.clear();
  int bound = currentMenuLength > 4 ? (firstOption + 4) : currentMenuLength;
  for (int i = firstOption; i < bound; i++) {
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

int* getNextAlarmTime(){
  bool set = false;

  int* ret = (int*)malloc(sizeof(int) * 3);
  for(int i = 0; i < 7; i++){
    int alarmHour, alarmMinutes, alarmDay;

    alarmDay = (i + day) % 7;
    alarmHour = alarmTimes[alarmDay][0];
    alarmMinutes = alarmTimes[alarmDay][1];
    if(alarmHour != 255 && (timeEquals(hours, minutes, alarmHour, alarmMinutes) == -1 || alarmDay != day)){
      ret[0] = alarmDay;
      ret[1] = alarmHour;
      ret[2] = alarmMinutes;
      set = true;
      break;
    }
  }

  if((
    (!set) || 
    (((nextDay < ret[0]) || (ret[0] < day)) && nextDay >= day) ||
    (nextDay == ret[0] && timeEquals(nextAlarm[0], nextAlarm[1], ret[1], ret[2]) == 1)
  ) && (nextDay != -1)){

    ret[0] = nextDay;
    ret[1] = nextAlarm[0];
    ret[2] = nextAlarm[1];
    set = true;
  }

  if(!set){
    ret[0] = 255;
  }
  
  return ret;
}

void drawMainScreen(){
  lcd.clear();
  lcd.noCursor();

  // The length of the time is always 8 chars with seconds
  lcd.setCursor(calculateCenterTextColumnStart(8), 0);
  lcd.printf("%02d:%02d:%02d", hours, minutes, seconds);


  int* nextAlarm = getNextAlarmTime();
  if(nextAlarm[0] != 255){
    centerPrint("Next alarm:", 2);
  
    char buffer[columns + 1];
    sprintf(buffer, "%02d:%02d %s", nextAlarm[1], nextAlarm[2], daysOfTheWeek[nextAlarm[0]]);
    centerPrint(buffer, 3);
  }
  free(nextAlarm);
}

void changeMenu(MenuItem* menu, int length){
  currentMenu = menu;
  currentMenuLength = length;
  isMenuOpen = true;
  menuOption = 0;
  firstMenuOption = 0;
  renderMenu(currentMenu, 0);
}

void minuteChange(){
  alarmSounded = false;
  if (minutes == 60) {
    minutes = 0;
    hours += 1;
    if (hours == 24) {
      hours = 0;
      day = (day + 1) % 7;
    }
  }
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

void playAlarm(){
  MenuItem* prev;
  int length;
  if(isMenuOpen){
    prev = currentMenu;
    length = currentMenuLength;
  }
  alarmSounded = true;

  isBacklightOn = true;
  backlightTimer = millis();
  lcd.backlight();
  lcd.clear();
  centerPrint("WAKE UP!", 1);

  do{
    alarmArray[selectedAlarm]();
  }while(digitalRead(SW));

  if(isMenuOpen){
    changeMenu(prev, length);
  }
}

int* selectAlarmTime(){
  int* ret = (int*)malloc(sizeof(int) * 2);
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
    delay(50);
    genericDelay = false;
  }while(digitalRead(SW));
  genericCouter = 0;
  delay(100);
  
  // Minutes
  do{
    if(genericCouter < 0){
      genericCouter = 59;
    }
    min = genericCouter % 60;
    sprintf(buffer, "%02d:%02d", h, min);
    centerPrint(buffer, 1);
    delay(50);
    genericDelay = false;
  }while(digitalRead(SW));

  genericCount = false;
  ret[0] = h;
  ret[1] = min;

  return ret;
}

int timeEquals(int h1, int min1, int h2, int min2){
  if(h1 == h2){
    if(min1 == min2){
      return 0;
    }else if(min1 > min2){
      return 1;
    }else{
      return -1;
    }
  }else if(h1 > h2){
    return 1;
  }else{
    return -1;
  }
}

void changeToMainMenu();

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

byte tempAlarm;
void confirmAlarmCallback(){
  if(menuOption == 0){
    selectedAlarm = tempAlarm;
    // Save to EEPROM

  }
  changeToMainMenu();
}

void testAlarmCallback(){
  int temp = selectedAlarm;
  selectedAlarm = tempAlarm;
  playAlarm();
  selectedAlarm = temp;
}

MenuItem alarmConfirmMenu[3] = { MenuItem("Set", confirmAlarmCallback), MenuItem("Cancel", confirmAlarmCallback), MenuItem("Test", testAlarmCallback) };

void changeAlarmSoundCallback(){
  tempAlarm = menuOption;
  changeMenu(alarmConfirmMenu, 3);  
}

const byte alarmSelectMenuLength = 2;
MenuItem alarmSelectMenu[alarmSelectMenuLength] = { MenuItem("Default alarm", changeAlarmSoundCallback), MenuItem("Rapid fire alarm", changeAlarmSoundCallback)};

void changeAlarmCallback(){
  changeMenu(alarmSelectMenu, alarmSelectMenuLength);
}

MenuItem alarmMenu[] = {
  MenuItem("Back", alarmMenuBackCallback), MenuItem("Weekdays", setupAlarmDayCallback), MenuItem("Weekend", setupAlarmDayCallback), MenuItem("Monday", setupAlarmDayCallback), 
  MenuItem("Tuesday", setupAlarmDayCallback),  MenuItem("Wednesday", setupAlarmDayCallback), MenuItem("Thursday", setupAlarmDayCallback), 
  MenuItem("Friday", setupAlarmDayCallback), MenuItem("Saturday", setupAlarmDayCallback),   MenuItem("Sunday", setupAlarmDayCallback)
};

MenuItem removeAlarmMenu[] = {
  MenuItem("Back", alarmMenuBackCallback), MenuItem("Weekdays", removeAlarmDayCallback), MenuItem("Weekend", removeAlarmDayCallback), MenuItem("Monday", removeAlarmDayCallback), 
  MenuItem("Tuesday", removeAlarmDayCallback),  MenuItem("Wednesday", removeAlarmDayCallback), MenuItem("Thursday", removeAlarmDayCallback), 
  MenuItem("Friday", removeAlarmDayCallback), MenuItem("Saturday", removeAlarmDayCallback),   MenuItem("Sunday", removeAlarmDayCallback)
};

void setupAlarmCallback(){
  changeMenu(alarmMenu, 10);
}

void setupAlarmDayCallback(){
  int selectedDay = menuOption - 1;
  int* time = selectAlarmTime();
  int h = time[0];
  int min = time[1];

  free(time);

  // Set the alarm and save to flash
  switch(selectedDay){
    case 0:
      for (int i = 0; i < 5; i++) {
        alarmTimes[i + 1][0] = h;
        alarmTimes[i + 1][1] = min;
      }
      break;
    case 1:
      alarmTimes[6][0] = h;
      alarmTimes[6][1] = min;
      alarmTimes[0][0] = h;
      alarmTimes[0][1] = min;
      break;
    case 2: case 3: case 4: case 5: case 6: case 7: case 8:
      alarmTimes[(selectedDay - 2 + 1) % 7][0] = h;
      alarmTimes[(selectedDay - 2 + 1) % 7][1] = min;
      break;
    default:
      Serial.println("How did we even get to this index??");
      exit(999);
      break;
  }

  saveAlarmsToEEPROM();
  Serial.println("Saved alarms to flash!");

  changeMenu(alarmMenu, 10);
}

void nextAlarmDaySelectCallback();

MenuItem nextAlarmTempMenu[2] = { MenuItem("Today", nextAlarmDaySelectCallback), MenuItem("Tomorrow", nextAlarmDaySelectCallback) };

void nextAlarmCallback(){
  int* time = selectAlarmTime();
  int h = time[0];
  int min = time[1];

  nextAlarm[0] = h;
  nextAlarm[1] = min; 
  free(time);

  changeMenu(nextAlarmTempMenu, 2);
}

//       |
//       |
//       ↓

void nextAlarmDaySelectCallback(){
  if(menuOption == 0){
    nextDay = day;
  }else{
    nextDay = (day + 1) % 7;
  }

  saveAlarmsToEEPROM();
  changeToMainMenu();
}

const byte mainMenuLength = 8;
MenuItem mainMenu[mainMenuLength] = {
  MenuItem("Back", closeMenu), MenuItem("Setup alarm", setupAlarmCallback), MenuItem("Modify next alarm", nextAlarmCallback), MenuItem("Remove next alarm", removeNextAlarmCallback), MenuItem("Remove alarm", removeAlarmCallback),
   MenuItem("Change alarm sound", changeAlarmCallback), MenuItem("Update time", updateTimeCallback), MenuItem("Test alarm", playAlarm)
};

void alarmMenuBackCallback(){
  changeMenu(mainMenu, mainMenuLength);
}

void removeAlarmCallback(){
  changeMenu(removeAlarmMenu, 10);
}

void removeAlarmDayCallback(){
  int selectedDay = menuOption - 1;

  // Set the alarm and save to flash
  switch(selectedDay){
    case 0:
      for (int i = 0; i < 5; i++) {
        alarmTimes[i + 1][0] = 255;
        alarmTimes[i + 1][1] = 255;
      }
      break;
    case 1:
      alarmTimes[6][0] = 255;
      alarmTimes[6][1] = 255;
      alarmTimes[0][0] = 255;
      alarmTimes[0][1] = 255;
      break;
    case 2: case 3: case 4: case 5: case 6: case 7: case 8:
      alarmTimes[(selectedDay - 2 + 1) % 7][0] = 255;
      alarmTimes[(selectedDay - 2 + 1) % 7][1] = 255;
      break;
    default:
      Serial.println("How did we even get to this index??");
      exit(999);
      break;
  }

  saveAlarmsToEEPROM();
  Serial.println("Saved alarms to flash!");

  lcd.clear();
  lcd.noCursor();
  centerPrint("Alarm removed", 1);
  delay(1000);

  changeMenu(alarmMenu, 10);
}

void removeNextAlarmCallback(){
  nextDay = -1;
  nextAlarm[1] = 255;
  nextAlarm[2] = 255;

  saveAlarmsToEEPROM();

  lcd.clear();
  lcd.noCursor();
  centerPrint("Next alarm removed", 1);
  delay(1000);

  changeToMainMenu();
}

void changeToMainMenu(){
  changeMenu(mainMenu, mainMenuLength);
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
  EEPROM.begin(64);

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
  Serial.print("\n\nStarting...");

  loadAlarmsFromEEPROM();
  Serial.println("Loaded alarms from EEPROM");

  // TODO: Remove and load from memory tomorrow's alarm
  for(int i = 0; i < 7; i++){
    Serial.printf("%d: %s -> hour: %d minute: %d\n", i, daysOfTheWeek[i], alarmTimes[i][0], alarmTimes[i][1]);
  }
  Serial.printf("Next alarm -> h: %d min: %d\nNext day %d\n", nextAlarm[0], nextAlarm[1], nextDay);

  // Encoder
  attachInterrupt(digitalPinToInterrupt(14), encoderRotateInterrupt, FALLING);
  
}

const int NTPUpdateMillisDelay = 1000 * 60 * 10;  // Update every 10 minutes

long long int prev = 0;
long long int lastTimeUpdate = -NTPUpdateMillisDelay;
byte prevSeconds = -1;

void loop() {
  // It's time
  if((alarmTimes[day][0] == hours && alarmTimes[day][1] == minutes && nextDay != day) || (nextAlarm[0] == hours && nextAlarm[1] == minutes && nextDay == day)){
    if(!alarmSounded){
      playAlarm();
    }
    if(nextAlarm[0] != 255 || nextAlarm[1] != 255 || nextDay != -1){
      nextAlarm[0] = 255;
      nextAlarm[1] = 255;
      nextDay = -1;
      saveAlarmsToEEPROM();
    }
  }else{
    alarmSounded = false;
  }

  if(!isMenuOpen){
    // Turn off backlight after 10 seconds
    if (isBacklightOn && millis() - backlightTimer > 10000) {
      toggleBacklight();
    }
    // Time logic
    if (millis() - lastTimeUpdate > NTPUpdateMillisDelay) {
      updateNTPTime();
      lastTimeUpdate = millis();
    }
  }else{
    backlightTimer = millis();
  }
  if (millis() - prev > 1000) {
    // One second has passed!

    seconds = (ntpEpochTime + ((millis() - ntpStart) / 1000)) % 60;
    if(prevSeconds == -1){
      prevSeconds = seconds;
    }

    if(seconds != prevSeconds){
      // One seconds has passed
      if(prevSeconds == 59){
        minutes += 1;
        minuteChange();
      }
      prevSeconds = seconds;
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
        changeMenu(mainMenu, mainMenuLength);
      }else{
        currentMenu[menuOption].executeCallback();
      }
    }
    delay(200);
  }
}