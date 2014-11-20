/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
//#define SHOWDEBUGONLCD
#define CONTROLLER_NAME "Jerry's Reef" //change this to your controller name
#define NTPSERVER 76,73,0,4  //pool.ntp.org, used if dns lookup fails
#define LOCAL_IP 192,168,0,15 //change this to a local fixed ip address
#define ROUTER_IP 192,168,0,1 //change this to your router ip address
#define DNS_IP 75,75,75,75 //dns of your internet service provider
#define MAC {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED}
#define ROUTER_PORT 80 //usually port 80 for the ip configuration web page
//////////////////////////////////////////////////
// DS18B20 Temperature Sensor Section
// Due to limited memory on Arduino,
// I do not recommend using more than 2 sensors
//////////////////////////////////////////////////
#define _TEMP  //comment out if no temp probe
//#define CELSIUS  //uncomment if using celsius unit
#define MAXTEMP 2 //number of temp sensors, up to 3 max, over 3 not recommended
#if !defined(_TEMP)
#define MAXTEMP 0
#endif
//edit the next line to specify the temp sensor name and address pair. One entry for each temp sensor
//The first temp is used to control heater and fan and MUST be present, the rest may not be present.
#define TEMPDEF {{"Temp",{0x28, 0xdf, 0x5d, 0x89, 0x05, 0x00, 0x00, 0xf8}},{"Ambient",{0x28,0xde,0x18,0x5a,0x05,0x00,0x00,0x7d}}}
#define TEMPALERT {{72,82},{60,88}}
//////////////////////////////////////////////////
// End DS18B20 Temperature Sensor Section
//////////////////////////////////////////////////
//////////////////////////////////////////////////
// Atlas Stamps Sensor Section
// Make sure you do not exceed 3 sensors
// As there are only 3 available Serial ports
//////////////////////////////////////////////////
#define _PH  //comment out if no ph probe or ph stamp
#define MAXPH 1 //number of ph stamps
#if !defined(_PH)
#define MAXPH 0
#endif
#define _PH1_NAME "pH"
#define _PH1_EZO false
#define _PH1_SERIAL Serial1 //you can select serial or i2c but not both
//#define _PH1_I2C 99  //default address is 99

#define _PH2_NAME "CalRX"
#define _PH2_EZO true
//#define _PH2_SERIAL Serial2 //you can select serial or i2c but not both
#define _PH2_I2C 99  //make sure i2c address is unique

#define PHALERT {{7.5}}
//uncomment the next line you have Atlas Orp sensor and make sure the addr has the right serial port
//#define _ORP
#if !defined(_ORP)
#define MAXORP 0
#else
#define MAXORP 1
#endif
#define _ORP_NAME "ORP"
//#define _ORP_SERIAL Serial2 //you can select serial or i2c but not both
#define _ORP_I2C 98  //default address is 98
#define _ORP_EZO true
#define ORPALERT {200,300}
//uncomment the next line if you have Atlas Conductivity sensor and make sure the addr has the right serial port or I2C address
#define _COND
//#define _CONDTEMPCOMPENSATE
#if !defined(_COND)
#define MAXCOND 0
#else
#define MAXCOND 1
#endif
#define _COND_NAME "Cond"
//#define _COND_SERIAL Serial3 //you can select serial or i2c but not both
#define _COND_I2C 100  //default address is 100
#define CONDALERT {25,45}
#define TOTALSENSORS MAXTEMP+MAXPH+MAXORP+MAXCOND
////////////////////////////////////////////////////
// End Atlas Stamps Sensor Section
////////////////////////////////////////////////////
#define _DOSER
#define MAXDOSERS 2
//default values  name (7 characters max), ml per day, times per day (0=disabled), interval (0=divide equally),
//starttime (minutes since midnight), calibration value (0=uncalibrated), full volume in ml
//MAKE SURE TO DEFINE AS MANY ENTRIES AS SPECIFIED IN MAXDOSERS
#define DOSERDEFAULT {"Cal",24,12,0,0,100,500},{"Alk", 24,12,0,10,100,500}
#define _HEATER
//#define _FAN
#define _SONAR
//#define _FEEDER
#define _FEEDER_V2

