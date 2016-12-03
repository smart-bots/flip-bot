#include <EEPROM.h>
#include <SPI.h>
#include <Servo.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <printf.h>

RF24 radio(7,8);
Servo SERVO;

#define nrfIrqPin 2
#define btnPin 3
#define buzzerPin 6
#define servoPin 9
#define onPosPin A0
#define offPosPin A1

const char token[] = "abcabcabc0"; // token

const uint64_t pipes[2] = {0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL};
// pipe.1: read | pipe.2: write -> oposite with hub

int onPos,offPos;

int state,buzzerVal,serPos,serTarPos,serSpeed = 15;

unsigned long time1,time2;

bool timeToSleep = false;

// state: 1 - 2 : off - on
struct recei {
  byte type;
  char token[11];
  short state;
} recei;

struct trans {
  byte type;
  char token[11];
  short state;
  float data;
} trans;

//----------------------------------------------------------------------------------------

void go_to_sleep() {
//  Serial.println("Sleeping...");
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
  sleep_disable();
}

//----------------------------------------------------------------------------------------

void handle_bot(short xState, byte type) {
  if (xState != state) {

    read_ser_pos();

    state = xState;
    
    if (state) {
//      Serial.println("ON");
      serTarPos = onPos;
    } else {
//      Serial.println("OFF");
      serTarPos = offPos;
    }
    
    buzzerVal = 1;
    
    EEPROM.update(1,state);

    timeToSleep = false;
  } else {
//    if (state) {
//      Serial.println("Already ON");
//    } else {
//      Serial.println("Already OFF");
//    }
  }
  
  if (type == 1) {
    trans.type = 3;
    
  } else {
    trans.type = 1;
  }
  strncpy(trans.token,token,10);
  trans.state = xState;
  trans.data = 0;

//  Serial.println("Sent: ");
//  Serial.println(trans.type);
//  Serial.println(trans.token);
//  Serial.println(trans.state);
//  Serial.println(trans.data);

  radio.stopListening();
  radio.write(&trans,sizeof(trans));
  radio.startListening();

//  Serial.println("Msg sent!");
//  Serial.println("Handled bot!");
}

//----------------------------------------------------------------------------------------

void check_radio(void) {
  
  bool tx,fail,rx;
  radio.whatHappened(tx,fail,rx);
  
  if (rx || radio.available()) {

    radio.read(&recei,sizeof(recei));

    Serial.println("Got -----------------");
    Serial.println(recei.type);
    Serial.println(recei.token);
    Serial.println(recei.state);
    Serial.println("---------------------");
 
    if (strcmp(recei.token,token) == 0) {
      handle_bot(recei.state,0);
    }
  }
}

//----------------------------------------------------------------------------------------

void toggle_bot() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time >= 200) 
  {
//    Serial.println("Btn pushed!");
    if (state == 1) {
      handle_bot(0,1);
    } else if (state == 0) {
      handle_bot(1,1);
    }
    last_interrupt_time = interrupt_time;
  }
}

//----------------------------------------------------------------------------------------

void read_state() {
  if (EEPROM.read(0) != 1)
  {
    EEPROM.update(0,1);
    EEPROM.update(1,1);
  }
  state = EEPROM.read(1);
//  Serial.print("Read state: ");
//  Serial.println(state);
}

void read_ser_pos() {
  onPos = map(analogRead(onPosPin),0,1022,0,180);
  offPos = map(analogRead(offPosPin),0,1022,0,180);
//  Serial.print("Read servo pos: ");
//  Serial.print(onPos);
//  Serial.print(" ");
//  Serial.println(offPos);
}

void setup()
{
  Serial.begin(9600);

  read_state();
  read_ser_pos();

  if (state) {
    SERVO.write(onPos);  
  } else {
    SERVO.write(offPos);
  }
  
  SERVO.attach(servoPin);
  SERVO.detach();

  radio.begin();
//  printf_begin();
//  radio.printDetails();
  radio.setAutoAck(true);
  radio.enableAckPayload();
  radio.enableDynamicPayloads();
  radio.openReadingPipe(1,pipes[0]);
  radio.openWritingPipe(pipes[1]);
  radio.startListening();

  pinMode(buzzerPin, OUTPUT);
  pinMode(btnPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(nrfIrqPin), check_radio, LOW);
  attachInterrupt(digitalPinToInterrupt(btnPin), toggle_bot, FALLING);
  
  timeToSleep = true;
}

//----------------------------------------------------------------------------------------

void handle_servo() {
  if (serPos != serTarPos) {
    SERVO.attach(servoPin);
    if (serPos < serTarPos) {
      serPos++;
      SERVO.write(serPos);
    } else if (serPos > serTarPos) {
      serPos--;
      SERVO.write(serPos);
    }
  } else {
    SERVO.detach();
    timeToSleep = true;
  }
}

//----------------------------------------------------------------------------------------

void handle_buzzer() {
  if (buzzerVal == 1)
  {
    analogWrite(buzzerPin, 255);
    buzzerVal = 0;
//    Serial.println("BEEP");
  } else {
    analogWrite(buzzerPin, 0);
  }
}

void loop() {
  //----------------------------------------------------------------------------------------
  if ((unsigned long)(millis()-time1) > 100)
  {
    handle_buzzer();
    time1 = millis();
  }
  //----------------------------------------------------------------------------------------
  if ( (unsigned long) (millis() - time2) > serSpeed)
  {
    handle_servo();
    time2 = millis();
  }
  if (timeToSleep) {
    delay(200);
    go_to_sleep();
  }
}

