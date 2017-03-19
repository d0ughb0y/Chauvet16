/*
 * Copyright (c) 2013 by Jerry Sy aka d0ughb0y
 * mail-to: j3rry5y@gmail.com
 * Commercial use of this software without
 * permission is absolutely prohibited.
*/
const char path0[] PROGMEM= "/";
const char path1[] PROGMEM = "/cgi-bin/status.json";
const char path2[] PROGMEM = "/cgi-bin/datalog.xml" "*";
const char path3[] PROGMEM = "/pwmwavedef.json";
const char path4[] PROGMEM = "/pwmpumpdata.json";
const char path5[] PROGMEM = "/cgi-bin/status.xml";
const char path6[] PROGMEM = "/cgi-bin/outlog.xml" "*";
const char path7[] PROGMEM = "/cgi-bin/files.json" "*";
const char path8[] PROGMEM = "/config.json";
const char path9[] PROGMEM = "/csutil.json";
const char path10[] PROGMEM = "/pwmset.json";
const char path11[] PROGMEM = "/doser.json";
const char path12[] PROGMEM = "/pwmfan.json";
const char path13[] PROGMEM = "/cgi-bin/status.cgi";
const char path14[] PROGMEM = "/upload/" "*";
const char path15[] PROGMEM = "/delete/" "*";
const char path16[] PROGMEM = "/" "*";
const char head0[]  PROGMEM = "Content-Length";
const char head1[]  PROGMEM = "Authorization";
const char head2[]  PROGMEM = "Accept-Encoding";


TinyWebServer::PathHandler handlers[] = {
  {path0, TinyWebServer::GET, &index_handler},
  {path1, TinyWebServer::GET, &apex_status_json_handler},
  {path2, TinyWebServer::GET, &apex_datalog_handler},
  {path3,TinyWebServer::GET, &apex_pwmwavedef_handler},
  {path4,TinyWebServer::GET, &apex_pwmpumpdata_handler},
  {path5, TinyWebServer::GET, &apex_status_handler},
  {path6, TinyWebServer::GET, &apex_outlog_handler},
  {path7, TinyWebServer::GET, &apex_filesjson_handler},
  {path8, TinyWebServer::ANY, &apex_config_handler},
  {path9, TinyWebServer::POST, &apex_csutil_handler},
  {path10,TinyWebServer::POST, &apex_pwmset_handler},
  {path11,TinyWebServer::ANY,&apex_doser_handler},
  {path12,TinyWebServer::POST, &apex_pwmfan_handler},
  {path13 , TinyWebServer::POST, &apex_command_handler},
  {path14, TinyWebServer::PUT, &secure_put_handler},
  {path15, TinyWebServer::GET, &delete_handler},
  {path16, TinyWebServer::GET, &file_handler},
  {NULL}
};

const char* headers[] = {
  head0,
  head1,
  head2,
  NULL
};

TinyWebServer web = TinyWebServer(handlers,headers);
  byte mac[] = MAC;
  IPAddress ip(LOCAL_IP);
  IPAddress router(ROUTER_IP);
#ifdef DNS_IP
  IPAddress dnsserver(DNS_IP);
#else
  IPAddress dnsserver(ROUTER_IP);
#endif

boolean netCheck() {
  EthernetClient client;
  IPAddress router(ROUTER_IP); //change the ip to your router ip address
  if (client.connect(router,ROUTER_PORT)==1) {
    client.stop();
    return true;
  } else {
      resetNetwork();
      return false;
  }
}

void resetNetwork() {
  Ethernet.begin(mac,ip,dnsserver, router);
  web.begin();
  chirp();
  logMessage(F("Ethernet and Webserver reset."));
}

void initNetwork() {
  //init ethernet shield
  Ethernet.begin(mac,ip,dnsserver, router);
  initClock();
  logMessage(F("System initializing..."));
  if (netCheck()) {
    p(F("Network OK.     "));
    initWebServer();
    p(F("Webserver Up.   "));
    logMessage(F("Network initialized."));
  } else {
     p(F("Network Down.   ")); 
  }
}

void send_file_name(TinyWebServer& web_server) {
  char* fullpath = (char*)web_server.get_path();
  char* endchar = strchr(fullpath,'?');
  if (endchar!=NULL) {
    fullpath[endchar-fullpath]='\0';
  }
  send_file_name(web_server,fullpath);
}

