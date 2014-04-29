/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
#define CONTROLLER_NAME "Jerry's Reef" //change this to your controller name
#define NTPSERVER 193,193,193,107  //pool.ntp.org
#define LOCAL_IP 192,168,1,15 //change this to a local fixed ip address
#define ROUTER_IP 192,168,1,1 //change this to your router ip address
#define ROUTER_PORT 80 //usually port 80 for the ip configuration web page
#define _TEMP  //comment out if no temp probe
#define _PH  //comment out if no ph probe or ph stamp
#define _HEATER
//#define _FAN
#define _SONAR
#define _FEEDER
#define RTC_ADDR 0x68
#define LCD_ADDR 0x20
#define LCD_ROWS 2
#define LCD_COLS 16
#define LCD_NUM_MSGS 4+MAXPWMPUMPS/2
#define LCD_MSG_CYCLE_SECS 2
#define LCD_EN 4
#define LCD_RW 5
#define LCD_RS 6
#define LCD_D4 0
#define LCD_D5 1
#define LCD_D6 2
#define LCD_D7 3
#define LCD_BACKLIGHT 7

#define STDTZOFFSET -8
//comment the next line if your location does not use daylight savings time
#define AUTODST

#define WEBSERVERPORT 8000
//go to http://base64-encoder-online.waraxe.us
//to create your encoded string
//BASICAUTH = username:password,  sample below is admin:password
//SMTPUSER = your email login, sample below is admin
//SMTPPASSWORD = your email password, sample below is password
#define BASICAUTH "YWRtaW46cGFzc3dvcmQ="
#define SMTPSERVER "my.smtp.net"
#define SMTPPORT 587
#define SMTPUSER "YWRtaW4="
#define SMTPPASSWORD "cGFzc3dvcmQ="
#define EMAILFROM "user@host.com"
#define EMAILTO "8005551212@txt.att.net"
#define SD_CS 4
#define ETHER_CS 10
#define OUTLOGSZ 6  //size of circular queue for outlet log

#define MAXOUTLETS  8 //either 8 or 16, currently no support for 16 yet
#define MAXMACROS  4 //fixed
#define MAXMACROACTIONS 6  //fixed for now, can be made longer if needed
#define EEPROMSIG 0xA0 //change this everytime you want the eeprom defaults to change
//define OUTLET names here, order is fixed
#define OUTLET1 "WP25"
#define OUTLET2 "Unused2"
#define OUTLET3 "WP25B"
#define OUTLET4 "Pump"
#define OUTLET5 "Heater"
#define OUTLET6 "Kalk"
#define OUTLET7 "Skimmer"
#define OUTLET8 "Return"
#define OUTLET9 "Unused"
#define OUTLET10 "Unused"
#define OUTLET11 "Unused"
#define OUTLET12 "Unused"
#define OUTLET13 "Unused"
#define OUTLET14 "Unused"
#define OUTLET15 "Unused"
#define OUTLET16 "Unused"

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

#define OUTLETDEFS WP25, Unused2, WP25B, Pump, Heater, Kalk, Skimmer, Return,\
                   Unused9, Unused10, Unused11, Unused12, Unused13, Unused14, Unused15, Unused16
//define the default outlet program here. outlets must appear in exact order defined in outlet names definition
//program outletname, initial off time, on time, off time, days active, mode
//days active can be set by ORing the days or use EVERYDAY
//example, for MWF, set as MON|WED|FRY
//if outlet state is reversed OR INVCYCLE to days active value
//example EVERYDAY|INVCYCLE
#define OUTLETSDEFAULT {{OUTLET1,0,SECS_PER_DAY,0,EVERYDAY,_auto},\
                        {OUTLET2,0,0,0,EVERYDAY,_auto},\
                        {OUTLET3,0,SECS_PER_DAY,0,EVERYDAY,_auto},\
                        {OUTLET4,0,SECS_PER_DAY,0,EVERYDAY,_auto},\
                        {OUTLET5,0,0,0,EVERYDAY,_auto},\
                        {OUTLET6,0,0,0,EVERYDAY,_auto},\
                        {OUTLET7,0,SECS_PER_DAY,0,EVERYDAY,_auto},\
                        {OUTLET8,0,SECS_PER_DAY,0,EVERYDAY,_auto}}
#define MACROSDEFAULT {{MACRO1,0,10,0,0xff,_disabled},\
                       {MACRO2,15*SECS_PER_HOUR,240,SECS_PER_DAY-15*SECS_PER_HOUR-240,EVERYDAY,_auto},\
                       {MACRO3, 0,10*SECS_PER_MIN,0,EVERYDAY,_disabled},\
                       {MACRO4,10*SECS_PER_MIN,0,0,EVERYDAY,_disabled}}
#define ACTIONSDEFAULT {{{Feeder,5,1},{End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}},\
                        {{Feeder,30,210},{Return,150,90}, {Pump0,240,0}, {End,0,0}, {End,0,0}, {End,0,0}},\
                        {{WP25,0,600}, {Pump0,600,0}, {End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}},\
                        {{WP25,600,0}, {End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}}}
//controls 1-2 pwm pumps                        
#define _PWMA
//uncomment if you have 3 or 4 pumps
//#define _PWMB
#if defined(_PWMB)
#define MAXPWMPUMPS 4
#elif !defined(_PWMB)
#define MAXPWMPUMPS 2
#endif
//change wavemode 6x per day at the followiing time (hours)
#define MAXINTERVALS 6
#define INTERVALS  {0,6,9,12,18,22}

//below are the settings I use on my pair of wp-25
//I set the default program to run pumps on H1 at 50% speed.
//change the program by specifying the {syncmode,wavemode,level,pulsewidth}
//in that order, 6x per pump. 
//valid level values are from 48-255, pulsewidth from 0-10
//if syncmode is not master, then the rest of the parameters don't need to be specified
#define PUMP0 {{_master,H1,128,0},{_master,W2,192,10},{_master,ELSE,192,10},{_master,W1,255,4},{_master,W3,255,10},{_master,H1,128,0}}
#define PUMP1 {{_sync},{_antisync},{_master,ELSE,192,10},{_antisync},{_antisync},{_sync}}
//#define PUMP0 {{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0}}
//#define PUMP1 {{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0}}
#define PUMP2 {{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0}}
#define PUMP3 {{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0},{_master,H1,128,0}}
#define CUSTOMPATTERN {128,180,220,228,220,180,188,196,200,196,188,160,\
                       168,176,180,176,168,140,148,150,148,144,136,128,\
                       128,177,218,246,255,246,218,177}     
#define CUSTOMPATTERNSTEPS 32

/////////////////////////////////
//   typedefs
/////////////////////////////////
enum {_auto, _manual, _disabled, _macro };
//pump 0 is always master
//pumps 1,2,3 can run in sync or antisync with respect to pump 0
enum {_master,_sync,_antisync};

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
  ORec_t outletRec[MAXOUTLETS];
  ORec_t macrosRec[MAXMACROS];  
  MacroActions_t actions[MAXMACROS][MAXMACROACTIONS];
  PumpDef_t pump[MAXPWMPUMPS][MAXINTERVALS];
  float htrlow;
  float htrhigh;
  float fanlow;
  float fanhigh;
  uint8_t sonarhigh;
  uint8_t sonarlow;
  uint8_t sonaralertval;
  boolean sonaralert;
  float alerttemplow;
  float alerttemphigh;
  float alertphlow;
  float alertphhigh;
  boolean soundalert;
  boolean emailalert;
  uint8_t initialized;
} conf_t;
