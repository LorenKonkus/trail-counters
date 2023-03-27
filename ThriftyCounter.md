# ThriftyCounter

ThriftyCounter is an AdafruitFeather M0 Adalogger based trail counter that records each trail user detection event as an entry in a .CSV file on a 
microSD card that can be collected and imported into a spreadsheet. ThriftyCounter was developed as a followup to the UnoCounter project in an attempt to 
lower energy usage and improve battery life. An Arduino Uno requires a fair amount of power even when the microcontrolleris in sleep mode, and this
has an impact on the battery required for extended deployment times out in a forest. Batteries are expensive, and if we can halve or quarter the amount
of capacity that we need we can reduce the cost significantly. ThriftyCounter was a great step in that direction. 

![ThriftyCounter assembled](/assets/images/ThriftyCounter.jpg)

## Hardware Platform

ThriftyCounter is based on an Adafruit Feather M0 Adalogger board, available at https://www.adafruit.com/product/2796 

In the same way that there's an ecosystem for Arduino Uno boards and shields that add additional capabilities, there's an ecosystem of Feather boards and "wings"
that stack and add capabilities. 

The adalogger board includes the processor, a LiPo battery charger, and a microSD card slot, so it has most of what we need (except a good real time clock - more on that
later). The ARM M0 processor on this board is different than the processor on the Uno board, but it is supported by the Arduino IDE development environment
so it was pretty straightforward to migrate the UnoCounter code to this platform. This processor has more memory and processing power available, and so there's
room to grow if we want to, say, enhance the software to collect data from multiple sensors.

The LiPo battery support on this board opens up a number of possibilities in power  management. If USB power is available, the adalogger board will 
automatically use that power source and charge an attached LiPo battery. If the USB power is lost, the adalogger board uses the LiPo battery as a source. 
This means you could use a cheap USB solar panel charger to power it during the day and keep the LiPo battery topped off, or periodically plug in an external USB
battery pack to top off an internal LiPo battery. Or just swap the LiPo battery when you're swapping the microSD card each month. 

One thing the adalogger board doesn't have is a good real time clock, or RTC. The M0 processor provides an RTC and the software is able to use it as a backup
time source, but any information about the current date and time is lost 
whenever power is lost or the reset button is pressed. Fortunately, there are plug in feather wings that adds a battery backed up RTC. I've used one based on
the DS3231 chip, available at https://www.adafruit.com/product/3028 Plug this wing into the adalogger feather, and it keeps excellent track of date and times 
across battery changes, restarts, and so forth. 

## Software Platform

The ThriftyCounter software can be found in ThriftyCounter.ino in this repository.


## Sensor

See [Sensor Overview](SensorOverview.md) for more information on sensors.

I used RCWL-0516 microwave sensors when using this trail counter. These sensors can be powered directly from the board (using the GND and 3.3V pins 
and the output logic signal can be tied to one of the input pins (I used DIO 11 in the softwear). This sensor works best when located a couple inches 
away from the feather boards and battery, with no metal or electronics directly in front or behind the antenna area on the sensor board.

The data sheets for the RCWL-0516 say that the board requires an input voltage between 4 and 28v, but that input is regulated to 3.3 volts to run board and the
samples that I've used seem totally happy running with the nice, clean, regulated 3.3v source that the adalogger feather can provide.

## Using the ThriftyCounter

This counter has a very minimal user interface that leverages the green and red LEDs on this board on the controller board.

Upon startup, either by inserting a battery or pressing the restart button, the software boots up and attempts to write to the
microSD card. Red LED flashes indicate an issue during startup:

-  2 blinks = error, no RTC clock found
-  3 blinks = error, no microSD card. Insert a FAT32 formatted card to clear condition
-  4 blinks = error, unable to write to a microSD card

After startup, the green LED on the feather is lit whenever the detection sensor is high. The red LED flashes once each time an event
is written to the SD card. Because writing to the SD card is energy intensive, several events may be buffered and written together. 

To harvest data, simply replace the microSD card with a new, preformatted card and press the restart button. The SD card should contain a .csv file
named "CYYMMDD".csv, where YYMMDD reflect the date that the counter was started. Each entry in the csv file will contain fields for the timestamp of the event, the date of the event, the time of the event, the duration of the event, and an event count. 

There's no provision for setting the date and time on the counter. Date and time are updated when the software is downloaded into the controller. The clock should keep accurate time throughout the life of the clock coin battery cell, which should be at least a couple years. At that point, you'll have to change the clock battery and re-load the software again. 


