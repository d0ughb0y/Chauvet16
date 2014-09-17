/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
void initOutlets() {
  DDRA = DDRC = 0xFF;
  PORTA = 0xFF;
  PORTC = 0;

  p(F("Outlet Timers OK"));
  logMessage(F("Outlet Timers initialized"));
}

inline uint16_t outletHandlerA() { 
  static uint8_t p = 0;
  static uint8_t q = 0;
  static uint8_t i = 0;
  static boolean checkmacros = true;
  static unsigned long starttime = 0;

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
          conf.outletRec[idx].mode==_auto)) || idx==Feeder || idx==Pump0) {       
          if (deviceTime>actions[i].initoff &&
            deviceTime<=(actions[i].initoff+actions[i].ontime)) {
              _outletOn(idx);
            } else {
              _outletOff(idx);
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
        FeedModeOFF();
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
      FeedModeOFF();
    }
    if (conf.outletRec[p].mode == _auto && total>0) {
      time_t currSecs = elapsedSecsToday(now2()) % (total);
      if (conf.outletRec[p].days & _BV(weekday(now2())) &&
        currSecs >= conf.outletRec[p].initoff && 
        currSecs < (conf.outletRec[p].initoff+conf.outletRec[p].ontime)) {
          if (conf.outletRec[p].days&INVCYCLE)//check if inverse logic
            _outletOff(p);
          else
            _outletOn(p);
      } else {
          if (conf.outletRec[p].days&INVCYCLE)//check if inverse logic
            _outletOn(p);
          else
            _outletOff(p);
      }
    }
    p++;
    if (p>=MAXOUTLETS) p=0;
    checkmacros = (p==0);
    if (p==0) {
      //microseconds elapsed since q=0
      unsigned long elapsed = micros()-starttime;
      if (elapsed<1000000) {
        unsigned long diff = 1000000 - elapsed;
        uint16_t modval = (diff/1020);
        if (modval<10 || modval>900)
          return 10;
        else
          return modval;
      } else 
        return 10;
    }
  }
  return 50;
}

inline void outletHandlerB() { //once per second handler
  lightToggle();
  time_t timenow = now2();
  int secnow = second(timenow);
  int minnow = minute(timenow);
  int hrnow = hour(timenow);
  int monnow = month(timenow);
  if (secnow==0) {  // one minute
    #ifdef AUTODST
    if (hrnow==2 && (monnow==3 || monnow==11)) {//2am
      testDst = true;
    }
    #endif
    if (minnow%10==0 && !logSensorsFlag) {
      logSensorsFlag = true; 
      updatePWMPumps();
    }
#ifdef _DOSER
    for (int i=0;i<MAXDOSERS;i++) {
      if (dosercalibrating || conf.doser[i].dosesperday==0 || conf.doser[i].rate==0) continue;//no auto dosing if not calibrated or doseperday is 0
      uint16_t mins = (uint16_t)(elapsedSecsToday(timenow)/60);
      if (mins<conf.doser[i].starttime)
        mins+=1440;
      mins -= conf.doser[i].starttime;
      uint16_t tint = conf.doser[i].interval==0?(1440/conf.doser[i].dosesperday):conf.doser[i].interval;
      if (mins%tint==0) {
        if (mins/tint < conf.doser[i].dosesperday){
          uint32_t countmatch = conf.doser[i].rate*(conf.doser[i].dailydose/conf.doser[i].dosesperday);
          if (countmatch < 60000UL * 15000UL / 1024)
            doserOn(i, countmatch);
        }
      }
    }
#endif
  }
  //check if network is still up at 1,21,41 mins past the hour (not a busy time)
  if ((minnow+1)%5==1 && secnow==15) { 
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
  if (hrnow==0 && minnow==0 && secnow==45) {//45 seconds past midnight, do ntp sync
    updateRTCFlag=true;
  }
  checkATO();
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
#ifdef OUTLET8INVERTED
    if (!(PORTA & _BV(p))) {//is off
      PORTA |= _BV(p);
#else
    if (!(~PORTA & _BV(p))) {//is off
      PORTA &= ~_BV(p);
#endif
      _outlogentry(p,true);
    }
#if defined(_FEEDER) || defined(_FEEDER_V2)
  } else if (p==Feeder) {
    feed();
#endif
  } else if (p==Pump0) {
    FeedModeOFF();
  }else {
#ifdef OUTLET16INVERTED
    if (!(~PORTC & _BV(p-8))) {
      PORTC &= ~_BV(p-8);
#else
    if (!(PORTC & _BV(p-8))) {
      PORTC |= _BV(p-8);
#endif
      _outlogentry(p,true);
    }
  }
}

void _outletOff(uint8_t p) {
  if (p<8) {
#ifdef OUTLET8INVERTED
    if (PORTA & _BV(p)) {//is on
      PORTA &= ~_BV(p);
#else
    if (~PORTA & _BV(p)) {//is on
      PORTA |= _BV(p);
#endif
      _outlogentry(p,false);
    }
  } else if (p==Feeder) {
  } else if (p==Pump0) {
     FeedModeON(); 
  }else {
#ifdef OUTLET16INVERTED
    if (~PORTC & _BV(p-8)) {
      PORTC |= _BV(p-8);
#else
    if (PORTC & _BV(p-8)) {
      PORTC &= ~_BV(p-8);
#endif
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
#ifdef OUTLET8INVERTED
      return PORTA & _BV(p);
#else
      return ~PORTA & _BV(p);
#endif
   } else {
#ifdef OUTLET16INVERTED
      return ~PORTC & _BV(p-8);
#else
      return PORTC & _BV(p-8);
#endif
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
