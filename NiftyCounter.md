# NiftyCounter

NiftyCounter is an Adafruit ESP32-S2 (or -S3) Reverse TFT Feather based trail counter that records each trail user detection event as an entry in a .CSV file on a 
microSD card that can be collected and imported into a spreadsheet. 

NiftyCounter was developed as a followup to the ThriftyCounter project. ThriftyCounter works pretty well and has been counting trail users for some time, but there
was room for improvement.

- There's no way to determine if someone is looking at it or not, and so the green LED is faithfully lit every time a user detection sensor is active. This takes
about 11 ma the entire duration that a user is present. It's not a lot, but it adds up over time. 
- It's an act of faith that the RTC clock battery is good and the date and time is correct when you deploy a counter
- There's no way to determine that the LiPo battery is low and needs to be replaced or recharged

As it turns out, for only an additional $10 or so you can add a very nice user interface to this design, and a user interface can address each of these
shortcomings. Hence, the NiftyCounter was created.

![NiftyCounter assembled](/assets/images/NiftyCounter.jpg)

## Hardware Platform

NiftyCounter is based on an Adafruit ESP32-S2 Reverse TFT Feather board, available at https://www.adafruit.com/product/5345 Alternately, it seems to run just fine on the -S3 variant as well, so use whatever is available.

The Reverse TFT Feather board includes the processor, a nice 240x135 TFT display with three buttons, a LiPo battery charger, WiFi, and other features we don't need (and will keep turned off to save power). 
The ESP32-S2 on this board is different than the processor on the NiftyCounter board, but it too is supported by the Arduino IDE development environment
so it was pretty straightforward to migrate the NiftyCounter code to this platform. This processor has much more memory and processing power available, so there's
room to code up a nice user interface and expand features in the future.

Like the ThriftyCounter, the LiPo battery support on this board opens up a number of possibilities in power  management. If USB power is available, the adalogger board will 
automatically use that power source and charge an attached LiPo battery. If the USB power is lost, the adalogger board uses the LiPo battery as a source. 
This means you could use a cheap USB solar panel charger to power it during the day and keep the LiPo battery topped off, or periodically plug in an external USB
battery pack to top off an internal LiPo battery. Or just swap the LiPo battery when you're swapping the microSD card each month. 

In addition to the ESP32-S2 feather board we'll need to add data recording and a battery backup real time clock. Adafruit has one of these at
https://www.adafruit.com/product/2922 that works nicely and can stack on the Reverse TFT Feather Board

![NiftyCounter assembled](/assets/images/NiftyCounterBack.jpg)

As I mentioned, the ESP32-S2 contains WiFi support, and the -S3 variant includes BLE as well. It would be possible to enable one of these and remote harvest
counter data without touching a counter, but there's two big issues with this:
- Power consumption.
- Counter Discovery. Broadcasting a signal makes a counter huntable by nefarious foreign agents.

It would be possible to code a button or menu option to turn on communications for a short period to harvest data, but that means having access to the counter and if 
you're there it's so much easier to just swap the microSD card. 


## Software Platform

The NiftyCounter software can be found in NiftyCounter.ino in this repository.


## Sensor

See [Sensor Overview](SensorOverview.md) for more information on sensors.

I used RCWL-0516 microwave sensors when using this trail counter. These sensors can be powered directly from the board (using the GND and 3.3V pins 
and the output logic signal can be tied to one of the input pins (I used DIO 11 in the software). This sensor works best when located a couple inches 
away from the feather boards and battery, with no metal or electronics directly in front or behind the antenna area on the sensor board.

The data sheets for the RCWL-0516 say that the board requires an input voltage between 4 and 28v, but that input is regulated to 3.3 volts to run board and the
samples that I've used seem totally happy running with the nice, clean, regulated 3.3v source that the adalogger feather can provide.

## Using the NiftyCounter

On startup, the display, clock, and SD card are initialized and readied. Users can follow along on the startup log

![NiftyCounter assembled](/assets/images/NiftyCounterStartup.jpg)

After a moment or two, the normal main dashboard is displayed. This dashboard includes information about the current time, battery status, sensor status, and count
values for the hour, day, and since last startup.

![NiftyCounter assembled](/assets/images/NiftyCounterDisplay.jpg)

The display remains visible for a few moments, and then is turned off to save power. Pressing any button on the left will reactivate the display again. 

Pressing the middle (D1) button will display the options menu:

![NiftyCounter assembled](/assets/images/NiftyCounterMenu.jpg)

These menu options can be used to select various features - disabling sensors, safely ejecting the microSD card, etc. D0 is the up button, D2 the down button, and D1
selects/enters. 

To harvest data, simply select "eject SD card" in the menu, replace the microSD card with a new, preformatted card and press the board Restart button. The SD 
card will contain a .csv file named "CYYMMDD".csv, where YYMMDD reflect the current date. Each entry in the csv file will contain fields for the timestamp of 
the event, the date of the event, the time of the event, the duration of the event, and an event count.




