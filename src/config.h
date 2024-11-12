#pragma once
#include "syntacticSugar.h"

// Hardware pin constants
// If you rewire the HexBoard then
// change these pin values as needed

const int_vec muxPins = {4,5,2,3};
const int_vec colPins = {6,7,8,9,10,11,12,13,14,15};
const int OLED_sdaPin = 16;
const int OLED_sclPin = 17;
const int rotaryPinA = 20;
const int rotaryPinB = 21;
const int ledPin = 22;
const int rotaryPinC = 24;
// const int_vec pwmPins = {23,25}; // audio must use PWM / digital write on all other pins
const int_vec pwmPins = {25};


// The particular neoPixel LEDs you use are
// probably going to be the same length of 140.
// But if that ever changes, you can put that here.
// Note that number of buttons might not equal
// the number of pixels, so it is not set to be
// the product of rows and columns. I mean, you never know.

const int pixelCount = 140;


// These are timing constants that are hardware dependent.
// If you use different components you may need to change
// these timings if there is unexpected behavior.

const int rotary_pin_fire_period_in_uS    =    768;
const int keyboard_pin_reset_period_in_uS =     16;
const int target_audio_sample_rate_in_Hz  = 31'250;
const int target_LED_frame_rate_in_Hz     =     30;

// This constant is a lookup table to match the keys to the strip of LED pixels
// If the wiring of the keyboard or the direction of the LED strips change
// then you'll want to update this table.

const int_matrix map_grid_to_button = {
	// MUX:
  // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F     COL: 
  {  0, 10, 20, 30, 40, 50, 60, 70, 80, 90,100,110,120,130,140,150}, // 0
  {  1, 11, 21, 31, 41, 51, 61, 71, 81, 91,101,111,121,131,141,151}, // 1
  {  2, 12, 22, 32, 42, 52, 62, 72, 82, 92,102,112,122,132,142,152}, // 2
  {  3, 13, 23, 33, 43, 53, 63, 73, 83, 93,103,113,123,133,143,153}, // 3
  {  4, 14, 24, 34, 44, 54, 64, 74, 84, 94,104,114,124,134,144,154}, // 4
  {  5, 15, 25, 35, 45, 55, 65, 75, 85, 95,105,115,125,135,145,155}, // 5
  {  6, 16, 26, 36, 46, 56, 66, 76, 86, 96,106,116,126,136,146,156}, // 6
  {  7, 17, 27, 37, 47, 57, 67, 77, 87, 97,107,117,127,137,147,157}, // 7
  {  8, 18, 28, 38, 48, 58, 68, 78, 88, 98,108,118,128,138,148,158}, // 8
  {  9, 19, 29, 39, 49, 59, 69, 79, 89, 99,109,119,129,139,149,159}  // 9
};

// Sometimes the keyboard sends a meaningful signal on a pin combination
// but it is not linked to a pixel. Might actually need a different map then.
// Probably will be equal to pixel by chance, but actually it will be more like

const int_matrix mapGridToPixel = {
	// MUX:
  // 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F     COL: 
  {  0, 10, 20, 30, 40, 50, 60, 70, 80, 90,100,110,120,130,140,150}, // 0
  {  1, 11, 21, 31, 41, 51, 61, 71, 81, 91,101,111,121,131,141,151}, // 1
  {  2, 12, 22, 32, 42, 52, 62, 72, 82, 92,102,112,122,132,142,152}, // 2
  {  3, 13, 23, 33, 43, 53, 63, 73, 83, 93,103,113,123,133,143,153}, // 3
  {  4, 14, 24, 34, 44, 54, 64, 74, 84, 94,104,114,124,134,144,154}, // 4
  {  5, 15, 25, 35, 45, 55, 65, 75, 85, 95,105,115,125,135,145,155}, // 5
  {  6, 16, 26, 36, 46, 56, 66, 76, 86, 96,106,116,126,136,146,156}, // 6
  {  7, 17, 27, 37, 47, 57, 67, 77, 87, 97,107,117,127,137,147,157}, // 7
  {  8, 18, 28, 38, 48, 58, 68, 78, 88, 98,108,118,128,138,148,158}, // 8
  {  9, 19, 29, 39, 49, 59, 69, 79, 89, 99,109,119,129,139,149,159}  // 9
};

