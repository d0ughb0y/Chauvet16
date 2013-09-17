Chauvet16
=========

Chauvet16 Aquarium Controller

Download the files and copy them into an arduino sketch folder named Chauvet16.

You will need to get the following arduino libraries:

Time
http://playground.arduino.cc/uploads/Code/Time.zip

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
