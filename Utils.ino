/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
static ofstream fileout_;

void initUtils() {
  pinMode(13,OUTPUT);  //led pin
  initLCD();
  initSD();
}

inline void lightToggle() {
  PINB |= _BV(PB7);
}

inline void lightOn() {
  PORTB |= _BV(PB7); 
}

inline void lightOff() {
  PORTB &= ~_BV(PB7);
}

char* freeRam () {
  static char value[6];
  extern int __heap_start, *__brkval; 
  int v; 
  return itoa((int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval),value,10); 
}


///////////////////////////////////////
//   EEPROM
////////////////////////////////////////
#include <avr/eeprom.h>
#define DOSERBLOCKSIZE 4*MAXDOSERS
#define DCADDR E2END-DOSERBLOCKSIZE

void readEEPROM() {
  eeprom_read_block((void*)&conf, (void*)0, sizeof(conf));
}

void writeEEPROM() {
  eeprom_write_block((const void*)&conf, (void*)0, sizeof(conf));
}
#ifdef _DOSER
void readDoserStatus(){
  eeprom_read_block((void*)&dosedvolume, (void*)(DCADDR), DOSERBLOCKSIZE);
  if (dosedvolume[0]>conf.doser[0].fullvolume*100ul) {
     //initialize
    memset((void*)&dosedvolume,0,DOSERBLOCKSIZE);
    writeDoserStatus();
  }
}

void writeDoserStatus( ){
  eeprom_write_block((const void*)&dosedvolume, (void*)(DCADDR), DOSERBLOCKSIZE);
}
#endif

void initializeConf() {
  ORec_t defaultorec[] = OUTLETSDEFAULT;
  for (int i=0;i<MAXOUTLETS;i++) {
    strcpy((char*)conf.outletRec[i].name, defaultorec[i].name);
    conf.outletRec[i].initoff=defaultorec[i].initoff;
    conf.outletRec[i].ontime=defaultorec[i].ontime;
    conf.outletRec[i].offtime=defaultorec[i].offtime;
    conf.outletRec[i].days=defaultorec[i].days;
    conf.outletRec[i].mode= defaultorec[i].mode;
  }
  ORec_t defaultmacros[] = MACROSDEFAULT;
  for (int i=0;i<MAXMACROS;i++) {
    strcpy((char*)conf.macrosRec[i].name,defaultmacros[i].name);
    conf.macrosRec[i].initoff=defaultmacros[i].initoff;
    conf.macrosRec[i].ontime=defaultmacros[i].ontime;
    conf.macrosRec[i].offtime=defaultmacros[i].offtime;
    conf.macrosRec[i].days=defaultmacros[i].days;
    conf.macrosRec[i].mode=defaultmacros[i].mode;  
  }
  MacroActions_t defaultactions[][MAXMACROACTIONS] = ACTIONSDEFAULT;
  for (int i=0;i<MAXMACROS;i++) {
    for (int j=0;j<MAXMACROACTIONS;j++) {
      conf.actions[i][j].outlet= defaultactions[i][j].outlet;
      conf.actions[i][j].initoff=defaultactions[i][j].initoff;
      conf.actions[i][j].ontime=defaultactions[i][j].ontime;
    }  
  }
  PumpDef_t defaultpump[][6] = {
                            #ifdef _PWMA
                            PUMP0,PUMP1
                            #endif
                            #ifdef _PWMB
                            ,PUMP2,PUMP3
                            #endif
                            };
  for (int i=0;i<MAXPWMPUMPS;i++) {
    for (int j=0;j<MAXINTERVALS;j++) {
      conf.pump[i][j].syncMode=defaultpump[i][j].syncMode;
      conf.pump[i][j].waveMode=defaultpump[i][j].waveMode;
      conf.pump[i][j].level=defaultpump[i][j].level;
      conf.pump[i][j].pulseWidth=defaultpump[i][j].pulseWidth;
    }
  }    
  DoserDef_t doserdefault[] = {DOSERDEFAULT};
  for (int i=0;i<MAXDOSERS;i++) {
     strcpy((char*)conf.doser[i].name, doserdefault[i].name);
     conf.doser[i].dailydose = doserdefault[i].dailydose;
     conf.doser[i].dosesperday = doserdefault[i].dosesperday;
     conf.doser[i].interval = doserdefault[i].interval;
     conf.doser[i].starttime = doserdefault[i].starttime;
     conf.doser[i].rate = doserdefault[i].rate;
     conf.doser[i].fullvolume=doserdefault[i].fullvolume;
  }
#ifdef _TEMP
  SensorAlert_t tempalerts[]=TEMPALERT;
  for (int i=0;i<MAXTEMP;i++) {
    conf.alert[i].lowalert=tempalerts[i].lowalert;
    conf.alert[i].highalert=tempalerts[i].highalert;
    conf.alert[i].type=_temp;
  }
#endif
#ifdef _PH
  SensorAlert_t phalerts[]=PHALERT;
  for (int i=0;i<MAXPH;i++){
    conf.alert[MAXTEMP+i].lowalert=phalerts[i].lowalert;
    conf.alert[MAXTEMP+i].highalert=phalerts[i].highalert;
    conf.alert[MAXTEMP+i].type=_ph;
  }
#endif
#ifdef _ORP
  SensorAlert_t orpalert = ORPALERT;
  conf.alert[MAXTEMP+MAXPH].lowalert=orpalert.lowalert;
  conf.alert[MAXTEMP+MAXPH].highalert=orpalert.highalert;
  conf.alert[MAXTEMP+MAXPH].type=_orp;
#endif
#ifdef _COND
  SensorAlert_t condalert = CONDALERT;
  conf.alert[MAXTEMP+MAXPH+MAXORP].lowalert=condalert.lowalert;
  conf.alert[MAXTEMP+MAXPH+MAXORP].highalert=condalert.highalert;
  conf.alert[MAXTEMP+MAXPH+MAXORP].type=_cond;
#endif
#ifdef _PWMFAN
  FanDef_t fan[]=PWMFANDEF;
  for (uint8_t i=0;i<MAXPWMFANS;i++) {
    strcpy((char*)conf.pwmfan[i].name,fan[i].name);
    conf.pwmfan[i].tempsensor=fan[i].tempsensor;
    conf.pwmfan[i].templow=fan[i].templow;
    conf.pwmfan[i].temphigh=fan[i].temphigh;
    conf.pwmfan[i].levellow=fan[i].levellow;
    conf.pwmfan[i].levelhigh=fan[i].levelhigh;
    conf.pwmfan[i].mode=fan[i].mode;
    conf.pwmfan[i].alert=fan[i].alert;
    conf.pwmfan[i].maxrpm=fan[i].maxrpm;
  }
#endif
  conf.fanlow = 80.0;
  conf.fanhigh = 80.5;
  conf.htrlow = 78.3;
  conf.htrhigh = 79.0;
  conf.sonarlow = 40;
  conf.sonarhigh = 5;
  conf.sonaralertval = 37;
  conf.sonaralert = false;
  conf.soundalert=false;
  conf.emailalert=false;
  conf.initialized=EEPROMSIG;
}

