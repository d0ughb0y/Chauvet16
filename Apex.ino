/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
FLASH_STRING(apex_dummy,"");
FLASH_STRING(apex_hwsw,"hardware=\"1.0\" software=\"4.20_1B13\">\n<hostname>");
FLASH_STRING(apex_intro1,"</hostname>\n<serial>AC4:12345</serial>\n<timezone>");
FLASH_STRING(apex_intro2,"</timezone>\n");
FLASH_STRING(apex_date1,"<date>");
FLASH_STRING(apex_date2,"</date>\n");
FLASH_STRING(apex_power,"<power><failed>none</failed><restored>none</restored></power>\n<probes>\n");
FLASH_STRING(apex_xml_probe1,"<probe><name>");
FLASH_STRING(apex_xml_probe2,"</name><value>");
FLASH_STRING(apex_xml_probe3,"</value><type>");
FLASH_STRING(apex_xml_probe4,"</type></probe>\n");
FLASH_STRING(apex_xml_probe5,"</probes>\n<outlets>\n");
FLASH_STRING(apex_status1,"<status ");
FLASH_STRING(apex_status2,"</outlets></status>");
FLASH_STRING(apex_outlet1,"<outlet><name>"); //getname
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
FLASH_STRING(apex_json3b,"},\"power\":{\"failed\":1367205445,\"restored\":1367205445},\"probes\":[");
FLASH_STRING(apex_json_probe1,"{\"did\":\"base_");
FLASH_STRING(apex_json_probe2,"\",\"t\":\"");
FLASH_STRING(apex_json_probe3,"\",\"n\":\"");
FLASH_STRING(apex_json_probe4,"\",\"v\":");
FLASH_STRING(apex_json_probe5,"}");
FLASH_STRING(apex_json6,"],\"outlets\":[");
FLASH_STRING(apex_json7,"{\"n\":\"");
FLASH_STRING(apex_json8,"\",\"s\":\"");
FLASH_STRING(apex_json9,"\",\"oid\":\"");
FLASH_STRING(apex_json10,"\",\"did\":\"1_");
FLASH_STRING(apex_json11,"\"}");
FLASH_STRING(apex_json12,"],\"inputs\":[");
FLASH_STRING(apex_json12a,"],\"pwmpumps\":[");
FLASH_STRING(apex_json_doser1,"],\"dosers\":[");
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
// to use reeftronics service, comment out the next line
//  if (!check_auth(webserver)) return true;
  char tmp[20];
  webserver.send_error_code(200);
  webserver.send_content_type(F("text/xml"));
  webserver.end_headers();
  EthernetClient& client = webserver.get_client();

  client << apex_status1 << apex_hwsw << CONTROLLER_NAME << apex_intro1;
  client << tz;
  client << apex_intro2 << apex_date1 << getdatestring(now2(),tmp);
  client << apex_date2 << apex_power;
#ifdef _TEMP
  for (int i=0;i<MAXTEMP;i++) {
    client << apex_xml_probe1 << tempdata[i].name << apex_xml_probe2 << dtostrf(getTemp(i), 4, 1, (char*)tmp);
    client << apex_xml_probe3 << F("Temp") << apex_xml_probe4;
  }
  client.flush();
#endif
#ifdef _PH
  for (int i=0;i<MAXPH;i++) {
    client << apex_xml_probe1 << phdata[i].name << apex_xml_probe2 << dtostrf(getAtlasAvg(phdata[i]), 5,2, (char*)tmp);
    client << apex_xml_probe3 << F("pH") << apex_xml_probe4;
  }
  client.flush();
#endif
#ifdef _COND
  client << apex_xml_probe1 << F("Cond") << apex_xml_probe2 << dtostrf(getAtlasAvg(conddata), 4,1, (char*)tmp);
  client << apex_xml_probe3 << F("Cond") << apex_xml_probe4;
