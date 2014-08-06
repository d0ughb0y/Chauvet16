/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
#define numReadings 8
#ifdef _TEMP
volatile uint16_t tempavg[MAXTEMP];
uint8_t addr[][8] = {TEMPADDR};
uint16_t sum[MAXTEMP];
#define TEMPPIN 48
OneWire ds(TEMPPIN);
#endif

void initSensors() {
  maxsensors = MAXTEMP+MAXPH;
#ifdef _TEMP
  if (initTemp()) {
    p(F("Temp OK.        "));
    logMessage(F("Temp sensor initialized."));
  } else {
    beepFail();
    p(F("Temp Init failed"));
    logMessage(F("Temp Init failed"));
  }
#endif
#ifdef _PH
  if (initPH()) {
    p(F("pH OK.          "));
    logMessage(F("pH sensor initialized."));   
  } else {
    beepFail();
    p(F("pH Init failed  "));
    logMessage(F("pH Init failed"));
  } 
#endif  
#ifdef _ORP
  maxsensors++;
#endif
#ifdef _COND
  maxsensors++;
#endif
}

boolean tempinit = false;
boolean phinit = false;

boolean initTemp() {
uint8_t _addr[8];
uint8_t x = 0;
uint8_t _numtemps = 0;
if (ds.reset()) {
  for (int n=0;n<MAXTEMP;n++) {
    if (!ds.search(_addr)) {
      ds.reset_search();
      return _numtemps>0;
    }
    if (OneWire::crc8(_addr,7) != _addr[7])
      return false;
    else {
      for (int i=0;i<MAXTEMP;i++) {
        if (OneWire::crc8(_addr,7)==OneWire::crc8(addr[i],7)) {
          logMessage(F("Found Temp "),i);
          x=i;
          break;
        }
      }
    }
    _numtemps++;
    if (ds.reset()) {
      ds.select(_addr);
      ds.write(0x44);
    } else
      return false;
    for (int x=0;x<250;x++) {
      if (ds.read()>0) break;
    }
    if (ds.reset()) {
      ds.select(_addr);
      ds.write(0xBE);
      uint8_t data[9];
      ds.read_bytes(data,9);
      if (ds.crc8(data,8)!=data[8]) {
        return false;
      }
      uint16_t tval = data[1] << 8 | data[0];
      if (tval > 256 && tval < 512 ) {
        sum[x] = tval * numReadings;
        tempavg[x] = tval;
      }
    } else
      return false;
  }
  ds.reset_search();
  delay(250);
  if (_numtemps==MAXTEMP) {
    tempinit=true;
    return true;
  } else
    return false;
} else //no temp probes
  return false;
}

float getTemp() {
  return getTemp(0);
}

float getTemp(int i) {
  if (!tempinit) return 0.0;
  static float lasttemp[MAXTEMP];
  static unsigned long lastread[MAXTEMP];
  unsigned long timenow = millis();
  if (timenow-lastread[i]>128) {
    uint8_t saveSREG = SREG;
    cli();
    uint16_t t = tempavg[i];
    SREG=saveSREG;
    lasttemp[i] = t / 16.0 * 1.8 + 32.0;
    lastread[i]=timenow;
  }
  return lasttemp[i];
}

void updateTemp() {
  if (!tempinit) {
    //test if temp probe is plugged in
    if (ds.reset()) {
      tempinit=true;
    } else
      return;
  }
  static uint8_t state = 0;
  static uint8_t t = 0;
  static unsigned long timestamp;
  static uint8_t bytecounter = 0;
  switch (state) {
    case 0: //convert
      if (ds.reset2()) { //500us
        state=1;
        timestamp=micros();
      } else
        tempinit=false;
      break;
    case 1:
      if (micros()-timestamp<420) return;
      ds.skip();  //540us
      state=2;
      break;
    case 2:
      ds.write(0x44); //550us
      state=3;
      break;
    case 3: //status
      if (ds.read()>0) { //528us
        state=4;
      }
      break;
    case 4: //read command
      if (ds.reset2()) { //500us
        state=5;
        bytecounter=0;
        timestamp=micros();
      } else {
        tempinit=false;
        t=0;
        state=0;
      }
      break;
    case 5:
      if (micros()-timestamp<420) return;
      if (MAXTEMP==1) {
        ds.skip(); //540us
        state=6;
      } else {
         if (bytecounter==0) {
           ds.write(0x55);
         } else {
           ds.write(addr[t][bytecounter-1]);
         }
         bytecounter++;
         if (bytecounter>=9)
           state=6;
      }
      break;
    case 6: //read data
      ds.write(0xBE); //530us
      state=7;
      bytecounter=0;
      break;
    case 7:
      static uint8_t data[9];
      if (bytecounter<9) {
        data[bytecounter++]=ds.read();
        return;
      }
      if (ds.crc8(data,8)==data[8]) {
        uint16_t tval = data[1] << 8 | data[0];
        if (tval > 256 && tval < 512 ) {
          uint8_t saveSREG=SREG;
          cli();
          uint16_t tavg = tempavg[t];
          SREG=saveSREG;
          sum[t] = (sum[t] - tavg) + tval;
          tempavg[t] = sum[t] / numReadings;
        }
      }
      t++;
      t%=MAXTEMP;
      if (t==0) {
        state=0;
      } else
        state=4;
      break;
  }
}