void initializeEEPROM() {
  readEEPROM();
  if (conf.initialized!=EEPROMSIG) {
    delay(500);
    initializeConf();
    beepOK();
    writeEEPROM();
    readEEPROM();
  }
}

//////////////////////////////////////////
//   LCD
/////////////////////////////////////////
//static uint8_t line = 0;
static boolean lcdpresent = false;

void initLCD(){
  lcd.begin(LCD_COLS,LCD_ROWS);
#ifdef TW_400
  TWBR=12;
#endif
  lcdpresent = checkLCD();
#ifdef LCD_BACKLIGHT_OFF_DELAY_MINS
  backlighton = true;
  lcdontime=millis();
#endif
  p(F("LCD init.       "));
}

boolean checkLCD() {
  Wire.beginTransmission(LCD_ADDR);
  return Wire.endTransmission()==0;
}

void p(const __FlashStringHelper *ifsh) {
  static const __FlashStringHelper *ifsh0=0;
  static boolean first=true;
  if (first) {
    lcd.setCursor(0,0);
    lcd << ifsh;
    first=false;
  } else if (ifsh0==0){
    lcd.setCursor(0,1);
    lcd << ifsh;
    ifsh0=ifsh;
  } else {
    lcd.setCursor(0,0);
    lcd << ifsh0;
    lcd.setCursor(0,1);
    lcd <<ifsh;
    ifsh0=ifsh;
  }
  delay(800);
}

