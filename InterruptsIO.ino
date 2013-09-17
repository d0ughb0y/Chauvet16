#define CYCLETIMEB 15624 //1s
#define CYCLETIMEA 1500 //~100ms
#define CYCLESKIP 15 //1ms
volatile static uint16_t beepcount[6];

inline void enablePCINT(){
  PCICR |= (1<<PCIE2);
}

inline void disablePCINT(){
  PCICR &= ~(1<<PCIE2);
}

void setupTimers(){
  TCCR4A = 0;
  TCCR4B = /*_BV(WGM42) |*/ _BV(CS42) | _BV(CS40);  //div 1024  64us / count Normal mode
  OCR4A = CYCLETIMEA;   
  OCR4B = CYCLETIMEB; //1 second
  TIMSK4 = 0;
  TCCR3A = 0;
  TCCR3B = _BV(WGM32) | _BV(CS30);
  OCR3A = 16000; //1ms
  TIMSK3=0;
  DDRE |= _BV(PE3);
  PORTE &= ~_BV(PE3);
}

inline void startOutletTimers() {
  TIMSK4 =  _BV(OCIE4A) | _BV(OCIE4B); //enable interrupts    
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
  if (TIMSK3!=0) return;
  uint8_t SaveSREG = SREG;
  cli();
  beepcount[0]=first;
  beepcount[1]=beepcount[3]=endpause;
  beepcount[2]=second;
  beepcount[4]=third;
  beepcount[5]=cyclepause;
  TIMSK3 = _BV(OCIE3A); 
  SREG=SaveSREG;
}

inline void beepoff() {
  TIMSK3 = 0; 
  PORTE &= ~_BV(PE3);
}

void alarmOn() {
 if (conf.soundalert) {
   uint8_t saveSREG=SREG;
   cli();
   beepcount[0]=200;
   beepcount[1]=50;
   beepcount[2]=200;
   beepcount[3]=50;
   beepcount[4]=200;
   beepcount[5]=2000;
   TIMSK3 = _BV(OCIE3A); 
   SREG=saveSREG; 
 }
}

ISR(PCINT2_vect) {
  uint8_t pins = PINK;
  static uint8_t lastPins;
  uint8_t mask = pins ^ lastPins;
  lastPins = pins;
#ifdef _X10 
  if (mask & _BV(PK1)) { //x10 clock
    x10Handler(pins);
  }
#endif
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
//#ifdef _IO
//  if (mask & _BV(PK6)) { //IO
//    io1Handler(pins);
//  }
//  if (mask & _BV(PK7)) { //IO
//    io2Handler(pins);
//  }
//#endif
}

ISR(TIMER4_COMPA_vect) { //once per second handler
  outletHandlerA();
}

ISR(TIMER4_COMPB_vect) { //once per minute handler
  outletHandlerB();
}