#ifndef _TEMP
#undef _FAN
#undef _HEATER
#endif

#ifdef SHOWDEBUGONLCD
#define LCD_NUM_MSGS 4+MAXPWMPUMPS/2
#else
#define LCD_NUM_MSGS 3+MAXPWMPUMPS/2
#endif
#define LCD_MSG_CYCLE_SECS 2
//you can just comment the line below to use 400khz TwoWire bus speed. It is not necessary to edit the Wire library
#define TW_400
#define RTC_ADDR 0x68

#define LCD_BACKLIGHT_OFF_DELAY_MINS 10
//I2C LCD Setting 1
#define LCD_ADDR 0x20
#define LCD_ROWS 2
#define LCD_COLS 16
#define LCD_EN 4
#define LCD_RW 5
#define LCD_RS 6
#define LCD_D4 0
#define LCD_D5 1
#define LCD_D6 2
#define LCD_D7 3
#define LCD_BACKLIGHT 7
#define LCD_POLARITY NEGATIVE

//I2C LCD setting 2
//#define LCD_ADDR 0x27
//#define LCD_ROWS 2
//#define LCD_COLS 16
//#define LCD_EN 2
//#define LCD_RW 1
//#define LCD_RS 0
//#define LCD_D4 4
//#define LCD_D5 5
//#define LCD_D6 6
//#define LCD_D7 7
//#define LCD_BACKLIGHT 3
//#define LCD_POLARITY POSITIVE

#define STDTZOFFSET -8
//comment the next line if your location does not use daylight savings time
#define AUTODST

//go to http://base64-encoder-online.waraxe.us
//to create your encoded string
//BASICAUTH = username:password,  sample below is admin:password
//SMTPUSER = your email login, sample below is admin
//SMTPPASSWORD = your email password, sample below is password
#define BASICAUTH "YWRtaW46cGFzc3dvcmQ="
#define SMTPSERVER "smtp.gmx.net"
#define SMTPPORT 587
#define SMTPUSER "YWRtaW4="
#define SMTPPASSWORD "cGFzc3dvcmQ="
#define EMAILFROM "user@gmx.com"
#define EMAILTO "8005551212@txt.att.net"

#define SD_CS 4
#define ETHER_CS 10
#define OUTLOGSZ 6  //size of circular queue for outlet log

#define MAXOUTLETS  8 //either 8 or 16
//#define OUTLET8INVERTED //uncomment this if you want outlets 1-8 to have inverse logic, like if you are using ULN2803
#define OUTLET16INVERTED  //uncomment this if you want outlets 9-16 to have inverse logic, like if you are using ULN2803
#define MAXMACROS  4 //fixed
#define MAXMACROACTIONS 6  //fixed for now, can be made longer if needed
#define EEPROMSIG 0xA0 //change this everytime you want the eeprom defaults to change
//define OUTLET names here, order is fixed
#define OUTLET1 "WP25"
#define OUTLET2 "Unused2"
#define OUTLET3 "LED"
#define OUTLET4 "Pump"
#define OUTLET5 "Heater"
#define OUTLET6 "ATO"
#define OUTLET7 "Skimmer"
#define OUTLET8 "Return"
#define OUTLET9 "Outlet9"
#define OUTLET10 "Outlet10"
#define OUTLET11 "Outlet11"
#define OUTLET12 "Outlet12"
#define OUTLET13 "Outlet13"
#define OUTLET14 "Outlet14"
#define OUTLET15 "Outlet15"
#define OUTLET16 "Outlet16"

#define MACRO1 "Feed Now"
#define MACRO2 "Feed"
#define MACRO3 "All Pumps On"
#define MACRO4 "All Pumps Off"

#define SUN _BV(1)
#define MON _BV(2)
#define TUE _BV(3)
#define WED _BV(4)
#define THU _BV(5)
#define FRI _BV(6)
#define SAT _BV(7)
#define EVERYDAY 0xFE
#define INVCYCLE _BV(0)
#define NORMALCYCLE 0

#define OUTLETDEFS WP25, Unused2, LED, Pump, Heater, ATO, Skimmer, Return,\
                   Outlet9, Outlet0, Outlet11, Outlet12, Outlet13, Outlet14, Outlet15, Outlet16
