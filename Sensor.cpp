/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
#include <Wire.h>
#include "Sensor.h"
#include "Arduino.h"

Sensor* ssensors[3];
uint8_t idx = 0;

Sensor::Sensor(char* name, SensorType type, SensorAddrType addrType, boolean isEZO){
  _type=type;
  _addrType = addrType;
  _isEZO = isEZO;
  _retrycount=0;
  strcpy(_name,name);
}

boolean Sensor::init() {
  _initialized=false;
  if (_isEZO) {
    if (_addrType==_serial) {
      send("");
      delay(300);
      getresponse();
      _scratch[0]=0;
      send("response,0");
      send("c,0");
    }
    send("L,1");
    if (_type==_cond) {
      send("o,ec,1");
      delay(300);
      send("o,tds,0");
      delay(300);
      send("o,sg,0");
      delay(300);
      send("o,s,1");
      delay(300);
      send("k,1.0");
      delay(300);
      getresponse();
    }
    send("r");
  } else {//non EZO is always Serial
    send("e");
    send("e");
    send("L1");
    send("r");
  }
  delay(1500);
  int countdown = 50;
  while (!getresponse() || _scratch[0]==0) {
    if (countdown-- == 0)
      return false;
    delay(100);
  }
  if (_type==_cond) {
    char* v = strchr(_scratch,',');
    if (v==0) return false;
    _value=atof(v+1);
  } else {
    _value=atof(_scratch);
    if (_value==0 && _scratch[0]!='0')
      return false;
  }
  if (_value>=0) {
      send("r");
    _initialized=true;
    return true;
  }
  return false;
}

void Sensor::update(uint16_t rawtemp) {
  float _tmp=0;
  if (!_initialized) return;
  if (getresponse()) {
    if (_type==_cond) {
      char* v = strchr(_scratch,',');
      if (v) {
        _tmp=atof(v+1);
        if (_tmp==0 && *(v+1)!='0')
          _tmp=-1;
        *v=0;
        _ec=atol(_scratch);
      } else {
        _tmp=_value;
      }
    } else {
      _tmp = atof(_scratch);
      if (_tmp==0 && _scratch[0]!='0')
        _tmp=-1;
    }
    uint8_t saveSREG=SREG;
    cli();
    _value = _tmp;
    SREG=saveSREG;
    if (rawtemp && rawtemp!=_temp) {
      _temp=rawtemp;
      if (_isEZO && _type==_cond) {
        char buffer[8];
        buffer[0]='T';
        buffer[1]=',';
        dtostrf(_temp/16.0,5,2,(char*)buffer+2);
        send(buffer);
        delay(300);
        getresponse();
      }
    }
    _ready=false;
    send("r");
    _retrycount=0;
  } else {
    if (_retrycount++ >= 10) {
      send("r");
      _retrycount=0;
      _value=-1;
    }
  }
}

void Sensor::update() {
  update(0);
}

float Sensor::getVal() {
  return _value;
}

void Sensor::calibrate(char* calstr) {
  if (_type==_ph) {
    if (!_isEZO)  {
      //find word low,mid,high
      if (strstr(calstr,"low")>0) {
        send("f");
        return;
      } else if (strstr(calstr,"mid")>0) {
        send("s");
        return;
      } else if (strstr(calstr,"high")>0) {
        send("t");
        return;
      }
    }
  }
  send(calstr);
}

void Sensor::reset(){
  if (_isEZO) {
    send("cal,clear");
  } else {
    send("x");
    send("e");
    send("L1");
    send("r");
  }
}

boolean Sensor::isInitialized(){
  return _initialized;
}

long Sensor::getEC(){
  return _ec;
}

char* Sensor::getName(){
  return _name;
}

SensorSerial::SensorSerial(char* name,SensorType type, HardwareSerial* saddr, boolean isEZO) :Sensor(name,type,_serial,isEZO) {
  ssensors[idx++]=this;
  _saddr = saddr;
  if (type==_cond)
    _isEZO=true;
  if (type==_orp && _isEZO==true)
    _saddr->begin(9600);
  else
    _saddr->begin(38400);
  _initialized=false;
}

void SensorSerial::send(char* command) {
  _saddr->print(command);
  _saddr->print("\r");
}

boolean SensorSerial::getresponse() {
  static int i = 0;
  while (_saddr->available()) {
    char c = (char)_saddr->read();
    if (c=='\r') {
      _scratch[i]=0;
      _ready=true;
      i=0;
    } else {
      _scratch[i++]=c;
      if (i==14) {
        _scratch[0]=0;
        i=0;
        _ready=false;
      }
    }
  }
  return _ready;
}

SensorI2C::SensorI2C(char* name,SensorType type, uint8_t i2caddr) :Sensor(name,type,_i2c,true) {
  _i2caddr = i2caddr;
  _initialized=false;
}

void SensorI2C::send(char* str) {
  Wire.beginTransmission(_i2caddr);
  Wire.write(str);
  Wire.endTransmission();
}

boolean SensorI2C::getresponse(){
  uint8_t i = 0;
  _scratch[0]=0;
  Wire.requestFrom(_i2caddr,(uint8_t)sizeof(_scratch));
  uint8_t _code=Wire.read();
  if (_code==1) {
    while(Wire.available() && i<sizeof(_scratch)){
      _scratch[i] = Wire.read();
      if (_scratch[i]==0) break;
      i++;
    }
  }
  Wire.endTransmission();
  return true;
}
void serialEventRun(){
  for (uint8_t i=0;i<idx;i++) {
    ssensors[i]->getresponse();
  }
}
