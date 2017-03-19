/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
#include <avr/pgmspace.h>
#define PWMDATALEN 32
//{name,resolution,function}
WaveDef_t wave[] = {{"H1",1,&H1Pattern},
                    {"W1",2,&W1Pattern},
                    {"W2",18,&W2Pattern},
                    {"W3",16,&W3Pattern},
                    {"ELSE",1,&ELSEPattern},
                    {"Feed",8,&FeedPattern},
                    {"NTM",120,&NTMPattern},
                    {"CUST",CUSTOMPATTERNSTEPS,&CustomPattern}};
                    
enum {H1,W1,W2,W3,ELSE,Feed,NTM,Custom};                    

volatile uint8_t _intervals[] = INTERVALS;

volatile uint8_t _syncMode[MAXPWMPUMPS];
volatile uint8_t _waveMode[MAXPWMPUMPS];
volatile uint8_t _level[MAXPWMPUMPS];
volatile uint8_t _pulseWidth[MAXPWMPUMPS];
#ifdef _PWMB
volatile uint8_t _pumpAuto[MAXPWMPUMPS]={1,1,1,1};
#else
volatile uint8_t _pumpAuto[MAXPWMPUMPS]={1,1};
#endif
volatile uint16_t _counter[MAXPWMPUMPS];
volatile uint8_t _step[MAXPWMPUMPS];
volatile uint8_t _pwmdata[MAXPWMPUMPS][PWMDATALEN];
volatile uint8_t _pwmdatahead[MAXPWMPUMPS];
volatile uint16_t* _OCRn[] = {&OCR1A,&OCR1B,&OCR5B,&OCR5C};

void initPWMPumps() {
  #ifdef _PWMA 
  DDRB |= _BV(PB5) | _BV(PB6);
  TCCR1A |= _BV(COM1A1)|_BV(COM1B1);
  p(F("PWM A OK        "));
  #endif
  #ifdef _PWMB
  DDRL |= _BV(PL5)|_BV(PL4);
  TCCR5A |= _BV(COM5B1)|_BV(COM5C1);
  p(F("PWM B OK        "));
  #endif
  for (int i=0;i<MAXPWMPUMPS;i++) {
     _pwmdatahead[i]=0; 
  }
}

void startPumps(){
  updatePWMPumps();
  #ifdef _PWMA 
  TIMSK1 |= _BV(TOIE1);
  #endif
  #ifdef _PWMB
  TIMSK5 |= _BV(TOIE5);
  #endif  
}

void updatePWMPumps() {
  if (_waveMode[0]==Feed && _ActiveMacro<4) {
     return; //do not update if in the middle of a feed mode
  }
  uint8_t saveSREG = SREG;
  cli();
  uint8_t _interval=getCurrentInterval();
  boolean clrbuffer=false;
  for (int i=0;i<MAXPWMPUMPS;i++) {
    if (_pumpAuto[i]==0) continue;
    _counter[i]=0;
    _step[i]=0;    
    _syncMode[i]=conf.pump[i][_interval].syncMode;
    _pulseWidth[i]=conf.pump[i][_interval].pulseWidth;
    if (_waveMode[i]!=conf.pump[i][_interval].waveMode) clrbuffer=true;
    _waveMode[i]=conf.pump[i][_interval].waveMode;    
    _level[i] = getCurrentLevel(_interval,i);
 }
 if (clrbuffer) {
  memset((void*)_pwmdata,0,sizeof(_pwmdata));
  memset((void*)_pwmdatahead,0,sizeof(_pwmdatahead));
 }
 SREG=saveSREG;
}

void FeedModeON() {
  uint8_t saveSREG = SREG;
  cli();
  for (int i=0;i<MAXPWMPUMPS;i++) {
    _pulseWidth[i]=2;
    _waveMode[i]=Feed;
    _level[i]=255;  
    _syncMode[i]=_master;
  }  
  SREG=saveSREG;
}

void FeedModeOFF() {
  updatePWMPumps();
}

void getpwmdata(EthernetClient& client, uint8_t channel) {
  cli();
  for (int i=0;i<PWMDATALEN;i++) {
    client << _pwmdata[channel][(_pwmdatahead[channel]+i)%PWMDATALEN] << (i==PWMDATALEN-1?"":","); 
  }
  sei();
}

void getpumpinfo(EthernetClient& client) {
  cli();
  client << F("{\"pwmpumps\":[");
  for (int i=0;i<MAXPWMPUMPS;i++) {
    getpumpinfo(client,i);
  }
  client << F("]}");
  sei();
}

void getpumpinfo(EthernetClient& client, uint8_t i) {
  uint8_t saveSREG = SREG;
  cli();
  client << F("{\"wm\":\"") << _waveMode[i] << F("\",\"sm\":\"") << _syncMode[i];
  client << F("\",\"l\":\"") << _level[i] << F("\",\"pw\":\"") << _pulseWidth[i];
  client << F("\",\"pa\":\"") << _pumpAuto[i];
  client << F("\"}") << (i==MAXPWMPUMPS-1?"":",");    
  SREG=saveSREG;
}

void getwavedef(EthernetClient& client) {
  cli();
  client << F("{\"wavedef\":[");
  int last = sizeof(wave)/sizeof(WaveDef_t);
  for (int i=0;i<last;i++) {
      client << F("{\"name\":\"")<<wave[i].name<<F("\",");
      client << F("\"res\":\"")<<wave[i].resolution<<F("\"}");
      client << (i<last-1?",":"");
  }
  client << F("]}");
  sei();  
}

inline void setpumpauto(uint8_t channel, uint8_t val) {
  cli();
  _pumpAuto[channel]=val;
  if (channel==0 && val==1) {
    for (uint8_t ch=1;ch<MAXPWMPUMPS;ch++) {
      _pumpAuto[ch]=val;
    }
  }
  updatePWMPumps();
  sei();  
}

