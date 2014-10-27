/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
#define numReadings 8
#ifdef _TEMP
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
  for (uint8_t i=0;i<MAXPH;i++) {
    if (ph[i]->init()) {
      p(F("pH OK.          "));
      logMessage(F("pH initialized. ph "),i);
    } else {
      beepFail();
      p(F("pH init failed. "));
      logMessage(F("pH init failed. ph "),i);
    }
  }
#endif
#ifdef _ORP
  maxsensors++;
  if (orp.init()) {
    p(F("ORP OK.         "));
    logMessage(F("ORP sensor initialized."));
  } else {
    beepFail();
    p(F("ORP init failed."));
    logMessage(F("ORP sensor init failed."));
  }
#endif
#ifdef _COND
  maxsensors++;
//  if (initAtlas(conddata)) {
  if (cond.init()) {
    p(F("Cond OK.        "));
    logMessage(F("Conductivity sensor initialized."));
  } else {
    beepFail();
    p(F("Cond init failed"));
    logMessage(F("Conductivity sensor init failed."));
  }
#endif
}

boolean initTemp() {
#ifdef _TEMP
uint8_t _addr[8];
int x = 0;
uint8_t _numtemps = 0;
int _lastfound = -1;
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
        if (OneWire::crc8(_addr,7)==OneWire::crc8(tempdata[i].addr,7)) {
          logMessage(F("Found Temp "),i);
          x=i;
          break;
        }
      }
      if (x!=_lastfound)
        _lastfound=x;
      else {
        logMessage(F("Found unregistered temp sensor. Please update config.h."));
        continue;
      }
    }
    _numtemps++;
    if (ds.reset()) {
      ds.select(_addr);
      ds.write(0x44);
    } else {
      logMessage(F("Init temp reset after addr fail."));
      return false;//always fail on any onewire bus error
    }
    for (int y=0;y<1024;y++) {
      if (ds.read()>0) break;
    }
    if (ds.reset()) {
      ds.select(_addr);
      ds.write(0xBE);
      uint8_t data[9];
      ds.read_bytes(data,9);
      if (ds.crc8(data,8)!=data[8]) {
        logMessage(F("Read crc data mismatch."));
        return false;
      }
      uint16_t tval = data[1] << 8 | data[0];
      if (tval > 0 ) {
        tempdata[x].sum = tval * numReadings;
        tempdata[x].average = tval;
        tempdata[x].initialized=true;
      }
    } else {
      logMessage(F("Temp data read reset fail."));
      return false;
    }
  }
  ds.reset_search();
  delay(250);
  if (tempdata[0].initialized) {//only temp 0 is required
    return true;
  } else {
    logMessage(F("Temp0 is not initialized."));
    return false;
  }
} else //no temp probes
#endif
  return false;
}

float getTemp(int i) {
  if (!tempdata[i].initialized) return 0.0;
  static float lasttemp[MAXTEMP];
  static unsigned long lastread[MAXTEMP];
  unsigned long timenow = millis();
  if (timenow-lastread[i]>128) {
    uint8_t saveSREG = SREG;
    cli();
    uint16_t t = tempdata[i].average;
    SREG=saveSREG;
    lasttemp[i] = t / 16.0
#ifndef CELSIUS
    * 1.8 + 32.0
#endif
    ;
    lastread[i]=timenow;
  }
  return lasttemp[i];
}

