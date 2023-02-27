# UnoCounter

UnoCounter is a simple an Arduino Uno based trail counter that records each trail user detection event as an entry in a .CSV file on 
an SD card that can be collected and imported into a spreadsheet. It's the first trail counter that I built and deployed to validate the
core project principle: using inexpensive off the shelf hardware components to build a workable counter. And it worked - you can build
this counter by simply plugging things together. 

![UnoCounter assembled](/assets/images/UnoCounterAssembled.JPG)

## Hardware Platform

Ardunino Uno is a ubiquitous platform that is available from many different suppliers and supported by lots of STEM educational kits,
so it makes a good platform for getting started with this technology since you can ask any kid in your local school's robotics club 
for help if you get into trouble. Details on all this can be found at
https://www.arduino.cc/ and there are many really good youtube videos to peruse.

UnoCounter is based on a standard Arduino Uno R3 board and the Arduino IDE development environment. The Uno board contains the microcontroller 
and is where all the software needed to create a trail counter runs. Several companies sell clones of this board or you can buy them on the
arduino site. The official Arduino boards may cost a few dollars more, but helps to support the whole open source infrastructure around this 
technology.

The Arduino Uno ecosystem is populated by a host of supporting "shield" components that can be plugged into, or "stacked" on top of an 
Uno board. One of these shields provides nearly everything we need to support a trail counter: the Adafruit Data Logger shield for Arduino, 
available at https://www.adafruit.com/product/1141  

The Data Logger shield adds two key capabilities to the counter: a Real Time Clock to keep track of the date and time, and a SD card interface
that is used to record events to an SD card which can then be harvested and read by a laptop or computer.

In addition, the UnoCounter software supports an additional shield: an LCD Display Shield, available at https://www.adafruit.com/product/772
If present, this shield adds a handy user interface for testing and verifying your sensor  operation and setup, but really doesn't add any
value to the core mission - logging user detection events - and can be omitted. 

That's it, aside from a battery and a detection sensor. 

![UnoCounter Boards](/assets/images/UnoCounterBoards.jpg)

Internally, Arduino Uno boards operate on a 5V supply. You can use any battery capable of proving between 5 and 12 volts to power the stack, 
but the simple linear voltage regulators on these boards aren't terribly efficient and you'll get the best battery life at 5 volts. I've had 
best results with arrays of 4 1.2 volt rechargable NiMH cells. 

## Software Platform

The UnoCounter software can be found in UnoCounter.ino in this repository. 





