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
#include <Flash.h>
#include <stdlib.h>
#include "TinyWebServer.h"
#include "config.h"

static uint32_t currentTime = 0;
static uint32_t previousTime = 0;
static uint16_t deltaTime = 0;

SdFat sd;
SdFile file_;
volatile uint32_t debug1=0;
volatile uint32_t debug2=0;
LiquidCrystal_I2C lcd(LCD_ADDR,LCD_EN,LCD_RW,LCD_RS,LCD_D4,LCD_D5,LCD_D6,LCD_D7,LCD_BACKLIGHT,NEGATIVE);

enum outlet {OUTLETDEFS, Feeder=20, Doser0=23, Doser1=24, Pump0=30, Pump1=31, Pump2=32, Pump3=33, PPump0=40, PPump1=41, End=255};

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
AtlasSensorDef_t phdata[MAXPH] = PHDEF;
#endif
#ifdef _ORP
AtlasSensorDef_t orpdata = ORPDEF;
#endif
#ifdef _COND
AtlasSensorDef_t conddata = CONDDEF;
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
  initBuzzer();
  initTimer();
  beep();
  initializeEEPROM();
  #if defined(_FEEDER) || defined(_FEEDER_V2)
  initFeeder();
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
  initPWM();
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
        case LCD_NUM_MSGS-1: pdebug();
                break;
      }
      lastdisplaymode = displaymode;
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
#ifdef _PH
    static uint8_t phidx = 0;
    updateAtlas(phdata[phidx++]);
    phidx%=MAXPH;
#endif
#ifdef _ORP
    updateAtlas(orpdata);
#endif
#ifdef _COND
    updateAtlas(conddata);
#endif
#ifdef _SONAR
    updateSonar();
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

