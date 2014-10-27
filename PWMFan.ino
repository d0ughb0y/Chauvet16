/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
#ifdef _PWMFAN
volatile uint16_t* __OCRn[] = {&OCR3A,&OCR3B,&OCR3C};
volatile uint8_t mask[] = {0x01,0x02,0x40};
volatile uint32_t _PWMFanWidth[MAXPWMFANS];

void initPWMFan(){
  //fan pins
  //            PWM       Power    Tachometer
  //        -----------  ------  --------------
  //pwmfan0 D5 PE3 OC3A  A2 PF2  A8 PK0 PCINT16
  //pwmfan1 D2 PE4 OC3B  A3 PF3  A9 PK1 PCINT17
  //pwmfan2 D3 PE5 OC3C  A4 PF4  A14 PK6 PCINT22
  DDRE |= _BV(PE3)|_BV(PE4)|_BV(PE5);
  PORTE &= ~(_BV(PE3)|_BV(PE4)|_BV(PE5));

  DDRF |= _BV(PF2)|_BV(PF3)|_BV(PF4);
  PORTF &= ~(_BV(PF2)|_BV(PF3)|_BV(PF4));
  
  DDRK &= ~(_BV(PK0)|_BV(PK1)|_BV(PK6));
  PORTK |= _BV(PK0)|_BV(PK1)|_BV(PK6);
  
#if (MAXPWMFANS>=1)
    PCMSK2 |= _BV(PCINT16);
#endif
#if (MAXPWMFANS>=2)
    PCMSK2 |= _BV(PCINT17);
#endif
#if (MAXPWMFANS>=3)
    PCMSK2 |= _BV(PCINT22);
#endif

  TCCR3A = TCCR3B = 0;
  TCCR3A |= _BV(WGM31) | _BV(COM3A1) | _BV(COM3B1) | _BV(COM3C1);
  TCCR3B |= _BV(WGM33)  | _BV(CS31);
  ICR3=100;
  OCR3A=OCR3B=OCR3C=0;
}


void PWMFanHandler(uint8_t i){
  static uint32_t timestamp[MAXPWMFANS];
  static uint32_t sample[MAXPWMFANS];
  uint32_t t = micros();
  uint32_t current = (t-timestamp[i]);
  if (abs(current-sample[i])<50000)
    _PWMFanWidth[i]=current;
  sample[i]=current;
  timestamp[i]=t;
}

uint16_t getPWMFanRPM(uint8_t i){
  static uint16_t cache[MAXPWMFANS];
  static unsigned long timestamp[MAXPWMFANS];

  if (isPWMFanON(i)) {
    if (millis()-timestamp[i]<1000) return cache[i];
    timestamp[i]=millis();
    uint8_t SaveREG=SREG;
    cli();
    uint32_t width=_PWMFanWidth[i];
    _PWMFanWidth[i]=0UL;
    SREG=SaveREG;
    if (width==0UL) {
      cache[i]=0;
      return 0;
    }
    cache[i]=(uint16_t)(30000000UL/width);
    return cache[i];
  } else
    return 0;
}

uint8_t getPWMFanLevel(uint8_t i) {
  if (isPWMFanON(i)) {
    uint8_t saveSREG = SREG;
    cli();
    uint8_t v = *__OCRn[i];
    SREG=saveSREG;
    return v;
  } else
    return 0;
}

inline void setPWMFanON(uint8_t i) {
  PORTF |= _BV(i+2);
}

inline void setPWMFanOFF(uint8_t i) {
  PORTF &= ~_BV(i+2);
}

boolean isPWMFanON(uint8_t i) {
  return PINF & _BV(i+2);
}

void setPWMFanLevel(uint8_t i, uint16_t level){
  static uint16_t lastlevel[MAXPWMFANS];
  if (level==lastlevel[i]) return;
  lastlevel[i]=level;
  uint8_t saveSREG = SREG;
  cli();
  *__OCRn[i] = level;
  SREG = saveSREG;
}

void setPWMFanMode(uint8_t i, uint8_t mode) {
  conf.pwmfan[i].mode=mode;
}

void updatePWMFans(){
 for (uint8_t i=0;i<MAXPWMFANS;i++){
   if (conf.pwmfan[i].mode!=_auto) continue;
   if (isPWMFanON(i) && getTemp(conf.pwmfan[i].tempsensor)<=(conf.pwmfan[i].templow-1)) {//turn off fan 1degree below temp low
     setPWMFanOFF(i);
   } else if (!isPWMFanON(i) && getTemp(conf.pwmfan[i].tempsensor)>= conf.pwmfan[i].templow){//turn on fan at temp low
     setPWMFanON(i);
     long temp = map(floor(getTemp(conf.pwmfan[i].tempsensor)),conf.pwmfan[i].templow,conf.pwmfan[i].temphigh,conf.pwmfan[i].levellow,conf.pwmfan[i].levelhigh);
     temp = constrain(temp,conf.pwmfan[i].levellow,conf.pwmfan[i].levelhigh);
     setPWMFanLevel(i,temp);
   }
 } 
}

void getpwmfanstatus(EthernetClient& client, uint8_t i, boolean full) {
  client << F("{");
  if (full) {
    client << F("\"n\":\"") << (const char*)conf.pwmfan[i].name << F("\",");
    client << F("\"mr\":\"") << conf.pwmfan[i].maxrpm << F("\",");
  }
  client << F("\"t\":\"") << getTemp(conf.pwmfan[i].tempsensor);
  client << F("\",\"r\":\"") << getPWMFanRPM(i) << F("\",\"m\":\"") << conf.pwmfan[i].mode;
  client << F("\",\"l\":\"") << getPWMFanLevel(i);
  client << F("\"}") << (i<MAXPWMFANS-1?F(","):F(""));
}
#endif
