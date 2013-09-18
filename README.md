Chauvet16
=========

Chauvet16 Aquarium Controller

Download the files and copy them into an arduino sketch folder named Chauvet16.

You will need to get the following arduino libraries:

Time
http://playground.arduino.cc/uploads/Code/Time.zip

You need to edit the Time.cpp and Time.h (instructions below).

OneWire
http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip

LiquidCrystal
https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/LiquidCrystal_V1.2.1.zip

DS1307RTC
http://www.pjrc.com/teensy/arduino_libraries/DS1307RTC.zip

SdFat
http://sdfatlib.googlecode.com/files/sdfatlib20130629.zip

Flash
http://arduiniana.org/libraries/flash/

Download and copy each of the libraries into your Arduino sketchbook folder in a folder named libraries.

You will also need to edit the Ethernet library to add a new method for getting the ip address of the incoming connection.

Edit EthernetClient.cpp and add this to the end of the file

uint8_t* EthernetClient::getRemoteIP(uint8_t remoteIP[])
{
  W5100.readSnDIPR(_sock, remoteIP);
  return remoteIP;
}

Edit EthernetClient.h and add this to line 25

uint8_t* getRemoteIP(uint8_t RemoteIP[]);//adds remote ip address

Editing the Wire library to make it run at 400khz is optional, but you will see the LCD update a lot faster with this change.

Edit Wire/utility/twi.h file line 25 to

  #define TWI_FREQ 400000L

Make sure to edit config.h defines before compiling and uploading to the mega board.


Editing the Time library to add a new now2 function that can be called from an Interrupt handler.
Edit Time.h file and add this line

time_t now2();

at line 104 after the now() function.

Edit Time.cpp file by adding a now2 function at line 263 right after the now() function

time_t now2(){
  while( millis() - prevMillis >= 1000){      
    sysTime++;
    prevMillis += 1000;	
#ifdef TIME_DRIFT_INFO
    sysUnsyncedTime++; // this can be compared to the synced time to measure long term drift     
#endif	
  }
  return sysTime;
}

It is essentially the same as the now() function except I removed the time sync code.

The reason for this change is if now() is called from an interrupt service routine, and the time sync is called, it retrieves the time from RTC, which uses I2C interrupt, hence will completely lock up the Arduino code.
So we make an "interrupt friendly" now2 function so we can determine the time from an interrupt service routine.