void send_file_name(TinyWebServer& web_server, const char* fullpath) {
  char* fn;
  EthernetClient& client = web_server.get_client();
  if (!fullpath) {
    web_server.send_error_code(404);
    web_server << F("Could not parse URL");
  } 
  else {
    TinyWebServer::MimeType mime_type
      = TinyWebServer::get_mime_type_from_filename(fullpath);
    char* f = strrchr(fullpath,'/'); 
    if (f>fullpath)
      ((char*)fullpath)[f-fullpath]=0;
    if ((f==fullpath?sd.chdir():sd.chdir(fullpath))) {
      fn = f+1;
      if (file_.open(fn,O_READ)) {
        client << F("HTTP/1.1 200 OK\r\n");
        web_server.send_content_type(mime_type);
        if (strcmp_P(fullpath,PSTR("/INDEX.HTM"))==0) {
          const char* compress = web_server.get_header_value(PSTR("Accept-Encoding"));
          if (compress!=NULL && strstr_P(compress,PSTR("gzip")) && strstr_P(compress,PSTR("deflate"))) {
            file_.close();
            if (file_.open("INDEX.GZ",O_READ))
              client << F("Content-Encoding: gzip\r\n");
            else {
              file_.close();
              file_.open(fn,O_READ);
            }
          }
        }
        client << F("Cache-Control: no-cache\r\n");
        client << F("Content-Length: ");
        client.println(file_.fileSize(),DEC);
        client << F("Connection: close\r\n");
        web_server.end_headers();
        web_server.send_file(file_);
      } else {
        web_server.send_error_code(404);
        web_server << F("Could not find file: ") << fn << "\n";
      }
      file_.close();
    } 
    else {
      web_server.send_error_code(404);
      web_server << F("Could not find file: ") << fn << "\n";
    }
  }
}

boolean delete_handler(TinyWebServer& web_server) {
  if (!check_auth(web_server)) return true;
  char* fullpath = (char*)web_server.get_path()+7;//check
  char* f = strrchr(fullpath,'/');
  fullpath[f-fullpath]=0;
  if ((f==fullpath?sd.chdir():sd.chdir(fullpath))) {
    char* fn = f+1;
    if ((file_.open(fn) && file_.rmRfStar()) || sd.remove(fn)) {
      web_server.send_error_code(200);
      web_server.send_content_type(F("text/plain"));
      web_server.end_headers();
      EthernetClient& client = web_server.get_client();
      client << F("File/Directory deleted!");
    } else {
      web_server.send_error_code(404);
      web_server << F("Delete failed.");    
    }
    file_.close();
  }
  return true;  
}

boolean check_auth(TinyWebServer& web_server) {
  logNetworkAccess(web_server);
  const char* value = web_server.get_header_value(PSTR("Authorization"));
  if (value==NULL || (value!=NULL && strcmp(value+6,BASICAUTH))) {//admin:password
    EthernetClient& client=web_server.get_client();
    client << F("HTTP/1.1 401\r\n");
    client << F("WWW-Authenticate: Basic realm=\"\"\r\n");
    web_server.end_headers();
    logMessage(F("Access denied."));
    if (value!=NULL)
      logMessage((char*)value);
    return false;
  }
  return true;
}

boolean file_handler(TinyWebServer& web_server) {
  if (!check_auth(web_server)) return true;
  send_file_name(web_server);
  return true;
}

boolean index_handler(TinyWebServer& web_server) {
  if (!check_auth(web_server)) return true;
  send_file_name(web_server, "/INDEX.HTM");
  return true;
}

boolean secure_put_handler(TinyWebServer& web_server) {
  if (!check_auth(web_server)) return true;
  TinyWebPutHandler::put_handler(web_server); 
}
void file_uploader_handler(TinyWebServer& web_server,
TinyWebPutHandler::PutAction action,
char* buffer, int size) {
  static uint32_t total_size;
  switch (action) {
    case TinyWebPutHandler::START:
    {
      total_size = 0;
      char* fullpath = (char*)web_server.get_path()+7;
      const char* f = strrchr(fullpath,'/');
      fullpath[f-fullpath]=0;
      if (!(f==fullpath?sd.chdir():sd.chdir(fullpath))) {
        if(f!=fullpath) {
          sd.mkdir(fullpath,true);
          sd.chdir(fullpath);
        }
      }
      const char* fn = f+1;
      file_.open(fn,O_CREAT | O_WRITE | O_TRUNC);
    }
    break;
    case TinyWebPutHandler::WRITE:
    if (file_.isOpen()) {
      file_.write((uint8_t*)buffer, size);
      total_size += size;
    }
    break;
    case TinyWebPutHandler::END:
    file_.close();
  }
}

void initWebServer() {
  TinyWebPutHandler::put_handler_fn = file_uploader_handler;
  web.begin();
}

inline void webprocess() {
  web.process(); 
}

