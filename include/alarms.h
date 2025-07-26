#ifndef ALARMS_H

#define ALARMS_H
#define buzzerPin D7
#define NOTE_BASE 4000

void millisDelay(int t){
  unsigned long temp = millis();
  while(millis() - temp < (unsigned long)t && digitalRead(SW)) {
  }
  return;
}

void note(int note, int duration = 200, int pause = 20){
  tone(buzzerPin, NOTE_BASE + 500 * note, duration);
  millisDelay(duration + pause);
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

void complexPresents(){
  note(-3);
  note(0, 200, 40);
  note(0);
  note(1);
  note(2);
  note(0);
  note(2);
  note(3);
  note(4, 200, 40);
  note(4);
  note(3);
  note(2);
}

void (*alarmArray[])(void) = { &defaultAlarm, &rapidFireAlarm, &unevenAlarm, &scaleAlarm, &doubleToneAlarm, &complexPresents };
#endif