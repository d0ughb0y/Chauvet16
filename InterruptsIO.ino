/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
volatile static uint16_t beepcount[6];
volatile static boolean buzzerbusy = false;
volatile boolean timerEnabled=false;

inline void enablePCINT(){
  PCICR |= (1<<PCIE2);
}

inline void disablePCINT(){
  PCICR &= ~(1<<PCIE2);
}

inline void initTimer() {
  OCR1C=128;
  TIMSK1 |= _BV(OCIE1C);  
}

void startTimer() {
  timerEnabled=true;
}

inline void chirp() {
  beepx(25,0,0,0,0);    
}

inline void chirp2() {
  beepx(25,25,0,25,0); 
}

#define beeplen 100

inline void beep() {
  beepx(beeplen,0,0,0,0); 
}

inline void beep2() {
  beepx(beeplen,beeplen,0,50,0); 
}

inline void beepOK() {
  beepx(50,50,200,50,0);
}

inline void beepFail() {
  beepx(200,200,50,50,0);  
}

void beepx(uint8_t first, uint8_t second, uint8_t third,
  uint8_t endpause, uint16_t cyclepause) {
  if (buzzerbusy) return;
  uint8_t SaveSREG = SREG;
  cli();
  beepcount[0]=first;
  beepcount[1]=beepcount[3]=endpause;
  beepcount[2]=second;
  beepcount[4]=third;
  beepcount[5]=cyclepause;
  buzzerbusy=true;
  SREG=SaveSREG;
}

inline void beepoff() {
  buzzerbusy=false;
  PORTF&=~_BV(PF0);
}

void alarmOn() {
 if (buzzerbusy) return;
 if (conf.soundalert) {
   uint8_t saveSREG=SREG;
   cli();
   beepcount[0]=200;
   beepcount[1]=50;
   beepcount[2]=200;
   beepcount[3]=50;
   beepcount[4]=200;
   beepcount[5]=2000;
   buzzerbusy=true;
   SREG=saveSREG; 
 }
}

ISR(PCINT2_vect) {
  uint8_t pins = PINK;
  static uint8_t lastPins;
  uint8_t mask = pins ^ lastPins;
  lastPins = pins;
#ifdef _FEEDER
  if (mask & _BV(PK2)) { //feeder
    feederHandler(pins);
  }
#endif
#ifdef _SONAR
  if (mask & _BV(PK5)) {
    sonarHandler(pins);
  }
#endif
}

ISR(TIMER1_COMPC_vect) { //interrupts every 1.02 ms
  static uint16_t counter = 0;
  static uint16_t outletmod = 10;
  //call 1ms tasks, make sure tasks executes in under 100us
  buzzerHandler();
  //ppumpHandler(); //perform dosing or water change
  if (!timerEnabled) return;

  if (counter%outletmod==0) {//10ms
    //call 10ms tasks, make sure tasks execute in under 1 ms
    outletmod=outletHandlerA();
    //after last outlet is processed, we make the next call after 1 full second
  }
  if (counter++%980==0) {//1second
    counter=1;
    //call 1 second tasks
    outletHandlerB();
  }  
}

void initBuzzer(){
  DDRD |= _BV(PD7);
  PORTD &= ~_BV(PD7);  
  buzzerbusy=false;
}

inline void buzzerHandler(){
  //buzzer pin D38 PD7
  static uint8_t state = 0;
  static uint16_t counter = 0;
  if (!buzzerbusy) {
    state=0;
    counter=0;
    return;
  }
  if (counter++ < beepcount[state]) {
    if (!(state%2))
      PORTD |= _BV(PD7);
  } else {
    if (state==5 && beepcount[5]==0) {
      buzzerbusy=false;
    }
    counter=0;
    state++;
    if (state>5) state=0;
    PORTD &= ~_BV(PD7);
  }
}
//////////////////////////////////////////
//   ATO
//////////////////////////////////////////
//use polling code for ATO
void initATO(){
  //ATO1 PK3 PCINT19
  //ATO2 PK4 PCINT20
  DDRK &= ~(_BV(PK3)|_BV(PK4)); //inputs
  PORTK |= (_BV(PK3) | _BV(PK4)); //pullups
}

