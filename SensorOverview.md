# Sensor Types

Sensor type and placement are the keys to accurate and correct counting of trail users. Commonly used sensors
are:
- Infrared
- Seismic
- Microwave
- Inductive Loop
- Ultrasound
- Pneumatic

Each has their advantages and disadvantages, but if you can obtain a logic level signal from a sensor when a trail user passes you can 
interface to it and count it.

It should be noted that none of these sensors is totally accurate. A sensor may not be able to distinguish between one hiker and two walking abreast. 
It may not be able to accurately count mountain bikers riding fast in a close cluster, as during a race event. Some sensors may be sensitive to 
the types of clothing being worn. A poorly placed sensor may count individual legs for footfalls. For these reasons, it's best to locate sensors in 
locations that minimize these potential errors:

- Place a sensor in a narrow constriction, as when a trail passes between two large trees or on a narrow bridge where traffic is naturally single file 
- Locate sensors after trail speed calming features, such as tight corners, narrow chicanes, or at the top of a steep climb to minimize issues with rapidly moving users
- Aim sensors at the torso of the typical trail user, not at head height or leg height

It may be possible to differentiate trail users based on sensor choice and positioning. For example, an active infrared sensor 8' off the ground would 
likely be tripped by an equestrian, but not a biker or hiker. An inductive loop would probably detect a bike, and maybe horseshoes, but not a hiker. 

When choosing a sensor strategy, one of the most important considerations will be power consumption. There are a number of really fine, accurate human presence
sensors but some of their power requirements make them really only suitable for line power applications or when large capacity batteries are employed.

## Active Infrared

to be written

## Passive infrared (PIR)

Passive Infrared sensors detect changes in received infrared radiation when a warm body, like a human or a horse, passes by the sensor. Since infrared 
radiation is light, these sensors can use lenses to control the range and detection area of a sensor.

These sensors are typically packaged as a group of one or more sensors and electronics that compares changes in the IR light detected by these sensors. Changes
are interpreted as motion and a digital logic output pulse is generated.

### Characteristics of PIR Sensors

- Because these sensors are detecting light, they must physically be open to the environment. Any environmental shielding must be transparent to IR radiation
- Many of the PIR sensors are wide field, intended for detecting movement within a room for instance. This is a consideration when siting, as we want to
detect only a single user at a time.
- PIR sensors can sometimes detect swaying branches, so try to site in an area away from shrubbery or low hanging branches that may be impacted by wind.
- PIR sensors can be overwhelmed by bright sunlight, and so try to site in an area with consistent shade. 
- Since PIR sensors do not requre emitting light or radiation, they can be much more energy efficient than active IR or microwave sensors.

### The HC-SR501 PIR Sensor

While there are many PIR sensors, this is a pretty convenient inexpensive package that is readily available. It includes a weatherproof IR transparent
shield (that conveniently fits in a 7/8" hole cut in an enclosure) and has potentiometers to control sensitivity and retrigger delay times. It's power efficient, requiring only about 50 ua and happily runs on either 5v or 3.3v sources so it's easy to power.

![sensor](/assets/images/HC-SR501.jpg)

I've found that setting the jumper for repeat triggering, the sensitivity to minimum, and the time delay to a minimum value yields the best results. Even then, the minimum time delay value may be too long for fast traffic. Still investigating. 

You can drill a 7.8 inch hole in a waterproof plastic enclosure so that the sensor lens is exposed to IR radiation while protecting the electronics and battery.

![sensor](/assets/images/HC-SR501-settings.jpg)

HC-SR501 sensors are available here: https://www.amazon.com/DIYmall-HC-SR501-Motion-Infrared-Arduino/dp/B012ZZ4LPM and at many other sources.

## Microwave

Microwave sensors work by broadcasting a low power microwave signal and listening for an echo. That signal is reflected strongly by water, and by 
happy coincidence humans are mostly water. Any change in the dopler shift of the received echo denotes movement, and movement and echo strength over a
certain level can be used to determine the presence of a nearby user. Sounds complicated, but all this has been figured out and bundled up in convenient 
inexpensive packages ready for use.

### Characteristice of microwave sensors

- Microwaves can pass through some solid materials such as wood. This means that a trail counter could be embedded inside a 4x4 wooden signpost next to a trail 
or located on the underside of a bridge plank and be complerely clandestine.
- Microwave sensors are affected by nearby metal, and so cannot be enclosed in a metal vandal resistent case or attached to a metal fencepost, for instance. 
- Microwaves seem less effective during rainstorms
- Microwave sensor operation is unaffected by bright sunlight, moving shadows, the darkness of night, or headlights on bikes.

### The RCWL-0516 sensor

This is a powerful yet inexpensive sensor, but a little hard to tune and senstive to surroundings. Tuning trigger and retrigger times means soldering SMD 
components. Still, it's my go-to for a microwave sensor. It's power efficient, requiring only about 1.2 ma and happily runs on either 5v or 3.3v sources so it's easy to power up in the field with batteries or solar. The output signal can be used directly as a logic input to a trail counter. 

![sensor](/assets/images/RCWL-0516.jpg)

This modules are available at https://www.amazon.com/gp/product/B07YYY7J7D

There's alot of tuning and usage information at https://github.com/jdesbonnet/RCWL-0516 - see especially the issues discussions.

By default, these sensors have about an active high trigger time of 2 seconds and a low retrigger time of 2 seconds between detection events. This isn't optimal for detecting individual riders in a group of mountain bikers, so I generally modify these boards to lessen that. 

![sensor](/assets/images/RCWL-0516Mods.jpg)

You can replace C2 (in red) with a smaller capacitor for a narrower trigger pulse time and you can replace C3 (in blue) with a smaller capacitor for a shorter retrigger time between pulses. Replacing both with .001 uf results in a minimum output pulse duration of about 300 ms and a minimum time between pulses of about 300 ms which works pretty well differentiating fast moving bike traffic.

### The HFS-DC06F sensor

This is a really nice, stable sensor package with easily adjustable range and trigger time settings. The output signal can be used directly as an 
input to an Arduino Uno counter. The only downside to this package is that it's a bit of a power pig, requiring 30ma at 5 volts in operation. Given 
that, it does work well as a detection sensor if you turn down the range and trigger time settings to their minimal values. This will result in a workable range of about 6 feet and about 2 seconds of high signal for each detection event. This is fine for hikers and relatively slow low volume bike traffic where users are 4 seconds apart, but may undercount if they are closer together than that.

![sensor](/assets/images/HFS-DC06F.jpg)

These modules are currently available at https://www.amazon.com/gp/product/B08TMQ36H6 or shop around.


## Inductive Loop

to be written

## Ultrasound

to be written

## Pneumatic

to be written


