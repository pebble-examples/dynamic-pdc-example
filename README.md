# dynamic-pdc-example

Example showing how to morph a vector image to a square (and back) for 
transitions.

![](screenshots/transition.gif)

This example uses an animation to make a callback to a series of functions. 
These functions use the normalized value of the animation to update the 
GDrawCommandImage. calculate_normalized_value is the function responsible for 
the math. The function determines (on one axis) which bound (0 or bounds) the 
point is closest to. It then returns where on the path the point should be 
(determined by normalized, the progress through the animation).

## Docs

[Animations](http://developer.getpebble.com/guides/pebble-apps/display-and-animations/property-animations/)

## Tools

The 
[SVG2PDC](https://github.com/pebble-examples/cards-example/blob/master/tools/svg2pdc.py) 
script converts SVG images to a PDC (Pebble Draw Command) binary format image or 
sequence.

#### Usage

First install all the python dependencies

```sh
pip install -r requirements.txt
```

Then use the [SVG2PDC](./tools/svg2pdc.py) tool to convert an SVG to PDC. Note that this will generate 
the PDC in the same directory as the source file (SVG).

```sh
python tools/svg2pdc.py resources/Pebble_50x50_Generic_weather.svg
```