inline void setpwmlevel(uint8_t channel, uint8_t val) {
  cli();
  _level[channel]=val;
  _pumpAuto[channel]=0;
  sei();  
}

inline void setwavemode(uint8_t channel, uint8_t val) {
  cli();
  _waveMode[channel]=val;
  _pumpAuto[channel]=0;
  if (channel==0 && val==Feed) {
    for (uint8_t ch=1;ch<MAXPWMPUMPS;ch++) {
      _waveMode[ch]=val;
      _syncMode[ch]=_sync;
      _pumpAuto[ch]=0;
    }
  }
  sei();
}

inline void setsyncmode(uint8_t channel, uint8_t val) {
  cli();
  _syncMode[channel]=val;
  _pumpAuto[channel]=0;
  sei();
}

inline void setpulsewidth(uint8_t channel, uint8_t val) {
  cli();
  _pulseWidth[channel]=val;
  _pumpAuto[channel]=0;
  sei();
}

uint8_t H1Pattern(uint8_t step) {
  //fixed at 100%
  return 255;  
}

uint8_t W1Pattern(uint8_t step) {
  //square pulse
  static const uint8_t val[]={0,255};
  return val[step];
}

PROGMEM const uint8_t w2val[]={136,156,166,176,186,196,201,208,224,
                              241,255,255,241,224,208,191,171,154};
uint8_t W2Pattern(uint8_t step) {
  //move up 9 steps, down 6 steps
  return pgm_read_byte(w2val+step);
}

PROGMEM const uint8_t w3val[]={128,136,156,166,176,203,230,255,
                              240,224,208,192,176,160,144,136};
uint8_t W3Pattern(uint8_t step) {
  //move up 6 steps, down 8 steps
  return pgm_read_byte(w3val+step);
}

uint8_t ELSEPattern(uint8_t step) {
  return random(126,255);
}

PROGMEM const uint8_t feedval[]={255,0,0,0,0,0,0,0};
uint8_t FeedPattern(uint8_t step) {
  return pgm_read_byte(feedval+step);
}

PROGMEM const uint8_t ntmval[]={255,255,255,255,255,255,255,255,255,255,255,255,
                 255,255,255,255,255,255,255,255,255,255,255,255,
                 255,255,255,255,255,255,255,255,255,255,255,255,
                 255,255,255,255,255,255,255,255,255,255,255,255,
                 0,0,0,0,0,0,0,0,0,0,0,0,
                 255,0,255,0,255,0,255,0,255,0,255,0,
                 255,0,255,0,255,0,255,0,255,0,255,0,
                 255,0,255,0,255,0,255,0,255,0,255,0,
                 255,0,255,0,255,0,255,0,255,0,255,0,
                 0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t NTMPattern(uint8_t step) {
  return pgm_read_byte(ntmval+step);
}

PROGMEM const uint8_t cval[]=CUSTOMPATTERN;
uint8_t CustomPattern(uint8_t step) {
  return pgm_read_byte(cval+step);
}

uint8_t getCurrentInterval() {
  uint8_t intv = 0;
  while (intv<MAXINTERVALS && (uint8_t)hour(now2())>=_intervals[intv]) intv++; 
  if (intv==MAXINTERVALS) 
    return MAXINTERVALS-1;
  else  
    return intv-1;
}

uint8_t getCurrentLevel(uint8_t interval, uint8_t channel) {
  uint8_t step=interval;
  int t0 = _intervals[step]*60;
  int l0 = conf.pump[channel][step].level;
  if (conf.pump[channel][step].syncMode!=_master) {
     l0 = conf.pump[0][step].level; 
  }
  step++;
  step%=6;
  int t1 = _intervals[step]*60;
  int l1 = conf.pump[channel][step].level;
  if (conf.pump[channel][step].syncMode!=_master) {
     l1 = conf.pump[0][step].level; 
  }
  int tnow = elapsedSecsToday(now2())/SECS_PER_MIN;
  return (uint8_t)map(tnow,t0,t1,l0,l1);
}

void pwmhandler(uint8_t _channel) {
  for (uint8_t channel=_channel;channel<=_channel+1;channel++) {
    if (_syncMode[channel]!=_master)
      continue;
    if ((_counter[channel]++%(110+_pulseWidth[channel]*22))==0) {
      uint16_t v0 = wave[_waveMode[channel]]._getLevel(_step[channel]++);
      *_OCRn[channel] = 1UL*v0*_level[channel]/255; 
      _step[channel]%=wave[_waveMode[channel]].resolution;
      _pwmdata[channel][_pwmdatahead[channel]++]=*_OCRn[channel];
      _pwmdatahead[channel]%=PWMDATALEN;
      if (channel==0) {
        for (int i=1;i<MAXPWMPUMPS;i++) {
          if (_syncMode[i]==_sync){
            *_OCRn[i] = OCR1A;
          }else if (_syncMode[i]==_antisync){
            uint16_t v1 = 255 - v0;
            uint8_t v2 = wave[_waveMode[0]]._getLevel(0);
            if (v1<v2) v1+=v2;
            *_OCRn[i] = 1UL*v1*_level[0]/255;
          } else 
            continue;
          _pwmdata[i][_pwmdatahead[i]++]=*_OCRn[i];
          _pwmdatahead[i]%=PWMDATALEN;
        }
      }
    }
  }
}

#ifdef _PWMA
ISR(TIMER1_OVF_vect) {//once every 2040us (one pulse)
  pwmhandler(0);
}
#endif
#ifdef _PWMB
ISR(TIMER5_OVF_vect) {
  pwmhandler(2);
}
#endif