ISR(TIMER3_COMPA_vect) { //buzzer
  static uint8_t state = 0;
  static uint16_t counter = 0;
  
  if (counter++ < beepcount[state]) {
    if (!(state%2))
      PORTE |= _BV(PE3);
  } else {
    if (state==5 && beepcount[5]==0) {
      TIMSK3 = 0;  
    }
    counter=0;
    state++;
    if (state>5) state=0;
    PORTE &= ~_BV(PE3);
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

////////////////////////////////
//  GENERIC IO
////////////////////////////////
//#define ENABLEIO1 PCMSK2 |= (1<<PCINT22)
//#define ENABLEIO2 PCMSK2 |= (1<<PCINT23)
#define IO1ON PORTK|=_BV(PK6)
#define IO1OFF PORTK&=~_BV(PK6)
#define IO2ON PORTK|=_BV(PK7)
#define IO2OFF PORTK&=~_BV(PK7)

void initIO(){
  //IO1 PK6 PCINT22
  //IO2 PK7 PCINT23
  DDRK &= ~(_BV(PK6)|_BV(PK7));
  IO1ON;
  IO2ON;
}

inline uint8_t getIO1(){
  return PINK & _BV(PK6);
}

inline uint8_t getIO2() {
  return PINK & _BV(PK7);
}

////////////////////////////////////
//  FEEDER
////////////////////////////////////
#define feedOutPin 49 //PL0  PORTL
#define feedInPin A10 //PK2 PCINT18
#define DISABLEFEEDER PCMSK2 &= ~(1<<PCINT18)
#define ENABLEFEEDER  PCMSK2 |= (1<<PCINT18)


void initFeeder() {
  pinMode(feedInPin,INPUT); 
  digitalWrite(feedInPin,HIGH);
  pinMode(feedOutPin,OUTPUT);
  digitalWrite(feedOutPin,HIGH); //turn feeder off
  //disable PCINT2
  //  PCICR |= (1<<PCIE0);
#ifdef _FEEDER
  ENABLEFEEDER;
#endif 
}

inline void feed(){
  if (!(~PORTL & _BV(PL0))) {
    PORTL &= ~_BV(PL0);
    _outlogentry(Feeder,true);
  }
}
 
inline void feederHandler(uint8_t pins) {
  static time_t last_int_time = 0;
  time_t int_time=millis();
  if ((time_t)(int_time-last_int_time)>=1000) {
    if (pins & _BV(PK2)) {
      if (~PORTL & _BV(PL0)) {
        PORTL |= _BV(PL0); 
        _outlogentry(Feeder,false);
      }
    }
  } 
  last_int_time=int_time;
}

//////////////////////////////
//    X10
//////////////////////////////
#define x10ClkPin A9 //PK1 PCINT17
#define x10DatPin 48

#define ENABLEX10 PCMSK2 |= (1<<PCINT17)
#define DISABLEX10 PCMSK2 &= ~(1<<PCINT17)
boolean x10transmitting = false;
volatile uint16_t x10Data;

void initX10(){
  pinMode(x10ClkPin,INPUT);
  pinMode(x10DatPin,OUTPUT); 
  digitalWrite(x10ClkPin,HIGH);
  digitalWrite(x10DatPin,HIGH);
  #ifdef _X10
  ENABLEX10;
  #endif
}

void write(uint8_t houseCode,uint8_t numberCode) {
  if (x10transmitting) return;
  x10Data = 0;
  ENABLEX10;
}

inline void x10Handler(uint8_t pins) {
  if (pins & _BV(PK1)) {
    lightOn();
  } 
  else {
    lightOff();
  }
}

///////////////////////////////////////////////
//    PWM
//////////////////////////////////////////////
#define PWM1 2
#define PWM2 3
boolean up = true;
int fadeValue = 0;

void initPWM() {
  pinMode(PWM1,OUTPUT);
  pinMode(PWM2,OUTPUT); 
  analogWrite(PWM1,0);
  analogWrite(PWM2,0);
}

void testPWM(){
  
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
#define DURATION_TO_CM 58
#define DURATION_TO_IN 148
#define SONAR_MAX 50 //400
#define ENABLESONAR PCMSK2 |= _BV(PCINT21)
#define DISABLESONAR PCMSK2 &= ~_BV(PCINT21)
#define SONARPINOUT DDRK |= _BV(PK5) 
#define SONARPININ DDRK &= ~_BV(PK5)

volatile static uint16_t sonarDistance = 0;

void initSonar() {
  pinMode(sonarEPin,INPUT);
  pinMode(sonarTPin,OUTPUT); 
  digitalWrite(sonarEPin,HIGH);
  digitalWrite(sonarTPin,LOW);
#ifdef _SONAR
  ENABLESONAR;
#endif

}

inline void sonarHandler(uint8_t pins) {
 static unsigned long starttime = 0;
 static const long maxval = SONAR_MAX*DURATION_TO_CM;
 if (pins & _BV(PK5)) {
   starttime = micros();
 } else {
   unsigned long duration = micros()-starttime;
   if (duration<=maxval)
     sonarDistance = duration/DURATION_TO_CM;  
//   DISABLESONAR;
 }   
}

void updateSonar()
{
  static uint32_t sum=0;
  if (sum)
    sum = (sum - sonaravg) + sonarDistance;
  else {
    sum = sonarDistance*8;
  }
  sonaravg = sum >> 3;
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
  if (tmp>highest || (highest - tmp)>2) {
    highest = tmp;
  }
  return highest;
}

uint8_t getSonarPct() {
  uint16_t numerator = conf.sonarlow-getSonar();
  uint16_t denominator = conf.sonarlow - conf.sonarhigh;
  return numerator*100/denominator; 
}