///////////////////////////////
//  EMAIL
///////////////////////////////
void sendEmail() {
  if (!conf.emailalert) return;
  logMessage(F("Sending email alert."));
  EthernetClient client;
  if (client.connect(SMTPSERVER, SMTPPORT)==1){
    if(!eRcv(client)) return;
    client << F("EHLO CHAUVET16\r\n");
    if(!eRcv(client)) return;
    client << F("AUTH LOGIN\r\n");
    if(!eRcv(client)) return;
    client << SMTPUSER << F("\r\n");
    if(!eRcv(client)) return;
    client << SMTPPASSWORD << F("\r\n");
    if(!eRcv(client)) return;
    client << F("MAIL FROM:<") << EMAILFROM << F(">\r\n");  
    if(!eRcv(client)) return;
    char rcpts[strlen(EMAILTO)+1];
    strcpy(rcpts,EMAILTO);
    char* sendto = strtok(rcpts,",");
    while (sendto!=NULL) {
      client << F("RCPT TO:<") << sendto << F(">\r\n");
      if(!eRcv(client)) return;
      sendto = strtok(NULL,",");
    }
    client << F("DATA\r\n");
    if(!eRcv(client)) return;
    client << F("From:") << EMAILFROM << F("\r\n");
    client << F("To:") << EMAILTO << F("\r\n");
    client << F("Subject: Alert from ") << CONTROLLER_NAME << F("\r\n"); 
    client << F("\r\n");
#ifdef _TEMP
    for (int i=0;i<MAXTEMP;i++) {
      client << (const char*)tempdata[i].name << ": " << getTemp(i) << F("\r\n");
    }
#endif
#ifdef _PH
    for (int i=0;i<MAXPH;i++) {
      client << (const char*)ph[i]->getName() << ": " << ph[i]->getVal() << F("\r\n");
    }
#endif
#ifdef _COND
    client << F("Cond: ") << cond.getVal() << F("\r\n");
#endif
#ifdef _ORP
    client << F("Orp: ") << orp.getVal() << F("\r\n");
#endif
#ifdef _SONAR
    client << F("Top Off water level: ") << (int)getSonarPct() << F("%\r\n");
#endif
#ifdef _DOSER
    for (int i=0;i<MAXDOSERS;i++) {
      client << F("Doser ") << i << F(": \r\n");
      client << F("  ") << (const char*)conf.doser[i].name << F("\r\n");
      client << F("  Dosed Volume: ") << dosedvolume[i]/100.0 << F("\r\n");
      client << F("  Remaining Volume:") << conf.doser[i].fullvolume -dosedvolume[i]/100.0 << F("\r\n");
    }
#endif
#ifdef _PWMFAN
    for (int i=0;i<MAXPWMFANS;i++) {
      client << F("PWM Fan ") << i << F(" : ") << (const char*)conf.pwmfan[i].name << F(" : ") << getPWMFanRPM(i) << F("rpm\r\n");
    }
#endif
    client << F(".\r\n");
    if(!eRcv(client)) return;
    client << F("QUIT\r\n");    
    if(!eRcv(client)) return;
    client.stop();
    logMessage(F("Successfully sent email."));
  } else {
     logMessage(F("Send email failed."));
  }
}


byte eRcv(EthernetClient& client)
{
  byte respCode;
  while(client.connected() && !client.available()){
    delay(1);
  }
  respCode = client.peek();
  while(client.connected() && client.available())
  {  
    client.read();    
  }
  if(respCode >= '4')
  {
    efail(client);
    return 0;  
  }
  return 1;
}


void efail(EthernetClient& client)
{
  client.println(F("QUIT"));
  while(client.connected() && !client.available()) {
    delay(1);
  }
  while(client.connected() && client.available())
  {  
    client.read();    
  }
  client.stop();
  beepFail();
}

////////////////////////////////////
//  CLOCK
///////////////////////////////////
#ifdef AUTODST
boolean isDst = false;
#endif

IPAddress timeServer;