//define the default outlet program here. outlets must appear in exact order defined in outlet names definition
//program outletname, initial off time, on time, off time, days active, mode
//days active can be set by ORing the days or use EVERYDAY
//example, for MWF, set as MON|WED|FRI
//if outlet state is reversed OR INVCYCLE to days active value
//example EVERYDAY|INVCYCLE
//this is for 8 outlets only, you need to modify this if you use more than 8 outlets
#define OUTLETSDEFAULT {{OUTLET1,0,SECS_PER_DAY,0,EVERYDAY,_auto},\
                        {OUTLET2,0,0,0,EVERYDAY|INVCYCLE,_auto},\
                        {OUTLET3,0,SECS_PER_DAY,0,EVERYDAY,_auto},\
                        {OUTLET4,0,15*SECS_PER_MIN,3*SECS_PER_HOUR-15*SECS_PER_MIN,EVERYDAY,_auto},\
                        {OUTLET5,0,0,0,EVERYDAY,_auto},\
                        {OUTLET6,0,0,0,EVERYDAY,_auto},\
                        {OUTLET7,0,3*SECS_PER_HOUR,SECS_PER_HOUR,EVERYDAY,_auto},\
                        {OUTLET8,0,SECS_PER_DAY,0,EVERYDAY,_auto}}
//                        ,{OUTLET9,0,0,0,EVERYDAY,_auto},\
//                        {OUTLET10,0,0,0,EVERYDAY,_auto},\
//                        {OUTLET11,0,0,0,EVERYDAY,_auto},\
//                        {OUTLET12,0,0,0,EVERYDAY,_auto},\
//                        {OUTLET13,0,0,0,EVERYDAY,_auto},\
//                        {OUTLET14,0,0,0,EVERYDAY,_auto},\
//                        {OUTLET15,0,0,0,EVERYDAY,_auto},\
//                        {OUTLET16,0,0,0,EVERYDAY,_auto}}
#define MACROSDEFAULT {{MACRO1,0,10,0,0xff,_disabled},\
                       {MACRO2,15*SECS_PER_HOUR,240,SECS_PER_DAY-15*SECS_PER_HOUR-240,SUN|TUE|THU|SAT,_auto},\
                       {MACRO3, 0,10*SECS_PER_MIN,0,EVERYDAY,_disabled},\
                       {MACRO4,10*SECS_PER_MIN,0,0,EVERYDAY,_disabled}}
#define ACTIONSDEFAULT {{{Feeder,5,1},{End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}},\
                        {{Feeder,30,1},{Return,150,90}, {Pump0,240,0}, {End,0,0}, {End,0,0}, {End,0,0}},\
                        {{WP25,0,600}, {Pump0,600,0}, {End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}},\
                        {{WP25,600,0}, {End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}}}
//controls 1-2 pwm pumps                        
#define _PWMA //must be defined
//uncomment if you have 3 or 4 pumps
//#define _PWMB
#if defined(_PWMB)
#define MAXPWMPUMPS 4
#elif !defined(_PWMB)
#define MAXPWMPUMPS 2
#endif
//change wavemode 6x per day at the following time (hours)
#define MAXINTERVALS 6
#define INTERVALS  {0,6,9,12,18,22}

//below are the settings I use on my pair of wp-25
//I set the default program to run pumps on H1 at 50% speed.
//change the program by specifying the {syncmode,wavemode,level,pulsewidth}
//in that order, 6x per pump. 
//valid level values are from 48-255, pulsewidth from 0-10
//if syncmode is not master, then the rest of the parameters don't need to be specified
#define PUMP0 {{_master,H1,115,0},{_master,W2,153,10},{_master,ELSE,153,10},{_master,W1,191,4},{_master,W3,191,10},{_master,H1,115,0}}
#define PUMP1 {{_sync},{_antisync},{_master,ELSE,153,10},{_antisync},{_antisync},{_sync}}
//#define PUMP0 {{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0}}
//#define PUMP1 {{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0}}
#define PUMP2 {{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0}}
#define PUMP3 {{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0}}
#define CUSTOMPATTERN {128,180,220,228,220,180,188,196,200,196,188,160,\
                       168,176,180,176,168,140,148,150,148,144,136,128,\
                       128,177,218,246,255,246,218,177}     