void updateTemp() {
  static uint8_t state = 0;
  static uint8_t t = 0;
  static unsigned long timestamp;
  static uint8_t bytecounter = 0;
  switch (state) {
    case 0: //convert
      if (ds.reset2()) { //500us
        state=1;
        timestamp=micros();
      }
      break;
    case 1:
      if (micros()-timestamp<410) return;
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
        logMessage(F("reset failed"));
        t=0;
        state=0;
      }
      break;
    case 5:
      if (micros()-timestamp<410) return;
      if (MAXTEMP==1) {
        ds.skip(); //540us
        state=6;
      } else {
         if (bytecounter==0) {
           ds.write(0x55);
         } else {
           ds.write(tempdata[t].addr[bytecounter-1]);
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
        if (tval > 256 && tval < 784) {
          uint8_t saveSREG=SREG;
          cli();
          tempdata[t].sum = (tempdata[t].sum - tempdata[t].average) + tval;
          tempdata[t].average = tempdata[t].sum / numReadings;
          SREG=saveSREG;
          tempdata[t].initialized=true;
        }
      } else {
        if (t!=0)
          tempdata[t].initialized=false;
        logMessage(F("crc mismatch"));
      }
      t++;
      t%=MAXTEMP;
      if (t>=MAXTEMP)
        t=0;
      if (t==0) {
        state=0;
      } else
        state=4;
      break;
  }
}

void checkTempISR(){
  if (!tempdata[0].initialized) return;
  #ifdef _HEATER
  if (getTemp(0) <= conf.htrlow && conf.outletRec[Heater].mode==_auto)
    _outletOn(Heater);
  if (getTemp(0) >= conf.htrhigh && conf.outletRec[Heater].mode==_auto)
    _outletOff(Heater);
  #endif
  #ifdef _FAN
  if (getTemp(0) >= conf.fanhigh && conf.outletRec[Fan].mode==_auto) {
      _outletOn(Fan);
  }
  if (getTemp(0) <= conf.fanlow && conf.outletRec[Fan].mode==_auto) {
      _outletOff(Fan);
  }
  #endif  
}

void checkAlarm() {
#ifdef _TEMP
  for(int i=0;i<MAXTEMP;i++) {
    if (tempdata[i].initialized && (getTemp(i)>conf.alert[i].highalert || getTemp(i)<conf.alert[i].lowalert))
      return setAlarm(true);
  }
#endif
#ifdef _PH
  for (int i=0;i<MAXPH;i++) {
    if (ph[i]->isInitialized() && (ph[i]->getAvg()>conf.alert[MAXTEMP+i].highalert || ph[i]->getAvg()<conf.alert[MAXTEMP+i].lowalert))
      return setAlarm(true);
  }
#endif
#ifdef _ORP
  if (orp.isInitialized() && (orp.getAvg()>conf.alert[MAXTEMP+MAXPH].highalert || orp.getAvg()<conf.alert[MAXTEMP+MAXPH].lowalert))
      return setAlarm(true);
#endif
#ifdef _COND
//  if (conddata.initialized && (getAtlasAvg(conddata)>conf.alert[MAXTEMP+MAXPH+MAXORP].highalert || getAtlasAvg(conddata)<conf.alert[MAXTEMP+MAXPH+MAXORP].lowalert))
  if (cond.isInitialized() && (cond.getAvg()>conf.alert[MAXTEMP+MAXPH+MAXORP].highalert || cond.getAvg()<conf.alert[MAXTEMP+MAXPH+MAXORP].lowalert))
      return setAlarm(true);
#endif
#ifdef _SONAR
  if (conf.sonaralert && sonaravg>conf.sonaralertval*10)
    return setAlarm(true);
#endif
#ifdef _DOSER
  if (!dosercalibrating) {
    for (int i=0;i<MAXDOSERS;i++) {
      if (conf.doser[i].dosesperday>0 && conf.doser[i].rate>0 && conf.doser[i].fullvolume-dosedvolume[i]/100.0<conf.doser[i].dailydose)
        return setAlarm(true);
    }
  }
#endif
#ifdef _PWMFAN
  for (uint8_t i=0;i<MAXPWMFANS;i++) {
    if (conf.pwmfan[i].mode==_auto && conf.pwmfan[i].alert) {
      if (getPWMFanLevel(i)>0) {
        if (getPWMFanRPM(i)==0) {
          delay(50);
          if (getPWMFanRPM(i)==0)
            return setAlarm(true);
        }
      }
    }
  }
#endif
  setAlarm(false);
}

void setAlarm(boolean state){
  if (state) {
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

