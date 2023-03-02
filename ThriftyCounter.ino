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
  is written to the SD card, or multiple blinks to indicate a specific error condition.

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

  // Initialize serial, just in case connected 
  while (! Serial); // Wait until Serial is ready
  Serial.begin(9600);
  Serial.println("\r\nThrifty Feather Counter");

  // Initialize the RTC. 
  DateTime compileTime = DateTime(F(__DATE__), F(__TIME__));
  
#if defined(USE_RTCZERO_TIME)
  rtc.begin();    // Start the RTC
  rtc.setTime(compileTime.hour(), compileTime.minute(), compileTime.second());   // Set the time
  rtc.setDate(compileTime.day(), compileTime.month(), compileTime.year()-2000);    // Set the date
  Serial.println("RTC set to " + getTimeString(compileTime));
#endif
#if defined(USE_DS3231_TIME)
  if (!rtc.begin()) {
    Serial.println("Expected external Real Time Clock not found");
    fatalError(2);
  }
  if (rtc.lostPower()) {
    Serial.println("Real Time Clock lost power; initializing");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
#endif
  Serial.print("Starting up at ");
  Serial.println(getTimeString(getCurrentTime()));

  // Initialize the SD card
  // No card indicated by three rapid red flashes
  while (!SD.begin(cardSelect)) {
    Serial.println("No SD card found, waiting");
    error(3);
    delay(3000);
  }
  
  // create an event log .csv file using the counter startup date
  String filename = eventsFileName(getCurrentTime());
  boolean newfile = false;
  if (! SD.exists(filename)) {
    newfile = true;
    Serial.print("Creating log file ");
    Serial.println(filename);
  } else {
    Serial.print("Opening log file ");
    Serial.println(filename);
  }

  logfile = SD.open(filename, FILE_WRITE); 
  if (!logfile) {
    Serial.println("couldnt open event log file");
    while (1) delay(10);
  }

  if (newfile) {
    // New file, so write out the spreadsheet column headers
    logfile.println("Event Time, Duration, Event Count");
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
  if (idleCounter>0) {
    idleCounter--;
    if (idleCounter == 0) {
      if (eventsLogged) {
        digitalWrite(redLED, HIGH);
        logfile.flush();
        delay(400);
        digitalWrite(redLED, LOW);
        eventsLogged = false;
      }
      LowPower.sleep();
    }
  }

  //
  // If an event occured, record it. The duration of the last event is a flag to
  // denote one occured.
  //
  if (lastEventDuration != 0) {
    DateTime eventTime = getCurrentTime();
    String eventLogRecord = getTimeString(eventTime);
    eventLogRecord += ",";
    eventLogRecord += lastEventDuration;
    eventLogRecord += ",1,";

    // Temp - write out battery voltage to log so we can debug power consumption over a long term
    float measuredvbat = analogRead(A7);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    eventLogRecord += measuredvbat;

    Serial.println(eventLogRecord);
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
String eventsFileName(DateTime aTime) {
  int aYear = aTime.year();
  if (aYear > 2000) {
    aYear = aYear - 2000;
  }
  char buffer[14];
  sprintf(buffer,"C%02d%02d%02d.csv",aYear,aTime.month(),aTime.day());
  return String(buffer);
}

//
// Get a time string in format mm-dd-yy hh:mm:ss, which seems to make 
// most spreadsheets happy. RTCs vary and may or may not support 4 digit dates,
// so we always just use last 2 for consistency.
//
String getTimeString(DateTime aTime) {
  int thisYear = aTime.year();
  if (thisYear > 2000) {
    thisYear = thisYear - 2000;
  }
  char buffer[18];
  sprintf(buffer,"%02d-%02d-%02d %02d:%02d:%02d",aTime.month(),aTime.day(),thisYear,aTime.hour(),aTime.minute(),aTime.second());
  return String(buffer);
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
