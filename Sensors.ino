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
    if (initAtlas(phdata[i])) {
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
  if (initAtlas(orpdata)) {
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
  if (initAtlas(conddata)) {
    p(F("Cond OK.        "));
    logMessage(F("Conductivity sensor initialized."));
  } else {
    beepFail();
    p(F("Cond init failed"));
    logMessage(F("Conductivity sensor init failed."));
  }
#endif
}

boolean phinit = false;
boolean condinit = false;
boolean orpinit = false;

boolean initTemp() {
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
      if (tval > 256 && tval < 512 ) {
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
#ifndef CELSIIUS
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
    if (phdata[i].initialized && (getAtlasAvg(phdata[i])>conf.alert[MAXTEMP+i].highalert || getAtlasAvg(phdata[i])<conf.alert[MAXTEMP+i].lowalert))
      return setAlarm(true);
  }
#endif
#ifdef _ORP
  if (orpdata.initialized && (getAtlasAvg(orpdata)>conf.alert[MAXTEMP+MAXPH].highalert || getAtlasAvg(orpdata)<conf.alert[MAXTEMP+MAXPH].lowalert))
      return setAlarm(true);
#endif
#ifdef _COND
  if (conddata.initialized && (getAtlasAvg(conddata)>conf.alert[MAXTEMP+MAXPH+MAXORP].highalert || getAtlasAvg(conddata)<conf.alert[MAXTEMP+MAXPH+MAXORP].lowalert))
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

/////////////////////////////////////////////////////////
// Atlas Sensors Functions
/////////////////////////////////////////////////////////
boolean initAtlas(AtlasSensorDef_t &data) {
  char responsechars[15];
  responsechars[0]=0;
  data.saddr.begin(38400);
  for (int i=0;i<255,strlen(responsechars)==0;i++) {
    if (data.type==_cond)//set conductivity stamp to return SG only, K=1.0
      data.saddr.print("response,0\rc,0\ro,ec,0\ro,tds,0\ro,sg,0\rk,1.0\rr\r");
    else {
#ifdef _PH_EZO
      if (data.type==_ph) {
        data.saddr.print("response,0\rc,0\rr\r");
      } else
#endif
      data.saddr.print("e\rr\r");
    }
#if defined(_PH_EZO) || defined(_COND)
    delay(1000);
#else
    delay(384);
#endif
    data.average=0;
    getresponse(data, responsechars);
  }
  if (strlen(responsechars)==0) {
    data.initialized=false;
    return false;
  }
  data.value=atof(responsechars);
  if (data.value>0) {
    data.isReady=true;
    data.initialized=true;
    updateAtlas(data);
    data.isReady=false;
  } else
    return false;
  return true;
}

void getresponse(AtlasSensorDef_t& data, char* response){
  int i = 0;
  while (data.saddr.available()) {
    char c = (char)data.saddr.read();
    if (c=='\r') {
      response[i]=0;
      i=0;
      return;
    } else {
      response[i++]=c;
      if (i==14) {
        i=0;
        response[0]=0;
        return;
      }
    }
  }
}

void updateAtlas(AtlasSensorDef_t& data) {
  if (!data.initialized) return;
  if (data.isReady) {
    data.isReady=false;
    if (data.value>0) {
      if (data.average) {
        data.sum = (data.sum-data.average)+data.value;
        cli();
        data.average = data.sum / numReadings;
        sei();
      } else {
        data.sum = data.value*numReadings;
        cli();
        data.average = data.value;
        sei();
      }
    }
    data.saddr.print("r\r");
  }
}

float getAtlasAvg(AtlasSensorDef_t& data) {
  if (!data.initialized) return 0.0;
  uint8_t saveSREG=SREG;
  cli();
  float p = data.average;
  SREG=saveSREG;
  return p;
}

void calibrateLow(AtlasSensorDef_t& data) {
  if (data.type==_ph) {
    //calibrate ph 7
#ifdef _PH_EZO
    data.saddr.print("cal,mid,7.00\r");
#else
    data.saddr.print("s\r");
#endif
  } else if (data.type==_orp) {
    data.saddr.print("-\r");
  } else if (data.type==_cond) {
    data.saddr.print("cal,dry\r");
  }
}

void calibrateHigh(AtlasSensorDef_t& data, long val) {
  if (data.type==_ph) {
    //calibrate ph 10
#ifdef _PH_EZO
    data.saddr.print("cal,high,10.00\r");
#else
    data.saddr.print("t\r");
#endif
  } else if (data.type==_orp) {
    data.saddr.print("+\r");
  } else if (data.type==_cond) {
    data.saddr.print("cal,one,");
    data.saddr.print(val);
    data.saddr.print("\r");
  }
}

void atlasSerialHandler(){
#ifdef _PH
  for (int i=0;i<MAXPH;i++) {
     if (phdata[i].saddr.available())
      return atlasHandler(phdata[i]);
  }
#endif
#ifdef _ORP
  if (orpdata.saddr.available())
    return atlasHandler(orpdata);
#endif
#ifdef _COND
  if (conddata.saddr.available())
    return atlasHandler(conddata);
#endif
}

void atlasHandler(AtlasSensorDef_t &data) {
  int ccount = data.saddr.available();
  for (int i=0;i<ccount;i++) {
    char c = (char)data.saddr.read();
    if (c=='\r') {
      data.scratch[data.i]=0;
      data.i=0;
      data.value=atof(data.scratch);
      data.isReady=true;
      return;
    } else {
      data.scratch[data.i++]=c;
      if (data.i==14) {
        data.i=0;
        data.scratch[0]=0;
        data.value=0;
        data.isReady=true;
        return;
      }
    }
  }
}


/////////////////////////////////////////////////////////
// Serial Event Handlers
/////////////////////////////////////////////////////////
//uncomment the serialEvent function corresponding to the Serial port with atlas stamp connected, and
//set the parameter to the corresponding sensor variable.
void serialEvent1() {
  atlasSerialHandler();
}

void serialEvent2(){
  atlasSerialHandler();
}

void serialEvent3() {
  atlasSerialHandler();
}