boolean psensors() {
  static uint8_t state = 0;
  static uint8_t x=0;  //sensors printed so far, print 2 at a time
  char buffer[5];
  if (!lcdpresent) return true;
  lcd.setCursor(0,1);
  if (x>=maxsensors) {
    x=0;
    state=0;
  }
  switch (state) {
  case 0:
#ifdef _TEMP
#if MAXTEMP==1
    lcd << F("T:") << getTemp(0) << F(" ");
    x++;
#else
    static int tindex = 0;
    while (tindex<MAXTEMP)  {
      lcd << F("T") << tindex << F(":") << dtostrf(getTemp(tindex),4,1,buffer) << F(" ");
      x++;
      tindex++;
      if (x%2==0) return x>=maxsensors;
    };
    tindex=0;
#endif
#endif
    state=1;
case 1:
#ifdef _PH
#if MAXPH==1
  lcd << F("pH:") << ph[0]->getVal()  << F(" ");
  x++;
  if (x%2==0) return x>=maxsensors;
#else
  static int pindex = 0;
  while (pindex<MAXPH)  {
    lcd << F("P") << pindex<<  F(":") << ph[pindex]->getVal() << F(" ");
    x++;
    pindex++;
    if (x%2==0) return x>=maxsensors;
  };
  pindex=0;
#endif
#endif
  state=2;
 case 2:
#ifdef _COND
 lcd << F("C:") << cond.getVal() << F("  ");
 x++;
 if (x%2==0) return x>=maxsensors;
#endif
  state=3;
  case 3:
#ifdef _ORP
    lcd << F("O:") << orp.getVal() << F("   ");
    x++;
    if (x%2==0) return x>=maxsensors;
#endif
    state=0;
  }
  if (x>=maxsensors) {
    x=0;
    return true;
  } else
    return false;
}

void ptime(time_t t) {
  if (!lcdpresent) {
    if (checkLCD()) {
      lcd.begin(LCD_COLS,LCD_ROWS);
#if (LCD_BACKLIGHT_OFF_DELAY_MINS>0)
      backlighton=true;
      lcdontime=millis();
#endif
    } else {
      return;
    }
  }
  lcd.setCursor(0,0);

  tmElements_t tm;
  breakTime(t, tm);
  lcd << (tm.Month<10?"0":"") << tm.Month ;
  lcd << (tm.Day<10?"0":"") << tm.Day ;
  lcd << tmYearToCalendar(tm.Year)-2000 << F(" ") ;
  lcd << (tm.Hour<10?"0":"") << tm.Hour << F(":");
  lcd << (tm.Minute<10?"0":"") << tm.Minute << F(":");
  lcd << (tm.Second<10?"0":"") << tm.Second;
  if (lcdpresent!=checkLCD()) {
    lcdpresent = checkLCD();
    if (lcdpresent)
      logMessage(F("LCD is present."));
    else
      logMessage(F("LCD not connected."));
  }
}

void pinputs() {
  if (!lcdpresent) return;
  lcd.setCursor(0,1);  
  lcd << F("TO:") << getSonarPct() << F(" A1:") << inputstate(getATO1()) << F(" A2:") << inputstate(getATO2()); 
}

char inputstate(uint8_t val) {
   return val==0?'C':'O'; 
}
void poutlets() {
  if (!lcdpresent) return;
  lcd.setCursor(0,1);
#ifdef OUTLET8INVERTED
  uint8_t oa = PORTA;
#else
  uint8_t oa = ~PORTA;
#endif
#ifdef OUTLET16INVERTED
  uint8_t ob = PORTC;
#else
  uint8_t ob = ~PORTC;
#else
  uint8_t ob = PORTC;
#endif
  for (int p=0;p<MAXOUTLETS;p++) {
    if (p<8) {
      if (oa & _BV(p)) {
        lcd << (char)(conf.outletRec[p].mode==_auto?('0'+p):'M');
      } 
      else {
        lcd << (char)(conf.outletRec[p].mode==_auto?'_':'m'); 
      } 
    } else {
      if (ob & _BV(p)) {
        lcd << (char)((conf.outletRec[p].mode==_auto)?(p<10?('0'+p):('7'+p)):'M');
      } else {
        lcd << (char)((conf.outletRec[p].mode==_auto)?'_':'m'); 
      } 
    }
  }
  if (MAXOUTLETS==8)
    lcd << F("        ");
}

void ppwmpump(uint8_t chan){
  if (!lcdpresent) return;
  lcd.setCursor(0,1);
  uint8_t llevel[2];
  uint8_t lwaveMode[2];  
  cli();
  for (int i=0;i<2;i++) {
    if (_syncMode[i+chan]!=_master) {
      llevel[i]=_level[0];
      lwaveMode[i]=_waveMode[0];
    } else {
      llevel[i]=_level[i+chan];
      lwaveMode[i]=_waveMode[i+chan];
    }
  }
  sei();
  for (int i=0;i<2;i++) {
    uint8_t namelen = strlen(wave[lwaveMode[i]].name);
    lcd << wave[lwaveMode[i]].name << (namelen<4?" ":"");
    if (llevel[i]==255) {
      if (namelen<4)
        lcd << F("100%");
      else
        lcd << F("100");
    } else
      lcd <<(uint8_t)llevel[i]*100/255 << F("%");
    if (i==0) lcd << F(" ");  
  }
  lcd << F("   ");
}

