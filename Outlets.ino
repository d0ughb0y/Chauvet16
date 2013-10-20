
void initOutlets() {
  DDRA = DDRC = 0xFF;
  PORTA = 0xFF;
  PORTC = 0;

  p(F("Outlet Timers OK"));
  logMessage(F("Outlet Timers initialized"));
}

inline void outletHandlerA() { 
  static uint8_t p = 0;
  static uint8_t q = 0;
  static uint8_t i = 0;
  static boolean checkmacros = true;
  static unsigned long starttime = 0;
  boolean skip = true;
  
  if (checkmacros) {
    if (q==0)
      starttime=micros();
    if (conf.macrosRec[q].mode == _auto && _ActiveMacro>=4) {
      time_t total = conf.macrosRec[q].initoff+conf.macrosRec[q].ontime+conf.macrosRec[q].offtime;   
      time_t currSecs = elapsedSecsToday(now2()) % (total);
      if (conf.macrosRec[q].days & _BV(weekday(now2())) &&
        currSecs == conf.macrosRec[q].initoff /*&&
        currSecs < (macrosRec[q].initoff+macrosRec[q].ontime)*/){
        _ActiveMacro=q; 
        _MacroTime=now2();   
      }
    } 
    time_t deviceTime;
    if (q==_ActiveMacro)  {
      MacroActions_t *actions = (MacroActions_t*)conf.actions[q];
      deviceTime = now2() - _MacroTime;
      if (i<MAXMACROACTIONS && actions[i].outlet != End) {
        uint8_t idx= actions[i].outlet;
        if ((idx < MAXOUTLETS && (conf.outletRec[idx].mode==_macro || 
          conf.outletRec[idx].mode==_auto)) || idx==Feeder) {       
          if (deviceTime>actions[i].initoff &&
            deviceTime<=(actions[i].initoff+actions[i].ontime)) {
              if (!isOutletOn(idx)){
                _outletOn(idx);
                skip=false;
              }
            } else {
              if (isOutletOn(idx)) {
                _outletOff(idx);
                skip=false;
              }
            }
          if (idx<MAXOUTLETS) {
            if (deviceTime >= conf.macrosRec[q].ontime) {
              conf.outletRec[idx].mode=_auto;
            } else {
              conf.outletRec[idx].mode=_macro;
            }
          }
        }
        i++;  
      } else {
         i=0; 
         checkmacros=false;
      }
      if (deviceTime >= conf.macrosRec[q].ontime) {
        _ActiveMacro=4;
        _MacroTime=0;
        _MacroCountDown=0; 
      } else {
        _MacroCountDown = conf.macrosRec[q].ontime - deviceTime;
      }
    }else {
      i=0;
      q++;
      if (q>=MAXMACROS) q=0;
      checkmacros = (q!=0);
    }
  } else {
    time_t total = conf.outletRec[p].initoff+conf.outletRec[p].ontime+conf.outletRec[p].offtime;
    if (conf.outletRec[p].mode == _macro && _ActiveMacro>=4 ) {
      conf.outletRec[p].mode=_auto;
      _MacroCountDown=0; 
    }
    if (conf.outletRec[p].mode == _auto && total>0) {
      time_t currSecs = elapsedSecsToday(now2()) % (total);
      if (conf.outletRec[p].days & _BV(weekday(now2())) &&
        currSecs >= conf.outletRec[p].initoff && 
        currSecs < (conf.outletRec[p].initoff+conf.outletRec[p].ontime)) {
        if (!isOutletOn(p)) {  
          _outletOn(p);
          skip=false;
        }
      } else {
        if (isOutletOn(p)) {
          _outletOff(p);
          skip=false;
        }
      }
    }
    p++;
    if (p>=MAXOUTLETS) p=0;
    checkmacros = (p==0);
    if (p==0) {
      unsigned long diff = 1000000 - (micros()-starttime);
      if (diff > 64){ //if mostly skipped and cycle completes in <1sec
        OCR4A += diff/64;  //return in diff to 1 second
        return;
      }
    }
  }
  if (skip) //if no switch change, return in 1ms
    OCR4A += CYCLESKIP;
  else  //if switch changed, return in 500ms
    OCR4A += CYCLETIMEA;
}