void initClock() {
  DNSClient dns;
  dns.begin(Ethernet.dnsServerIP());
  if (dns.getHostByName("pool.ntp.org",timeServer)==1) {
    p(F("Got NTP IP"));
  } else {
    p(F("NTP DNS failed."));
    IPAddress naddr(NTPSERVER);
    timeServer=naddr;
  }

  unsigned long ntptime = getNtpTime(timeServer); //get standard time
  if (ntptime>0) {
    setTime(ntptime);
    #ifdef AUTODST
    logMessage(F("AutoDST enabled."));
    if (IsDST(ntptime)) 
    {
      logMessage(F("It is currently DST."));
      //dstoffset = STDTZOFFSET+1L; //dst
      isDst = true;
      ntptime += SECS_PER_HOUR;
      setTime(ntptime);  //adjust to dst
    } else {
       logMessage(F("It is currently not DST."));
    }
    #else
    logMessage(F("AutoDST disabled."));
    #endif
    logMessage(F("Got NTP time "));
    p(F("NTP sync OK.    "));
    char buf[20];
    logMessage(F("NTP time is "),getdatestring(ntptime,buf));
    //check if RTC is present
    Wire.beginTransmission(RTC_ADDR);
    uint8_t error = Wire.endTransmission();
    if (!error) {
      RTC.set(ntptime);
      unsigned long rtctime = RTC.get();
      logMessage(F("RTC time"),getdatestring(rtctime,buf));
      logMessage(F("Using RTC as sync provider."));
      setSyncProvider(RTC.get);
      p(F("RTC time OK.    "));
    } else {
      logMessage(F("RTC is not present."));
    }
  } else {
    Wire.beginTransmission(RTC_ADDR);
    uint8_t error = Wire.endTransmission();
    if (!error) {
      char buf[20];
      unsigned long rtctime = RTC.get();
      setTime(rtctime);
      logMessage(F("Get ntp failed."));
      logMessage(F("Using RTC as sync provider."));
      logMessage(F("RTC time"),getdatestring(rtctime,buf));
      setSyncProvider(RTC.get);
      p(F("Using RTC time. "));
    } else {
      beepFail();  
      for(;;);
    }
  }
  #ifdef AUTODST
  tz = IsDST(now())?(STDTZOFFSET+1):STDTZOFFSET;
  #endif
}

void updateRTC(){
  //check if RTC is present
  Wire.beginTransmission(RTC_ADDR);
  uint8_t error = Wire.endTransmission();
  if (!error) {
    unsigned long ntptime = getNtpTime(timeServer); //get standard time
    if (ntptime>0){
      #ifdef AUTODST
      if (IsDST(ntptime)) 
      {
        ntptime += SECS_PER_HOUR;
      }
      #endif
      RTC.set(ntptime);
    }
  }
}

#ifdef AUTODST
void autoDST(time_t t) {
  if (IsDST(t)==isDst) return;
  isDst = !isDst;
  if (isDst) {
    RTC.set(t+SECS_PER_HOUR);
    tz = STDTZOFFSET+1;
  } else {
    RTC.set(t-SECS_PER_HOUR);
    tz = STDTZOFFSET;
  }
  setTime(RTC.get());
  logMessage(F("Auto adjusted DST time."));
  char buf[20];
  logMessage(F("Current time is "),getdatestring(now(),buf));
}

boolean IsDST(time_t t)
{
  tmElements_t te;
  te.Year = year(t)-1970;
  te.Month =3;
  te.Day =1;
  te.Hour = 0;
  te.Minute = 0;
  te.Second = 0;
  time_t dstStart,dstEnd;
  dstStart = makeTime(te);
  if (dayOfWeek(dstStart)!=dowSunday)//if Mar 1 is already a Sunday, skip one call to nextSunday
    dstStart = nextSunday(dstStart);
  dstStart = nextSunday(dstStart); //second sunday in march
  dstStart += 2*SECS_PER_HOUR;
  te.Month=11;
  dstEnd = makeTime(te);
  if (dayOfWeek(dstEnd)!=dowSunday) //if Nov 1 is already a Sundau, skip call to nextSunday
    dstEnd = nextSunday(dstEnd); //first sunday in november
  dstEnd += SECS_PER_HOUR;
  return (t>=dstStart && t<dstEnd);
}
#endif
unsigned long getNtpTime(IPAddress timeServer)
{
  EthernetUDP Udp;
  const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
  byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

  Udp.begin(7777);
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // wait to see if a reply is available
  int retries = 3;
  while (retries>0) {
    Udp.beginPacket(timeServer, 123); //NTP requests are to port 123
    Udp.write(packetBuffer,NTP_PACKET_SIZE);
    Udp.endPacket(); 
    delay(500);  
    if ( Udp.parsePacket() ) {  
      // We've received a packet, read the data from it
      Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer
      unsigned long seventy_years = 2208988800UL - (STDTZOFFSET * 60 * 60);        
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;  
      return secsSince1900 -  seventy_years;    
    }
    //      Serial.print("Retries ");Serial.println(retries);
    retries--;
  }
  return 0; // return 0 if unable to get the time
}

void logNetworkAccess(TinyWebServer &web_server) {
  char* path = (char*)web_server.get_path();
  if (strcmp_P(path,PSTR("/pwmpumpdata.json"))==0 ||
   strcmp_P(path,PSTR("/csutil.json"))==0 ||
   strcmp_P(path,PSTR("/pwmfan.json"))==0) return;
  uint8_t remoteip[4];
  EthernetClient& client = web_server.get_client();
  client.getRemoteIP(remoteip);
  logMessage(remoteip, path);
}

