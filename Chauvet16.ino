//////////////////////////////////////////////////////
//   CHAUVET16 Program for the CHAUVET16 CONTROLLER //
//   Copyright (c) 2013 by Jerry Sy aka d0ughb0y    //
//   mail-to: j3rry5y@gmail.com                     //
//   Commercial use of this software without        //
//   permission is absolutely prohibited.           //
//   make sure to edit config.h first before        //
//   uploading to the mega.                         //
//////////////////////////////////////////////////////
#include <OneWire.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Time.h>
#include <DS1307RTC.h>
#include <SdFat.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Dns.h>
#include <Flash.h>
#include <stdlib.h>
#include "TinyWebServer.h"
#include "Sensor.h"
#include "config.h"

static uint32_t currentTime = 0;
static uint32_t previousTime = 0;
static uint16_t deltaTime = 0;
static uint16_t  counter = 0;

SdFat sd;
SdFile file_;
volatile uint32_t debug1=0;
volatile uint32_t debug2=0;
LiquidCrystal_I2C lcd(LCD_ADDR,LCD_EN,LCD_RW,LCD_RS,LCD_D4,LCD_D5,LCD_D6,LCD_D7,LCD_BACKLIGHT,LCD_POLARITY);
#ifdef LCD_BACKLIGHT_OFF_DELAY_MINS
unsigned long lcdontime = 0;
boolean backlighton = true;
#endif
enum outlet {OUTLETDEFS, Feeder=20, Doser0=23, Doser1=24, Doser2=25, Doser3=26, Pump0=30, Pump1=31, Pump2=32, Pump3=33, PWMFan0=40, PWMFan1=41, PWMFan2=42, End=255};

volatile uint8_t _ActiveMacro = 4;
volatile time_t _MacroTime = 0;
volatile time_t _MacroCountDown = 0;
volatile conf_t conf;
volatile int tz = STDTZOFFSET;
  
volatile OutLogRec_t _outlog[OUTLOGSZ];
volatile uint8_t _head = 0;
volatile uint8_t _tail = 0;
volatile boolean alarm = false;
#ifdef AUTODST
volatile boolean testDst = false;
#endif
volatile boolean emailFlag = false;
volatile boolean logalarmFlag = false;
volatile boolean logSensorsFlag = false;
volatile boolean netCheckFlag = false;
volatile boolean updateRTCFlag = false;
volatile uint16_t sonaravg = 0;
volatile uint8_t displaymode = 0;
volatile uint8_t maxsensors = 0;
#ifdef _TEMP
TempSensorDef_t tempdata[] = TEMPDEF;
#endif
#ifdef _PH
Sensor* ph[MAXPH];
#if (MAXPH>=1)
#if defined(_PH1_SERIAL)
SensorSerial ph1 = SensorSerial(_PH1_NAME,Sensor::_ph,&_PH1_SERIAL,_PH1_EZO);
#endif
#if defined (_PH1_I2C)
SensorI2C ph1 = SensorI2C(_PH1_NAME,Sensor::_ph,_PH1_I2C);
#endif
#endif
#if (MAXPH==2)
#if defined(_PH2_SERIAL)
SensorSerial ph2 = SensorSerial(_PH2_NAME,Sensor::_ph,&_PH2_SERIAL,_PH2_EZO);
#endif
#if defined (_PH2_I2C)
SensorI2C ph2 = SensorI2C(_PH2_NAME,Sensor::_ph,_PH2_I2C);
#endif
#endif
#endif
#ifdef _ORP
#ifdef _ORP_SERIAL
SensorSerial orp = SensorSerial(_ORP_NAME,Sensor::_orp,&_ORP_SERIAL,_ORP_EZO);
#endif
#ifdef _ORP_I2C
SensorI2C orp = SensorI2C(_ORP_NAME,Sensor::_orp,_ORP_I2C);
#endif
#endif
#ifdef _COND
#ifdef _COND_SERIAL
SensorSerial cond = SensorSerial(_COND_NAME,Sensor::_cond,&_COND_SERIAL,true);
#endif
#ifdef _COND_I2C
SensorI2C cond = SensorI2C(_COND_NAME,Sensor::_cond,_COND_I2C);
#endif
#endif
#ifdef _DOSER
volatile boolean doseractive[MAXDOSERS];
volatile uint32_t dosercounter[MAXDOSERS];
volatile uint32_t dosercountmatch[MAXDOSERS];
volatile uint32_t dosedvolume[MAXDOSERS];
volatile boolean updateDoserStatusFlag = false;
volatile boolean dosercalibrating = false;
volatile uint16_t calibrationcount = 0;
#endif

