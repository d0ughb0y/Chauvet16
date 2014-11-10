/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
#ifndef _Sensor_h
#define _Sensor_h
#include "Arduino.h"
#define numReadings 8

class Sensor {
  public:
    enum SensorType {_ph,_orp,_cond};
    enum SensorAddrType {_serial, _i2c};
    Sensor(char* name, SensorType type, SensorAddrType addr, boolean isEZO);
    virtual boolean init();
    virtual void update();
    virtual void update(uint16_t rawtemp);
    virtual float getAvg();
    virtual float getVal();
    virtual void calibrate(char* calstr);
    virtual boolean isInitialized();
    virtual long getEC();
    virtual char* getName();
  protected:
    boolean _initialized;
    boolean _isEZO;
    long _ec;
    char _name[8];
    SensorType _type;
    SensorAddrType _addrType;
    char _scratch[15];//store serial reading here
    float _value; //last reading value
    float _value2; //2 consecutive readings must be within +/- 10%
    float _sum; //for calculating running average
    uint16_t _temp; //temp used for temp compensation
    volatile float _average; //the running average
    virtual void send(char* command) {};
    virtual boolean getresponse() {};
};


class SensorSerial:public Sensor {
  public:
    SensorSerial(char* name, SensorType type, HardwareSerial* saddr, boolean isEZO);
  protected:
    HardwareSerial* _saddr;
    boolean getresponse();
    void send(char* command);
};

class SensorI2C:public Sensor {
  public:
    SensorI2C(char* name, SensorType type, uint8_t i2caddr);
  protected:
    uint8_t _i2caddr;
    boolean getresponse();
    void send(char* command);
};
#endif