#endif
#ifdef _ORP
  client << apex_xml_probe1 << F("Orp") << apex_xml_probe2 << dtostrf(getAtlasAvg(orpdata), 3,0, (char*)tmp);
  client << apex_xml_probe3 << F("Orp") << apex_xml_probe4;
#endif
  client << apex_xml_probe5;
  //outlets
  for (int i=0;i<MAXOUTLETS;i++) {
    client << apex_outlet1;
    client << (char*)conf.outletRec[i].name;
    client << apex_outlet2;
    client  << i;
    client << apex_outlet3;
    client << getOutletState(i);
    client << apex_outlet4;
    client << i;
    client  << apex_outlet5;
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
  webserver.send_content_type(F("text/xml"));
  webserver.end_headers();
  EthernetClient& client = webserver.get_client();

  client << apex_outlog1 << apex_hwsw << CONTROLLER_NAME << apex_intro1;
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
  webserver.send_content_type(F("text/xml"));
  webserver.end_headers();
  EthernetClient& client = webserver.get_client();
  client << apex_datalog1 << apex_hwsw << CONTROLLER_NAME << apex_intro1;
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
  EthernetClient& client = webserver.get_client();
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
  webserver.send_content_type(F("application/json"));
  webserver.end_headers();
  EthernetClient& client = webserver.get_client();
  client << apex_json1 << apex_json1b << CONTROLLER_NAME << apex_json1c << tz;
  client << apex_json2 << (now2()- tz*SECS_PER_HOUR);
  client << apex_json3;
  client << getFeedName();
  client << apex_json3a;
  client << getFeedActive();
  client << apex_json3b;
  char buffer[6];
  int sensorcount = 0;
#ifdef _TEMP
  for (int i=0;i<MAXTEMP;i++) {
    client << apex_json_probe1 << F("Temp") << i << apex_json_probe2 << F("Temp") << apex_json_probe3 << tempdata[i].name;
    client << apex_json_probe4 << dtostrf(getTemp(i),4,1,buffer) << apex_json_probe5;
    if (++sensorcount<maxsensors)
      client << F(",");
  }
#endif
#ifdef _PH
  for (int i=0;i<MAXPH;i++) {
    client << apex_json_probe1 << F("pH") << i << apex_json_probe2 << F("pH") << apex_json_probe3 << phdata[i].name;
    client << apex_json_probe4 << dtostrf(getAtlasAvg(phdata[i]),5,2,buffer) << apex_json_probe5;
    if (++sensorcount<maxsensors)
      client << F(",");
  }
#endif
#ifdef _COND
    client << apex_json_probe1 << F("Cond") << apex_json_probe2 << F("Cond") << apex_json_probe3 << F("Cond");
    client << apex_json_probe4 << dtostrf(getAtlasAvg(conddata),4,1,buffer) << apex_json_probe5;
    if (++sensorcount<maxsensors)
      client << F(",");
#endif
#ifdef _ORP
  client << apex_json_probe1 << F("Orp") << apex_json_probe2 << F("Orp") << apex_json_probe3 << F("Orp");
  client << apex_json_probe4 << dtostrf(getAtlasAvg(orpdata),3,0,buffer) << apex_json_probe5;
    if (++sensorcount<maxsensors)
      client << F(",");
#endif
  client << apex_json6;
  for (int i=0;i<MAXOUTLETS;i++) {
    client << apex_json7 << (char*)conf.outletRec[i].name << apex_json8;
    client << getOutletState(i) << apex_json9 << i << apex_json10 << i << apex_json11;
    if (i<MAXOUTLETS-1)
      client << F(",");
  }
  client << apex_json12; 
#ifdef _SONAR
  client << F("{\"did\":\"base_I1\",\"n\":\"Top Off\",\"v\":");
  client << getSonarPct() << F("},");
#endif
  client << F("{\"did\":\"base_I2\",\"n\":\"ATO Upper\",\"v\":");
  client << (getATO1()?0:1) << F("},");
  client << F("{\"did\":\"base_I3\",\"n\":\"ATO Lower\",\"v\":");
  client << (getATO2()?0:1) << F("}");
  client << apex_json12a;
  for (int i=0;i<MAXPWMPUMPS;i++) {
     getpumpinfo(client,i);
  }
  client << apex_json_doser1;
  for (int i=0;i<MAXDOSERS;i++) {
    getdoserstatus(client,i);
  }
  client << apex_json13 << F("\n");
  return true;  
}