void pdebug() {
  if (!lcdpresent) return;
  lcd.setCursor(0,1);
  cli();
  uint32_t d1 = debug1;
  uint32_t d2 = debug2;
  sei();
  lcd << d1 << F("us ") << d2 << F("us            ");
}

///////////////////////////////
//   SD
//////////////////////////////
void initSD(){
  pinMode(ETHER_CS,OUTPUT);
  digitalWrite(ETHER_CS,HIGH);
  pinMode(SD_CS,OUTPUT);
  digitalWrite(SD_CS,HIGH);
  SdFile::dateTimeCallback(dateTime);
  if (!sd.begin(SD_CS, SPI_FULL_SPEED)){
    p(F("SD Error        "));
  } else {
    p(F("SD OK.          ")); 
  }
}

void logOutlet() {
  cli();
  uint8_t ltail=_tail;
  uint8_t lhead=_head;
  time_t ts = _outlog[ltail].timestamp;
  sei();
  if (lhead != ltail) {
    char buffer[20];
    boolean inoutlog = false;
    if (logSetup(ts, buffer, "OUT","txt")) {
      time_t currtime = ts;
      while (lhead != ltail) {
        fileout_ << F("<record><date>") << buffer << F("</date><name>");
        if (_outlog[ltail].changed==Feeder) {
          fileout_ << F("Feeder");
        } else if (_outlog[ltail].changed==Doser0) {
          fileout_ << F("Doser0");
        } else if (_outlog[ltail].changed==Doser1) {
          fileout_ << F("Doser1");
        } else if (_outlog[ltail].changed==Doser2) {
          fileout_ << F("Doser2");
        } else if (_outlog[ltail].changed==Doser3) {
          fileout_ << F("Doser3");
        } else {
          fileout_ << (const char*)conf.outletRec[_outlog[ltail].changed].name;
        }
        fileout_ << F("</name><value>") << (_outlog[ltail].state?F("ON"):F("OFF"));
        fileout_ << F("</value></record>\n");
        if (++ltail>=OUTLOGSZ) {
          ltail=0;
        }
        cli();
        ts = _outlog[ltail].timestamp;
        sei();
        if (ts!=currtime) break;
      }
      cli();
      _tail=ltail;
      sei();
      fileout_.close();
    }
  } 
}

void logSensors() {
  char buffer[20];
  if (logSetup(now2(),buffer,"SEN","txt")) {
    fileout_ << F("<record><date>") << buffer << F("</date>");
#ifdef _TEMP
    for (int i=0;i<MAXTEMP;i++) {
      fileout_ << F("<probe><name>") << tempdata[i].name << F("</name><value>");
      fileout_ << setprecision(1) << getTemp(i)<< F("</value></probe>");
    }
#endif
#ifdef _PH
    for (int i=0;i<MAXPH;i++) {
      fileout_ << F("<probe><name>") << ph[i]->getName() << F("</name><value>");
      fileout_ << setprecision(2);
      fileout_ << (ph[i]->getVal()<10.0?" ":"") << ph[i]->getVal() << F("</value></probe>");
    }
#endif
#ifdef _COND
      fileout_ << F("<probe><name>Cond</name><value>");
      fileout_ << setprecision(1);
      fileout_ << cond.getVal() << F("</value></probe>");
#endif
#ifdef _ORP
      fileout_ << F("<probe><name>Orp</name><value>");
      fileout_ << setprecision(0);
      fileout_ << orp.getVal() << F("</value></probe>");
#endif
    fileout_ << F("</record>\n");
    fileout_.close();
  }
}

void logMessage(const __FlashStringHelper *msg,const char* str) {
  char buffer[20];
  if (logSetup(now2(),buffer, "LOG","log")) {
    fileout_ << buffer << F(" ") << msg << F(" ") << str  << F("\n");
    fileout_.close();
  }   
}

void logMessage( const __FlashStringHelper *ifsh) {
  logMessage(ifsh,"");
}

void logMessage(char* msg) {
  logMessage(F(""),msg);
}

void logMessage(const __FlashStringHelper *msg, int n) {
  char num[5];
  logMessage(msg, itoa(n,num, 10));
}

void logMessage(uint8_t ip[], const char* msg) {
  char buffer[20];
  if (logSetup(now2(),  buffer, "LOG", "log")) {
     fileout_ << buffer << F(" ") << (int)ip[0] << F(".") << (int)ip[1] << F(".") << (int)ip[2];
     fileout_ << F(".") << (int)ip[3] << F(" ") << msg << F("\n");
     fileout_.close();
  }
}

