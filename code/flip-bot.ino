#include <EEPROM.h>
#include <SPI.h>
#include <Servo.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <printf.h>
RF24 radio(7,8); // ce,cs
Servo SERVO;
#define buzzerPin 6
#define servoPin 9
#define buttonPin 2
const uint64_t addr = 0x0a0c0a0c0aLL;
const char on[] = "abcabcabc11";
const char off[] = "abcabcabc10";
const char oned[] = "abcabcabc110";
const char offed[] = "abcabcabc100";
const char onedHard[] = "abcabcabc111";
const char offedHard[] = "abcabcabc101";
char down[12],up[12];
const int onPos = 0,offPos = 100;
int tus,buzzerVal,pos,tagPos,serSpeed = 15;
unsigned long time1,time2;
//----------------------------------------------------------------------------------------
void readTus() {
  if (EEPROM.read(0) != 1)
  {
    EEPROM.update(0,1);
    EEPROM.update(1,1);
  }
  tus = EEPROM.read(1);
}
//---------------------------------------------------------------------
void ctrlServo() {
  /*
    Param: servoPin,tagPos,pos,serSpeed
  */
  if (pos != tagPos) {
    SERVO.attach(servoPin);
    if (pos < tagPos) {
      pos++;
      SERVO.write(pos);
    } else if (pos > tagPos) {
      pos--;
      SERVO.write(pos);
    }
  } else {
    SERVO.detach();
  }
}
//----------------------------------------------------------------------------------------
void ctrlBuzzer() {
  /*
   * Param: buzzerPin,buzzerVal
   */
  if (buzzerVal == 1)
  {
    analogWrite(buzzerPin, 255);
    buzzerVal = 0;
    Serial.println("BEEP");
  } else {
    analogWrite(buzzerPin, 0);
  }
}
//----------------------------------------------------------------------------------------
void sendOffEd() {
  radio.stopListening();
  radio.write(offed,sizeof(offed));
  radio.startListening();
}
//----------------------------------------------------------------------------------------
void sendOnEd() {
  radio.stopListening();
  radio.write(oned,sizeof(oned));
  radio.startListening();
}
//----------------------------------------------------------------------------------------
void sendOffEdHard() {
  radio.stopListening();
  radio.write(offedHard,sizeof(offedHard));
  radio.startListening();
}
//----------------------------------------------------------------------------------------
void sendOnEdHard() {
  radio.stopListening();
  radio.write(onedHard,sizeof(onedHard));
  radio.startListening();
}
//----------------------------------------------------------------------------------------
void downMsg() {
  memset(down,' ',sizeof(down));
  radio.read(down,sizeof(down));
  Serial.println(down);
}
//----------------------------------------------------------------------------------------
boolean isOnCmd() {
  if (strcmp(down,on) == 0) {
    return true;
  } else {
    return false;
  }
}
//----------------------------------------------------------------------------------------
boolean isOffCmd() {
  if (strcmp(down,off) == 0) {
    return true;
  } else {
    return false;
  } 
}
//----------------------------------------------------------------------------------------
boolean isOn() {
  if (tus == 1) {
    return true;
  } else {
    return false;
  }
}
//----------------------------------------------------------------------------------------
boolean isOff() {
  if (tus == 0) {
    return true;
  } else {
    return false;
  } 
}
//----------------------------------------------------------------------------------------
void onBot() {
  Serial.println("ON");
  tagPos = onPos;
  buzzerVal = 1;
  tus = 1;
  EEPROM.update(1,tus);
}
//----------------------------------------------------------------------------------------
void offBot() {
  Serial.println("OFF");
  tagPos = offPos;
  buzzerVal = 1;
  tus = 0;
  EEPROM.update(1,tus);
}
//----------------------------------------------------------------------------------------
void toggleBot() {
  if (isOn()) {
    offBot();
    sendOffEdHard();
  } else if (isOff()) {
    onBot();
    sendOnEdHard();
  }
}
//----------------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  SERVO.attach(servoPin);
  pos = SERVO.read();
  SERVO.detach();
  radio.begin();
  printf_begin();
  radio.printDetails();
  radio.openWritingPipe(addr);
  radio.openReadingPipe(1,addr);
  pinMode(buzzerPin, OUTPUT);
  radio.startListening();
  readTus();
  pinMode(buttonPin, INPUT_PULLUP);
  //attachInterrupt(0, toggleBot, FALLING);
}
//----------------------------------------------------------------------------------------
void loop() {
  if (radio.available())
  {
    downMsg();
    if (isOnCmd()) {
      if (isOff()) {
        onBot();
      }
      sendOnEd();
    } else if (isOffCmd()) {
      if (isOn()) {
        offBot();
      }
      sendOffEd();
    }
  }
  //----------------------------------------------------------------------------------------
  if ((unsigned long)(millis()-time1) > 100)
  {
    if (digitalRead(buttonPin) == LOW) {
      toggleBot();
    }
    ctrlBuzzer();
    time1 = millis();
  }
  //----------------------------------------------------------------------------------------
  if ( (unsigned long) (millis() - time2) > serSpeed)
  {
    ctrlServo();
    time2 = millis();
  }
}
