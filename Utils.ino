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

void readEEPROM() {
  eeprom_read_block((void*)&conf, (void*)0, sizeof(conf));
}

void writeEEPROM() {
  eeprom_write_block((const void*)&conf, (void*)0, sizeof(conf));
}

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
  conf.fanlow = 79.0;
  conf.fanhigh = 79.7;
  conf.htrlow = 77.3;
  conf.htrhigh = 78.0;
  conf.sonarlow = 40;
  conf.sonarhigh = 5;
  conf.sonaralertval = 37;
  conf.sonaralert = false;
  conf.alerttemplow = 76.0;
  conf.alerttemphigh = 82.0;
  conf.alertphlow = 7.5;
  conf.alertphhigh = 9.0;
  conf.soundalert=false;
  conf.emailalert=false;
  conf.initialized=EEPROMSIG;
}

void initializeEEPROM() {
  readEEPROM();
  if (conf.initialized!=EEPROMSIG) {
    beepx(50,50,200,50,0);
    initializeConf();
    writeEEPROM();
  }
  readEEPROM();
}

//////////////////////////////////////////
//   LCD
/////////////////////////////////////////
//static uint8_t line = 0;
static boolean lcdpresent = false;

void initLCD(){
  lcd.begin(LCD_COLS,LCD_ROWS);
  lcdpresent = checkLCD();
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

void psensors() {
  if (!lcdpresent) return;
  lcd.setCursor(0,1);
  lcd <<  F("T:") << getTemp() << F(" pH:") << getph() << F(" ");
}

void ptime(time_t t) {
  if (!lcdpresent) {
    if (checkLCD()) {
      lcd.begin(LCD_COLS,LCD_ROWS);
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
  uint8_t oa = ~PORTA;
  uint8_t ob = PORTC;
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

void pdebug() {
  if (!lcdpresent) return;
  lcd.setCursor(0,1);
//  cli();
//  uint32_t d1 = debug1;
//  uint32_t d2 = debug2;
//  sei();
  lcd << debug1 << F("us ") << debug2 << F("us            ");
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
  if (now2()-ts < 5) {return;} //postpone
  if (lhead != ltail) {
    char buffer[20];
    boolean inoutlog = false;
    if (logSetup(ts, buffer, "OUT","txt")) {
      time_t currtime = ts;
      while (lhead != ltail) {
        fileout_ << F("<record><date>") << buffer << F("</date><name>");
        if (_outlog[ltail].changed==Feeder) {
          fileout_ << F("Feeder");
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
    netCheck();//workaround for EMI problem with relays dropping ethernet connection
  } 
}

void logSensors() {
  char buffer[20];
  if (logSetup(now2(),buffer,"SEN","txt")) {
    fileout_ << F("<record><date>") << buffer << F("</date><probe><name>Temp</name><value>");
    fileout_ << setprecision(1);
    fileout_ << getTemp()<< F("</value></probe><probe><name>pH</name><value>");
    fileout_ << setprecision(2);
    fileout_ << (getph()<10.0?" ":"") << getph() << F("</value></probe></record>");
    fileout_ << F("\n");
    fileout_.close();
  }
}

void logMessage(const __FlashStringHelper *msg, char* str) {
  char buffer[20];
  if (logSetup(now2(),buffer, "LOG","log")) {
    fileout_ << buffer << " " << msg << " " << str  << "\n";
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

void logMessage(uint8_t ip[], char* msg) {
  char buffer[20];
  if (logSetup(now2(),  buffer, "LOG", "log")) {
     fileout_ << buffer << " " << (int)ip[0] << "." << (int)ip[1] << "." << (int)ip[2];
     fileout_ << "." << (int)ip[3] << " " << msg << "\n";
     fileout_.close();
  }
}

void logAlarm() {
  char buffer[20];
  if (logSetup(now2(),  buffer, "LOG", "log")) {
     fileout_ << buffer << F(" Alarm Event : Temp=") << getTemp();
     fileout_ << F(" ph=") << getph();
     fileout_ << F(" Top Off water level: ") << (uint8_t)getSonarPct() << F("%\n");
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
void getfiles(Client& client, char* rootstr) {
  char fn[13];
  if (sd.chdir(rootstr)) {
    client << F("{\"files\":[\n");
    boolean first = true;
    while(file_.openNext(sd.vwd(),O_READ)) {
      if (!first) client << F(",\n");
      first=false;
      dir_t d;
      file_.dirEntry(&d);
      client << F("{\"name\":\"");
      file_.getFilename(fn);
      client << fn;
      client << F("\",\"date\":\"");
      uint8_t x = FAT_MONTH(d.lastWriteDate);
      client << (x<10?"0":"") << x << "/";
      x = FAT_DAY(d.lastWriteDate);
      client << (x<10?"0":"") << x << "/" << FAT_YEAR(d.lastWriteDate);
      client << F("\",\"time\":\"");
      x = FAT_HOUR(d.lastWriteTime);
      client << (x<10?"0":"") << x << ":";
      x = FAT_MINUTE(d.lastWriteTime);
      client << (x<10?"0":"") << x << ":";
      x = FAT_SECOND(d.lastWriteTime);
      client << (x<10?"0":"") << x;
      client << F("\",\"size\":\"");
      if (!file_.isDir())
        client << file_.fileSize();
      else
        client <<  "-1";
      client << F("\"}");
      file_.close();
    }
    client << F("]}\n");
  }
}

