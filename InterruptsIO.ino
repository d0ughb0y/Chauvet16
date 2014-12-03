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
#ifdef PASSIVEBUZZER
  TCCR2A &= ~_BV(COM2B0);
#endif
  PORTH&=~_BV(PH6);
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
#ifdef _SONAR
  if (mask & _BV(PK5)) {
    sonarHandler(pins);
  }
#endif
#ifdef _PWMFAN
  if (mask & _BV(PK0)){
    if (pins & _BV(PK0))
      PWMFanHandler(0);
  }
#if (MAXPWMFANS>=2)
  if (mask & _BV(PK1)) {
    if (pins & _BV(PK1))
      PWMFanHandler(1);
  }
#endif
#if (MAXPWMFANS>=3)
  if (mask & _BV(PK6)) {
    if (pins & _BV(PK6))
      PWMFanHandler(2);
  }
#endif
#endif
#ifdef _FEEDER
  if (mask & _BV(PK2)) { //feeder
    feederHandler(pins);
  }
#endif
}

ISR(TIMER1_COMPC_vect) { //interrupts every 1.02 ms
  static uint16_t counter = 0;
  static uint16_t outletmod = 10;
  //call 1ms tasks, make sure tasks executes in under 100us
  buzzerHandler();
  if (!timerEnabled) return;
#ifdef _DOSER
  doserHandler();
#endif
#ifdef _FEEDER_V2
  feedHandler();
#endif
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
  DDRH |= _BV(PH6); //pin D9 (PH6) output
  PORTH &= ~_BV(PH6);
#ifdef PASSIVEBUZZER
  TCCR2A=TCCR2B=0;
  TCCR2A|=_BV(COM2B0)|_BV(WGM21);  //ctc toggle
  TCCR2B|=_BV(CS21)|_BV(CS20); //clk/32
  OCR2A=108; //2300 hz
#endif
  buzzerbusy=false;
}

