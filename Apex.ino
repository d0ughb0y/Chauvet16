/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
FLASH_STRING(apex_hwsw,"hardware=\"1.0\" software=\"4.20_1B13\">\n\t<hostname>");
FLASH_STRING(apex_intro1,"</hostname>\n\t<serial>AC4:12345</serial>\n\t<timezone>");
FLASH_STRING(apex_intro2,"</timezone>\n");
FLASH_STRING(apex_date1,"<date>");
FLASH_STRING(apex_date2,"</date>\n\t");
FLASH_STRING(apex_power,"<power><failed>none</failed><restored>none</restored></power>\n\t");
FLASH_STRING(apex_temp1,"<probes>\n\t\t<probe><name>Temp</name><value>");
FLASH_STRING(apex_temp2,"</value><type>Temp</type></probe>\n\t\t");
FLASH_STRING(apex_ph1,"<probe><name>pH</name><value>");
FLASH_STRING(apex_ph2,"</value><type>pH</type></probe>\n\t</probes>\n\t<outlets>\n");
FLASH_STRING(apex_status1,"<status ");
FLASH_STRING(apex_status2,"\t</outlets>\n</status>");
FLASH_STRING(apex_outlet1,"\t\t<outlet><name>"); //getname
FLASH_STRING(apex_outlet2,"</name><outputID>");  //get order
FLASH_STRING(apex_outlet3,"</outputID><state>"); //get state auto and on or off
FLASH_STRING(apex_outlet4,"</state><deviceID>1_"); //hard coded device id
FLASH_STRING(apex_outlet5,"</deviceID></outlet>\n");
FLASH_STRING(apex_datalog1,"<datalog ");
FLASH_STRING(apex_datalog2,"</datalog>");
FLASH_STRING(apex_outlog1,"<outlog ");
FLASH_STRING(apex_outlog2,"</outlog>");
FLASH_STRING(apex_json1,"{\"pstat\":");
FLASH_STRING(apex_json1b,"{\"hostname\":\"");
FLASH_STRING(apex_json1c,"\",\"software\":\"4.20_1B13\",\"hardware\":\"1.0\",\"serial\":\"AC4:12345\",\"timezone\":\"");
FLASH_STRING(apex_json2,"\",\"d\":");
FLASH_STRING(apex_json3,",\"feed\":{\"name\":");
FLASH_STRING(apex_json3a,",\"active\":");
FLASH_STRING(apex_json3b,"},\"power\":{\"failed\":1367205445,\"restored\":1367205445},");
FLASH_STRING(apex_json4,"\"probes\":[{\"did\":\"base_Temp\",\"t\":\"Temp\",\"n\":\"Temp\",\"v\":");
FLASH_STRING(apex_json5,"},{\"did\":\"base_pH\",\"t\":\"pH\",\"n\":\"pH\",\"v\":");
FLASH_STRING(apex_json6,"}],\"outlets\":[");
FLASH_STRING(apex_json7,"{\"n\":\"");
FLASH_STRING(apex_json8,"\",\"s\":\"");
FLASH_STRING(apex_json9,"\",\"oid\":\"");
FLASH_STRING(apex_json10,"\",\"did\":\"1_");
FLASH_STRING(apex_json11,"\"}");
FLASH_STRING(apex_json12,"],\"inputs\":[");
FLASH_STRING(apex_json12a,"],\"pwmpumps\":[");
FLASH_STRING(apex_json13,"]}}");

boolean process_params(char* inputstring, char* params_[], uint8_t len) {
  char **ap;
  for (int i=0;i<len;i++) {
    params_[i]=0;
  }
  for (ap = params_; (*ap = strsep(&inputstring, "=&\n")) != NULL;) {
    if (**ap != '\0') {
      if (++ap >= &params_[len]) 
        break;
    }
  }
  return true;
}

char* date2file(time_t t,char* base, char* buf) {
  tmElements_t tm;
  breakTime(t, tm);
  return date2file(tm, base, buf);
}