boolean apex_error(TinyWebServer& webserver, const __FlashStringHelper* msg, int i) {
  webserver.send_error_code(400);
  webserver << msg << F(" ") << i << F("\n");
  beepFail();
  return true;   
}

boolean apex_doser_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  EthernetClient& client = webserver.get_client();
  const char* hdrlen = webserver.get_header_value(PSTR("Content-Length"));
  if (hdrlen==NULL) {
    return apex_doser_get(webserver);
  } else {
    return apex_doser_post(webserver, atol(hdrlen));  
  }    
}

boolean apex_doser_post(TinyWebServer& webserver, long postlen) {
  //cmd don, doff, cstart, cstop, cadj, csave, cabort, mdose, setdv
  //idx 0,1
  //vol in ml
  //{"cmd":"don";"idx":"0"}

  EthernetClient& client = webserver.get_client();
  char json[postlen+1];
  char delims[] = "{}[],:\"";
  int len=0;
  int avail = postlen;
  uint8_t* ptr = (uint8_t*)&json[0];
  uint32_t start_time = 0;
  boolean watchdog_start = false;
  char command[7];
  char idx[2];
  char volume[6];

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
  if (len!=postlen) return apex_error(webserver,F("len mismatch. "),len);
  json[postlen]=0;
  char* name = strtok(json,delims);
  while (name!=NULL) {
    if (strcmp_P(name,PSTR("cmd"))==0) {
      strcpy(command,strtok(NULL,delims));
    } else if (strcmp_P(name,PSTR("idx"))==0) {
      strcpy(idx,strtok(NULL,delims));
    } else if (strcmp_P(name,PSTR("v"))==0) {
      strcpy(volume,strtok(NULL,delims));
    } else return apex_error(webserver,F("Invalid json request. "),0);
    name=strtok(NULL,delims);
  }
  if (idx==NULL) return apex_error(webserver,F("Invalid json, idx is null. "),0);
  int i = atoi(idx);
  if (i<0 || i>MAXDOSERS-1) return apex_error(webserver,F("Invalid idx value."),i);
  if (strcmp_P(command,PSTR("don"))==0){
    doserOn(i,0);
  } else if (strcmp_P(command,PSTR("doff"))==0) {
    doserOff(i);
  } else if (strcmp_P(command,PSTR("cstart"))==0) {
    calstart(i);
  } else if (strcmp_P(command,PSTR("cstop"))==0) {
    calstop(i);
  } else if (strcmp_P(command,PSTR("cadj"))==0) {
    if (volume!=NULL) {
      int vol = atoi(volume);
      caladjust(i,vol);
    } else
      return apex_error(webserver,F("Invalid json, Time unit not specified. "),0);
  } else if (strcmp_P(command,PSTR("csave"))==0) {
    if (volume!=NULL && calibrationcount>0) {
      int vol = atoi(volume);
      if (vol>0)
        calsave(i,vol);
      else
        return apex_error(webserver,F("Cannot save due to invalid volume value. "),vol);
    }
  }  else if (strcmp_P(command,PSTR("cabort"))==0) {
    calibrationcount=0;
    dosercalibrating=false;
  } else if (strcmp_P(command,PSTR("mdose"))==0) {
    if (conf.doser[i].rate==0) {
      return apex_error(webserver,F("Please calibrate this doser before using. Doser "),i);
    }
    if (volume!=NULL) {
      int vol = atoi(volume);
      if (vol>0 && vol/10.0*conf.doser[i].rate < 15000UL * 60000UL / 1024)
        manualDoseOn(i,vol);
      else
        return apex_error(webserver,F("Volume must result in Doser pump On time <=15 minutes. "),vol);
    } else return apex_error(webserver,F("Invalid json request, volume not specified. "),0);
  } else if (strcmp_P(command,PSTR("setdv"))==0) {
    if (volume!=NULL) {
      int vol = atoi(volume);
      dosedvolume[i]=vol*100ul;
      writeDoserStatus();
      logMessage(F("Doser volume set."));
      logMessage(F("Doser0 dosed volume is "),dosedvolume[0]/100.0);
      logMessage(F("Doser1 dosed volume is "),dosedvolume[1]/100.0);
    }
  } else {
    return apex_error(webserver,F("Invalid json request. "),0);
  }
  return apex_doser_get(webserver);
}