void logAlarm() {
  char buffer[20];
  if (logSetup(now2(),  buffer, "LOG", "log")) {
     fileout_ << buffer << F(" Alarm Event: ");
#ifdef _TEMP
    for (uint8_t i=0;i<MAXTEMP;i++) {
       fileout_ << (const char*)tempdata[i].name << F(":") << getTemp(i) << F(" ");
    }
#endif
#ifdef _PH
    for (uint8_t i=0;i<MAXPH;i++) {
      fileout_ << (const char*)ph[i]->getName() << F(":") << ph[i]->getVal() << F(" ");
    }
#endif
#ifdef _COND
    fileout_ << F("Cond:") << cond.getVal() << F(" ");
#endif
#ifdef _ORP
    fileout_ << F("Orp:") << orp.getVal() << F(" ");
#endif
#ifdef _DOSER
    for (int i=0;i<MAXDOSERS;i++) {
      fileout_ << (const char*)conf.doser[i].name << F(":") << dosedvolume[i]/100.0 << F(" ");
    }
#endif
#ifdef _SONAR
    fileout_ << F("Sonar:") << (int)getSonarPct() << F("%");
#endif
#ifdef _PWMFAN
    for (uint8_t i=0;i<MAXPWMFANS;i++) {
      fileout_ << (const char*)conf.pwmfan[i].name << F(":") << getPWMFanRPM(i) << F("rpm ");
    }
#endif
    fileout_ << F("\n");
    fileout_.close();
  }
}

boolean logSetup(const time_t thetime, char* buffer, char* dir, char* ext) {
  getdatestring(thetime,buffer);
  char fullpath[13] = {
    '/',dir[0],dir[1],dir[2],'/',buffer[6],buffer[7],buffer[8],buffer[9],
    '/',buffer[0],buffer[1],0};
  char fname[7] ={ buffer[3],buffer[4],'.',ext[0],ext[1],ext[2],0};
  if (!sd.chdir(fullpath)) {  //LOG
    if (!sd.mkdir(fullpath,true)) {
      beep2();
      sd.begin(SD_CS, SPI_FULL_SPEED);
      return false;
    }
  }    
  fileout_.open(fname,ios::out|ios::app);
  if (!fileout_.is_open()) {
    fileout_.close();
    beepFail();
    sd.begin(SD_CS, SPI_FULL_SPEED);
    return false;
  }
  return true;
}

void dateTime(uint16_t* date, uint16_t* time) {
  time_t t = now2();
  *date = FAT_DATE(year(t), month(t),day(t));
  *time = FAT_TIME(hour(t), minute(t), second(t));
}

char* getdatestring(const time_t t, char* buffer) {
  tmElements_t tm;
  char* ret = buffer;
  breakTime(t, tm);
  *buffer++ = tm.Month/10 + '0';
  *buffer++ = tm.Month%10 + '0';
  *buffer++ = '/';
  *buffer++ = tm.Day/10 + '0';
  *buffer++ = tm.Day%10 + '0';
  *buffer++ = '/';
  uint16_t yr = tmYearToCalendar(tm.Year);
  *buffer++ = yr/1000 + '0';
  *buffer++ = yr%1000/100 + '0';
  *buffer++ = yr%100/10 + '0';
  *buffer++ = yr%10 + '0';
  *buffer++ = ' ';
  *buffer++ = tm.Hour/10 + '0';
  *buffer++ = tm.Hour%10 + '0';
  *buffer++ = ':';
  *buffer++ = tm.Minute/10 + '0';
  *buffer++ = tm.Minute%10 + '0';
  *buffer++ = ':';
  *buffer++ = tm.Second/10 + '0';
  *buffer++ = tm.Second%10 + '0';
  *buffer = 0;
  return ret;
}
void getfiles(EthernetClient& client, char* rootstr) {
  char fn[13];
  if (sd.chdir(rootstr)) {
    client << F("{\"files\":[\n");
    boolean first = true;
    while(file_.openNext(sd.vwd(),O_READ)) {
      if (!first) client << F(",\n");
      first=false;
      dir_t d;
      file_.dirEntry(&d);
      client << F("{\"n\":\"");
      file_.getFilename(fn);
      client << fn;
      client << F("\",\"s\":\"");
      if (!file_.isDir())
        client << file_.fileSize();
      else
        client <<  F("-1");
      client << F("\"}");
      file_.close();
    }
    client << F("]}\n");
  }
}

