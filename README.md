# trail-counters

Sooner or later, anyone involved in building hiking or mountain biking trails on public lands is faced with questions about their user community. Whether to justify more trails, improvement in existing trails, apply for grants or funding, understand usage patterns, or just bragging rights, everyone likes objective metrics. How many people are using their trails. When are they using it - are there peak times? Are there favorite loops that see a lot more use than other routes? You built it; did they come? 

Really good commercial trail counter systems are available from several companies. These packages are rugged, tested, and well supported but all that comes with a price. A recent survey found sensor prices ranging from $130 to $995 each, most designed as part of an overall system requiring the purchase of proprietary  hardware and ongoing software subscriptions for data collection and analysis from these sensors. If you've got the budget for this, there are several solid choices in that market. Go for it - read no further.

For the nonprofit trail advocacy groups I work with, an investment of a few thousand dollars to get started has always felt like a lot of money for just data. Stripped to its core, this is a simple task: detecting the presence of a user and recording that event in a time stamped log file. You don't need specialized analysis software. Standard spreadsheet programs are really good at crunching dates and times and making pretty graphs. Once you have event information you can import it and cut and slice it anyway that you'd like for presentation. You do need sensors and data collection hardware, but all the basic building blocks are readily available off the shelf and inexpensive, and that means you can use them in more places. You need to be very careful siting and securing a $500 sensor asset, but you can be a lot more casual about placing a $40 counter. 

That's the theme of this repository: removing the cost barriers by sharing ideas, hardware references, and code to make it easy to create low-cost trail counters for anyone who's interested in trail advocacy.

## Roadmap

The following overview documents might be useful:

- SensorOverview.md - an overview of the types of sensors commonly in use
- PackagingOptions.md - an overview of the ways I've packaged up hardware for field deployments
- HelpfulReferences.md - pointers to docs, videos, and commercial trail counter systems

Along this path, I've built several trail counters to test new ideas and refine the technology. These are:

- UnoCounter - an Arduino UNO based trail counter that records trail usaage event data on an SD card.
- ThriftyCounter - a minimal Adafruit M0 Adalogger feather based counter that records trail usage data on a microSD card. The primary goal was low power usage to minimize battery cost and size for extended unattended operation.
- NiftyCounter - a ThriftyCounter with a user interface based on the Adafruit ESP32-S2 Reverse TFT Feather. In development.