boolean apex_doser_get(TinyWebServer& webserver) {
  webserver.send_error_code(200);
  webserver.send_content_type(F("application/json"));
  webserver.end_headers();
  EthernetClient& client = webserver.get_client();
  client << F("{\"dosers\":[");
  for (int i=0;i<MAXDOSERS;i++) {
    getdoserstatus(client,i);
  }
  client << F("]}\r\n");
  return true;
}

boolean apex_config_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  EthernetClient& client = webserver.get_client();
  const char* hdrlen = webserver.get_header_value(PSTR("Content-Length"));
  if (hdrlen==NULL) {
    return apex_config_get(webserver);
  } else {
    return apex_config_post(webserver, atol(hdrlen));  
  }  
}

boolean apex_config_post(TinyWebServer& webserver, long postlen) {
  EthernetClient& client = webserver.get_client();
  char json[postlen+1];
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
  json[postlen]=0;
  conf_t myconf;
  cli();
  memcpy((void*)&myconf,(void*)&conf,sizeof(conf_t));
  sei();
  char* name = strtok(json,delims);
  if (strcmp_P(name,PSTR("config"))==0) name=strtok(NULL,delims);
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
    } else if (strcmp_P(name,PSTR("dosers"))==0) {
      for (int i=0;i<MAXDOSERS;i++) {
        strtok(NULL,delims);
        strcpy((char*)myconf.doser[i].name,strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.doser[i].dailydose =atoi(strtok(NULL,delims));
        strtok(NULL,delims);        
        myconf.doser[i].dosesperday =(uint8_t)atoi(strtok(NULL,delims));
        strtok(NULL,delims);        
        myconf.doser[i].interval =(uint8_t)atoi(strtok(NULL,delims));
        strtok(NULL,delims);        
        myconf.doser[i].starttime =atoi(strtok(NULL,delims));
        strtok(NULL,delims);
        strtok(NULL,delims);//skip rate
        strtok(NULL,delims);
        myconf.doser[i].fullvolume=(uint16_t)atoi(strtok(NULL,delims));
        if (myconf.doser[i].fullvolume!=conf.doser[i].fullvolume) {
          dosedvolume[i]=0;
          updateDoserStatusFlag=true;
        }
      }
    } else if (strcmp_P(name,PSTR("alerts"))==0) {
      for (int i=0;i<TOTALSENSORS;i++) {
        strtok(NULL,delims);
        myconf.alert[i].lowalert=atof(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.alert[i].highalert=atof(strtok(NULL,delims));
        strtok(NULL,delims);
        myconf.alert[i].type=atoi(strtok(NULL,delims));
      }
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
  webserver.send_content_type(F("text/html"));
  webserver.end_headers();
  client << F("OK");
  if (updatepwmpump) updatePWMPumps();
  beepOK();
  return true; 
}

boolean apex_config_get(TinyWebServer& webserver) {
  webserver.send_error_code(200);
  webserver.send_content_type(F("application/json"));
  webserver.end_headers();
  EthernetClient& client = webserver.get_client();
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
      client << F("{\"out\":\"") << conf.actions[i][j].outlet;
      client << F("\",\"iof\":\"") << conf.actions[i][j].initoff;
      client << F("\",\"ont\":\"") << conf.actions[i][j].ontime << F("\"}");
      client << (j<MAXMACROACTIONS-1?",":"") << F("\n");
    }    
    client << F("]") << (i<MAXMACROS-1?",":"") << F("\n");
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
  client << F("],\"dosers\":[\n");
  for (int i=0;i<MAXDOSERS;i++) {
    client << F("{\"n\":\"") << (char*)conf.doser[i].name;
    client << F("\",\"dd\":\"") << conf.doser[i].dailydose;
    client << F("\",\"dpd\":\"") << conf.doser[i].dosesperday;
    client << F("\",\"i\":\"") << conf.doser[i].interval;
    client << F("\",\"st\":\"") << conf.doser[i].starttime;
    client << F("\",\"r\":\"") << conf.doser[i].rate;
    client << F("\",\"fv\":\"") << conf.doser[i].fullvolume;
    client << F("\"}") << (i<MAXDOSERS-1?",":"");
  }
  client << F("],\"alerts\":[\n");
  for (int i=0;i<TOTALSENSORS;i++) {
    client << F("{\"l\":\"") << conf.alert[i].lowalert;
    client << F("\",\"h\":\"") << conf.alert[i].highalert;
    client << F("\",\"t\":\"") << conf.alert[i].type;
    client << F("\"}") << (i<TOTALSENSORS-1?",":"");
  }
  client << F("],\n\"misc\":{\n");
  client << F("\"hl\":\"") << conf.htrlow;
  client << F("\",\n\"hh\":\"") << conf.htrhigh;
  client << F("\",\n\"fl\":\"") << conf.fanlow;
  client << F("\",\n\"fh\":\"") << conf.fanhigh;
  client << F("\",\n\"sl\":\"") << conf.sonarlow;
  client << F("\",\n\"sh\":\"") << conf.sonarhigh;
  client << F("\",\n\"sav\":\"") << conf.sonaralertval;
  client << F("\",\n\"sa\":\"") << (conf.sonaralert?F("true"):F("false"));
  client << F("\",\n\"snda\":") << (conf.soundalert?F("true"):F("false"));
  client << F(",\n\"ema\":") << (conf.emailalert?F("true"):F("false"));
  client << F(",\n\"init\":\"")<< conf.initialized << F("\"}}}\n");
  return true;     
}

boolean apex_csutil_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  EthernetClient& client = webserver.get_client();
  const char* hdrlen = webserver.get_header_value(PSTR("Content-Length"));
  if (hdrlen!=NULL) {
    int len = atol(hdrlen)+1;
    char json[len];
    char command[8];
    int sensortype;
    long value;
    char* scratch;
    int i=0;
    while (client.available() && i<len) {
      json[i++]=(char)client.read();
    }
    json[i]=0;
    char delims[] = "{}[],:\"";
    scratch = strtok(json,delims);
    while (scratch!=NULL){
      if (strcmp_P(scratch,PSTR("cmd"))==0) {
         strcpy(command,strtok(NULL,delims));
      } else if (strcmp_P(scratch,PSTR("type"))==0) {
         strcpy(scratch,strtok(NULL,delims));
        sensortype = atoi(scratch);
      } else if (strcmp_P(scratch,PSTR("value"))==0){
        strcpy(scratch,strtok(NULL,delims));
        value=atol(scratch);
      } else {
        return apex_error(webserver,F("Invalid csutil data."),0);
      }
      scratch=strtok(NULL,delims);
    }
    if (false) {
#ifdef _SONAR
    } else if (sensortype==_sonar) {
      if (strcmp_P(command,PSTR("getval"))==0) {
        return csutilreply(webserver, getSonar()/10.0);
      } else {
        return apex_error(webserver,F("Invalid csutil data."),1);
      }
#endif
#ifdef _PH
    } else if (sensortype==_ph) {
      if (value<0 || value>=MAXPH)
        return apex_error(webserver,F("Invalid csutil data."),2);
      if (strcmp_P(command,PSTR("getval"))==0) {
        return csutilreply(webserver, getAtlasAvg(phdata[value]));
      } else if (strcmp_P(command,PSTR("clow"))==0) {
        calibrateLow(phdata[value]);
      } else if (strcmp_P(command,PSTR("chigh"))==0) {
        calibrateHigh(phdata[value],0);
      } else {
        return apex_error(webserver,F("Invalid csutil data."),3);
      }
#endif
#ifdef _ORP
    } else if (sensortype==_orp) {
      if (strcmp_P(command,PSTR("getval"))==0) {
        return csutilreply(webserver, getAtlasAvg(orpdata));
      } else if (strcmp_P(command,PSTR("clow"))==0) {
        calibrateLow(orpdata);
      } else if (strcmp_P(command,PSTR("chigh"))==0) {
        calibrateHigh(orpdata,0);
      } else {
        return apex_error(webserver,F("Invalid csutil data."),4);
      }
#endif
#ifdef _COND
    } else if (sensortype==_cond) {
      if (value==0)
        return apex_error(webserver,F("Invalid csutil data."),5);
      if (strcmp_P(command,PSTR("getval"))==0) {
        return csutilreply(webserver, getAtlasAvg(conddata));
      } else if (strcmp_P(command,PSTR("clow"))==0) {
        calibrateLow(conddata);
      } else if (strcmp_P(command,PSTR("chigh"))==0) {
        calibrateHigh(conddata,value);
      } else {
        return apex_error(webserver,F("Invalid csutil data."),6);
      }
#endif
    } else {
      return apex_error(webserver,F("incorrect csutil data."),7);
    }
    beepOK();
    webserver.send_error_code(200);
    webserver.send_content_type(F("text/plain"));
    webserver.end_headers();
    webserver.get_client() << F("Calibrate command OK.");
    return true;
  } 
  return apex_error(webserver,F("incorrect csutil data."),8);
}

