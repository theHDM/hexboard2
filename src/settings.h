#pragma once
#include "utils.h"

struct settings_t {
  int settingValueA;

  void pack();
  void unpack();
};


int transposeSteps = 0;
int scaleLock = 0;
int perceptual = 1; // to deprecate
int paletteBeginsAtKeyCenter = 1;
int animationFPS = 32; // actually frames per 2^20 microseconds. close enough to 30fps; to deprecate
int wheelMode = 0; // standard vs. fine tune mode
int modSticky = 0;
int pbSticky = 0;
int velSticky = 1;
int modWheelSpeed = 8;
int pbWheelSpeed = 1024;
int velWheelSpeed = 8;
bool rotaryInvert = false;
bool screenSaverOn = 0;      
timeStamp screenSaverTimeout = (1u << 23); // 2^23 microseconds ~ 8 seconds

#define RAINBOW_MODE 0
#define TIERED_COLOR_MODE 1
#define ALTERNATE_COLOR_MODE 2
int colorMode = RAINBOW_MODE;

#define ANIMATE_NONE 0
#define ANIMATE_STAR 1 
#define ANIMATE_SPLASH 2 
#define ANIMATE_ORBIT 3 
#define ANIMATE_OCTAVE 4 
#define ANIMATE_BY_NOTE 5
int animationType = ANIMATE_NONE;

#define BRIGHT_MAX 255
#define BRIGHT_HIGH 210
#define BRIGHT_MID 180
#define BRIGHT_LOW 150
#define BRIGHT_DIM 110
#define BRIGHT_DIMMER 70
#define BRIGHT_OFF 0
int globalBrightness = BRIGHT_MID;