void checkTempISR(){
  if (!tempinit) return;
  #ifdef _HEATER
  if (getTemp() <= conf.htrlow && conf.outletRec[Heater].mode==_auto)
    _outletOn(Heater);
  if (getTemp() >= conf.htrhigh && conf.outletRec[Heater].mode==_auto)
    _outletOff(Heater);
  #endif
  #ifdef _FAN
  if (getTemp() >= conf.fanhigh && conf.outletRec[Fan].mode==_auto) {
      _outletOn(Fan);
  }
  if (getTemp() <= conf.fanlow && conf.outletRec[Fan].mode==_auto) {
      _outletOff(Fan);
  }
  #endif  
}

void checkAlarm() {
  if ((tempinit && (getTemp()>=conf.alerttemphigh || getTemp()<=conf.alerttemplow)) ||
    (phinit && (phavg>=conf.alertphhigh || phavg<=conf.alertphlow)) 
#ifdef _SONAR    
    || (conf.sonaralert && sonaravg>conf.sonaralertval*10)
#endif    
#ifdef _DOSER
    || (conf.doser[0].fullvolume-dosedvolume[0]/100.0<conf.doser[0].dailydose)
    || (conf.doser[1].fullvolume-dosedvolume[1]/100.0<conf.doser[1].dailydose)
#endif
    ) {
      if (!alarm) {
        alarmOn();
        emailFlag=true;
        logalarmFlag=true;
        alarm=true;    
      }
  } else {
    if (alarm) {
      alarm=false;
      beepoff();
    }
  }  
}

///////////////////////////////////////////////////////
//  pH section
///////////////////////////////////////////////////////
#ifdef _PH
static float phreading=0;
static boolean phready = false;
#endif
boolean initPH() {
#ifdef _PH  
  char phchars[15];
  phchars[0]=0;
  Serial1.begin(38400);
  for (int i=0;i<255,strlen(phchars)==0;i++) {
    Serial1.print("e\rr\r");
    delay(384);
    getresponse(phchars);
  }
  if (strlen(phchars)==0) {
    phinit=false;
    return false;
  }
  phreading=atof(phchars);
  if (phreading>0) {
    phready=true;
    phinit=true;
    updatePh();
    phready=false;
    return true;
  } else 
#endif  
    return false;
}

void updatePh() {
  if (!phinit) return;
  static float sum=0;
  static float pha = 0;
  if (phready) {
    phready=false;
    if (phreading>0) {
      if (pha) {
        sum = (sum-pha)+phreading;
        pha = sum / numReadings; 
        cli();
        phavg=pha;
        sei();
      } else {
         sum = phreading*numReadings;
         pha=phreading;
         phavg=phreading;
      }
    }
    Serial1.print("r\r");
  }
}

float getph() {
  if (!phinit) return 0.0;
  uint8_t saveSREG=SREG;
  cli();
  float p = phavg;
  SREG=saveSREG;
  return p;
}

float getph(int i) {
   return getph();
}

void calibrate7() {
  Serial1.print("s\r");
}

void calibrate10(){
  Serial1.print("t\r");
}

void calibrate4() {
  Serial1.print("f\r");
}

void getresponse(char* phchars) {
  int i = 0;
  while (Serial1.available()) {
    char c = (char)Serial1.read(); 
    if (c=='\r') {
      phchars[i]=0;
      i=0;
      return;
    } else {
      phchars[i++]=c;
      if (i==14) {
        i=0;
        phchars[0]=0;
        return;
      }
    }
  }
}

void serialEvent1() {
  static char phchars[15];
  static int i = 0;
  char c = (char)Serial1.read(); 
  if (c=='\r') {
    phchars[i]=0;
    i=0;
    phreading=atof(phchars);  
    phready=true;
    return;
  } else {
    phchars[i++]=c;
    if (i==14) {
      i=0;
      phchars[0]=0;
      phreading=0;
      phready=true;
    }
  }
}

/////////////////////////////////////////////////////////
// Conductivity Section
/////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////
// Orp Section
/////////////////////////////////////////////////////////