boolean csutilreply(TinyWebServer& webserver, float val) {
  webserver.send_error_code(200);
  webserver.send_content_type(F("application/json"));
  webserver.end_headers();
  webserver.get_client() << F("{\"value\":\"") << val << F("\"}");
  return true;
}

boolean apex_pwmpumpdata_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  EthernetClient& client = webserver.get_client();
  webserver.send_error_code(200);
  webserver.send_content_type(F("application/json"));
  webserver.end_headers();
  client << F("{\"pwmpumpdata\":[");
  for (int i=0;i<MAXPWMPUMPS;i++) {
    client << F("{\"data\":[");
    getpwmdata(client,i);
    client << F("]}");
    client << (i==(MAXPWMPUMPS-1)?F(""):F(","));  
  }
  client << F("]}");
  return true; 
}

boolean apex_pwmwavedef_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  EthernetClient& client = webserver.get_client();
  webserver.send_error_code(200);
  webserver.send_content_type(F("application/json"));
  webserver.end_headers();
  getwavedef(client);
  return true;  
}
boolean apex_pwmset_handler(TinyWebServer& webserver) {
  if (!check_auth(webserver)) return true;
  EthernetClient& client = webserver.get_client();
  const char* hdrlen = webserver.get_header_value(PSTR("Content-Length"));
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
  webserver.send_content_type(F("application/json"));
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
  webserver.send_content_type(F("application/json"));
  webserver.end_headers();
  EthernetClient& client = webserver.get_client();
  getfiles(client,root);
  return true;  
}

void scantofirstline(EthernetClient& client, char* tmp, int size, tmElements_t tm) {
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
