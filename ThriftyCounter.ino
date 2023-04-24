/*
  ThriftyCounter - A low cost adafruit M0 adalogger feather based Trail Counter

  A low cost trail use counter using off the shelf hardware and 
  inexpensive, readily available microwave sensors that's appropriate when you
  just want to place a few counters out on trails in the forest and are able
  to visit them every month or two to switch batteries and harvest the SD card. 

  Required components:
  - adafruit M0 adalogger feather board
  - a microwave sensor. Tested using RCWL-0516 and HFS-DC06F, but similar devices should
    work. Really, any sensor that provides a high detection pulse should work.

  Time stamped trail use data is recorded in a .csv file on the SD card that can be 
  imported into a spreadsheet for data analysis. Each entry in the log file will have
  a timestamp and the duration of the event. The green LED on the feather is lit
  whenever the detection sensor is high. The red LED flashes once each time an event
  is written to the SD card, or multiple blinks to indicate a specific error condition:

  2 blinks = error, no RTC clock found
  3 blinks = error, no microSD card. Insert one to clear condition
  4 blinks = error, unable to write to a microSD card

  Copyright © 2023 Loren Konkus
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 */

#include <Wire.h>
#include "RTClib.h"
#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoLowPower.h>

// Select which kind of real time clock we should use
//#define USE_RTCZERO_TIME
#define USE_DS3231_TIME
 
#if defined(ARDUINO_SAMD_ZERO) 
  // Required for Serial on Zero based boards
  #define Serial SERIAL_PORT_USBVIRTUAL
#endif

// We use this digital in for the microwave sensor active detection. This can
// change, but we'll sleep the processor to save power between count events so whatever 
// you choose needs to be able to wake the processor up
const int sensorIn = 11;

// The SD card interface is tied to output 4
const int cardSelect = 4;

// LEDs we use to communicate with humans
const int redLED = 13;
const int greenLED = 8;

boolean eventsLogged = false;
volatile unsigned long eventStartTime = 0;
volatile unsigned long lastEventDuration = 0;
volatile int idleCounter = 1000;
File logfile;

#if defined(USE_RTCZERO_TIME) 
  RTCZero rtc;    
#endif
#if defined(USE_DS3231_TIME) 
  RTC_DS3231 rtc;    
#endif

//
// Interupt routine called when a proximity sensor event occurs. At the
// start of the event we'll remember the start time and at the end we'll
// compute the length of the duration, which will be a trigger to record
// the event to the SD card in the main loop. During the event, the green
// LED will be lit. 
//
void onSensorChanged() {
  if (digitalRead(sensorIn) == HIGH) {
    eventStartTime = millis();
    lastEventDuration = 0;
    digitalWrite(greenLED, HIGH);
  } else {
    lastEventDuration = millis() - eventStartTime;
    eventStartTime = 0;
    digitalWrite(greenLED, LOW);
  }
  idleCounter = 100;
}

