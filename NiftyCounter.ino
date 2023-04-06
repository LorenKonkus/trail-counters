/*
  NiftyCounter - A low cost Trail Counter with a nifty user interface

  A low cost trail use counter using off the shelf hardware and 
  inexpensive, readily available microwave sensors that's appropriate when you
  just want to place a few counters out on trails in the forest and are able
  to visit them every month or two to switch batteries and harvest the microSD card. 

  Required components:
  - adafruit Adafruit ESP32-S2 Reverse TFT feather board
  - adafruit Adalogger feather wing
  - a microwave sensor. Tested using RCWL-0516 and HFS-DC06F, but similar devices should
    work. Really, any sensor that provides a high detection pulse should work.

  Time stamped trail use data is recorded in a .csv file on the SD card that can be 
  viewed and summarized in a spreadsheet. Current trail use statistics can be viewed by
  using the TFT display.

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

// The Adafruit ESP32-S2 card's buit in Battery monitor
Adafruit_MAX17048 batteryMonitor;

// The Adafruit Reverse TFT card's built in display
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define DISPLAY_IDLE_COUNTER_RESET 60
volatile int displayIdleCounter = DISPLAY_IDLE_COUNTER_RESET;

// We use this digital in for the trail user sensor active detection. This can change,
// but it needs to be a GPIO that has RTC functionality.
#define SENSOR_IN 12
RTC_DATA_ATTR boolean sensorEnabled = true;

// Chip select for the microSD card
#define SD_CARD_SELECT 10

//
// Slots for user counts
//
RTC_DATA_ATTR int totalSinceStart = 0;
RTC_DATA_ATTR int totalThisDay = 0;
RTC_DATA_ATTR int totalThisHour = 0;
RTC_DATA_ATTR int hourTrip = -1;
RTC_DATA_ATTR int dayTrip = -1;

volatile unsigned long eventStartTime = 0;
volatile unsigned long lastEventDuration = 0;

File logfile;
RTC_DATA_ATTR boolean recordingEvents = false;
RTC_DATA_ATTR char logFileName[20];



#define IDLE_COUNTER_RESET 100
volatile int idleCounter = IDLE_COUNTER_RESET;

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
int setClockValues[5];
const String setClockLabels[6] = {"Year:","Month:","Day:","Hour:","Minute:","Save"};
const int setClockValuesMin[5] = {23,1,1,0,0};
const int setClockValuesMax[5] = {99,12,31,23,59};

//
// Interupt routine called when a proximity sensor event occurs. At the
// start of the event we'll remember the start time and at the end we'll
// compute the length of the duration, which will be a trigger to record
// the event to the SD card in the main loop. 
//
void onSensorChanged() {
  if (sensorEnabled) {
    if (digitalRead(SENSOR_IN) == HIGH) {
      eventStartTime = millis();
      lastEventDuration = 0;
    } else {
      if (eventStartTime != 0) {
        lastEventDuration = millis() - eventStartTime;
        eventStartTime = 0;
      }
    }
  }
  idleCounter = IDLE_COUNTER_RESET;
  displayIdleCounter = DISPLAY_IDLE_COUNTER_RESET;
}

//
// Interupt routine called whenever a button is pressed. Too much work to
// do in an interupt routine so if a button is pressed we'll note it for 
// the next loop iteration.
//
// On the reverse TFT feather board, D0 is normally high, D1, D2 are normally
// low.
//
void onButtonPressed() {

  // Debounce buttons
  static unsigned long last_interupt_time = 0;
  unsigned long interupt_time = millis();
  if (interupt_time - last_interupt_time > 300) {

    if (digitalRead(0) == LOW) {
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
  displayIdleCounter = DISPLAY_IDLE_COUNTER_RESET;
}

void setup() {

  Serial.begin(115200);

  // We can get here either by a normal reset startup or by waking up from a deep sleep.
  // When in deep sleep, memory and connections can be lost so they need to be re-established,
  // but we don't want the verbose UI startup dialog or delays when waking
  boolean coldStart = true;
  switch(esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_EXT0 : coldStart = false; break;
    case ESP_SLEEP_WAKEUP_EXT1 : coldStart = false; break;
  }

  // If this is a wakeup from a sensor or button, we can't expect that the interupt 
  // routine was run (since we haven't called attach to interrupt yet). So call them
  // explicitly. This is non-destructive if an event or button press hadn't happened.
  if (!coldStart) {
    buttonPressed = 0;
    onButtonPressed();
    eventStartTime = 0;
    onSensorChanged();
  }
  
  // set LED to be an output pin
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // We won't use the NeoPixel
  pinMode(NEOPIXEL_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_POWER, LOW);

  // Setup the TFT Display
  pinMode(TFT_BACKLITE, OUTPUT);
  pinMode(TFT_I2C_POWER, OUTPUT);
  tftOn();
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3); // landscape mode with buttons on left
  tft.fillScreen(ST77XX_BLACK);

  // Start the startup splash screen
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  if (coldStart) {
    tft.println("Nifty Counter");
    tft.println("version 1.0");
    tft.setTextSize(1);
    tft.println("\nCopyright (c) 2023 Loren Konkus\n");
  }

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
  
  if (coldStart) {
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);
    tft.print("\nCurrent time: ");
    tft.setTextColor(ST77XX_WHITE);
    tft.println(getTimestampString(getCurrentTime()));
  }

  // Initialize the battery monitor
  if (!batteryMonitor.begin()) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(1);
    tft.println("Battery monitor not available");
  } else {    
    if (coldStart) {
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
  }

  // Initialize the SD card
  pinMode(SD_CARD_SELECT, OUTPUT);
  if (!SD.begin(SD_CARD_SELECT)) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println("Insert SD Card and");
    tft.println("press reset to");
    tft.println("proceed.");
    while (1) delay (1000);
  }

  // create an event log .csv file using the counter startup date
  boolean newfile = false;
  if (coldStart) {
    eventsFileName(logFileName, getCurrentTime());
    if (!SD.exists(logFileName)) {
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
    tft.println(logFileName);
  }

  logfile = SD.open(logFileName, FILE_WRITE);
  if (!logfile) {
    tft.setTextColor(ST77XX_RED);
    tft.setTextSize(2);
    tft.println("Error opening log");
    tft.print("file. Check SD card");
    while (1) delay (1000);
  }
  recordingEvents = true;

  if (newfile) {
    logfile.println("Event Time, Duration, Event Count, Hour Count, Day Count, Cummulative Count");
    logfile.flush();
  }

  // Proximity detector digital in
  pinMode(SENSOR_IN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSOR_IN), onSensorChanged, CHANGE);
  
  // Buttons
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLDOWN);
  pinMode(2, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(0), onButtonPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(1), onButtonPressed, RISING);
  attachInterrupt(digitalPinToInterrupt(2), onButtonPressed, RISING);

  // We'll go into light sleep mode to save power between events. We'll wake up
  // for either a sensor input or a button press.
  #define BUTTON_PIN_BITMASK 0x000001006 // io GPIO 1,2,12 in hex
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  rtc_gpio_pullup_en(GPIO_NUM_0);
  rtc_gpio_pulldown_en(GPIO_NUM_1);
  rtc_gpio_pulldown_en(GPIO_NUM_2);

  // Initialization complete
  if (coldStart) {
    tft.setTextColor(ST77XX_GREEN);
    tft.setTextSize(1);
    tft.println("\nStartup Successful");
    delay (5000);
  }
  
  idleCounter = IDLE_COUNTER_RESET;
  displayIdleCounter = DISPLAY_IDLE_COUNTER_RESET;
  activePage = 0;
  displayDashboardPage();
}

//
// Running mode
//
void loop() {

  // Manage the timeout. If we've gone DISPLAY_IDLE_COUNTER_RESET loops without
  // a button press, turn off the display. Users can press any button to turn it on again.
  if (displayIdleCounter>0) {
    displayIdleCounter--;
  }
  if (displayIdleCounter == 0) {
    // Time to sleep...
    tftOff();
    if (logfile) {
      logfile.close();
    }
    esp_deep_sleep_start();
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
  DateTime now = getCurrentTime();
  if (now.day() != dayTrip) {
    totalThisDay = 0;
    dayTrip = now.day();
    totalThisHour = 0;
    hourTrip = now.hour();
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
      String eventLogRecord = getTimestampString(now);
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
void eventsFileName(char* buffer, DateTime aTime) {
  int aYear = aTime.year();
  if (aYear > 2000) {
    aYear = aYear - 2000;
  }
  sprintf(buffer,"/C%02d%02d%02d.csv",aYear,aTime.month(),aTime.day());
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

  // change the sensor enable to whatever it isn't
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
  totalSinceStart = 0;
  totalThisDay = 0;
  totalThisHour = 0;

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
  if (!SD.begin(SD_CARD_SELECT)) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(30, 60);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(2);
    tft.print("SD card error");
    delay (5000);
    return false;
  }

  // create an event log .csv file using the counter startup date
  eventsFileName(logFileName, getCurrentTime());
  boolean newfile = false;
  if (!SD.exists(logFileName)) {
    newfile = true;
  }

  if (logfile) {
    logfile.close();
  }
  logfile = SD.open(logFileName, FILE_WRITE);
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