inline uint8_t getATO1(){
  return PINK & _BV(PK3);
}

inline uint8_t getATO2() {
  return PINK & _BV(PK4);
}

void checkATO(){
  //this routine is for ATO using KALK reactor.
  //if you do not use KALK, just replace Kalk with the ATO pump
  //and remove the ph test condition
  if (conf.outletRec[Kalk].mode == _auto) {
    if (!getATO2()&& !getATO1() && isOutletOn(Return) && phavg<8.7
#ifdef _SONAR
        && sonaravg<conf.sonaralertval*10
#endif
      ) {
        _outletOn(Kalk);
        return;
    }
    _outletOff(Kalk);
  }
}

////////////////////////////////
//  PERISTALTIC PUMPS
////////////////////////////////
void initPeristalticPumps(){
  //PG0
  //PG2
  DDRG |= _BV(PG0)|_BV(PG2);
  ppump0Off();
  ppump1Off();
}

inline void ppump0On() {
  PORTG|=_BV(PG0);
}

inline void ppump0Off() {
  PORTG&=~_BV(PG0);
}

inline void ppump1On() {
  PORTG|=_BV(PG2);
}

inline void ppump1Off() {
  PORTG&=~_BV(PG2);
}

inline uint8_t getppump0(){
  return PINK & _BV(PK6);
}

inline uint8_t getppump1() {
  return PINK & _BV(PK7);
}

////////////////////////////////////
//  FEEDER
////////////////////////////////////
//#define feedOutPin 49 //PL0  PORTL
//#define feedInPin A10 //PK2 PCINT18

void initFeeder() {
  DDRK &= ~_BV(PK2);
  PORTK |= _BV(PK2);
  DDRL |= _BV(PL0);
  PORTL |= _BV(PL0);
  PCMSK2 |= (1<<PCINT18);
}

inline void feed(){
  if (!(~PORTL & _BV(PL0))) {
    PORTL &= ~_BV(PL0);
    _outlogentry(Feeder,true);
  }
}
 
inline void feederHandler(uint8_t pins) {
  static unsigned long last_int_time = 0;
  unsigned long int_time=micros();
  if ((int_time-last_int_time)>=2000000) {
    if (pins & _BV(PK2)) {
      if (~PORTL & _BV(PL0)) {
        PORTL |= _BV(PL0); 
        _outlogentry(Feeder,false);
      }
    }
  } 
  last_int_time=int_time;
}

///////////////////////////////////////////////
//    PWM
//////////////////////////////////////////////
//these are standard arduino PWM
//use analogWrite to set PWM duty cycle from main loop
//these will work with your LED lights routine
//add op amp circuit if you need 10v PWM
//unused PWM pump pins can be used here for a total of up to 12 PWM lines
#define PWM1 2
#define PWM2 3
//#define PWM3 5
//#define PWM4 6
//#define PWM5 7
//#define PWM6 8
//#define PWM7 9
//#define PWM8 46
//#ifndef _PWMB
//#define PWM9 44
//#define PWM10 45
//#endif
//#ifndef _PWMA
//#define PWM11 11
//#define PWM12 12
//#endif

