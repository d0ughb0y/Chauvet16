#define CONTROLLER_NAME "Jerry's Reef" //change this to your controller name
#define NTPSERVER 193,193,193,107  //pool.ntp.org
#define LOCAL_IP 192,168,1,15 //change this to a local fixed ip address
#define ROUTER_IP 192,168,1,1 //change this to your router ip address
#define ROUTER_PORT 80 //usually port 80 for the ip configuration web page
//#define _HEATER
#define _FAN
#define _SONAR
#define _FEEDER
//#define _X10
#define RTC_ADDR 0x68
#define LCD_ADDR 0x20
#define LCD_ROWS 2
#define LCD_COLS 16
#define LCD_NUM_MSGS 4
#define LCD_MSG_CYCLE_SECS 2
#define LCD_EN 4
#define LCD_RW 5
#define LCD_RS 6
#define LCD_D4 0
#define LCD_D5 1
#define LCD_D6 2
#define LCD_D7 3
#define LCD_BACKLIGHT 7

#define STDTZOFFSET -8L
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
#define OUTLET1 "Koralia1"
#define OUTLET2 "Koralia2"
#define OUTLET3 "Koralia3"
#define OUTLET4 "Koralia4"
#define OUTLET5 "Skimmer"
#define OUTLET6 "Kalk"
#define OUTLET7 "Fan"
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

#define OUTLETDEFS Koralia1, Koralia2, Koralia3, Koralia4, Skimmer, Kalk, Fan, Return,\
                   Unused9, Unused10, Unused11, Unused12, Unused13, Unused14, Unused15, Unused16
//define the default outlet program here. outlets must appear in exact order defined in outlet names definition
//program outletname, initial off time, on time, off time, days active, mode
#define OUTLETSDEFAULT {{OUTLET1,0,1200,900,0xff,_auto},\
                        {OUTLET2,300,1200,600,0xff,_auto},\
                        {OUTLET3,600,1200,300,0xff,_auto},\
                        {OUTLET4,900,1200,0,0xff,_auto},\
                        {OUTLET5,0,SECS_PER_DAY,0,0xff,_auto},\
                        {OUTLET6,0,0,0,0xff,_auto},\
                        {OUTLET7,0,0,0,0xff,_auto},\
                        {OUTLET8,0,SECS_PER_DAY,0,0xff,_auto}}      
#define MACROSDEFAULT {{MACRO1,0,1,0,0xff,_disabled},\
                       {MACRO2,15*SECS_PER_HOUR,240,SECS_PER_DAY-15*SECS_PER_HOUR-240,0xff,_auto},\
                       {MACRO3, 0,10*SECS_PER_MIN,0,0xff,_disabled},\
                       {MACRO4,10*SECS_PER_MIN,0,0,0xff,_disabled}}                    
#define ACTIONSDEFAULT {{{Feeder,0,2},{End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}, {End,0,0}},\
                        {{Koralia1,240,0},{Koralia2,240,0},{Koralia3,240,0},{Koralia4,240,0},{Return,150,90},{Feeder,30,2}},\
                        {{Koralia1,0,600},{Koralia2,0,600},{Koralia3,0,600},{Koralia4,0,600},{End,0,0}, {End,0,0}},\
                        {{Koralia1,600,0},{Koralia2,600,0},{Koralia3,600,0},{Koralia4,600,0},{End,0,0}, {End,0,0}}}
                        
                       
/////////////////////////////////
//   typedefs
/////////////////////////////////
typedef enum {_auto, _manual, _disabled, _macro };

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