char* date2file(tmElements_t tm,char* base, char* buf) {
  char* c = buf;
  *c++='/';
  *c++=base[0];
  *c++=base[1];
  *c++=base[2];
  *c++='/';
  uint16_t yr = tmYearToCalendar(tm.Year);
  *c++ = yr/1000 + '0';
  *c++ = yr%1000/100 + '0';
  *c++ = yr%100/10 + '0';
  *c++ = yr%10 + '0';
  *c++ = '/';
  *c++ = tm.Month/10 + '0';
  *c++ = tm.Month%10 + '0';
  *c++ = '/';
  *c++ = 0;
  char* ret = c;
  *c++ = tm.Day/10 + '0';
  *c++ = tm.Day%10 + '0';
  *c++ = '.';
  *c++ = 't';
  *c++ = 'x';
  *c++ = 't';
  *c = 0;  
  return ret;
}

boolean apex_status_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  char tmp[20];
  webserver.send_error_code(200);
  webserver.send_content_type("text/xml");
  webserver.end_headers();
  Client& client = webserver.get_client();

  client << apex_status1;
  client << apex_hwsw << CONTROLLER_NAME << apex_intro1;
  client << tz;
  client << apex_intro2;
  client << apex_date1;
  client << getdatestring(now2(),tmp);
  client << apex_date2;
  client << apex_power;
  client << apex_temp1;
  client << dtostrf(getTemp(), 4, 1, (char*)tmp);
  client << apex_temp2;
  client << apex_ph1;
  client << dtostrf(getph(), 5, 2, (char*)tmp);
  client << apex_ph2;
  //outlets
  for (int i=0;i<MAXOUTLETS;i++) {
    client << apex_outlet1;
    client << (char*)conf.outletRec[i].name;
    client << apex_outlet2;
    client << i;
    client << apex_outlet3;
    client << getOutletState(i);
    client << apex_outlet4;
    client << i;
    client << apex_outlet5;    
  }
  client << apex_status2;
  return true;
}

void prelog(TinyWebServer& webserver, tmElements_t &tm, time_t& currdate, int& days){
  char* params_[4];
  char tmp[3];
  const char* path = webserver.get_path();
  char* paramstr = strchr(path,'?');
  if (paramstr==NULL) {
    currdate = now2();
    breakTime(currdate,tm);
    tm.Hour=tm.Minute=tm.Second=0;
    days = 1;
    return;
  } 
  else 
    paramstr+1;

  process_params(paramstr, params_, 4); //get the start date
  char* sdate = params_[1];
  days = atoi(params_[3]);
  tmp[0]=sdate[0];//year
  tmp[1]=sdate[1];
  tmp[2]=0;
  tm.Year = CalendarYrToTm(2000+atoi(tmp));
  tmp[0]=sdate[2];//month
  tmp[1]=sdate[3];
  tm.Month = atoi(tmp);
  tmp[0]=sdate[4];//day
  tmp[1]=sdate[5];
  tm.Day = atoi(tmp);
  tmp[0]=sdate[6]; //hour
  tmp[1]=sdate[7];
  tm.Hour = atoi(tmp);
  tmp[0]=sdate[8]; //minutes
  tmp[1]=sdate[9];
  tm.Minute = atoi(tmp);
  tm.Second = 0;
  tmElements_t tm2;
  tm2.Year=tm.Year;
  tm2.Month=tm.Month;
  tm2.Day=tm.Day;
  tm2.Hour=tm2.Minute=tm2.Second=0;
  currdate = makeTime(tm2);
}

boolean apex_outlog_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  char tmp[512];
  tmElements_t tm; //exact start date and time
  time_t currdate;// date at midnight;
  int days;
  prelog(webserver,tm, currdate, days);
  char path[21];
  char* fname;
  webserver.send_error_code(200);
  webserver.send_content_type("text/xml");
  webserver.end_headers();
  Client& client = webserver.get_client();

  client << apex_outlog1;
  client << apex_hwsw << CONTROLLER_NAME << apex_intro1;
  client << tz;
  client << apex_intro2;
  size_t s;
  boolean firstday = true;
  for (int d=0;currdate<=now2()&&d<days;currdate+=SECS_PER_DAY,d++) {
    fname = date2file(currdate,"OUT",path);
    if (sd.chdir(path) && file_.open(fname,O_READ)) { //this must exists if something is logged
      if (firstday) {
        scantofirstline(client,tmp,sizeof(tmp),tm);
      }
      while ((s=file_.read(tmp,sizeof(tmp)))>0){
        if (!client.connected()) break;
        client.write((uint8_t*)tmp,s);
      }
    }
    firstday=false;
    file_.close();
  }
  client << apex_outlog2;
  return true;
}

