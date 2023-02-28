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

- Place a sensor in a narrow construction, as when a trail passes between two large trees or on a narrow bridge where traffic is naturally single file 
- Locate sensors after trail speed calming features, such as tight corners, narrow chicanes, or at the top of a steep climb to minimize issues with rapidly moving users
- Aim sensors at the torso of the typical trail user, not at head height or leg height

It may be possible to differentiate trail users based on sensor choice and positioning. For example, an active infrared sensor 8' off the ground would 
likely be tripped by an equestrian, but not a biker or hiker. An inductive loop would probably detect a bike, and maybe horseshoes, but not a hiker. 

## Active Infrared

to be written

## Passive infrared (PIR)

to be written

## Microwave

Microwave sensors work by broadcasting a low power microwave signal and listening for an echo. That signal is reflected strongly by water, and by 
happy coincidence humans are mostly water. Any change in the dopler shift of the received echo denotes movement, and movement and echo strength over a
certain level can be used to determine the presence of a nearby user. Sounds complicated, but all this has been figured out and bundled up in convenient 
inexpensive packages ready for use.

### Characteristice of microwave sensors

- Microwaves can pass through some solid materials such as wood. This means that a trail counter could be embedded inside a 4x4 wooden signpost next to a trail 
or located on the underside of a bridge plank and be complerely clandestine. It cannot be enclosed in a solid metal enclosure. 
- Microwaves seem less effective during rainstorms

### The RCWL-0516 package

This is a powerful yet inexpensive sensor, but a little hard to tune and senstive to surroundings. Tuning trigger and retrigger times means soldering SMD 
components. Still, it's my go-to for a microwave sensor.



This modules are available at https://www.amazon.com/gp/product/B07YYY7J7D

There's alot of tuning and usage information at https://github.com/jdesbonnet/RCWL-0516 - see especially the issues discussions.

### The HFS-DC06F package

This is a really nice, stable sensor package with easily adjustable range and retrigger time settings. The output signal can be used directly as an 
input to an Arduino Uno counter. The only downside to this package is that it's a bit of a power pig, requiring 30ma at 5 volts in operation. Given 
that, it does work well as a detection sensor. 



These modules are currently available at https://www.amazon.com/gp/product/B08TMQ36H6 or better prices if you shop around.


## Inductive Loop

to be written

## Ultrasound

to be written

## Pneumatic

to be written


