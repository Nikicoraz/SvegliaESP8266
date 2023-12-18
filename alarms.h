#ifndef ALARMS_H

#define ALARMS_H
#define buzzerPin D7


void millisDelay(int t){
  unsigned long temp = millis();
  while(millis() - temp < t && digitalRead(SW)) {
  }
  return;
}

void defaultAlarm(){
  tone(buzzerPin, 5000, 1000);
  millisDelay(1200);
}

void rapidFireAlarm(){
  tone(buzzerPin, 5000, 90);
  millisDelay(100);
}

void unevenAlarm(){
  tone(buzzerPin, 5000, 300);
  millisDelay(400);
  tone(buzzerPin, 500, 700);
  millisDelay(800);
}

void scaleAlarm(){
  for(int i = 0; i < 10; i++){
    tone(buzzerPin, 500 * i, 200);
    millisDelay(220);
  }
  for(int i = 9; i >= 0; i--){
    tone(buzzerPin, 500 * i, 200);
    millisDelay(220);
  }
}

void doubleToneAlarm(){
  tone(buzzerPin, 5000, 200);
  millisDelay(240);
  tone(buzzerPin, 5000, 200);
  millisDelay(500);
}

void (*alarmArray[])(void) = { &defaultAlarm, &rapidFireAlarm, &unevenAlarm, &scaleAlarm, &doubleToneAlarm };
#endif