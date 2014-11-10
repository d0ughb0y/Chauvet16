/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
#include <Wire.h>
#include "Sensor.h"
#include "Arduino.h"

Sensor::Sensor(char* name, SensorType type, SensorAddrType addrType, boolean isEZO){
  _type=type;
  _addrType = addrType;
  _isEZO = isEZO;
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
  } else {
    send("");
    send("L1");
    send("e");
    send("r");
  }
  delay(1500);
  _average=0.0;
  while (!getresponse() || strlen(_scratch)==0) delay(100);
  if (_type==_cond) {
    char* v = strchr(_scratch,',');
    if (v==0) return false;
    _value=atof(v+1);
  } else
    _value=atof(_scratch);
  if (_value>=0) {
    _sum = _value*numReadings;
    _average = _value;
    send("r");
    _initialized=true;
    return true;
  }
  return false;
}

void Sensor::update() {
  update(0);
}

void Sensor::update(uint16_t rawtemp) {
  if (!_initialized) return;
  if (getresponse()) {
    if (_type==_cond) {
      char* v = strchr(_scratch,',');
      if (v) {
        _value=atof(v+1);
        *v=0;
        _ec=atol(_scratch);
      }
    } else
      _value = atof(_scratch);
    if (_value>0) {
      if (fabs(_value-_value2)<_value2*0.1) {
        _sum = (_sum-_average)+_value;
        cli();
        _average = _sum / numReadings;
        sei();
      }
      _value2 = _value;
    }
    if (rawtemp && rawtemp!=_temp) {
      _temp=rawtemp;
      if (_isEZO) {
        char buffer[8];
        buffer[0]='T';
        buffer[1]=',';
        dtostrf(_temp/16.0,5,2,(char*)buffer+2);
        send(buffer);
        delay(300);
        getresponse();
      } else if (_type==_ph) {
        char buffer[6];
        dtostrf(rawtemp/16.0,5,2,(char*)buffer);
        send(buffer);
      }
    }
    send("r");
  }
}

float Sensor::getAvg() {
  if (!_initialized) return 0.0;
  uint8_t saveSREG=SREG;
  cli();
  float p = _average;
  SREG=saveSREG;
  return p;
}

float Sensor::getVal() {
  uint8_t saveSREG=SREG;
  cli();
  float p = _value;
  SREG=saveSREG;
  return p;
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
      i=0;
      return true;
    } else {
      _scratch[i++]=c;
      if (i==14) {
        i=0;
        _scratch[0]=0;
        return true;
      }
    }
  }
  return false;
}

SensorI2C::SensorI2C(char* name,SensorType type, uint8_t i2caddr) :Sensor(name,type,_i2c,true) {
  _i2caddr = i2caddr;
  _initialized=false;
}

//boolean SensorI2C::isPresent(){
//  Wire.beginTransmission(_i2caddr);
//  return Wire.endTransmission()==0;
//}

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
