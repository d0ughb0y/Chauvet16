#define numReadings 8
volatile uint16_t tempavg = 0;

void initSensors() {
  if (initTemp()) {
    p(F("Temp OK.        "));
    logMessage(F("Temp sensor initialized."));
  } else {
    beepFail();
    p(F("Temp Init failed"));
    logMessage(F("Temp Init failed"));
    for(;;);
  }
  if (initPH()) {
    p(F("pH OK.          "));
    logMessage(F("pH sensor initialized."));   
  } else {
    beepFail();
    p(F("pH Init failed  "));
    logMessage(F("pH Init failed"));
    for(;;);   
  } 
}

#define TEMPPIN 46
OneWire ds(TEMPPIN);

boolean initTemp() {
  if ( ds.reset()) {
     for (int i=0;i<255;i++) {
        updateTemp();
        delay(200);
        if (getTemp()>32.0) {
          return true;
        }  
     }
     return false;
  }
}

float getTemp() {
  uint8_t saveSREG = SREG;
  cli();
  float t = tempavg;
  SREG=saveSREG;
  return t/16.0*1.8+32.0;
}

void updateTemp() {
  static uint8_t state = 0;
  static uint16_t sum = 0;

  switch (state) {
    case 0: //convert
      if (ds.reset()) {
        ds.skip();
        ds.write(0x44);
        if (ds.read())
          state=2;
        else
          state=1;
        }
        break;    
    case 1: //status
      if (ds.read()>0) {
        state=2;
      }
      break;
    case 2: //read
      if (ds.reset()) {
        ds.skip();
        ds.write(0xBE);
        uint8_t data[9];
        ds.read_bytes(data,9);
        if (ds.crc8(data,8)!=data[8]) {
          state=0; //bad crc
          break;
        }
        uint16_t tval = data[1] << 8 | data[0];  
        if (tval > 256 && tval < 512 ) {
          if(sum) {
            uint8_t saveSREG=SREG;
            cli();
            float t = tempavg;
            SREG=saveSREG;
            sum = (sum - t) + tval;
          } else {
            sum = tval * numReadings;
          } 
          tempavg = sum / numReadings;
        }
      } 
      state=0;
      break;
  }
}

void checkTempISR(){
  #ifdef _HEATER
  if (getTemp() <= conf.htrlow && conf.outletRec[Heater].mode==_auto)
    _outletOn(Heater);
  if (getTemp() >= conf.htrhigh && conf.outletRec[Heater].mode==_auto)
    _outletOff(Heater);
  #endif
  #ifdef _FAN
  if (getTemp() >= conf.fanhigh && conf.outletRec[Fan].mode==_auto) {
      _outletOn(Fan);
  }
  if (getTemp() <= conf.fanlow && conf.outletRec[Fan].mode==_auto) {
      _outletOff(Fan);
  }
  #endif  
}

void checkAlarm() {
  if (getTemp()>=conf.alerttemphigh || getTemp()<=conf.alerttemplow ||
    getph()>=conf.alertphhigh || getph()<=conf.alertphlow ||
    (conf.sonaralert && getSonar()>conf.sonaralertval)) {
      if (!alarm) {
        alarmOn();
        emailFlag=true;
        logalarmFlag=true;
        alarm=true;    
      }
  } else {
    if (alarm) {
      alarm=false;
      beepoff();
    }
  }  
}

///////////////////////////////////////////////////////
//  pH section
///////////////////////////////////////////////////////
volatile float phavg=0;
static float phreading=0;
static boolean phready = false;

boolean initPH() {
  char phchars[15];
  Serial1.begin(38400);
  for (int i=0;i<255,strlen(phchars)==0;i++) {
    Serial1.print("e\r");
    delay(384);
    getresponse(phchars);
  }
  if (strlen(phchars)==0) {
    return false;
  }
  phreading=atof(phchars);
  phready=true;
  updatePh();
  phready=false;
  return true;
}

void updatePh() {
  static float sum=0;
  static float pha = 0;
  if (phready) {
    phready=false;
    if (phreading>0) {
      if (sum) {
        sum = (sum-pha)+phreading;
        pha = sum / numReadings; 
        cli();
        phavg=pha;
        sei();
      } else {
         sum = phreading*numReadings;
         pha=phreading;
         phavg=phreading;
      }
      Serial1.print("r\r");
    }else {
      Serial1.print("e\r");
    }
  }
}

float getph() {
  uint8_t saveSREG=SREG;
  cli();
  float p = phavg;
  SREG=saveSREG;
  return p;
}

void calibrate7() {
  Serial1.print("s\r");
}

void calibrate10(){
  Serial1.print("t\r");
}

void calibrate4() {
  Serial1.print("f\r");
}

void getresponse(char* phchars) {
  int i = 0;
  while (Serial1.available()) {
    char c = (char)Serial1.read(); 
    if (c=='\r') {
      phchars[i]=0;
      i=0;
      return;
    } else {
      phchars[i++]=c;
      if (i==14) {
        i=0;
        phchars[0]=0;
        return;
      }
    }
  }
}

void serialEvent1() {
  static char phchars[15];
  static int i = 0;
  char c = (char)Serial1.read(); 
  if (c=='\r') {
    phchars[i]=0;
    i=0;
    phreading=atof(phchars);  
    phready=true;
    return;
  } else {
    phchars[i++]=c;
    if (i==14) {
      i=0;
      phchars[0]=0;
      phreading=0;
      phready=true;
    }
  }
}

