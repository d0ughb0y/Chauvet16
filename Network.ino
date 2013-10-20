TinyWebServer::PathHandler handlers[] = {
  {"/", TinyWebServer::GET, &index_handler},
  {"/upload/" "*", TinyWebServer::PUT, &secure_put_handler},
  {"/delete/" "*", TinyWebServer::GET, &delete_handler},
  {"/cgi-bin/status.xml", TinyWebServer::GET, &apex_status_handler},
  {"/cgi-bin/status.json" "*", TinyWebServer::GET, &apex_status_json_handler},
  {"/cgi-bin/files.json" "*", TinyWebServer::GET, &apex_filesjson_handler},
  {"/cgi-bin/outlog.xml" "*", TinyWebServer::GET, &apex_outlog_handler},
  {"/cgi-bin/datalog.xml" "*", TinyWebServer::GET, &apex_datalog_handler},
  {"/config.json" , TinyWebServer::ANY, &apex_config_handler},
  {"/phval.json", TinyWebServer::ANY, &apex_phval_handler},
  {"/cgi-bin/status.cgi" , TinyWebServer::POST, &apex_command_handler},
  {"/" "*", TinyWebServer::GET, &file_handler},
  {NULL}
};

const char* headers[] = {
  "Content-Length",
  "Authorization",
  NULL
};

TinyWebServer web = TinyWebServer(handlers,headers);
  byte mac[] = {
    0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED    };
  IPAddress ip(LOCAL_IP);

boolean netCheck() {
  static IPAddress router(ROUTER_IP); //change the ip to your router ip address
  EthernetClient client;
  if (client.connect(router,ROUTER_PORT)) {
    client.stop();
    return true;
  } else {
    resetNetwork();
    return false;
  }
}

void resetNetwork() {
  Ethernet.begin(mac,ip);
  web.begin();
  chirp();
  logMessage(F("Ethernet and Webserver reset."));
}

void initNetwork() {
  //init ethernet shield
  Ethernet.begin(mac,ip);
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

void send_file_name(TinyWebServer& web_server, char* fullpath) {
  char* fn;
  if (!fullpath) {
    web_server.send_error_code(404);
    web_server << F("Could not parse URL");
  } 
  else {
    TinyWebServer::MimeType mime_type
      = TinyWebServer::get_mime_type_from_filename(fullpath);
    char* f = strrchr(fullpath,'/'); 
    if (f>fullpath)
      fullpath[f-fullpath]=0;
    if ((f==fullpath?sd.chdir():sd.chdir(fullpath))) {
      fn = f+1;
      if (file_.open(fn,O_READ)) {
        web_server.send_error_code(200);
        web_server.send_content_type(mime_type);
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
      web_server.send_content_type("text/plain");
      web_server.end_headers();
      Client& client = web_server.get_client();
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
  const char* value = web_server.get_header_value("Authorization");
  if (value==NULL || (value!=NULL && strcmp(value+6,BASICAUTH))) {//admin:password
    web_server.get_client().println(F("HTTP/1.1 401"));
    web_server.get_client().println(F("WWW-Authenticate: Basic realm=\"\""));  
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
  EthernetClient& client = web.get_client();
  if (client.connect(SMTPSERVER, SMTPPORT)){
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
    client << F("RCPT TO:<") << EMAILTO << F(">\r\n");
    if(!eRcv(client)) return;
    client << F("DATA\r\n");
    if(!eRcv(client)) return;
    client << F("\r\n");
    client << F("Temp:") << getTemp() << "\r\n";
    client << F("pH:") <<getph() << "\r\n";
    client << F("Top Off water level:") << (uint8_t)getSonarPct() << "%\r\n.\r\n";
    if(!eRcv(client)) return;
    client << F("QUIT\r\n");    
    if(!eRcv(client)) return;
    client.stop();
    logMessage(F("Successfully sent email."));
  }
}


byte eRcv(EthernetClient& client)
{
  byte respCode;
  while(!client.available()) delay(1);
  respCode = client.peek();
  while(client.available())
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
  while(!client.available()) delay(1);
  while(client.available())
  {  
    client.read();    
  }
  client.stop();
  beepFail();
}

////////////////////////////////////
//  CLOCK
///////////////////////////////////
boolean isDst = false;
IPAddress timeServer(NTPSERVER);
EthernetUDP Udp;

time_t getntp(){
  long start = millis();
  time_t ntptime = getNtpTime(Udp, timeServer);
  char buf[20];
  logMessage(F("Time NTP sync "), getdatestring(ntptime,buf));
  long elapsed = millis() - start;
  logMessage(F("Time to execute sync "),elapsed);
  return ntptime;
}

void initClock() {
  unsigned int localPort = 7777;      // local port to listen for UDP packets
  Udp.begin(localPort);
  unsigned long ntptime = getNtpTime(Udp, timeServer); //get standard time
  if (ntptime>0) {
    setTime(ntptime);
    if (IsDST(ntptime)) 
    {
      //dstoffset = STDTZOFFSET+1L; //dst
      isDst = true;
      ntptime += SECS_PER_HOUR;
      setTime(ntptime);  //adjust to dst
    }
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
  tz = IsDST(now())?-7:-8;
}

void autoDST(time_t t) {
  if (IsDST(t)==isDst) return;
  isDst = !isDst;
  if (isDst)
    RTC.set(t+SECS_PER_HOUR);
  else
    RTC.set(t-SECS_PER_HOUR);
  setTime(RTC.get());
  logMessage(F("Auto adjusted DST time."));
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
  time_t dstStart,dstEnd, current;
  dstStart = makeTime(te);
  dstStart = nextSunday(dstStart);
  dstStart = nextSunday(dstStart); //second sunday in march
  dstStart += 2*SECS_PER_HOUR;//2AM
  te.Month=11;
  dstEnd = makeTime(te);
  dstEnd = nextSunday(dstEnd); //first sunday in november
  dstEnd += SECS_PER_HOUR; //1AM
  return (t>=dstStart && t<dstEnd);
}

unsigned long getNtpTime(EthernetUDP Udp, IPAddress timeServer)
{

  const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
  byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 


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

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  //    Udp.beginPacket(timeServer, 123); //NTP requests are to port 123
  //    Udp.write(packetBuffer,NTP_PACKET_SIZE);
  //    Udp.endPacket(); 

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
  uint8_t remoteip[4];
  EthernetClient& client = web_server.get_client();
  client.getRemoteIP(remoteip);
  logMessage(remoteip, (char*)web_server.get_path());
}

