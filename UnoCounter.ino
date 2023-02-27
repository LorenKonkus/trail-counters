/*
  UnoCounter - A low cost Arduino Uno platform Trail Counter

  A low cost trail use counter using off the shelf Arduino shields and 
  inexpensive, readily available microwave sensors. 

  Required components:
  - Arduino Uno R3
  - adafruit data logging shield for arduino
  - a microwave sensor. Tested using RCWL-0516 and HFS-DC06F, but similar devices should
    work. Really, any sensor that provides a high detection pulse should work.

  Optional component:
  - adafrut 16x2 LCD shield for arduino (the kind that communicates via the I2C bus)

  Time stamped trail use data is recorded in a .csv file on the SD card that can be 
  viewed and summarized in a spreadsheet. Current trail use statistics can be viewed by
  scrolling through the LCD display, if that display is present.

  Copyright © 2022 Loren Konkus
 
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
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <avr/sleep.h>

// Backlighting the LCD takes power and bettery power is a premium out on
// the trail, so we'll only light it for a few seconds after a button press
#define BACKLIGHT_OFF 0x0
#define BACKLIGHT_ON 0x7 
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
volatile int backlightCounter = 300;

RTC_PCF8523 rtc;

// We use this digital in for the microwave sensor active detection. This can
// change, but we'll sleep the processor to save power between count events so whatever 
// you choose needs to be able to wake the processor up
const int sensorIn = 3;

int totalSinceStart = 0;
int totalThisDay = 0;
int totalThisHour = 0;
int hourTrip = -1;
int dayTrip = -1;
int displayPane = 0;
volatile int lastTimeDisplayed = -1;
volatile int lastCountDisplayed = -1;
volatile unsigned long eventStartTime = 0;
volatile unsigned long lastEventDuration = 0;
volatile int idleCounter = 1000;
DateTime lastEventTime;
File logfile;
const int chipSelect = 10;

//
// Interupt routine called when a proximity sensor event occurs. At the
// start of the event we'll remember the start time and at the end we'll
// compute the length of the duration, which will be a trigger to record
// the event to the SD card in the main loop. 
//
void onSensorChanged() {
  if (digitalRead(sensorIn) == HIGH) {
    eventStartTime = millis();
    lastEventDuration = 0;
  } else {
    lastEventDuration = millis() - eventStartTime;
  }
  sleep_disable();
  idleCounter = 1000;
}

//
// One time initialization
//
void setup() {

  // Serial monitor for debugging
  Serial.begin(9600);

  // Liquid Crystal Display, 2 lines 16 chars
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.println("UnoCounter       ");
  lcd.setCursor(0, 1);
  lcd.print("V 01.01         ");
  delay(2000);
  lcd.print("Initializing    ");
  delay(2000);

  // Real time clock
  if (! rtc.begin()) {
    Serial.println("No RTC");
    Serial.flush();
    lcd.setCursor(0, 1);
    lcd.print("No RTC Found    ");
    while (1) delay(10);
  }
  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("Updating RTC clock");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.start();

  // Initialize the SD card
  pinMode(chipSelect, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("No SD card");
    lcd.setCursor(0, 1);
    lcd.print("No SD card found");
    while (1) delay(10);
  }
  
  // create a new event log file
  String filename = eventsFileName(rtc.now());
  Serial.print("Creating log file ");
  Serial.println(filename);
  boolean newfile = false;
  if (! SD.exists(filename)) {
    newfile = true;
  }

  logfile = SD.open(filename, FILE_WRITE); 
  if (!logfile) {
    Serial.println("couldnt create event log file");
    lcd.setCursor(0, 1);
    lcd.print("Create file err ");
    while (1) delay(10);
  }

  if (newfile) {
    logfile.println("Event Time, Duration, Event Count, Hour Count, Day Count, Cummulative Count");
  }

  // Proximity detector digital in
  pinMode(sensorIn, INPUT);
  attachInterrupt(digitalPinToInterrupt(sensorIn), onSensorChanged, CHANGE);
  
  Serial.println("Ready");
  lcd.setCursor(0, 1);
  lcd.print("Ready           ");
  delay(2000);
}

void loop() {
  
  // Manage backlighting. Only on for 30ish seconds after a button pressed
  // to save a bit of battery
  uint8_t buttons = lcd.readButtons();
  if (buttons) {
    idleCounter = 1000;
    lcd.setBacklight(BACKLIGHT_ON);
    backlightCounter = 300;
    if (buttons & BUTTON_SELECT) {
      lcd.setBacklight(BACKLIGHT_ON);
      backlightCounter = 300;
    }
    if (buttons & BUTTON_UP) {
      displayPane--;
      if (displayPane < 0) {
        displayPane = 3;
      }
      lastCountDisplayed = -1;
    }
    if (buttons & BUTTON_DOWN) {
      displayPane++;
      if (displayPane > 3) {
        displayPane = 0;
      }
      lastCountDisplayed = -1;
    }

  }
  if (backlightCounter>0) {
    backlightCounter--;
    if (backlightCounter == 0) {
      lcd.setBacklight(BACKLIGHT_OFF);
    }
  }

  //
  // Manage low power mode. If no events have occured in the last minute, then enter a sleep
  // state to save power
  //
  if (idleCounter>0) {
    idleCounter--;
    if (idleCounter == 0) {
      sleep_enable();
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      sleep_cpu();
    }
  }

  //
  // Reset counters if needed
  //
  DateTime now = rtc.now();
  if (now.day() != dayTrip) {
    totalThisDay = 0;
    dayTrip = now.day();
  }
  if (now.hour() != hourTrip) {
    totalThisHour = 0;
    hourTrip = now.hour();
  }

  //
  // Update the display, based on the pane selected. This only updates once each
  // minute or whenever a count is detected
  //
  int thisMinute = now.minute();
  if (lastTimeDisplayed != thisMinute ||
      lastCountDisplayed != totalSinceStart) {
    if (displayPane == 0) {
      // Display date and time on first line, overall count on second
      String line0 = getShortTimeString(now);
      line0 += "     ";
      lcd.setCursor(0, 0);
      lcd.print(line0);
      lastTimeDisplayed = thisMinute;
      String line1 = String("Users: ");
      line1 += totalSinceStart;
      line1 += "     ";
      lcd.setCursor(0, 1);
      lcd.print(line1);
      lastCountDisplayed = totalSinceStart;
    }
    if (displayPane == 1) {
      // Display last event time
      lcd.setCursor(0, 0);
      lcd.print("Last event time");
      String line1 = getTimeOfDayString(lastEventTime);
      lcd.setCursor(0, 1);
      lcd.print(line1);
      lastCountDisplayed = totalSinceStart;
    }
    if (displayPane == 2) {
      // Display count of passes this hour
      lcd.setCursor(0, 0);
      lcd.print("Users this hour ");
      String line1 = String(totalThisHour);
      line1 += "            ";
      lcd.setCursor(0, 1);
      lcd.print(line1);
      lastCountDisplayed = totalSinceStart;
    }
    if (displayPane == 3) {
      // Display count of passes this day
      lcd.setCursor(0, 0);
      lcd.print("Users this day  ");
      String line1 = String(totalThisDay);
      line1 += "            ";
      lcd.setCursor(0, 1);
      lcd.print(line1);
      lastCountDisplayed = totalSinceStart;
    }
  }
  
  //
  // If an event occured, record it. The duration of the last event is a flag to
  // denote one occured.
  //
  if (lastEventDuration != 0) {
    totalSinceStart++;
    totalThisDay++;
    totalThisHour++;
    lastEventTime = now;
    String eventLogRecord = getTimeString(lastEventTime);
    eventLogRecord += ",";
    eventLogRecord += lastEventDuration;
    eventLogRecord += ",1,";
    eventLogRecord += totalThisHour;
    eventLogRecord += ",";
    eventLogRecord += totalThisDay;
    eventLogRecord += ",";
    eventLogRecord += totalSinceStart;
    Serial.println(eventLogRecord);
    logfile.println(eventLogRecord);
    logfile.flush();
    
    lastEventDuration = 0;
  }

  delay(100);
}

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
// Get a time string in format mm-dd-yy hh:mm, which fits on the display
//
String getShortTimeString(DateTime aTime) {
  int thisYear = aTime.year();
  if (thisYear > 2000) {
    thisYear = thisYear - 2000;
  }
  char buffer[16];
  sprintf(buffer,"%02d-%02d-%02d %02d:%02d",aTime.month(),aTime.day(),thisYear,aTime.hour(),aTime.minute());
  return String(buffer);
}

//
// Get a time string in format mm-dd-yy hh:mm, which fits on the display
//
String getTimeOfDayString(DateTime aTime) {
  char buffer[10];
  sprintf(buffer,"%02d:%02d:%02d",aTime.hour(),aTime.minute(),aTime.second());
  return String(buffer);
}