void initPWM() {
  #ifdef PWM1
  pinMode(PWM1,OUTPUT);
  analogWrite(PWM1,0);
  #endif
  #ifdef PWM2
  pinMode(PWM2,OUTPUT); 
  analogWrite(PWM2,0);
  #endif
  #ifdef PWM3
  pinMode(PWM3,OUTPUT);
  analogWrite(PWM3,0);
  #endif
  #ifdef PWM4
  pinMode(PWM4,OUTPUT); 
  analogWrite(PWM4,0);
  #endif
  #ifdef PWM5
  pinMode(PWM5,OUTPUT);
  analogWrite(PWM5,0);
  #endif
  #ifdef PWM6
  pinMode(PWM6,OUTPUT); 
  analogWrite(PWM6,0);
  #endif
  #ifdef PWM7
  pinMode(PWM7,OUTPUT);
  analogWrite(PWM7,0);
  #endif
  #ifdef PWM8
  pinMode(PWM8,OUTPUT); 
  analogWrite(PWM8,0);
  #endif
  #ifdef PWM9
  pinMode(PWM9,OUTPUT);
  analogWrite(PWM9,0);
  #endif
  #ifdef PWM10
  pinMode(PWM10,OUTPUT); 
  analogWrite(PWM10,0);
  #endif
  #ifdef PWM11
  pinMode(PWM11,OUTPUT);
  analogWrite(PWM11,0);
  #endif
  #ifdef PWM12
  pinMode(PWM12,OUTPUT); 
  analogWrite(PWM12,0);
  #endif
}

void testPWM(){
  static boolean up = true;
  static int fadeValue = 0;
  
  analogWrite(PWM1, fadeValue);         
  analogWrite(PWM2, fadeValue);             
  if (up) {
    fadeValue += 5;
    if (fadeValue>255) {
      up = false;
      fadeValue = 255;
    }
  } else {
    fadeValue -=5; 
    if (fadeValue<0) {
      up = true;
      fadeValue = 0;
    }
  }
  
}

////////////////////////////
//  SONAR
////////////////////////////
#define sonarEPin    A13 //PK5 PCINT21
#define sonarTPin    47  //PL2
#define SONAR_TRIGGER_HI PORTL |= _BV(PL2);
#define SONAR_TRIGGER_LO PORTL &= ~_BV(PL2);
//#define SONAR_TRIGGER_HI PORTL |= _BV(PK5);
//#define SONAR_TRIGGER_LO PORTL &= ~_BV(PK5);
#define DURATION_TO_MM 6
#define DURATION_TO_CM 58
#define DURATION_TO_IN 148
#define SONAR_MAX 50 //400
#define SONARPINOUT DDRK |= _BV(PK5) 
#define SONARPININ DDRK &= ~_BV(PK5)

volatile static uint16_t sonarDistance = 0;

void initSonar() {
  pinMode(sonarEPin,INPUT);
  pinMode(sonarTPin,OUTPUT); 
  digitalWrite(sonarEPin,HIGH);
  digitalWrite(sonarTPin,LOW);
  PCMSK2 |= _BV(PCINT21);
}

inline void sonarHandler(uint8_t pins) {
 static unsigned long starttime = 0;
 static const long maxval = SONAR_MAX*DURATION_TO_CM;
 if (pins & _BV(PK5)) {
   starttime = micros();
 } else {
   unsigned long duration = micros()-starttime;
   if (duration<=maxval)
     sonarDistance = duration/DURATION_TO_MM;  
//   DISABLESONAR;
 }   
}

void updateSonar()
{
  static uint32_t sum=0;
  if (sonarDistance>0) {
    if (sum)
      sum = (sum - sonaravg) + sonarDistance;
    else {
      sum = sonarDistance*256;
    }
    sonaravg = sum /256;
  }
//  SONARPINOUT;
//  delayMicroseconds(2);
  SONAR_TRIGGER_LO
  delayMicroseconds(2);
  SONAR_TRIGGER_HI
  delayMicroseconds(10);
  SONAR_TRIGGER_LO
  delayMicroseconds(2);
//  SONARPININ;
//  delayMicroseconds(2);
//  ENABLESONAR;
}

uint16_t getSonar() {
  static uint16_t highest = 0;
  uint8_t saveSREG = SREG;
  cli();
  uint16_t tmp = sonaravg;
  SREG=saveSREG;
//  if (tmp>highest || (highest - tmp)>2) {
//    highest = tmp;
//  }
//  return highest;
  return tmp;
}

uint8_t getSonarPct() {
  uint16_t numerator = conf.sonarlow*10-getSonar();
  uint16_t denominator = conf.sonarlow - conf.sonarhigh;
  return numerator*10/denominator; 
}