boolean apex_datalog_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  char tmp[512];
  tmElements_t tm;
  time_t currdate;// = makeTime(tm);
  int days;
  prelog(webserver,tm, currdate, days);
  char path[21];
  char* fname;
  webserver.send_error_code(200);
  webserver.send_content_type("text/xml");
  webserver.end_headers();
  Client& client = webserver.get_client();
  client << apex_datalog1;
  client << apex_hwsw << CONTROLLER_NAME << apex_intro1;
  client << tz;
  client << apex_intro2;
  size_t s;
  boolean firstday = true;
  for (int d=0;currdate<=now2()&& d< days;currdate+=SECS_PER_DAY,d++) {
    fname = date2file(currdate,"SEN",path);
    if (sd.chdir(path) && file_.open(fname,O_READ)) {
      if (firstday) {
        scantofirstline(client, tmp, sizeof(tmp), tm);
      }
      while ((s=file_.read(tmp,sizeof(tmp)))>0){
        if (!client.connected()) break;
        client.write((uint8_t*)tmp,s);
      }
    }
    firstday=false;
    file_.close();
  }
  client << apex_datalog2;
  return true;
}

const int getparamidx(const char* name,  char* params_[]) {
  if (!params_) {
    return -1;
  }
  for (int i = 0; params_[i]; i+=2) {
    if (strstr(params_[i], name)) {
      return i;
    }
  }
  return -1;
}

boolean apex_command_handler(TinyWebServer& webserver) {
  char parambuffer[40];
  char* params_[5];
  params_[4]=0;
  if (!check_auth(webserver)) return true;
  webserver.send_error_code(200);
  //  webserver.send_content_type("text/plain");
  webserver.end_headers();
  Client& client = webserver.get_client();
  int i=0;
  while (client.available() && i<40) {
    parambuffer[i++]=(char)client.read();
  }
  if (i==40) {
    beepFail(); //we have a problem
    logMessage(F("POST command parameter too long"));
    return true;
  }
  parambuffer[i]=0;
  logMessage(parambuffer);
  process_params(parambuffer, params_, 4); //get the start date
  int idx = 0;
  //  if (strstr(params_[0],"Update")!=NULL || strstr(params_[2],"Update")!=NULL) {
  if ((idx=getparamidx("_state",params_))!=-1) {
    char* dev = params_[idx++];
    char* st = params_[idx];
    char* e = strchr(dev,'_');
    *e = '\0';
    int state = atoi(st);
    uint8_t p = getOutletChannel(dev);
    switch (state) {
    case 0: //auto
      logMessage(F("Manual Auto on Port "),p);
      outletAuto(p);
      break;
    case 1: //off
      logMessage(F("Manual Off on Port "),p);
      outletOff(p, false);
      break;
    case 2: //on
      logMessage(F("Manual On on Port "),p);
      outletOn(p, false);
      break; 
    } 
  } else if ((idx=getparamidx("FeedCycle",params_))!=-1) {
    if ((idx=getparamidx("FeedSel",params_))!= -1) {
      int fmode = atoi(params_[idx+1]);
      if (_ActiveMacro>=4 || (fmode==4 && _ActiveMacro < 4)) {
        cli();
        _ActiveMacro = fmode;
        _MacroCountDown=0;
        if (fmode<4)
          _MacroTime=now2();
        else
          _MacroTime=0;
        if (fmode==4)
          FeedModeOFF();
        sei();
        logMessage(F("Execute Feed Command "),fmode);
      }
    }
  }  
  return true;  
}

