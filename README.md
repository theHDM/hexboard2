Notes from my phone as of Nov 7 2024

```
DONE:
designate core 2 as the background operation core

DONE: KEYBOARD.h
create one global object, takes in the grid of input pins, outputs an array of read statuses
can change whether to loop by mux or by col
mutex = 0 during read phase, 1 during write phase
digital or analog read -> update mux/col
pre-map the pins to an output order
you shouldn't need to reverse the map

BASICALLY DONE: ROTARY.h
create one object -- 3 pins, 2 outputs
MUTEX = 0 when running the update function, 1 otherwise
overload read() to pop a turn or click from the queue

AUDIO.h
create one object per audio source, with multiple pin outs
use circle bugger and mutex pointers when needed

PIXEL.h
poll() to send pixel data each frame (60fps, or 16384uS per frame)
do all color coversions before polling -- should only send the buffer.

consider running the OLED / menu on core 1 because it's blocky.

struct hex_coordinate {int q, int r, int s};
make an ordered map of pixel ID to coordinate and back
can look up "coordinates of pixel" or "pixel at this coordinate"

struct instruction {
  enum {note on, note off, control change, program change, palette swap, preset swap, etc.}
  int some sort of parameter
}

class oneSingleHex {
  friend keyboard_obj (allow inheritance)
  current pressure value {0 .. 127}
  prior pressure value {0 .. 127}
  instructions for:           pressure: curr  prior
      key_stays_off                       0  =  0
      key_turns_on                        0  >  1+
      key_stays_on_no_pressure_change     1+ =  1+
      key_stays_on_pressure_increase      1+ >  2+
      key_stays_on_pressure_decrease      2+ >  1+
      key_turns_off                       1+ >  0
}

struct noteValue {
  float MIDInoteValueWithMicrotones [0.0 .. 128.0>
  pair of int convertToPlainMIDINotePlusPitchBend(int pitchBendRange)
  (i.e. convert 69.5 (2 semitones) = 69 + 2048, 69.75 = 70 - 1024)
  (i.e. convert 69.25 (48 semitones) = 69 + 43)
  float associatedFrequencyForSynth {2^MIDI-69 * 440}
}
  
struct noteToHexagonPairOrMap {noteValue N, hex_coordinate H}

class generator {
  textStream KBMdata -- this will give the info needed to:
    -- set scale lock true/falses
    -- set anchor note
    -- size of period
  textStream scalaData -- this will give the info needed to:
    -- calculate the notes in each key
  int steps in each direction
    2 dimensions: Q, R
    3 dimensions: X, Y, Z
  dimensions...  2=  00  01  02  03
                       10  11  12  13
                         20  21  22  23
                 3= A00 A01 A02 A03
                      B00 B01 B02 B03
                    A10 A11 A12 A13
                      B10 B11 B12 B13
  color pattern {palette select for each scale step}
    used if user selects "color by number" instead of "color by tuning" or "rainbow"
  lattice maker function to calculate the layout
}
/*
https://github.com/MarkCWirt/libscala-file
#include "scala_file.hpp"
// Get an open file pointer
ifstream test_scale;
test_scale.open("scale.scl");
// Pass it to read_scl to get a scale
// or pointer to the text stream as a class object??
scala::scale loaded_scale = scala::read_scl(test_scale);
ifstream test_scale;
test_scale.open("scales/tet-12.scl");
scala::scale scale = scala::read_scl(test_scale);
for (int i = 0; i < scale.get_scale_length(); i++ ){
  cout << "Degree:  "<< i << " Ratio: " << scale.get_ratio(i) << endl;
};
#include "scala_file.hpp"
ifstream test_kbm;
test_kbm.open("kbm/12-tet.kbm");
scala::kbm loaded_kbm = scala::read_kbm(test_kbm);
*/


class musicalLayout {
  generator select_the_generator_this_is_based_on
  std::vector<noteToHexagonPair> list of manual overrides
}

class keyboard_obj {
  make a key sensitivity map / function
  i.e. LERP along some set of thresholds based on hall effect sensor testing:
  analog read [uint16]  0 .. 4096 .. 32768 .. 49152 .. 65535 
  pressure    [uint8]   0 .. 0    .. 64    .. 128   .. 128
  current key state = vector<int> from the KEYBOARD driver
  prior key state = vector<int> from the KEYBOARD driver
  void update() -- convert key states into pressure values and pass to each hex button object
  std::vector<oneSingleHex> of 160 hexagon states
}
  
class gradient {
  this will be a method to store or generate gradients.
  three generators:
  "shades/tints of a color" "hot/cold gradient" "rainbow cycle"
  shade_tint(HSV base)      gradient(pair of HSV and point on line)
  rainbow(constant SV, constant SL, etc.)
}

class reactiveColoring {
  instructions to lookup along a gradient based on parameter
  i.e.
  reactToKeyPress(off = 0, dim = 1/4, on = 1/2, pressure = [1/2..1], animate = 1)
  reactToValue(&value, map low, map high)
  passiveCycle(speed)
}

struct pixelDefinition {
  gradient,
  colorRule
}

SETTINGS
use littleFS to store menu-related options and active layout (i.e. 1 thru 10)
use C++ headers to let the user assign layouts to preset numbers i.e. 1 thru 10

IDEAS FOR OFFLINE WEB APPS
something to connect sevish scale workshop and terpstra
e.g. create a scala and KBM in sevish webpage
load it into this app
designate your base color palette and your layout steps
preview it in terpstra
export it to a CPP header
```
