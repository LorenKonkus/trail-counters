# NiftyCounterSTHS34PF80

NiftyCounterSTHS34PF80 is a derivitive of the NiftyCounter with code updated to work with an ST Microelectronics STHS34PF80 TMOS motion and presence sensor
on an I2C bus and separate interupt input. 
With this sensor, the Adafruit ESP32-S2 (or -S3) Reverse TFT Feather microcontroller waits for an interupt and then communicates with the sensor to determine 
presence or motion within its field of view, records each trail user detection event as an entry in a .CSV file on a 
microSD card that can be collected and imported into a spreadsheet. 

![NiftyCounter assembled](/assets/images/NiftyCounterSTHS34PF80Hardware.JPG)

## Hardware Platform

NiftyCounterSTHS34PF80 is based on an Adafruit ESP32-S2 Reverse TFT Feather board, available at https://www.adafruit.com/product/5345 Alternately, it seems to run just fine on the -S3 variant as well, so use whatever is available.

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

The NiftyCounterSTHS34PF80 software can be found in NiftyCounterSTHS34PF80.ino in this repository. Well, it will be as soon as I'm happy enough 
with the software to share.


## Sensor

The NiftyCounterSTHS34PF80 software was designed for the ST Microelectronics STHS34PF80 TMOS motion and presence sensor, available at https://www.sparkfun.com/sparkfun-mini-human-presence-and-motion-sensor-sths34pf80-qwiic.html

See [Sensor Overview](SensorOverview.md) for more information on sensors.


## Using the NiftyCounterSTHS34PF80

On startup, the display, clock, and SD card are initialized and readied. Users can follow along on the startup log

![NiftyCounter assembled](/assets/images/NiftyCounterI2CStartup.jpg)

After a moment or two, the normal main dashboard is displayed. This dashboard includes information about the current time, battery status, sensor status, and count
values for the hour, day, and since last startup.

![NiftyCounter assembled](/assets/images/NiftyCounterDisplay.jpg)

The display remains visible for a few moments, and then is turned off to save power. Pressing any button on the left will reactivate the display again. 

Pressing the middle (D1) button will display the options menu:

![NiftyCounter assembled](/assets/images/NiftyCounterI2CMenu.jpg)

These menu options can be used to select various features - debugging sensors, safely ejecting the microSD card, etc. D0 is the up button, D2 the down button, and D1
selects/enters. 

Selecting the Sensor Details menu option will display a dynamic page displaying position and motion sensor values. Highlighted reverse text indicates that the values are at the level
that would be considered "active":

![NiftyCounter assembled](/assets/images/NiftyCounterI2CSensor.jpg)

To harvest data, simply select "eject SD card" in the menu, replace the microSD card with a new, preformatted card and press the board Restart button. The SD 
card will contain a .csv file named "CYYMMDD".csv, where YYMMDD reflect the current date. Each entry in the csv file will contain fields for the timestamp of 
the event, the date of the event, the time of the event, the duration of the event, and an event count.
