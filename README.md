Chauvet16
=========
Chauvet16 Aquarium Controller

Download the files and copy them into an arduino sketch folder named Chauvet16.

The index.htm file you will need to copy into an SD card that goes into the SD card slot on the Ethernet-SD shield. Once you have the program running on the Arduino, you can upload updates to the index.htm file via this curl command.

    curl -0 -v -T index.htm http://user:password@<your controller ip>:<port>/upload/index.htm
    
Beginning with v20140805, you will need to gzip the index.htm to file index.gz and upload that as well whenver you edit index.htm. The command to upload is

    curl -0 -v -T index.gz http://user:password@<your controller ip>:<port>/upload/index.gz

You can download curl for pretty much any platform here. 
http://curl.haxx.se/download.html

You can download gzip from here. Get the latest version that supports the -k option so the index.htm file is not deleted after compressing.
http://www.gzip.org/#exe

I use this command to gzip the file

    gzip -k -9 -f index.htm

This results in file index.htm.gz. You need to rename it to index.gz before uploading to the SD card.

Documentation Links
-------------------
[Using the Doser](http://reefcentral.com/forums/showthread.php?p=22993871#post22993871)

[Wiring version 2 of Feeder.](http://reefcentral.com/forums/showthread.php?p=22994359#post22994359)

[Jebao Pump Control.](http://reefcentral.com/forums/showthread.php?p=22191660#post22191660)

[Jebao pump cable wiring.](http://reefcentral.com/forums/showthread.php?p=22913454#post22913454)

[Changing the login username and password.](http://reefcentral.com/forums/showthread.php?p=22277294#post22277294)

[Programming the outlets.](http://reefcentral.com/forums/showthread.php?p=22592718#post22592718)

[More on outlet programming.](http://reefcentral.com/forums/showthread.php?p=22710535#post22710535)

[Outlets inverse cycle.](http://reefcentral.com/forums/showthread.php?p=22709149#post22709149)

[How to setup email using gmx and how to send to multiple email recipients.](http://reefcentral.com/forums/showthread.php?p=22748159#post22748159)

Setting Up Arduino Libraries
----------------------------

You will need to get the following arduino libraries:

Time and DS1370RTC
http://playground.arduino.cc/uploads/Code/Time.zip

You need to edit the Time.cpp and Time.h (instructions below).

OneWire
http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip
Edit OneWire.h by adding this line after uint8_t reset(void);

    uint8_t reset2(void);

Then edit OneWire.cpp by changing uint8_t reset(void) to

    uint8_t reset2(void)

then commenting out this line at the end of the function

    //delayMicroseconds(420);

then add this code above the reset2 function

    uint8_t OneWire::reset(void) {
      uint8_t r = reset2();
      delayMicroseconds(420);
      return r;
    }


LiquidCrystal
https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads/LiquidCrystal_V1.2.1.zip

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