boolean apex_status_json_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  webserver.send_error_code(200);
  webserver.send_content_type("application/json");
  webserver.end_headers();
  Client& client = webserver.get_client();
  client << apex_json1 << apex_json1b << CONTROLLER_NAME << apex_json1c << tz;
  client << apex_json2 << (now2()- tz*SECS_PER_HOUR);
  client << apex_json3;
  client << getFeedName();
  client << apex_json3a;
  client << getFeedActive();
  client << apex_json3b;
  client << apex_json4;
  char buffer[6];
  client << dtostrf(getTemp(), 4, 1, buffer) << apex_json5;
  client << dtostrf(getph(),5,2,buffer) << apex_json6;
  for (int i=0;i<MAXOUTLETS;i++) {
    client << apex_json7 << (char*)conf.outletRec[i].name << apex_json8;
    client << getOutletState(i) << apex_json9 << i << apex_json10 << i << apex_json11;
    if (i<MAXOUTLETS-1)
      client << F(",");
  }
  client << apex_json12; 
  client << F("{\"did\":\"base_I1\",\"n\":\"Top Off\",\"v\":");
  client << getSonarPct() << F("},");
  client << F("{\"did\":\"base_I2\",\"n\":\"ATO Upper\",\"v\":");
  client << (getATO1()?0:1) << F("},");
  client << F("{\"did\":\"base_I3\",\"n\":\"ATO Lower\",\"v\":");
  client << (getATO2()?0:1) << F("}");
  client << apex_json12a;
  for (int i=0;i<MAXPWMPUMPS;i++) {
     getpumpinfo(client,i);
  }
  client << apex_json13 << F("\n");
  return true;  
}