void setup() {
#ifdef _PH
#if (MAXPH>=1)
    ph[0]=&ph1;
#endif
#if (MAXPH==2)
    ph[1]=&ph2;
#endif
#endif
  initBuzzer();
  initTimer();
  beep();
  initializeEEPROM();
  #if defined(_FEEDER) || defined(_FEEDER_V2)
  initFeeder();
  #endif
  #ifdef _PWMFAN
  initPWMFan();
  #endif
  initATO();
  #ifdef _SONAR
  initSonar();
  #endif
  enablePCINT();
  initUtils(); //init led, i2c pullup, lcd, SD
  lightOn();
  initNetwork(); //ethernet, clock, webserver, start logging here
  initSensors(); //temp and ph
  initOutlets();
  #ifdef _DOSER
  initDosers();
  #endif
  #ifdef _PWMA
  initPWMPumps();
  #endif
  lightOff();
  #ifdef _SONAR
  for (int i=0;i<255;i++) {
    updateSonar();
    delay(2);
    if (getSonar()>0) {
      logMessage(F("ATO water level "),getSonarPct());
      break;
    }
  }  
  #endif
  startTimer();
  #ifdef _PWMA
  startPumps();
  #endif
  p(F("Boot Completed. "));
  logMessage(F("Initialization Completed."));
  logMessage(freeRam());
  delay(1000);
  beep2();
  lcd.clear();
}


void loop() {
  currentTime = millis();
  deltaTime = currentTime - previousTime;
  if (deltaTime > 16) { //60hz ~ every 16ms
    counter++;
    profile(true);
    time_t timenow = now();
    static time_t lasttimenow = 0;
    if (timenow!=lasttimenow) {
      ptime(timenow); //print time once a second
      lasttimenow = timenow;
    }
    static uint8_t lastdisplaymode = 255;
    if (displaymode!=lastdisplaymode) {
      switch (displaymode) { //switch 2nd line every 5 secs
        case 0: if (!psensors()) {
                  displaymode=255;
                }
                break;
        case 1: poutlets();
                break;
        case 2: pinputs();
                break;
#if defined(_PWMA)
        case 3: ppwmpump(0);
                break; 
#endif                
#if defined(_PWMB)
        case 4: ppwmpump(2);
                break; 
#endif
#ifdef SHOWDEBUGONLCD
        case LCD_NUM_MSGS-1: pdebug();
                break;
#endif
      }
      lastdisplaymode = displaymode;
#if (LCD_BACKLIGHT_OFF_DELAY_MINS>0)
      if (backlighton && (currentTime-lcdontime)>LCD_BACKLIGHT_OFF_DELAY_MINS*60000) {
        backlighton=false;
        lcd.setBacklight(LOW);
      }
#endif
    }
#ifdef AUTODST
    if (testDst) {
      autoDST(timenow);
      testDst = false;
    }
#endif
    if (logSensorsFlag) { //every 10 minutes only
      logSensorsFlag = false;
      logSensors();
    }
    if (emailFlag) {
      sendEmail();
      emailFlag=false;
    }
    if (logalarmFlag) {
      logAlarm();
      logalarmFlag=false;
    }
    if (netCheckFlag) {
      netCheck();
      netCheckFlag=false; 
    }
    if (updateRTCFlag) {
      updateRTC();
      updateRTCFlag=false; 
    }
#ifdef _DOSER
    if (updateDoserStatusFlag) {
      writeDoserStatus();
      updateDoserStatusFlag=false;
    }
#endif
#ifdef _SONAR
    if (counter%50==0) {
      updateSonar();
    }
#endif
#ifdef _PWMFAN
    if (counter%60==0) {
      updatePWMFans();
    }
#endif
#ifdef _PH
    if (counter%65==0) {
      static uint8_t phidx = 0;
      ph[phidx++]->update();
      phidx%=MAXPH;
    }
#endif
#ifdef _ORP
    if (counter%70==0) {
      orp.update();
    }
#endif
#ifdef _COND
    if (counter%80==0) {
      cond.update();
    }
#endif
    logOutlet();
    previousTime = currentTime;
    profile(false);
  }
  //do every cycle  
#ifdef _TEMP
  updateTemp(); //this takes 500us
#endif
  webprocess();
}


void profile(boolean start) {
  //debug1 and debug2 can be any number you want to display on lcd for debugging purposes
  //here I display the current and average execution time in microseconds
  //profile function is safe to call inside ISR
  //just add profile(true) at the start of function
  //and add profile(false) at end of function (or before every return)
  static uint32_t time1;
  if (start){
    time1 = micros();
    return;
  }
  uint8_t saveSREG=SREG;
  cli();
  debug2 = micros()-time1;
  static uint32_t sum = 0;
  if (sum) {
    sum = (sum-debug1)+debug2;
  } else {
    if (debug2>0)
      sum = debug2 * 60;
  }
  debug1=sum/60;
  SREG=saveSREG;
}

