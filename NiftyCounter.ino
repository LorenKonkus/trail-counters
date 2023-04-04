/*
  NiftyCounter - A low cost  Trail Counter

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

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include "Adafruit_MAX1704X.h"
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include "driver/rtc_io.h"


// The Adafruit Adalogger board uses a 8523 as the Real time clock
RTC_PCF8523 rtc;

// Use dedicated hardware SPI pins for display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
const int displayIdleCounterReset = 100;
volatile int displayIdleCounter = displayIdleCounterReset;

// We use this digital in for the trail user sensor active detection. This can change,
// but it needs to be a GPIO that has RTC functionality: 0,2,4,12-15,25-27,32-39
const int sensorIn = 12;
boolean sensorEnabled = true;

// Chip select for the microSD card
const int cardSelect = 10;

// Battery monitor
Adafruit_MAX17048 batteryMonitor;

int led = LED_BUILTIN;

//
// Slots for user counts
//
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
DateTime lastEventTime;
File logfile;
boolean recordingEvents = false;
const int idleCounterReset = 100;
volatile int idleCounter = idleCounterReset;

volatile int buttonPressed = 0;
#define UP_BUTTON 1
#define SELECT_BUTTON 2
#define DOWN_BUTTON 3

int activePage = 0;
#define MENU_PAGE 1
#define DASHBOARD_PAGE 2
#define SD_PAGE 3
#define SET_CLOCK_PAGE 4

int menuItemSelected = 1;
#define DASHBOARD_MENUITEM 1
#define SENSOR_MENUITEM 2
#define SD_MENUITEM 3
#define RESET_MENUITEM 4
#define SETCLOCK_MENUITEM 5
#define MAX_MENUITEM 5

int setClockLabelSelected = 0;
int setClockValueSelected = -1;
String setClockLabels[6] = {"Year:","Month:","Day:","Hour:","Minute:","Save"};
int setClockValues[5];
int setClockValuesMin[5] = {23,1,1,0,0};
int setClockValuesMax[5] = {99,12,31,23,59};

//
// Interupt routine called when a proximity sensor event occurs. At the
// start of the event we'll remember the start time and at the end we'll
// compute the length of the duration, which will be a trigger to record
// the event to the SD card in the main loop. 
//
void onSensorChanged() {
  if (sensorEnabled) {
    if (digitalRead(sensorIn) == HIGH) {
      eventStartTime = millis();
      lastEventDuration = 0;
    } else {
      lastEventDuration = millis() - eventStartTime;
      eventStartTime = 0;
    }
  }
  //  sleep_disable();
  idleCounter = idleCounterReset;
}

//
// Interupt routine called whenever a button is pressed. Too much work to
// do in an interupt routine so if a button is pressed we'll note it for 
// the next loop iteration.
//
void onButtonPressed() {

  // Debounce buttons
  static unsigned long last_interupt_time = 0;
  unsigned long interupt_time = millis();
  if (interupt_time - last_interupt_time > 300) {

    if (digitalRead(0) == HIGH) {
      buttonPressed = UP_BUTTON;
    }
    if (digitalRead(1) == HIGH) {
      buttonPressed = SELECT_BUTTON;
    }
    if (digitalRead(2) == HIGH) {
      buttonPressed = DOWN_BUTTON;
    }
  }
  last_interupt_time = interupt_time;
  displayIdleCounter = displayIdleCounterReset;
}

void setup() {

  Serial.begin(115200);

  // set LED to be an output pin
  pinMode(led, OUTPUT);

  // Setup the TFT Display
  pinMode(TFT_BACKLITE, OUTPUT);
  pinMode(TFT_I2C_POWER, OUTPUT);
  tftOn();

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3); // landscape mode with buttons on left
  tft.fillScreen(ST77XX_BLACK);

  // Start the startup splash screen
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("Nifty Counter");
  tft.println("version 1.0");
  tft.setTextSize(1);
  tft.println("\nCopyright (c) 2023 Loren Konkus\n");

  // Initialize the RTC
  if (! rtc.begin()) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println("Fatal Error");
    tft.println("No Real Time Clock");
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, setting the time");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(1);
    tft.println("Real Time Clock initialized");
  }
  rtc.start();

  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.print("\nCurrent time: ");
  tft.setTextColor(ST77XX_WHITE);
  tft.println(getTimestampString(getCurrentTime()));

  // Initialize the battery monitor
  if (!batteryMonitor.begin()) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.println("Battery monitor not available");
  } else {    
    delay(1000); // battery monitor needs a moment to collect itself... 
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);
    tft.print("\nBattery Status: ");
    tft.setTextColor(ST77XX_WHITE);
    tft.print(batteryMonitor.cellVoltage(),1);
    tft.print("v, ");
    tft.print(batteryMonitor.cellPercent(),0);
    tft.println("%");
  }

  // Initialize the SD card
  pinMode(cardSelect, OUTPUT);
  if (!SD.begin(cardSelect)) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println("Insert SD Card and");
    tft.println("press reset to");
    tft.println("proceed.");
    while (1) delay (1000);
  }

  // create an event log .csv file using the counter startup date
  String filename = eventsFileName(getCurrentTime());
  boolean newfile = false;
  if (!SD.exists(filename)) {
    newfile = true;
  }
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  if (newfile) {
    tft.print("\nCreating file: ");
  } else {
    tft.print("\nOpening file: ");
  }
  tft.setTextColor(ST77XX_WHITE);
  tft.println(filename);

  logfile = SD.open(filename, FILE_WRITE);
  if (!logfile) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println("Error opening log");
    tft.println("file. Check SD card");
    while (1) delay (1000);
  }
  recordingEvents = true;

  if (newfile) {
    logfile.println("Event Time, Duration, Event Count, Hour Count, Day Count, Cummulative Count");
    logfile.flush();
  }

  // Proximity detector digital in
  pinMode(sensorIn, INPUT);
  attachInterrupt(digitalPinToInterrupt(sensorIn), onSensorChanged, CHANGE);
  
  // Buttons
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLDOWN);
  pinMode(2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(0), onButtonPressed, RISING);
  attachInterrupt(digitalPinToInterrupt(1), onButtonPressed, RISING);
  attachInterrupt(digitalPinToInterrupt(2), onButtonPressed, RISING);

  // We'll go into light sleep mode to save power between events. We'll wake up
  // for either a sensor input or a button press.
  #define BUTTON_PIN_BITMASK 0x000000F07 // io 0,1,2,12 in hex
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  rtc_gpio_pullup_en(GPIO_NUM_0);
  rtc_gpio_pulldown_en(GPIO_NUM_1);
  rtc_gpio_pulldown_en(GPIO_NUM_2);

  // Initialization complete
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.println("\nStartup Successful");
  delay (5000);

  displayDashboardPage();
}

//
// Running mode
//
void loop() {

  // Manage the display timeout. If we've gone displayIdleCounterReset loops without
  // a button press, turn off the display. Users can press any button to turn it on again.
  if (displayIdleCounter>0) {
    displayIdleCounter--;
  }
  if (displayIdleCounter == 0) {
    tftOff();
  } else {
    tftOn();
  }

  // If there was a button pressed, then take some action in the UI. UIs are messy - this is about
  // as complicated as I'd want to build before moving to a finite state machine
  if (buttonPressed != 0) {
    switch (activePage) {
      
      case DASHBOARD_PAGE:
        switch (buttonPressed) {
          
          case SELECT_BUTTON:
            displayMenuPage();
            break;
        }
        break;
      
      case SD_PAGE:
        switch (buttonPressed) {
          
          case SELECT_BUTTON:
            if (doLoadSDCard()) {
              displayMenuPage();
            }
            break;
        }
        break;
      
      case SET_CLOCK_PAGE:
        switch (buttonPressed) {
          
          case UP_BUTTON:
            if (setClockLabelSelected >= 0) {
              if (--setClockLabelSelected < 0) {
                setClockLabelSelected = 5;
              }
            }
            if (setClockValueSelected >= 0) {
              if (++setClockValues[setClockValueSelected] > setClockValuesMax[setClockValueSelected]) {
                setClockValues[setClockValueSelected] = setClockValuesMin[setClockValueSelected];
              }
            }
            break;
                   
          case DOWN_BUTTON:
            if (setClockLabelSelected >= 0) {
              if (++setClockLabelSelected > 5) {
                setClockLabelSelected = 0;
              }
            }
            if (setClockValueSelected >= 0) {
              if (--setClockValues[setClockValueSelected] < setClockValuesMin[setClockValueSelected]) {
                setClockValues[setClockValueSelected] = setClockValuesMax[setClockValueSelected];
              }
            }
            break;
                    
          case SELECT_BUTTON:
            if (setClockLabelSelected == 5) {
              doUpdateClock();
              displayMenuPage();
              break;
            }
            if (setClockLabelSelected >= 0) {
              setClockValueSelected = setClockLabelSelected;
              setClockLabelSelected = -1;
              break;
            }
            if (setClockValueSelected >= 0) {
              setClockLabelSelected = setClockValueSelected + 1;
              setClockValueSelected = -1;
            }
            break;
        }
        break;
      
      case MENU_PAGE:
        switch (buttonPressed) {
          
          case UP_BUTTON:
            if (--menuItemSelected < 1) {
              menuItemSelected = MAX_MENUITEM;
            }
            break;
                    
          case DOWN_BUTTON:
            if (++menuItemSelected > MAX_MENUITEM) {
              menuItemSelected = 1;
            }
            break;
         
          case SELECT_BUTTON:
            switch (menuItemSelected) {
            
              case DASHBOARD_MENUITEM:
                displayDashboardPage();
                break;
                
              case SENSOR_MENUITEM:
                doToggleSensors();
                break;
                
              case SD_MENUITEM:
                doRemoveSDCard();
                break;
                
              case RESET_MENUITEM:
                doResetCounters();
                break;
                
              case SETCLOCK_MENUITEM:
                displaySetClock();
                break;
            }
            break;
         
        }
        break;
    }
    buttonPressed = 0;
  }
  
  // Refresh the active page
  switch (activePage) {
    case MENU_PAGE:
      displayMenuPage();
      break;
    case SD_PAGE:
      doRemoveSDCard();
      break;
    case SET_CLOCK_PAGE:
      displaySetClock();
      break;
    case DASHBOARD_PAGE:
      displayDashboardPage();
      break;
  }
  
  //
  // Reset the hourly and daily counters if needed
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
  // If an event occured, record it. The duration of the last event is a flag to
  // denote one occured.
  //
  if (lastEventDuration != 0) {
    if (recordingEvents) {
      totalSinceStart++;
      totalThisDay++;
      totalThisHour++;
      lastEventTime = now;
      String eventLogRecord = getTimestampString(lastEventTime);
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
    }
    
    lastEventDuration = 0;
  }

  delay(1000);               // wait for a second
}

//
// Get the current time
//
DateTime getCurrentTime() {
  return rtc.now();
}

//
// Get a time string in format mm-dd-yy hh:mm:ss, which seems to make 
// most spreadsheets happy. RTCs vary and may or may not support 4 digit dates,
// so we always just use last 2 for consistency.
//
String getTimestampString(DateTime aTime) {
  int thisYear = aTime.year();
  if (thisYear > 2000) {
    thisYear = thisYear - 2000;
  }
  char buffer[20];
  sprintf(buffer,"%02d-%02d-%02d %02d:%02d:%02d",aTime.month(),aTime.day(),thisYear,aTime.hour(),aTime.minute(),aTime.second());
  return String(buffer);
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
  sprintf(buffer,"/C%02d%02d%02d.csv",aYear,aTime.month(),aTime.day());
  return String(buffer);
}

//
// Routine to turn on the TFT display. 
//
void tftOn() {
  digitalWrite(TFT_BACKLITE, HIGH);
  digitalWrite(TFT_I2C_POWER, HIGH);  
}

//
// Turn off the TFT display backlight and I2C power supply. There's a timer so when not 
// in use the display is turned off to save power
//
void tftOff() {
  digitalWrite(TFT_BACKLITE, LOW);
  digitalWrite(TFT_I2C_POWER, LOW);
}

// 
// User requested that the sensors be enabled or disabled
//
void doToggleSensors() {

  // reset counters
  sensorEnabled = !sensorEnabled;

  // tell the user
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(30, 60);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  if (sensorEnabled) {
    tft.print("Sensors enabled");
  } else {
    tft.print("Sensors offline");
  }
  delay (5000);
  tft.fillScreen(ST77XX_BLACK);
}

// 
// User requested that counters be reset to 0
//
void doResetCounters() {

  // reset counters
//  totalSinceStart = 0;
//  totalThisDay = 0;
//  totalThisHour = 0;

  // tell the user
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(30, 60);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.print("Sleep Test");
  delay (5000);
  esp_light_sleep_start();
    pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLDOWN);
  pinMode(2, INPUT_PULLDOWN);

  // tell the user
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(30, 60);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.print("Counters reset");
  delay (5000);
  tft.fillScreen(ST77XX_BLACK);
  
}

// 
// User wants to update the date and time
//
void displaySetClock() {
  if (activePage != SET_CLOCK_PAGE) {
    DateTime aTime = getCurrentTime();
    setClockValues[0] = aTime.year();
    if (setClockValues[0] > 2000) {
      setClockValues[0] = setClockValues[0] - 2000;
    }
    setClockValues[1] = aTime.month();
    setClockValues[2] = aTime.day();
    setClockValues[3] = aTime.hour();
    setClockValues[4] = aTime.minute();
  
    // Initial page display
    tft.fillScreen(ST77XX_BLACK);
    tft.drawChar(0, 0, 30, ST77XX_CYAN, ST77XX_BLACK, 2);
    tft.drawChar(0, 60, 15, ST77XX_CYAN, ST77XX_BLACK, 2);
    tft.drawChar(0, 125, 31, ST77XX_CYAN, ST77XX_BLACK, 2);
    tft.setCursor(30, 0);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.print("  Set Clock");
    
    setClockLabelSelected = 0;
    setClockValueSelected = -1;
    activePage = SET_CLOCK_PAGE;
  }
  
  // Display the dynamic content
  for (int i=0; i<6; i++) {
    displaySetClockLabel(i);
  } 
  for (int i=0; i<5; i++) {
    displaySetClockValue(i);
  }
}

//
// On the Set Clock page, display the label for one of the clock fields
//
void displaySetClockLabel(int item) {
  if (setClockLabelSelected == item) {
    tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
  } else {
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  }
  tft.setCursor(30,25+item*17);
  tft.print(setClockLabels[item]);
}

//
// On the Set Clock page, display the current value of one of the clock fields
void displaySetClockValue(int item) {
  if (setClockValueSelected == item) {
    tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
  } else {
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  }
  tft.setCursor(140,25+item*17);
  if (setClockValues[item]<10) {
    tft.print("0");
  }
  tft.print(setClockValues[item]);
}

//
// User of the Set Clock page has updated the time - set the RTC clock to reflect it
//
void doUpdateClock() {
  DateTime newTime = DateTime(setClockValues[0]+2000,setClockValues[1],setClockValues[2],setClockValues[3],setClockValues[4],0);
  rtc.adjust(newTime);
}

// 
// User requested that the SD card be dismounted so that they can replace it
//
void doRemoveSDCard() {

  if (recordingEvents) {
    if (logfile) {
      logfile.flush();
      logfile.close();
    }
    recordingEvents = false;
  }
  
  // tell the user
  tft.fillScreen(ST77XX_BLACK);
  tft.drawChar(0, 0, 30, ST77XX_CYAN, ST77XX_BLACK, 2);
  tft.drawChar(0, 60, 15, ST77XX_CYAN, ST77XX_BLACK, 2);
  tft.drawChar(0, 125, 31, ST77XX_CYAN, ST77XX_BLACK, 2);
  tft.setCursor(30, 10);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.println("SD card can be");
  tft.setCursor(30, 30);
  tft.println("safely removed.");
  tft.setCursor(30, 60);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.println("Insert new ");
  tft.setCursor(30, 80);
  tft.println("formatted card");
  tft.setCursor(30, 100);
  tft.println("and press restart");
//  tft.println("and press *");
  activePage = SD_PAGE;
}

//
// User has inserted an SD card and is ready to start using it
//
// Note that this doesn't actually work. I don't see that the SD library
// has any means to detect that an SD card has been replaced and that a new
// connection needs to be established to the card. Leaving this here for future
// debugging - for now, the screen instructions say press restart and that
// works.
//
boolean doLoadSDCard() {
  if (!SD.begin(cardSelect)) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(30, 60);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(2);
    tft.print("SD card error");
    delay (5000);
    return false;
  }

  // create an event log .csv file using the counter startup date
  String filename = eventsFileName(getCurrentTime());
  boolean newfile = false;
  if (!SD.exists(filename)) {
    newfile = true;
  }

  if (logfile) {
    logfile.close();
  }
  logfile = SD.open(filename, FILE_WRITE);
  if (!logfile) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(30, 60);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(2);
    tft.print("log file error");
    delay (5000);
    return false;
  }

  if (newfile) {
    logfile.println("Event Time, Duration, Event Count, Hour Count, Day Count, Cummulative Count");
    logfile.flush();
  }

  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(30, 60);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setTextSize(2);
  tft.print("log file ready");
  delay (5000);
  
  recordingEvents = true;
  return true;
}

//
// Display the Menu page
//
void displayMenuPage() {
  if (activePage != MENU_PAGE) {
    // Display button icons 
    tft.fillScreen(ST77XX_BLACK);
    tft.drawChar(0, 0, 30, ST77XX_CYAN, ST77XX_BLACK, 2);
    tft.drawChar(0, 60, 15, ST77XX_CYAN, ST77XX_BLACK, 2);
    tft.drawChar(0, 125, 31, ST77XX_CYAN, ST77XX_BLACK, 2);
  }
  // Display menu items
  displayMenuItem(DASHBOARD_MENUITEM,"Back to Display");
  if (sensorEnabled) {
    displayMenuItem(SENSOR_MENUITEM,"Sensor: On ");
  } else {
    displayMenuItem(SENSOR_MENUITEM,"Sensor: Off");
  }
  displayMenuItem(SD_MENUITEM,"Eject SD Card");
  displayMenuItem(RESET_MENUITEM,"Reset Counts");
  displayMenuItem(SETCLOCK_MENUITEM,"Set Date/Time");
  activePage = MENU_PAGE;
}

void displayMenuItem(int itemNumber, String itemText) {
  if (menuItemSelected == itemNumber) {
    tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
  } else {
    tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  }
  tft.setCursor(30, (itemNumber-1)*20);
  tft.setTextSize(2);
  tft.print(itemText);
}

//
// Display the default overview page
//
void displayDashboardPage() {

  // If this is the initial time the page is requested, populate the static content
  if (activePage != DASHBOARD_PAGE) {
    tft.fillScreen(ST77XX_BLACK);
    tft.drawChar(0, 0, 30, ST77XX_CYAN, ST77XX_BLACK, 2);
    tft.drawChar(0, 60, 15, ST77XX_CYAN, ST77XX_BLACK, 2);
    tft.drawChar(0, 125, 31, ST77XX_CYAN, ST77XX_BLACK, 2);
    tft.setCursor(30, 0);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.print("Nifty Counter\n");
    tft.setTextSize(1);
    tft.setCursor(125, 39);
    tft.setTextColor(ST77XX_GREEN);
    tft.print("Battery:");
    tft.setTextSize(2);
    tft.setCursor(30, 60);
    tft.print("This hour:");
    tft.setCursor(30, 80);
    tft.print("This day:");
    tft.setCursor(30, 100);
    tft.print("Total:");

    activePage = DASHBOARD_PAGE;
  }

  // Populate the dynamic content
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(30, 25);
  tft.print(getTimestampString(getCurrentTime()));
  
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.fillRect(30, 39, 85, 10, ST77XX_BLACK);
  tft.setCursor(30, 39);
  if (sensorEnabled) {
    if (eventStartTime != 0) {
      tft.print("Sensor Active");
    } else {
      tft.print("Ready");
    }
  } else {
     tft.print("Sensor Off");
  }

  tft.fillRect(175, 39, 65, 10, ST77XX_BLACK);
  tft.setCursor(175, 39);
  tft.print(batteryMonitor.cellVoltage(),1);
  tft.print("v ");
  tft.print(batteryMonitor.cellPercent(),0);
  tft.print("%");

  tft.setTextSize(2);
  tft.fillRect(160, 60, 80, 16, ST77XX_BLACK);
  tft.setCursor(160, 60);
  tft.print(totalThisHour);
 
  tft.setCursor(160, 80);
  tft.fillRect(160, 80, 80, 16, ST77XX_BLACK);
  tft.print(totalThisDay);

  tft.setCursor(160, 100);
  tft.fillRect(160, 100, 80, 16, ST77XX_BLACK);
  tft.print(totalSinceStart);
}