boolean apex_error(TinyWebServer& webserver, const __FlashStringHelper* msg, int i) {
  webserver.send_error_code(400);
  webserver.send_content_type("text/plain");
  webserver.end_headers();
//  webserver << F("Unable to update conf\n");
  webserver << msg << F(" ") << i << F("\n");
  beepFail();
  return true;   
}
boolean apex_config_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  Client& client = webserver.get_client();
  const char* hdrlen = webserver.get_header_value("Content-Length");
  if (hdrlen==NULL) {
    return apex_config_get(webserver);
  } else {
    return apex_config_post(webserver, atol(hdrlen));  
  }  
}
boolean apex_config_post(TinyWebServer& webserver, long postlen) {
  Client& client = webserver.get_client();
  char json[postlen];
  char delims[] = "{}[],:\"";
  int len=0;
  int avail = postlen;
  uint8_t* ptr = (uint8_t*)&json[0];
  uint32_t start_time = 0;
  boolean watchdog_start = false;
  boolean updatepwmpump = false;
  for (len=0;len<postlen && client.connected();) {
    int ln = client.available()?client.read(ptr,avail):0;
    if (!ln) {
      if (watchdog_start) {
        if (millis() - start_time > 10000) {
          break;
        }
      } else {
        start_time = millis();
        watchdog_start = true;
      }
      continue;
    }
   
    len += ln;
    ptr+=ln;
    avail-=ln;
  }
  if (len!=postlen) return apex_error(webserver,F("len mismatch"),len);
  conf_t myconf;
  cli();
  memcpy((void*)&myconf,(void*)&conf,sizeof(conf_t));
  sei();
  char* name = strtok(json,delims);
  while (name!=NULL) {
    if (strcmp_P(name,PSTR("outlets"))==0) {
      for (int i=0;i<MAXOUTLETS;i++) {
        strtok(NULL,delims);
        strcpy((char*)myconf.outletRec[i].name,strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.outletRec[i].initoff=atol(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.outletRec[i].ontime=atol(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.outletRec[i].offtime=atol(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.outletRec[i].days=(uint8_t)atoi(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.outletRec[i].mode=(uint8_t)atoi(strtok(NULL,delims));
      }
    } else if (strcmp_P(name,PSTR("macros"))==0) {
      for (int i=0;i<MAXMACROS;i++) {
        strtok(NULL,delims);
        strcpy((char*)myconf.macrosRec[i].name,strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.macrosRec[i].initoff=atol(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.macrosRec[i].ontime=atol(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.macrosRec[i].offtime=atol(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.macrosRec[i].days=(uint8_t)atoi(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.macrosRec[i].mode=(uint8_t)atoi(strtok(NULL,delims));
      }
    } else if (strcmp_P(name,PSTR("actions"))==0) {
      for (int i=0;i<MAXMACROS;i++) {
        for (int j=0;j<MAXMACROACTIONS;j++) {
          strtok(NULL,delims);
          myconf.actions[i][j].outlet=(uint8_t)atoi(strtok(NULL,delims));
          strtok(NULL,delims);
          myconf.actions[i][j].initoff=atol(strtok(NULL,delims));
          strtok(NULL,delims);
          myconf.actions[i][j].ontime=atol(strtok(NULL,delims));
        } 
      }
    } else if (strcmp_P(name,PSTR("pumps"))==0) {
      for (int i=0;i<MAXPWMPUMPS;i++) {
        for (int j=0;j<MAXINTERVALS;j++) {
          strtok(NULL,delims);
//          if (strcmp_P(strtok(NULL,delims),PSTR("wm"))!=0) return apex_error(webserver,F("wm"),i);;
          myconf.pump[i][j].waveMode=(uint8_t)atoi(strtok(NULL,delims));
          strtok(NULL,delims);
//          if (strcmp_P(strtok(NULL,delims),PSTR("sm"))!=0) return apex_error(webserver,F("sm"),i);;
          myconf.pump[i][j].syncMode=(uint8_t)atoi(strtok(NULL,delims));
          strtok(NULL,delims);
//          if (strcmp_P(strtok(NULL,delims),PSTR("l"))!=0) return apex_error(webserver,F("l"),i);;
          myconf.pump[i][j].level=(uint8_t)atoi(strtok(NULL,delims));
          strtok(NULL,delims);
//          if (strcmp_P(strtok(NULL,delims),PSTR("pw"))!=0) return apex_error(webserver,F("pw"),i);;
          myconf.pump[i][j].pulseWidth=(uint8_t)atoi(strtok(NULL,delims));
        }
      }  
      updatepwmpump=true;
    } else if (strcmp_P(name,PSTR("misc"))==0) {
      strtok(NULL,delims);
      myconf.htrlow =atof(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.htrhigh=atof(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.fanlow=atof(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.fanhigh=atof(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.sonarlow=atoi(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.sonarhigh=atoi(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.sonaralertval=atoi(strtok(NULL,delims));      
      strtok(NULL,delims);
      myconf.sonaralert=strcmp(strtok(NULL,delims),"true")==0?true:false;
      strtok(NULL,delims);
      myconf.alerttemplow=atof(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.alerttemphigh=atof(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.alertphlow=atof(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.alertphhigh=atof(strtok(NULL,delims));
      strtok(NULL,delims);
      myconf.soundalert=strcmp_P(strtok(NULL,delims),PSTR("true"))==0?true:false;
      strtok(NULL,delims);
      myconf.emailalert=strcmp_P(strtok(NULL,delims),PSTR("true"))==0?true:false;
      strtok(NULL,delims);
      myconf.initialized=(uint8_t)atoi(strtok(NULL,delims));
    } else {
      beepFail();
      return apex_error(webserver,F("unable to save config"),0);
    }
    name = strtok(NULL,delims);  
  }
  cli();
  memcpy((void*)&conf,(void*)&myconf,sizeof(conf_t));
  sei();
  writeEEPROM();  
  webserver.send_error_code(200);
  webserver.send_content_type("text/html");
  webserver.end_headers();
  client << F("OK");
  if (updatepwmpump) updatePWMPumps();
  beepOK();
  return true; 
}
boolean apex_config_get(TinyWebServer& webserver) {
  webserver.send_error_code(200);
  webserver.send_content_type("application/json");
  webserver.end_headers();
  Client& client = webserver.get_client();
  client << F("{\"config\":{\"outlets\":[");
  for (int i=0;i<MAXOUTLETS;i++) {
    client << F("{\"n\":\"") << (char*)conf.outletRec[i].name;
    client << F("\",\"iof\":\"") << conf.outletRec[i].initoff;
    client << F("\",\"ont\":\"") << conf.outletRec[i].ontime;
    client << F("\",\"oft\":\"") << conf.outletRec[i].offtime;
    client << F("\",\"d\":\"") << conf.outletRec[i].days;
    client << F("\",\"m\":\"") << conf.outletRec[i].mode << F("\"}");
    client << ((i<MAXOUTLETS-1)?",":"") ;
  }
  client << F("],\"macros\":["); 
  for (int i=0;i<MAXMACROS;i++) {
    client << F("{\"n\":\"") << (char*)conf.macrosRec[i].name;
    client << F("\",\"iof\":\"") << conf.macrosRec[i].initoff;
    client << F("\",\"ont\":\"") << conf.macrosRec[i].ontime;
    client << F("\",\"oft\":\"") << conf.macrosRec[i].offtime;
    client << F("\",\"d\":\"") << conf.macrosRec[i].days;
    client << F("\",\"m\":\"") << conf.macrosRec[i].mode << F("\"}");
    client << ((i<MAXMACROS-1)?",":"");
  }
  client << F("],\"actions\":[");
  for (int i=0;i<MAXMACROS;i++) {
    client << "[";
    for (int j=0;j<MAXMACROACTIONS;j++) {
      client << "{\"out\":\"" << conf.actions[i][j].outlet;
      client << "\",\"iof\":\"" << conf.actions[i][j].initoff;
      client << "\",\"ont\":\"" << conf.actions[i][j].ontime << "\"}";
      client << (j<MAXMACROACTIONS-1?",":"") << F("\n");
    }    
    client << "]" << (i<MAXMACROS-1?",":"") << F("\n");
  }  
  client << F("],\"pumps\":[\n");
  for (int i=0;i<MAXPWMPUMPS;i++) {
    client << F("[\n");
    for (int j=0;j<MAXINTERVALS;j++) {
      client << F("{\"wm\":\"") << conf.pump[i][j].waveMode;
      client << F("\",\"sm\":\"") << conf.pump[i][j].syncMode;
      client << F("\",\"l\":\"") << conf.pump[i][j].level;
      client << F("\",\"pw\":\"") << conf.pump[i][j].pulseWidth;
      client << F("\"}") << (j<(MAXINTERVALS-1)?",":"");
    }
    client << F("]") << (i<(MAXPWMPUMPS-1)?",":"");
  }
  client << F("],\n\"misc\":{\n");
  client << F("\"hl\":\"") << conf.htrlow; 
  client << F("\",\n\"hh\":\"") << conf.htrhigh; 
  client << F("\",\n\"fl\":\"") << conf.fanlow; 
  client << F("\",\n\"fh\":\"") << conf.fanhigh;
  client << F("\",\n\"sl\":\"") << conf.sonarlow; 
  client << F("\",\n\"sh\":\"") << conf.sonarhigh;
  client << F("\",\n\"sav\":\"") << conf.sonaralertval;
  client << F("\",\n\"sa\":\"") << (conf.sonaralert?"true":"false");
  client << F("\",\n\"atl\":\"") << conf.alerttemplow;
  client << F("\",\n\"ath\":\"") << conf.alerttemphigh;
  client << F("\",\n\"aphl\":\"") << conf.alertphlow;
  client << F("\",\n\"aphh\":\"") << conf.alertphhigh;
  client << F("\",\n\"snda\":") << (conf.soundalert?"true":"false");
  client << F(",\n\"ema\":") << (conf.emailalert?"true":"false");
  client << F(",\n\"init\":\"")<< conf.initialized << F("\"}}}\n");
  return true;     
}

boolean apex_phval_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  Client& client = webserver.get_client();
  const char* hdrlen = webserver.get_header_value("Content-Length");
  if (hdrlen!=NULL) {
    int len = atol(hdrlen)+1;
    char json[len];
    char calval[2];
    int i=0;
    while (client.available() && i<len) {
      json[i++]=(char)client.read();
    }
    json[i]=0;
    logMessage(json);
    //{"cal":"s"} 
    char delims[] = "{}[],:\"";
    if (strcmp_P(strtok(json,delims),PSTR("cal"))!=0) return apex_error(webserver,F("cal"),0);
    strcpy((char*)calval,strtok(NULL,delims));
    if (calval[0]=='f') {
      calibrate4();
      logMessage("calibrate 4");
    } else if (calval[0]=='s') {
      calibrate7();
      logMessage("calibrate 7");
    } else if (calval[0]=='t') {
      calibrate10();
      logMessage("calibrate 10");
    } else {
       return apex_error(webserver,F("incorrect calibrate command"),0);
    }
    beepOK();
  } 
  webserver.send_error_code(200);
  webserver.send_content_type("application/json");
  webserver.end_headers();
  client << F("{\"phval\":\"") << getph() << F("\",\"sonarval\":\"");
  client << getSonar() << F("\"}");
  return true;  
}

boolean apex_pwmpumpdata_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  Client& client = webserver.get_client();
  webserver.send_error_code(200);
  webserver.send_content_type("application/json");
  webserver.end_headers();
  client << F("{\"pwmpumpdata\":[");
  for (int i=0;i<MAXPWMPUMPS;i++) {
    client << F("{\"data\":[");
    getpwmdata(client,i);
    client << F("]}");
    client << (i==(MAXPWMPUMPS-1)?"":",");  
  }
  client << F("]}");
  return true; 
}

boolean apex_pwmwavedef_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  Client& client = webserver.get_client();
  webserver.send_error_code(200);
  webserver.send_content_type("application/json");
  webserver.end_headers();
  getwavedef(client);
  return true;  
}
boolean apex_pwmset_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  Client& client = webserver.get_client();
  const char* hdrlen = webserver.get_header_value("Content-Length");
  if (hdrlen!=NULL) {
    int len = atol(hdrlen)+1;
    char json[len];
    char val[4];
    char chan[4];
    int i=0;
    while (client.available() && i<len) {
      json[i++]=(char)client.read();
    }
    json[i]=0;
    char delims[] = "{}[],:\"";
    if (strcmp_P(strtok(json,delims),PSTR("channel"))!=0) 
      return apex_error(webserver,F("invalid pwm set value"),0);
    strcpy((char*)chan,strtok(NULL,delims));
    char* prop=strtok(NULL,delims);
    if (strcmp_P(prop,PSTR("level"))==0) {
      strcpy((char*)val,strtok(NULL,delims));
      setpwmlevel(atoi(chan),atoi(val));      
    } else if (strcmp_P(prop,PSTR("pulsewidth"))==0) {
      strcpy((char*)val,strtok(NULL,delims));
      setpulsewidth(atoi(chan),atoi(val));
    } else if (strcmp_P(prop,PSTR("wavemode"))==0) {
      strcpy((char*)val,strtok(NULL,delims));
      setwavemode(atoi(chan),atoi(val));
    } else if (strcmp_P(prop,PSTR("syncmode"))==0) {
      strcpy((char*)val,strtok(NULL,delims));
      setsyncmode(atoi(chan),atoi(val));      
    } else if(strcmp_P(prop,PSTR("pumpauto"))==0 ||
        strcmp_P(prop,PSTR("pumpmanual"))==0) {
      strcpy((char*)val,strtok(NULL,delims));
      setpumpauto(atoi(chan),atoi(val));
    } else {
//      chirp();
      return apex_error(webserver,F("invalid pwm set value"),0);
    }
  }  
  webserver.send_error_code(200);
  webserver.send_content_type("application/json");
  webserver.end_headers();
//return local pwm values
  getpumpinfo(client);
  return true;
}

boolean apex_filesjson_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  char* params_[2];
  const char* path = webserver.get_path();
  char* paramstr = strchr(path,'?');
  if (paramstr==NULL) {
  } 
  else 
    paramstr+1;

  process_params(paramstr, params_, 2); //get the start date
  char* root = params_[1];

  webserver.send_error_code(200);
  webserver.send_content_type("application/json");
  webserver.end_headers();
  Client& client = webserver.get_client();
  getfiles(client,root);
  return true;  
}

void scantofirstline(Client& client, char* tmp, int size, tmElements_t tm) {
  while (file_.fgets(tmp,size)>0) {
    char compa[3];
    compa[0]=tmp[25];
    compa[1]=tmp[26];
    compa[2]=0;
    if (atoi(compa)>=tm.Hour) { //line hour>= needed hour
      if (atoi(compa)>tm.Hour) { //if line hour> needed hour, then we start
        client << tmp;
        break;
      } 
      else {
        compa[0]=tmp[28];
        compa[1]=tmp[29];
        if (atoi(compa)>=tm.Minute) { //if hour= then compare min>=
          client << tmp;
          break;
        }
      }
    }
  }  
}