inline void outletHandlerB() { //once per minute handler
  lightToggle();
  time_t timenow = now2();
  int secnow = second(timenow);
  int minnow = minute(timenow);
  int hrnow = hour(timenow);
  int monnow = month(timenow);
  if (secnow==0) {  // one minute
    if (hrnow==2 && (monnow==3 || monnow==11)) {//2am
      testDst = true;
    }
    if (minnow%10==0 && !logSensorsFlag) {
       logSensorsFlag = true; 
    }
  }
  //check if network is still up at 1,21,41 mins past the hour (not a busy time)
  if ((minnow+1)%20==1) { 
    netCheckFlag=true;
  }
  if (secnow%LCD_MSG_CYCLE_SECS==0) {
    displaymode++;
    if (displaymode>=LCD_NUM_MSGS) displaymode=0;    
  }
  if (secnow%15==0) {
    checkTempISR();
    checkAlarm();
  }
  static uint8_t lastATO2 = true;
  uint8_t ato1 = getATO1();
  uint8_t ato2 = getATO2();
  float ph = getph();

  if (ato2!=lastATO2) {
    if (ato2 || ato1) {// open, pin high
      _outletOff(Kalk); //turn off both kalk and ato
    } else { //closed, pin low
      if (isOutletOn(Return) && ph < 8.7 && conf.outletRec[Kalk].mode == _auto) {
        _outletOn(Kalk);//on if rtn is on, ph not high, and kalk outlet on auto
      }
    }
  }
  lastATO2=ato2;
  if (ph>=8.7 || ato1) {
    _outletOff(Kalk);
  }
  OCR4B += CYCLETIMEB;
}

void _outlogentry(uint8_t p, boolean state) {
  uint8_t h = _head;
  if (++h>=OUTLOGSZ) h=0;
  if (h==_tail) {
    return;//don't overlap, we lose this log 
  }
  _outlog[_head].timestamp = now2();
  _outlog[_head].changed = p;
  _outlog[_head].state = state; 
  _head=h;
}

void _outletOn(uint8_t p){
  if (p<8) {
    if (!(~PORTA & _BV(p))) {//is off
      PORTA &= ~_BV(p);  
      _outlogentry(p,true);
    }
  } else if (p==Feeder) {
    feed();
  } else {
    if (!(PORTC & _BV(p-8))) {
      PORTC |= _BV(p-8);
      _outlogentry(p,true);
    }
  }
}

void _outletOff(uint8_t p) {
  if (p<8) {
    if (~PORTA & _BV(p)) {//is on
      PORTA |= _BV(p);
      _outlogentry(p,false);
    }
  } else if (p==Feeder) {
  } else {
    if (PORTC & _BV(p-8)) {
      PORTC &= ~_BV(p-8);
      _outlogentry(p,false);
    }
  }
}

void outletOn(uint8_t _p, boolean isAuto) {
  cli();
  if (isAuto && conf.outletRec[_p].mode==_auto) {
    _outletOn(_p);        
  } else if (!isAuto && conf.outletRec[_p].mode!=_macro) {
    conf.outletRec[_p].mode=_manual;
    _outletOn(_p);    
  }
  beepx(20,20,0,50,0);
  sei();
}

void outletOff(uint8_t _p, boolean isAuto) {
  cli();
  if (isAuto && conf.outletRec[_p].mode==_auto) {
    _outletOff(_p);  
  } else if (!isAuto && conf.outletRec[_p].mode!=_macro) {
    conf.outletRec[_p].mode=_manual;
    _outletOff(_p);    
  }
  beepx(20,0,0,0,0);
  sei();
}

void outletAuto(uint8_t _p) {
  cli();
  if (conf.outletRec[_p].mode!=_macro)
    conf.outletRec[_p].mode = _auto; 
  beepx(20,20,20,50,0);
  sei();
}

boolean isOutletOn(uint8_t p) {
   if (p<8){
      return ~PORTA & _BV(p);
   } else {
      return PORTC & _BV(p-8);
   }
}

char* getOutletState(uint8_t p) {
  static char buffer[4];
  boolean s = isOutletOn(p);
  if (conf.outletRec[p].mode==_auto) {
    buffer[0]='A';
    buffer[1]='O';
    s?buffer[2]='N':buffer[2]='F';
  } else {
    buffer[0]='O';
    if (s) {
      buffer[1]='N';
      buffer[2]=0;
    } else {
      buffer[1]='F';
      buffer[2]='F';
    }
  }
  buffer[3]=0;
  return buffer; 
}

uint8_t getOutletChannel(char* name) {
  for (int i=0;i<MAXOUTLETS;i++) {
     if (strcmp(name,(const char*)conf.outletRec[i].name)==0) {
       return i;
     }
  } 
  return 0;
}

uint8_t getFeedName() {
  uint8_t name = _ActiveMacro;
  if (name<4)
    return name+1;
  else
    return 0;  
}

time_t getFeedActive() {
  cli();
  time_t val = _MacroCountDown;
  sei();
  return val; 
}