#define CUSTOMPATTERNSTEPS 32

//#define _PWMFAN
#define MAXPWMFANS 3  //maximum 3
#ifndef _PWMFAN
#define MAXPWMFANS 0
#endif
//define name,tempsensor#,low temp, high temp, low fan level, high fan level, mode, alert
#define PWMFANDEF {{"DispFan",0,78,82,30,100,_auto,1500,false},{"SumpF1",1,76,80,30,100,_auto,1200,false},{"SumpF2",1,76,80,30,100,_auto,1200,false}}

#if defined(_PWMFAN) && !defined(_TEMP)
#error _TEMP must be defined if _PWMFAN is defined
#endif

//connect buzzer to pin D9, uncomment next line if buzzer is passive type
//#define PASSIVEBUZZER
/////////////////////////////////
//   typedefs
/////////////////////////////////
enum {_auto, _manual, _disabled, _macro };
//pump 0 is always master
//pumps 1,2,3 can run in sync or antisync with respect to pump 0
enum {_master,_sync,_antisync};
enum {_temp,_ph,_orp,_cond,_sonar,_fantach};

//function that returns 8 bit value for the given step over the resolution of the wave pattern
typedef uint8_t(*PatternFP)(uint8_t);

typedef struct {
  char       name[5];
  uint8_t    resolution;
  PatternFP  _getLevel;
} WaveDef_t;

typedef struct {
  uint8_t   syncMode;
  uint8_t   waveMode;
  uint8_t   level;//48-255
  uint8_t   pulseWidth;//0-10
} PumpDef_t;

typedef struct {
  char     name[8];
  uint16_t dailydose; //volume per day, all volume units in ml
  uint8_t  dosesperday; //divide by this over 24 hour perdiod, choices are 1,2,4,6,12,24,48,96,144,240
  uint8_t  interval; //0=auto, spread evenly over 24 hours, otherwise interval in minutes, up to 255
  uint16_t starttime; //use this for non even interval, otherwise defaults to midnight
  uint16_t rate; //calibration value, ticks per ml of liquid, 1 tick=1.024ms, 980 ticks = 1 sec.
  uint16_t fullvolume;
} DoserDef_t;

typedef struct {
  char     name[14];
  time_t   initoff;
  time_t   ontime;
  time_t   offtime;
  uint8_t  days;
  uint8_t  mode;
} ORec_t;

typedef struct {
  time_t timestamp;
  uint8_t changed;
  boolean state;
} OutLogRec_t;

typedef struct {
  uint8_t outlet;
  time_t initoff;
  time_t ontime;
} MacroActions_t;

typedef struct {
  float lowalert;
  float highalert;
  uint8_t type;
} SensorAlert_t;

typedef struct {
  char name[8];
  uint8_t tempsensor; //temp sensor associated with this fan
  uint8_t templow;
  uint8_t temphigh;
  uint8_t levellow;
  uint8_t levelhigh;
  uint8_t mode;
  uint16_t maxrpm;
  boolean alert;
} FanDef_t;

typedef struct {
  ORec_t outletRec[MAXOUTLETS];
  ORec_t macrosRec[MAXMACROS];  
  MacroActions_t actions[MAXMACROS][MAXMACROACTIONS];
  PumpDef_t pump[MAXPWMPUMPS][MAXINTERVALS];
  DoserDef_t doser[MAXDOSERS];
  SensorAlert_t alert[TOTALSENSORS];
  FanDef_t pwmfan[MAXPWMFANS];
  float htrlow;
  float htrhigh;
  float fanlow;
  float fanhigh;
  uint8_t sonarhigh;
  uint8_t sonarlow;
  uint8_t sonaralertval;
  boolean sonaralert;
  boolean soundalert;
  boolean emailalert;
  uint8_t initialized;
} conf_t;

typedef struct {
  char name[8];
  uint8_t addr[8];
  volatile boolean initialized;
  volatile uint16_t average;
  uint16_t sum;
} TempSensorDef_t;