// This is the physical coordinate location of each pixel.
// It matters for note layout and animation purposes.

/*
const std::vector<hexCoord> mapPixelToCoord = {
  //   0       1       2       3       4       5       6       7       8       9 
  {-5, 0},{-1,-6},{ 0,-6},{ 1,-6},{ 2,-6},{ 3,-6},{ 4,-6},{ 5,-6},{ 6,-6},{ 7,-6},     // +  0
      {-2,-5},{-1,-5},{ 0,-5},{ 1,-5},{ 2,-5},{ 3,-5},{ 4,-5},{ 5,-5},{ 6,-5},{ 7,-5}, // + 10
  {-6, 1},{-2,-4},{-1,-4},{ 0,-4},{ 1,-4},{ 2,-4},{ 3,-4},{ 4,-4},{ 5,-4},{ 6,-4},     // + 20
      {-3,-3},{-2,-3},{-1,-3},{ 0,-3},{ 1,-3},{ 2,-3},{ 3,-3},{ 4,-3},{ 5,-3},{ 6,-3}, // + 30
  {-6, 2},{-3,-2},{-2,-2},{-1,-2},{ 0,-2},{ 1,-2},{ 2,-2},{ 3,-2},{ 4,-2},{ 5,-2},     // + 40
      {-4,-1},{-3,-1},{-2,-1},{-1,-1},{ 0,-1},{ 1,-1},{ 2,-1},{ 3,-1},{ 4,-1},{ 5,-1}, // + 50
  {-7, 3},{-4, 0},{-3, 0},{-2, 0},{-1, 0},{ 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},{ 4, 0},     // + 60
      {-5, 1},{-4, 1},{-3, 1},{-2, 1},{-1, 1},{ 0, 1},{ 1, 1},{ 2, 1},{ 3, 1},{ 4, 1}, // + 70
  {-7, 4},{-5, 2},{-4, 2},{-3, 2},{-2, 2},{-1, 2},{ 0, 2},{ 1, 2},{ 2, 2},{ 3, 2},     // + 80
      {-6, 3},{-5, 3},{-4, 3},{-3, 3},{-2, 3},{-1, 3},{ 0, 3},{ 1, 3},{ 2, 3},{ 3, 3}, // + 90
  {-8, 5},{-6, 4},{-5, 4},{-4, 4},{-3, 4},{-2, 4},{-1, 4},{ 0, 4},{ 1, 4},{ 2, 4},     // +100
      {-7, 5},{-6, 5},{-5, 5},{-4, 5},{-3, 5},{-2, 5},{-1, 5},{ 0, 5},{ 1, 5},{ 2, 5}, // +110
  {-8, 6},{-7, 6},{-6, 6},{-5, 6},{-4, 6},{-3, 6},{-2, 6},{-1, 6},{ 0, 6},{ 1, 6},     // +120
      {-8, 7},{-7, 7},{-6, 7},{-5, 7},{-4, 7},{-3, 7},{-2, 7},{-1, 7},{ 0, 7},{ 1, 7}  // +130
};
*/

/*
  Note names and palette arrays are allocated in memory
  at runtime. Their usable size is based on the number
  of steps (in standard tuning, semitones) in a tuning 
  system before a new period is reached (in standard
  tuning, the octave). This value provides a maximum
  array size that handles almost all useful tunings
  without wasting much space.
  In the future this will not be necessary since
  we'll just store things as vectors.
*/
#define MAX_SCALE_DIVISIONS 72