//
// the setup function runs once when you press reset or power the board
//
void setup() {
  // configure IO pins
  pinMode(sensorIn, INPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(cardSelect, OUTPUT);

  // Initialize the RTC. 
  DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
  
#if defined(USE_RTCZERO_TIME)
  rtc.begin();    // Start the RTC
  rtc.setTime(compileTime.hour(), compileTime.minute(), compileTime.second());   // Set the time
  rtc.setDate(compileTime.day(), compileTime.month(), compileTime.year()-2000);    // Set the date
#endif
#if defined(USE_DS3231_TIME)
  if (!rtc.begin()) {
    fatalError(2);
  }
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
#endif

  // Initialize the SD card
  // No card indicated by three rapid red flashes
  while (!SD.begin(cardSelect)) {
    error(3);
    delay(3000);
  }
  
  // create an event log .csv file using the counter startup date
  char filename[14];
  getEventsFileName(getCurrentTime(), filename);
  boolean newfile = false;
  if (! SD.exists(filename)) {
    newfile = true;
  }

  logfile = SD.open(filename, FILE_WRITE); 
  if (!logfile) {
    fatalError(4);
  }

  if (newfile) {
    // New file, so write out the spreadsheet column headers
    logfile.println("Timestamp, Date, Time, Duration, Event Count");
  } else {
    // So this shouldn't be necessary with FILE_WRITE, but for whatever reason an existing file
    // is occasionally positioned at beginning when re-opened and all your hard earned counter
    // data is overwritten. This forces an append at end of file.
    logfile.seek(logfile.size());
  }

  // Ready for sensor interupts
  LowPower.attachInterruptWakeup(sensorIn, onSensorChanged, CHANGE);
}

// 
// Main loop where we record events
//
void loop() {

  //
  // Manage low power mode. If no events have occured in the last few moments, then enter a sleep
  // state to save power. If events were written to the SD card, flush the buffer to ensure they 
  // are written to the card. (Actual SD card updates are power expensive, so we don't do that on
  // every event).
  //
  if (idleCounter >  0 || eventStartTime != 0) {
    idleCounter--;
    if (idleCounter == 0) {
      if (eventsLogged) {
        digitalWrite(redLED, HIGH);
        logfile.flush();
        delay(400);
        digitalWrite(redLED, LOW);
        eventsLogged = false;
      }
      LowPower.deepSleep();
    }
  }

  //
  // If an event occured, record it. The duration of the last event is a flag to
  // denote one occured.
  //
  if (lastEventDuration != 0) {
    DateTime eventTime = getCurrentTime();
    char eventLogRecord[50];
    getTimestampString(eventTime, eventLogRecord);
    strcat(eventLogRecord, ",");
    getDateString(eventTime, &eventLogRecord[strlen(eventLogRecord)]);
    strcat(eventLogRecord, ",");
    getTimeString(eventTime, &eventLogRecord[strlen(eventLogRecord)]);
    strcat(eventLogRecord, ",");
    sprintf(&eventLogRecord[strlen(eventLogRecord)], "%lu", lastEventDuration);
    strcat(eventLogRecord, ",1");

    digitalWrite(redLED, HIGH);
    logfile.println(eventLogRecord);
    eventsLogged = true;
    digitalWrite(redLED, LOW);
    
    lastEventDuration = 0;
  }

  delay(100);
}

//
// Get the current time
//
DateTime getCurrentTime() {
#if defined(USE_RTCZERO_TIME) 
  return DateTime(rtc.getYear(),rtc.getMonth(), rtc.getDay(), rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
#endif
#if defined(USE_DS3231_TIME) 
  return rtc.now();
}
#endif

//
// Compute a FAT filename for the events log
//
// aTime - a DataTime object
// buffer - a char array of at least 14 chars to receive the filename
//
void getEventsFileName(DateTime aTime, char* buffer) {
  int aYear = aTime.year();
  if (aYear > 2000) {
    aYear = aYear - 2000;
  }
  sprintf(buffer,"C%02d%02d%02d.csv",aYear,aTime.month(),aTime.day());
}

//
// Get a time string in format mm-dd-yy hh:mm:ss, which seems to make 
// most spreadsheets happy. RTCs vary and may or may not support 4 digit dates,
// so we always just use last 2 for consistency.
//
// aTime - a DateTime object
// buffer - a char array of at least 20 chars
//
void getTimestampString(DateTime aTime, char* buffer) {
  int thisYear = aTime.year();
  if (thisYear > 2000) {
    thisYear = thisYear - 2000;
  }
  sprintf(buffer,"%02d-%02d-%02d %02d:%02d:%02d",aTime.month(),aTime.day(),thisYear,aTime.hour(),aTime.minute(),aTime.second());
}

//
// Get a date string in format mm-dd-yy, which seems to make 
// most spreadsheets happy. RTCs vary and may or may not support 4 digit dates,
// so we always just use last 2 for consistency.
//
// aTime - a DateTime object
// buffer - a char array of at least  10 chars
//
void getDateString(DateTime aTime, char* buffer) {
  int thisYear = aTime.year();
  if (thisYear > 2000) {
    thisYear = thisYear - 2000;
  }
  sprintf(buffer,"%02d-%02d-%02d",aTime.month(),aTime.day(),thisYear);
}

//
// Get a time string in format hh:mm:ss, which seems to make 
// most spreadsheets happy. RTCs vary and may or may not support 4 digit dates,
// so we always just use last 2 for consistency.
//
//  aTime - a DateTime object
//  buffer - char array of at least 10 characters
//
void getTimeString(DateTime aTime, char* buffer) {
  sprintf(buffer,"%02d:%02d:%02d",aTime.hour(),aTime.minute(),aTime.second());
}

//
// blink out an error code
//
void error(uint8_t errno) {
  uint8_t i;
  for (i=0; i<errno; i++) {
    digitalWrite(redLED, HIGH);
    delay(200);
    digitalWrite(redLED, LOW);
    delay(200);
  }
}

//
// blink out an error code every 3 seconds until reset
//
void fatalError(uint8_t errno) {
  while(1) {
    error(errno);
    delay(3000);
  }
}
