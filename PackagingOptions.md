# Packaging Options

Here are several alternatives I've used for packaging a counter:

- ZipLoc Bag
- ABS Waterproof Box
- 3d Printed Enclosures

## ZipLoc Bag

The simpliest option when deploying a trail counter is to tuck it in a simple ZipLoc bag. These are waterproof, low cost, and easy to locate in small, odd areas. 
Bags are great for quickly deploying a prototype out on the trail. 

![ziploc](/assets/images/ziploc.jpg)

On the downside, they aren't particularly robust, are vulnerable to animals, and may be mistaken as rubbish. Some sensors are very sensitive to their surroundings
and may be affected by batteries and controller locations, and that's hard to control in a bag. 

## ABS Waterproof Box

Commercial ABS waterproof junction boxes are solid and robust and available in many sizes and formats. Here's one that I've used.

![case](/assets/images/exposurecase.jpg)

This is relatively small at 6"x4"x3" but sill big enough for an Arduino Uno and is available at https://www.amazon.com/Zulkit-Waterproof-Electrical-Transparent-150x100x70/dp/B07RPNWD47

Being ABS, it's easy to use styrene solvent to glue plastic standoffs wherever needed to mount a circuit board.

![case](/assets/images/exposurecasesensor.jpg)
![case](/assets/images/exposurecaseclosed.jpg)

If you have a transparent cover for the box, boards can be positioned so that LEDs are easily visible to confirm operation after positioning.

If you're using a microwave sensor that can sense presence through wood, one of my favorite placements is on a support beam under a bridge. Narrow bridges encourage
users to pass single file and at relatively slow speeds and help to ensure accurate counts.

![case](/assets/images/exposurecasebridge.jpg)


## 3d Printed Enclosures

### PostIt Frame

The PostIt frame was designed to conceal a feather based trail counter within a 2 inch diameter, 8 inch deep hole bored into a wooden post. 4x4 posts are common
sights along a trail, often festooned with decals and pointers, and are not obvious targets for anyone looking for a trail counter. This design relies on the 
post for environmental protection, and can only be used with sensors that can detect presence through wood (like a microwave sensor). 

![case](/assets/images/FeatherComponentFrameDesign.jpg)

The basic design is a cylinder with space for a 4400 mah LiPo battery at the bottom, a stack of two feather boards, and space for a sensor at the top. It's open to allow access 
to the front and back of a feather stack to replace the battery, plug in a USB cord, and swap microSD cards without removing any boards from the frame. There's a 
recess for a rubber band to hold the battery at the bottom, and a finger hold at the top to ease extraction from a post. (Or you can tie a lanyard to it so it's easier to retrieve from a deeper bore.) The cylinder is 1.75 inches in diameter which provides lots of clearance in a 2 inch bore in a post. 

This frame can hold components for either the ThrifyCounter or NiftyCounter. Here's what it looks like with NiftyCounter components mounted:

![case](/assets/images/FeatherComponentFramePrototype.jpg)

The STL file for the PostIt frame can be found in the repository at PostIt.stl. When 3d printing, it's easiest to print in two sections, front and back, to eliminate
issues with horizontal spans. These sections can either be glued together or assembled using 2.5 mm machine screws. You can find a pre-split version at PostItSplit.stl. 

The model has tap holes for a RCWL-0516 microwave sensor (mount with 1.4 mm machine screws) and tap holes for feather boards (mount with 2.5 mm machine screws). You
might need to tweak this somewhat when using different sensors.


## Tagging

Regardless of what package you use, it's good practice to include your name (or your organization's name) and a contact phone number in the package. If the 
package is misplaced, this information may help it find its way home. If it's located by a suspicious individual, a phone call can help the bomb squad determine that 
it's a LiPo battery and not C4. 