inline void buzzerHandler(){
  //buzzer pin D9 PH6
  static uint8_t state = 0;
  static uint16_t counter = 0;
  if (!buzzerbusy) {
    state=0;
    counter=0;
    return;
  }
  if (counter++ < beepcount[state]) {
    if (!(state%2)) {
#ifdef PASSIVEBUZZER
      TCCR2A |= _BV(COM2B0);
#else
      PORTH |= _BV(PH6);
#endif
    }
  } else {
    if (state==5 && beepcount[5]==0) {
      buzzerbusy=false;
    }
    counter=0;
    state++;
    if (state>5) state=0;
#ifdef PASSIVEBUZZER
    TCCR2A &= ~_BV(COM2B0);
#endif
    PORTH &= ~_BV(PH6);
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
  if (conf.outletRec[ATO].mode == _auto) {
    if (!getATO2()&& !getATO1() && isOutletOn(Return)
#ifdef _SONAR
        && sonaravg<conf.sonaralertval*10
#endif
      ) {
        _outletOn(ATO);
        return;
    }
    _outletOff(ATO);
  }
}

////////////////////////////////
//  Doser PUMPS
////////////////////////////////
#ifdef _DOSER
void initDosers(){
  //PG0 Digital 41 doser0
  //PG2 Digital 39 doser1
  //PG1 Digital 40 doser2
  //PL7 Digital 42 doser3
  readDoserStatus();
#if (MAXDOSERS>=1)
  DDRG |= _BV(PG0);
  PORTG&=~_BV(PG0);
#endif
#if (MAXDOSERS>=2)
  DDRG |= _BV(PG2);
  PORTG&=~_BV(PG2);
#endif
#if (MAXDOSERS>=3)
  DDRG |= _BV(PG1);
  PORTG&=~_BV(PG1);
#endif
#if (MAXDOSERS==4)
  DDRL |= _BV(PL7);
  PORTL&=~_BV(PL7);
#endif
  if (conf.doser[0].rate==0||conf.doser[1].rate==0) {
    p(F("Configure Doser."));
    logMessage(F("Please configure doser pumps before using."));
  } else {
    p(F("Dosers OK.      "));
    logMessage(F("Doser pumps initialized."));
#if (MAXDOSERS>=1)
    logMessage(F("Doser0 dosed volume since reset is "),dosedvolume[0]/100.0);
#endif
#if (MAXDOSERS>=2)
    logMessage(F("Doser1 dosed volume since reset is "),dosedvolume[1]/100.0);
#endif
#if (MAXDOSERS>=3)
    logMessage(F("Doser2 dosed volume since reset is "),dosedvolume[2]/100.0);
#endif
#if (MAXDOSERS==4)
    logMessage(F("Doser3 dosed volume since reset is "),dosedvolume[3]/100.0);
#endif
  }
  cli();
  for (int i=0;i<MAXDOSERS;i++){
    doseractive[i]=false;
    dosercounter[i]=dosercountmatch[i]=0ul;
  }
  updateDoserStatusFlag=false;
  calibrationcount=0;
  sei();
}

inline void doserHandler(){
  for (int i=0;i<MAXDOSERS;i++) {
    if (doseractive[i]) {
      if (++dosercounter[i]>=dosercountmatch[i]) {
       doserOff(i);
      }
    }
  }
}

inline void doserOn(uint8_t i, uint32_t countmatch) {
  if (isDoserOn(i) || dosercalibrating) return;
  uint8_t SaveREG=SREG;
  cli();
  if (dosedvolume[i]>=conf.doser[i].fullvolume*100ul) {
    SREG=SaveREG;
    return;
  }
  dosercounter[i]=0;
  dosercountmatch[i]=countmatch;
  if (i==0) {
    PORTG|=_BV(PG0);
    _outlogentry(Doser0,true);
  } else if (i==1) {
    PORTG|=_BV(PG2);
    _outlogentry(Doser1,true);
  } else if (i==2) {
    PORTG|=_BV(PG1);
    _outlogentry(Doser2,true);
  } else if (i==3) {
    PORTL|=_BV(PL7);
    _outlogentry(Doser3,true);
  }
  doseractive[i]=true;
  SREG=SaveREG;
}

inline void doserOff(uint8_t i) {
  if (dosercalibrating) return calstop(i);
  if (isDoserOn(i)) {
    uint8_t SaveSREG=SREG;
    cli();
    doseractive[i]=false;
    if (i==0) {
      PORTG&=~_BV(PG0);
      _outlogentry(Doser0,false);
    } else if (i==1) {
      PORTG&=~_BV(PG2);
      _outlogentry(Doser1,false);
    } else if (i==2) {
      PORTG&=~_BV(PG1);
      _outlogentry(Doser2,false);
    } else if (i==3) {
      PORTL&=~_BV(PL7);
      _outlogentry(Doser3,false);
    }
    //set log
    if (conf.doser[i].rate>0)
      dosedvolume[i]+=100ul*dosercounter[i]/conf.doser[i].rate;
    dosercountmatch[i]=0;
    dosercounter[i]=0;
    if (updateDoserStatusFlag==false) {
      updateDoserStatusFlag=true;
    }
    SREG=SaveSREG;
  }
}

inline boolean isDoserOn(uint8_t i){
  if (i==0)
    return PORTG & _BV(PG0);
  else if (i==1)
    return PORTG & _BV(PG2);
  else if (i==2)
    return PORTG & _BV(PG1);
  else if (i==3)
    return PORTL & _BV(PL7);
}

void manualDoseOn(uint8_t i, uint16_t vol) {
  if (conf.doser[i].rate==0 || dosercalibrating) return;
  doserOn(i, vol/10.0 * conf.doser[i].rate);
}

void calstart(uint8_t i) {
  cli();
  calibrationcount=0;
  dosercountmatch[i] = 0;
  dosercounter[i] = 0;
  dosercalibrating=true;
  logMessage(F("Doser "),i);
  logMessage(F("Calibration start."));
  sei();
}


void calstop(uint8_t i) {
  uint8_t SaveSREG = SREG;
  cli();
  doseractive[i]=false;
  if (i==0) {
    PORTG&=~_BV(PG0);
  } else  {
    PORTG&=~_BV(PG2);
  }
  calibrationcount+=dosercounter[i];
  dosercounter[i]=dosercountmatch[i]=0;
  SREG = SaveSREG;
}

void caladjust(uint8_t i,uint32_t addcount) {
  cli();
  dosercountmatch[i] = addcount;
  dosercounter[i] = 0;
  if (i==0) {
    PORTG|=_BV(PG0);
  } else if (i==1) {
    PORTG|=_BV(PG2);
  } else if (i==2) {
    PORTG|=_BV(PG1);
  } else if (i==3) {
    PORTL|=_BV(PL7);
  }
  doseractive[i]=true;
  sei();
}

void calsave(uint8_t i, uint16_t vol) {
  conf.doser[i].rate = calibrationcount/vol;
  calibrationcount=0;
  dosercalibrating=false;
  writeEEPROM();  
  logMessage(F("Doser "),i);
  logMessage(F("Calibration End. Dose rate = "),(int)conf.doser[i].rate);
}

void getdoserstatus(EthernetClient& client, uint8_t i) {
  client << F("{\"n\":\"") << (const char*)conf.doser[i].name << F("\",\"dd\":\"") << conf.doser[i].dailydose;
  client << F("\",\"dpd\":\"") << conf.doser[i].dosesperday << F("\",\"i\":\"") << conf.doser[i].interval;
  client << F("\",\"st\":\"") << conf.doser[i].starttime;
  uint8_t saveSREG = SREG;
  cli();
  client << F("\",\"dv\":\"") << dosedvolume[i] << F("\",\"fv\":\"") << conf.doser[i].fullvolume;
  client << F("\",\"s\":\"") << (isDoserOn(i)?F("on"):F("off"));
  SREG=saveSREG;
  client << F("\"}") << (i<MAXDOSERS-1?F(","):F(""));
}
#endif
////////////////////////////////////
//  FEEDER
////////////////////////////////////
//#define feedOutPin 49 //PL0  PORTL
//#define feedInPin A10 //PK2 PCINT18
#ifdef _FEEDER
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
#endif
//////////////////////////////////////////////
//  FEEDER V2
//////////////////////////////////////////////
#ifdef _FEEDER_V2
volatile uint8_t feed2counter=0;

void initFeeder(){
  //use A10 PK2 only
  //set PK2 to tristate mode
  DDRK &= ~_BV(PK2);//input
  PORTK &= ~_BV(PK2);//no pullup
}

inline void feed() {
  DDRK |= _BV(PK2);//output
  PORTK |= _BV(PK2);//high
  feed2counter=100;
  _outlogentry(Feeder,true);
}

inline void feedHandler(){
  if (feed2counter>0) {
    if (--feed2counter==0) {
      PORTK &= ~_BV(PK2);//low
      DDRK &= ~_BV(PK2);//input
      _outlogentry(Feeder,false);
    }
  }
}
#endif

////////////////////////////
//  SONAR
////////////////////////////
#define sonarEPin    A13 //PK5 PCINT21
#define sonarTPin    47  //PL2
#define SONAR_TRIGGER_HI PORTL |= _BV(PL2);
#define SONAR_TRIGGER_LO PORTL &= ~_BV(PL2);
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
 }   
}

void updateSonar()
{
  static uint32_t sum=0;
  uint8_t saveSREG=SREG;
  cli();
  uint16_t tmpavg=sonaravg;
  uint16_t tmpdist=sonarDistance;
  SREG=saveSREG;

  if (tmpdist>0) {
    if (sum)
      sum = (sum - tmpavg) + tmpdist;
    else {
      sum = tmpdist*16;
    }
    tmpavg = sum /16;
    saveSREG=SREG;
    cli();
    sonaravg=tmpavg;
    SREG=saveSREG;
  }
  SONAR_TRIGGER_LO
  delayMicroseconds(2);
  SONAR_TRIGGER_HI
  delayMicroseconds(10);
  SONAR_TRIGGER_LO
  delayMicroseconds(2);
}

uint16_t getSonar() {
  uint8_t saveSREG = SREG;
  cli();
  uint16_t tmp = sonaravg;
  SREG=saveSREG;
  return tmp;
}

uint8_t getSonarPct() {
  uint16_t numerator = conf.sonarlow*10-getSonar();
  uint16_t denominator = conf.sonarlow - conf.sonarhigh;
  return numerator*10/denominator;
}

