#pragma once
#include "syntacticSugar.h"
#include "config.h"
#include "config.h"

// @palettes
/*
  This section defines the code needed
  to determine colors for each hex.
*/  
/*
  LED colors are defined in the code
  on a perceptual basis. Instead of 
  calculating RGB codes, the program
  uses an artist's color wheel approach.
  
  For value / brightness, two sets of
  named constants are defined. The BRIGHT_
  series (see the defaults section above)
  corresponds to the overall
  level of lights from the HexBoard, from
  dim to maximum. The VALUE_ series
  is used to differentiate light and dark
  colors in a palette. The BRIGHT and VALUE
  are multiplied together (and normalized)
  to get the output brightness.
*/
#define VALUE_BLACK 0
#define VALUE_LOW   127
#define VALUE_SHADE 164
#define VALUE_NORMAL 180
#define VALUE_FULL  255
/*
  Saturation is zero for black and white, and 255
  for fully chromatic color. Value is the
  brightness level of the LED, from 0 = off
  to 255 = max.
*/
#define SAT_BW 0
#define SAT_TINT 32
#define SAT_DULL 85
#define SAT_MODERATE 120
#define SAT_VIVID 255
/*
  Hues are angles from 0 to 360, starting
  at red and towards yellow->green->blue
  when the hue angle increases. 
*/
#define HUE_NONE 0.0
#define HUE_RED 0.0
#define HUE_ORANGE 36.0
#define HUE_YELLOW 72.0
#define HUE_LIME 108.0
#define HUE_GREEN 144.0
#define HUE_CYAN 180.0
#define HUE_BLUE 216.0
#define HUE_INDIGO 252.0
#define HUE_PURPLE 288.0
#define HUE_MAGENTA 324.0
/*
  This class is a basic hue, saturation,
  and value triplet, with some limited
  transformation functions. Rather than
  load a full color space library, this
  program uses non-class procedures to
  perform conversions to and from LED-
  friendly color codes.
*/
class colorDef {
public:
  float hue;
  int sat;
  int val;
  colorDef tint() {
    colorDef temp;
    temp.hue = this->hue;
    temp.sat = ((this->sat > SAT_MODERATE) ? SAT_MODERATE : this->sat);
    temp.val = VALUE_FULL;
    return temp;
  }
  colorDef shade() {
    colorDef temp;
    temp.hue = this->hue;
    temp.sat = ((this->sat > SAT_DULL) ? SAT_DULL : this->sat);
    temp.val = VALUE_LOW;
    return temp;
  }
};
/*
  This class defines a palette, which is
  a map of musical scale degrees to
  colors. A palette is tied to a specific
  tuning but not to a specific layout.
*/
class paletteDef {
public:
  colorDef swatch[MAX_SCALE_DIVISIONS]; // the different colors used in this palette
  int colorNum[MAX_SCALE_DIVISIONS];   // map key (c,d...) to swatches
  colorDef getColor(int givenStepFromC) {
    return swatch[colorNum[givenStepFromC] - 1];
  }
  float getHue(int givenStepFromC) {
    return getColor(givenStepFromC).hue;
  }
  int getSat(int givenStepFromC) {
    return getColor(givenStepFromC).sat;
  }
  int getVal(int givenStepFromC) {
    return getColor(givenStepFromC).val;
  }
};
/*
  Palettes are defined by creating
  a set of colors, and then making
  an array of numbers that map the
  intervals of that tuning to the
  chosen colors. It's like paint
  by numbers! Note that the indexes
  start with 1, because the arrays are
  padded with 0 for entries after
  those intialized.
*/
paletteDef palette[] = {
  // 12 EDO
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL}
    , {HUE_BLUE,    SAT_DULL,  VALUE_SHADE }
    , {HUE_CYAN,    SAT_DULL,  VALUE_NORMAL}
    , {HUE_INDIGO,  SAT_VIVID, VALUE_NORMAL}
    },{1,2,1,2,1,3,4,3,4,3,4,3}},
  // 17 EDO
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL}
    , {HUE_INDIGO,  SAT_VIVID, VALUE_NORMAL}
    , {HUE_RED,     SAT_VIVID, VALUE_NORMAL}
    },{1,2,3,1,2,3,1,1,2,3,1,2,3,1,2,3,1}},
  // 19 EDO
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_YELLOW,  SAT_VIVID, VALUE_NORMAL} //  #
    , {HUE_BLUE,    SAT_VIVID, VALUE_NORMAL} //  b
    , {HUE_MAGENTA, SAT_VIVID, VALUE_NORMAL} // enh
    },{1,2,3,1,2,3,1,4,1,2,3,1,2,3,1,2,3,1,4}},
  // 22 EDO
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_BLUE,    SAT_VIVID, VALUE_NORMAL} // ^
    , {HUE_MAGENTA, SAT_VIVID, VALUE_NORMAL} // mid
    , {HUE_YELLOW,  SAT_VIVID, VALUE_NORMAL} // v
    },{1,2,3,4,1,2,3,4,1,1,2,3,4,1,2,3,4,1,2,3,4,1}},
  // 24 EDO
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_LIME,    SAT_DULL,  VALUE_SHADE } //  +
    , {HUE_CYAN,    SAT_VIVID, VALUE_NORMAL} //  #/b  
    , {HUE_INDIGO,  SAT_DULL,  VALUE_SHADE } //  d
    , {HUE_CYAN,    SAT_DULL,  VALUE_SHADE } // enh
    },{1,2,3,4,1,2,3,4,1,5,1,2,3,4,1,2,3,4,1,2,3,4,1,5}},
  // 31 EDO
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_RED,     SAT_DULL,  VALUE_NORMAL} //  +
    , {HUE_YELLOW,  SAT_DULL,  VALUE_SHADE } //  #
    , {HUE_CYAN,    SAT_DULL,  VALUE_SHADE } //  b
    , {HUE_INDIGO,  SAT_DULL,  VALUE_NORMAL} //  d
    , {HUE_RED,     SAT_DULL,  VALUE_SHADE } //  enh E+ Fb
    , {HUE_INDIGO,  SAT_DULL,  VALUE_SHADE } //  enh E# Fd
    },{1,2,3,4,5,1,2,3,4,5,1,6,7,1,2,3,4,5,1,2,3,4,5,1,2,3,4,5,1,6,7}},
  // 41 EDO
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_RED,     SAT_DULL,  VALUE_NORMAL} //  ^
    , {HUE_BLUE,    SAT_VIVID, VALUE_NORMAL} //  +
    , {HUE_CYAN,    SAT_DULL,  VALUE_SHADE } //  b
    , {HUE_GREEN,   SAT_DULL,  VALUE_SHADE } //  #
    , {HUE_MAGENTA, SAT_DULL,  VALUE_NORMAL} //  d
    , {HUE_YELLOW,  SAT_VIVID, VALUE_NORMAL} //  v
    },{1,2,3,4,5,6,7,1,2,3,4,5,6,7,1,2,3,1,2,3,4,5,6,7,
        1,2,3,4,5,6,7,1,2,3,4,5,6,7,1,6,7}},
  // 53 EDO
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_ORANGE,  SAT_VIVID, VALUE_NORMAL} //  ^
    , {HUE_MAGENTA, SAT_DULL,  VALUE_NORMAL} //  L
    , {HUE_INDIGO,  SAT_VIVID, VALUE_NORMAL} // bv
    , {HUE_GREEN,   SAT_VIVID, VALUE_SHADE } // b
    , {HUE_YELLOW,  SAT_VIVID, VALUE_SHADE } // #
    , {HUE_RED,     SAT_VIVID, VALUE_NORMAL} // #^
    , {HUE_PURPLE,  SAT_DULL,  VALUE_NORMAL} //  7
    , {HUE_CYAN,    SAT_VIVID, VALUE_SHADE } //  v
    },{1,2,3,4,5,6,7,8,9,1,2,3,4,5,6,7,8,9,1,2,3,9,1,2,3,4,5,6,7,8,9,
        1,2,3,4,5,6,7,8,9,1,2,3,4,5,6,7,8,9,1,2,3,9}},
  // 72 EDO
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_GREEN,   SAT_DULL,  VALUE_SHADE } // ^
    , {HUE_RED,     SAT_DULL,  VALUE_SHADE } // L
    , {HUE_PURPLE,  SAT_DULL,  VALUE_SHADE } // +/d
    , {HUE_BLUE,    SAT_DULL,  VALUE_SHADE } // 7
    , {HUE_YELLOW,  SAT_DULL,  VALUE_SHADE } // v
    , {HUE_INDIGO,  SAT_VIVID, VALUE_SHADE } // #/b
    },{1,2,3,4,5,6,7,2,3,4,5,6,1,2,3,4,5,6,7,2,3,4,5,6,1,2,3,4,5,6,1,2,3,4,5,6,
        7,2,3,4,5,6,1,2,3,4,5,6,7,2,3,4,5,6,1,2,3,4,5,6,7,2,3,4,5,6,1,2,3,4,5,6}},
  // BOHLEN PIERCE
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL}
    , {HUE_INDIGO,  SAT_VIVID, VALUE_NORMAL}
    , {HUE_RED,     SAT_VIVID, VALUE_NORMAL}
    },{1,2,3,1,2,3,1,1,2,3,1,2,3}},
  // ALPHA
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_YELLOW,  SAT_VIVID, VALUE_NORMAL} // #
    , {HUE_INDIGO,  SAT_VIVID, VALUE_NORMAL} // d
    , {HUE_LIME,    SAT_VIVID, VALUE_NORMAL} // +
    , {HUE_RED,     SAT_VIVID, VALUE_NORMAL} // enharmonic
    , {HUE_CYAN,    SAT_VIVID, VALUE_NORMAL} // b
    },{1,2,3,4,1,2,3,5,6}},
  // BETA
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_INDIGO,  SAT_VIVID, VALUE_NORMAL} // #
    , {HUE_RED,     SAT_VIVID, VALUE_NORMAL} // b
    , {HUE_MAGENTA, SAT_DULL,  VALUE_NORMAL} // enharmonic
    },{1,2,3,1,4,1,2,3,1,2,3}},
  // GAMMA
    {{{HUE_NONE,    SAT_BW,    VALUE_NORMAL} // n
    , {HUE_RED,     SAT_VIVID, VALUE_NORMAL} // b
    , {HUE_BLUE,    SAT_VIVID, VALUE_NORMAL} // #
    , {HUE_YELLOW,  SAT_VIVID, VALUE_NORMAL} // n^
    , {HUE_PURPLE,  SAT_VIVID, VALUE_NORMAL} // b^
    , {HUE_GREEN,   SAT_VIVID, VALUE_NORMAL} // #^
    }, {1,4,2,5,3,6,1,4,1,4,2,5,3,6,1,4,2,5,3,6}},
};
