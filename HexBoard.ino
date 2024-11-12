//  HexBoard TESTING BRANCH
//  Copyright 2022-2023 Jared DeCook and Zach DeCook
//  with help from Nicholas Fox
//  Licensed under the GNU GPL Version 3.
#include <Arduino.h>       // this is necessary to talk to the Hexboard
#include <Wire.h>          // this is necessary to deal with the pins and wires
#include "src/utils.h"     // type definitions, global functions, etc.
#include "src/config.h"    // pin numbers, mappings, etc.
#include "src/hardware.h"  // drivers for rotary / key input and audio output
#include "src/midi.h"      // enables MIDI output
#include "src/synth.h"     // enables PWM audio synthesis
#include "src/control.h"   // associates inputs with hexboard commands
#include "src/layout.h"    // routines to assign look/sound of hexboard
#include "src/presets.h"   // install layout and palette presets
#include "src/settings.h"  // create persistent user settings / prefs

// GEM is a library of code to create menu objects on the B&W display
#include <GEM_u8g2.h>          
U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R2, /* reset=*/ U8X8_PIN_NONE);

#define HARDWARE_UNKNOWN 0
#define HARDWARE_V1_1 1
#define HARDWARE_V1_2 2           // Software-detected hardware revision
byte Hardware_Version = 0;       // 0 = unknown, 1 = v1.1 board. 2 = v1.2 board.

                   



class presetDef { 
  public:
    str presetName; 
    int tuningIndex;     // instead of using pointers, i chose to store index value of each option, to be saved to a .pref or .ini or something
    int layoutIndex;
    int scaleIndex;
    int keyStepsFromA; // what key the scale is in, where zero equals A.
    int transpose;
    // define simple recall functions
    tuningDef tuning() {
      return tuningOptions[tuningIndex];
    }
    layoutDef layout() {
      return layoutOptions[layoutIndex];
    }
    scaleDef scale() {
      return scaleOptions[scaleIndex];
    }
    int layoutsBegin() {
      if (tuningIndex == TUNING_12EDO) {
        return 0;
      } else {
        int temp = 0;
        while (layoutOptions[temp].tuning < tuningIndex) {
          temp++;
        }
        return temp;
      }
    }
    int keyStepsFromC() {
      return tuning().spanCtoA() - keyStepsFromA;
    }
    int pitchRelToA4(int givenStepsFromC) {
      return givenStepsFromC + tuning().spanCtoA() + transpose;
    }
    int keyDegree(int givenStepsFromC) {
      return positiveMod(givenStepsFromC + keyStepsFromC(), tuning().cycleLength);
    }
  };
  presetDef current = {
    "Default",      // name
    TUNING_12EDO,   // tuning
    0,              // default to the first layout, wicki hayden
    0,              // default to using no scale (chromatic)
    -9,             // default to the key of C, which in 12EDO is -9 steps from A.
    0               // default to no transposition
  };
#include "LittleFS.h"       // code to use flash drive space as a file system -- not implemented yet, as of May 2024
void setupFileSystem() {
  Serial.begin(115200);     // Set serial to make uploads work without bootsel button
  LittleFSConfig cfg;       // Configure file system defaults
  cfg.setAutoFormat(true);  // Formats file system if it cannot be mounted.
  LittleFS.setConfig(cfg);
  LittleFS.begin();         // Mounts file system.
  if (!LittleFS.begin()) {
    sendToLog("An Error has occurred while mounting LittleFS");
  } else {
    sendToLog("LittleFS mounted OK");
  }
}


timeStamp runTime = 0;                // microseconds since power on
timeStamp lapTime = 0;                // length of last loop of main core processes
void timeTracker() {
    lapTime = getTheCurrentTime() - runTime;
    runTime += lapTime;
}
timeStamp screenTime = 0;                        // GFX timer to count if screensaver should go on
#define DIAGNOSTICS_ON true 
void sendToLog(str msg) {
  if (DIAGNOSTICS_ON) {
    Serial.println(msg.c_str());
  }
}



#define CMDBTN_0 0
#define CMDBTN_1 20
#define CMDBTN_2 40
#define CMDBTN_3 60
#define CMDBTN_4 80
#define CMDBTN_5 100
#define CMDBTN_6 120
#define CMDCOUNT 7

class buttonDef {
public:
  byte     btnState = 0;        // binary 00 = off, 01 = just pressed, 10 = just released, 11 = held
  void interpBtnPress(bool isPress) {
    btnState = (((btnState << 1) + isPress) & 3);
  }
  int   coordRow = 0;        // hex coordinates
  int   coordCol = 0;        // hex coordinates
  timeStamp timePressed = 0;     // timecode of last press
  colorCode LEDcodeAnim = 0;     // calculate it once and store value, to make LED playback snappier 
  colorCode LEDcodePlay = 0;     // calculate it once and store value, to make LED playback snappier
  colorCode LEDcodeRest = 0;     // calculate it once and store value, to make LED playback snappier
  colorCode LEDcodeOff = 0;      // calculate it once and store value, to make LED playback snappier
  colorCode LEDcodeDim = 0;      // calculate it once and store value, to make LED playback snappier
  bool     animate = 0;         // hex is flagged as part of the animation in this frame, helps make animations smoother
  int      stepsFromC = 0;      // number of steps from C4 (semitones in 12EDO; microtones if >12EDO)
  bool     isCmd = 0;           // 0 if it's a MIDI note; 1 if it's a MIDI control cmd
  bool     inScale = 0;         // 0 if it's not in the selected scale; 1 if it is
  byte     note = UNUSED_NOTE;  // MIDI note or control parameter corresponding to this hex
  int      bend = 0;            // in microtonal mode, the pitch bend for this note needed to be tuned correctly
  byte     MIDIch = 0;          // what MIDI channel this note is playing on
  byte     synthCh = 0;         // what synth polyphony ch this is playing on
  float    frequency = 0.0;     // what frequency to ring on the synther
};
/*
  This class is like a virtual wheel.
  It takes references / pointers to 
  the state of three command buttons,
  translates presses of those buttons
  into wheel turns, and converts
  these movements into corresponding
  values within a range.
  
  This lets us generalize the
  behavior of a virtual pitch bend
  wheel or mod wheel using the same
  code, only needing to modify the
  range of output and the connected
  buttons to operate it.
*/
/*
  When sending smoothly-varying pitch bend
  or modulation messages over MIDI, the
  code uses a cool-down period of about
  1/30 of a second in between messages, enough
  for changes to sound continuous without
  overloading the MIDI message queue.
*/
#define CC_MSG_COOLDOWN_MICROSECONDS 32768

class wheelDef {
public:
  byte* alternateMode; // two ways to control
  byte* isSticky;      // TRUE if you leave value unchanged when no buttons pressed
  byte* topBtn;        // pointer to the key Status of the button you use as this button
  byte* midBtn;
  byte* botBtn;
  int16_t minValue;
  int16_t maxValue;
  int* stepValue;      // this can be changed via GEM menu
  int16_t defValue;    // snapback value
  int16_t curValue;
  int16_t targetValue;
  uint64_t timeLastChanged;
  void setTargetValue() {
    if (*alternateMode) {
      if (*midBtn >> 1) { // middle button toggles target (0) vs. step (1) mode
        int16_t temp = curValue;
            if (*topBtn == 1)     {temp += *stepValue;} // tap button
            if (*botBtn == 1)     {temp -= *stepValue;} // tap button
            if (temp > maxValue)  {temp  = maxValue;} 
        else if (temp <= minValue) {temp  = minValue;}
        targetValue = temp;
      } else {
        switch (((*topBtn >> 1) << 1) + (*botBtn >> 1)) {
          case 0b10:   targetValue = maxValue;     break;
          case 0b11:   targetValue = defValue;     break;
          case 0b01:   targetValue = minValue;     break;
          default:     targetValue = curValue;     break;
        }
      }
    } else {
      switch (((*topBtn >> 1) << 2) + ((*midBtn >> 1) << 1) + (*botBtn >> 1)) {
        case 0b100:  targetValue = maxValue;                         break;
        case 0b110:  targetValue = (3 * maxValue + minValue) / 4;    break;
        case 0b010:
        case 0b111:
        case 0b101:  targetValue = (maxValue + minValue) / 2;        break;
        case 0b011:  targetValue = (maxValue + 3 * minValue) / 4;    break;
        case 0b001:  targetValue = minValue;                         break;
        case 0b000:  targetValue = (*isSticky ? curValue : defValue); break;
        default: break;
      }
    }
  }
  bool updateValue(uint64_t givenTime) {
    int16_t temp = targetValue - curValue;
    if (temp != 0) {
      if ((givenTime - timeLastChanged) >= CC_MSG_COOLDOWN_MICROSECONDS ) {
        timeLastChanged = givenTime;
        if (abs(temp) < *stepValue) {
          curValue = targetValue;
        } else {
          curValue = curValue + (*stepValue * (temp / abs(temp)));
        }
        return 1;
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }   
};

const byte assignCmd[] = { 
  CMDBTN_0, CMDBTN_1, CMDBTN_2, CMDBTN_3, 
  CMDBTN_4, CMDBTN_5, CMDBTN_6
};

/*
  define h, which is a collection of all the 
  buttons from 0 to 139. hex[i] refers to the 
  button with the LED address = i.
*/

std::vector<buttonDef> hex;
int_matrix readHexState;

wheelDef modWheel = { &wheelMode, &modSticky,
  &hex[assignCmd[4]].btnState, &hex[assignCmd[5]].btnState, &hex[assignCmd[6]].btnState,
  0, 127, &modWheelSpeed, 0, 0, 0, 0
};
wheelDef pbWheel =  { &wheelMode, &pbSticky, 
  &hex[assignCmd[4]].btnState, &hex[assignCmd[5]].btnState, &hex[assignCmd[6]].btnState,
  -8192, 8191, &pbWheelSpeed, 0, 0, 0, 0
};
wheelDef velWheel = { &wheelMode, &velSticky, 
  &hex[assignCmd[0]].btnState, &hex[assignCmd[1]].btnState, &hex[assignCmd[2]].btnState,
  0, 127, &velWheelSpeed, 96, 96, 96, 0
};

bool toggleWheel = 0; // 0 for mod, 1 for pb

void setupGrid() {
  // this will get majorly improved in v2.0
  resize_matrix(readHexState, colPins.size(), 1 << muxPins.size());
  size_t btnCount = colPins.size() << muxPins.size();
  hex.resize(btnCount);
  for (byte i = 0; i < btnCount; i++) {
    hex[i].coordRow = (i / 10);
    hex[i].coordCol = (2 * (i % 10)) + (hex[i].coordRow & 1);
    hex[i].isCmd = 0;
    hex[i].note = UNUSED_NOTE;
    hex[i].btnState = 0;
  }
  for (byte c = 0; c < CMDCOUNT; c++) {
    hex[assignCmd[c]].isCmd = 1;
    hex[assignCmd[c]].note = CMDB + c;
  }
  // "flag" buttons as commands if there's no pixel associated
  for (byte i = pixelCount; i < btnCount; i++) {
    hex[i].isCmd = 1;
  }
  // On version 1.2, "button" 140 is shorted (always connected)
  hex[140].note = HARDWARE_V1_2;
}

// @LED
/*
  This section of the code handles sending
  color data to the LED pixels underneath
  the hex buttons.
*/
  // global define
#include <Adafruit_NeoPixel.h>  // library of code to interact with the LED array
Adafruit_NeoPixel strip;  

int32_t rainbowDegreeTime = 65'536; // microseconds to go through 1/360 of rainbow
/*
  This is actually a hacked together approximation
  of the color space OKLAB. A true conversion would
  take the hue, saturation, and value bits and
  turn them into linear RGB to feed directly into
  the LED class. This conversion is... not very OK...
  but does the job for now. A proper implementation
  of OKLAB is in the works.
  
  For transforming hues, the okLAB hue degree (0-360) is
  mapped to the RGB hue degree from 0 to 65535, using
  simple linear interpolation I created by hand comparing
  my HexBoard outputs to a Munsell color chip book.
*/
int16_t transformHue(float h) {
  float D = fmod(h,360);
  if (!perceptual) {
    return 65536 * D / 360;
  } else {
    //                red            yellow             green        cyan         blue
    int hueIn[] =  {    0,    9,   18,  102,  117,  135,  142,  155,  203,  240,  252,  261,  306,  333,  360};
    //              #ff0000          #ffff00           #00ff00      #00ffff     #0000ff     #ff00ff
    int hueOut[] = {    0, 3640, 5861,10922,12743,16384,21845,27306,32768,38229,43690,49152,54613,58254,65535};
    byte B = 0;
    while (D - hueIn[B] > 0) {
      B++;
    }
    float T = (D - hueIn[B - 1]) / (float)(hueIn[B] - hueIn[B - 1]);
    return (hueOut[B - 1] * (1 - T)) + (hueOut[B] * T);
  }
}
/*
  Saturation and Brightness are taken as is (already in a 0-255 range).
  The global brightness / 255 attenuates the resulting color for the
  user's brightness selection. Then the resulting RGB (HSV) color is
  "un-gamma'd" to be converted to the LED strip color.
*/
uint32_t getLEDcode(colorDef c) {
  return strip.gamma32(strip.ColorHSV(transformHue(c.hue),c.sat,c.val * globalBrightness / 255));
}
/*
  This function cycles through each button, and based on what color
  palette is active, it calculates the LED color code in the palette, 
  plus its variations for being animated, played, or out-of-scale, and
  stores it for recall during playback and animation. The color
  codes remain in the object until this routine is called again.
*/
void setLEDcolorCodes() {
  for (auto &h : hex) {
    if (!(h.isCmd)) {
      colorDef setColor;
      byte paletteIndex = positiveMod(h.stepsFromC,current.tuning().cycleLength);
      if (paletteBeginsAtKeyCenter) {
        paletteIndex = current.keyDegree(paletteIndex);
      }
      switch (colorMode) {
        case TIERED_COLOR_MODE: // This mode sets the color based on the palettes defined above.
          setColor = palette[current.tuningIndex].getColor(paletteIndex);
          break;
        case RAINBOW_MODE:      // This mode assigns the root note as red, and the rest as saturated spectrum colors across the rainbow.
          setColor = 
            { 360 * ((float)paletteIndex / (float)current.tuning().cycleLength)
            , SAT_VIVID
            , VALUE_NORMAL
            };
          break;
        case ALTERNATE_COLOR_MODE:
          // This mode assigns each note a color based on the interval it forms with the root note.
          // This is an adaptation of an algorithm developed by Nicholas Fox and Kite Giedraitis.
          float cents = current.tuning().stepSize * paletteIndex;
          bool perf = 0;
          float center = 0.0;
                  if                    (cents <   50)  {perf = 1; center =    0.0;}
            else if ((cents >=  50) && (cents <  250)) {          center =  147.1;}
            else if ((cents >= 250) && (cents <  450)) {          center =  351.0;}
            else if ((cents >= 450) && (cents <  600)) {perf = 1; center =  498.0;}
            else if ((cents >= 600) && (cents <= 750)) {perf = 1; center =  702.0;}
            else if ((cents >  750) && (cents <= 950)) {          center =  849.0;}
            else if ((cents >  950) && (cents <=1150)) {          center = 1053.0;}
            else if ((cents > 1150) && (cents < 1250)) {perf = 1; center = 1200.0;}
            else if ((cents >=1250) && (cents < 1450)) {          center = 1347.1;}
            else if ((cents >=1450) && (cents < 1650)) {          center = 1551.0;}
            else if ((cents >=1650) && (cents < 1850)) {perf = 1; center = 1698.0;}
            else if ((cents >=1800) && (cents <=1950)) {perf = 1; center = 1902.0;}
          float offCenter = cents - center;
          int16_t altHue = positiveMod((int)(150 + (perf * ((offCenter > 0) ? -72 : 72)) - round(1.44 * offCenter)), 360);
          float deSaturate = perf * (abs(offCenter) < 20) * (1 - (0.02 * abs(offCenter)));
          setColor = { 
            (float)altHue, 
            (byte)(255 - round(255 * deSaturate)), 
            (byte)(cents ? VALUE_SHADE : VALUE_NORMAL) };
          break;
      }
      h.LEDcodeRest = getLEDcode(setColor);
      h.LEDcodePlay = getLEDcode(setColor.tint()); 
      h.LEDcodeDim  = getLEDcode(setColor.shade());  
      setColor = {HUE_NONE,SAT_BW,VALUE_BLACK};
      h.LEDcodeOff  = getLEDcode(setColor);                // turn off entirely
      h.LEDcodeAnim = h.LEDcodePlay;
    }
  }
  sendToLog("LED codes re-calculated.");
}

void resetVelocityLEDs() {
  colorDef tempColor = 
    { (runTime % (rainbowDegreeTime * 360)) / (float)rainbowDegreeTime
    , SAT_MODERATE
    , byteLerp(0,255,85,127,velWheel.curValue)
    };
  strip.setPixelColor(assignCmd[0], getLEDcode(tempColor));

  tempColor.val = byteLerp(0,255,42,85,velWheel.curValue);
  strip.setPixelColor(assignCmd[1], getLEDcode(tempColor));
  
  tempColor.val = byteLerp(0,255,0,42,velWheel.curValue);
  strip.setPixelColor(assignCmd[2], getLEDcode(tempColor));
}
void resetWheelLEDs() {
  // middle button
  byte tempSat = SAT_BW;
  colorDef tempColor = {HUE_NONE, tempSat, (byte)(toggleWheel ? VALUE_SHADE : VALUE_LOW)};
  strip.setPixelColor(assignCmd[3], getLEDcode(tempColor));
  if (toggleWheel) {
    // pb red / green
    tempSat = byteLerp(SAT_BW,SAT_VIVID,0,8192,abs(pbWheel.curValue));
    tempColor = {(float)((pbWheel.curValue > 0) ? HUE_RED : HUE_CYAN), tempSat, VALUE_FULL};
    strip.setPixelColor(assignCmd[5], getLEDcode(tempColor));

    tempColor.val = tempSat * (pbWheel.curValue > 0);
    strip.setPixelColor(assignCmd[4], getLEDcode(tempColor));

    tempColor.val = tempSat * (pbWheel.curValue < 0);
    strip.setPixelColor(assignCmd[6], getLEDcode(tempColor));
  } else {
    // mod blue / yellow
    tempSat = byteLerp(SAT_BW,SAT_VIVID,0,64,abs(modWheel.curValue - 63));
    tempColor = {
      (float)((modWheel.curValue > 63) ? HUE_YELLOW : HUE_INDIGO), 
      tempSat, 
      (byte)(127 + (tempSat / 2))
    };
    strip.setPixelColor(assignCmd[6], getLEDcode(tempColor));

    if (modWheel.curValue <= 63) {
      tempColor.val = 127 - (tempSat / 2);
    }
    strip.setPixelColor(assignCmd[5], getLEDcode(tempColor));
    
    tempColor.val = tempSat * (modWheel.curValue > 63);
    strip.setPixelColor(assignCmd[4], getLEDcode(tempColor));
  }
}
uint32_t applyNotePixelColor(byte x) {
          if (hex[x].animate) { return hex[x].LEDcodeAnim;
  } else if (hex[x].MIDIch)  { return hex[x].LEDcodePlay;
  } else if (hex[x].inScale) { return hex[x].LEDcodeRest;
  } else if (scaleLock)    { return hex[x].LEDcodeOff;
  } else                   { return hex[x].LEDcodeDim;
  }
}
void lightUpLEDs() {   
  for (byte i = 0; i < pixelCount; i++) {      
    if (!(hex[i].isCmd)) {
      strip.setPixelColor(i,applyNotePixelColor(i));
    }
  }
  resetVelocityLEDs();
  resetWheelLEDs();
  strip.show();
}

// @MIDI
/*
  This section of the code handles all
  things related to MIDI messages.
*/
#include <Adafruit_TinyUSB.h>   // library of code to get the USB port working
#include <MIDI.h>               // library of code to send and receive MIDI messages
/*
  These values support correct MIDI output.
  Note frequencies are converted to MIDI note
  and pitch bend messages assuming note 69
  equals concert A4, as defined below. 
*/
#define CONCERT_A_HZ 440.0
/*
  Pitch bend messages are calibrated 
  to a pitch bend range where
  -8192 to 8191 = -200 to +200 cents, 
  or two semitones.
*/
#define PITCH_BEND_SEMIS 2
/*
  We use pitch bends to retune notes in MPE mode.
  Some setups can adjust to fit this, but some need us to adjust it.
*/
byte MPEpitchBendSemis = 2;
/*
  Create a new instance of the Arduino MIDI Library,
  and attach usb_midi as the transport.
*/
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, UMIDI);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, SMIDI);
// midiD takes the following bitwise flags
#define MIDID_NONE 0
#define MIDID_USB 1
#define MIDID_SER 2
#define MIDID_BOTH 3
byte midiD = MIDID_USB | MIDID_SER;

// What program change number we last sent (General MIDI/Roland MT-32)
byte programChange = 0;

std::queue<byte> MPEchQueue;
byte MPEpitchBendsNeeded; 

float freqToMIDI(float Hz) {             // formula to convert from Hz to MIDI note
  return 69.0 + 12.0 * log2f(Hz / 440.0);
}
float MIDItoFreq(float midi) {           // formula to convert from MIDI note to Hz
  return 440.0 * exp2((midi - 69.0) / 12.0);
}
float stepsToMIDI(int16_t stepsFromA) {  // return the MIDI pitch associated
  return freqToMIDI(CONCERT_A_HZ) + ((float)stepsFromA * (float)current.tuning().stepSize / 100.0);
}

void setPitchBendRange(byte Ch, byte semitones) {
  if (midiD&MIDID_USB) {
      UMIDI.beginRpn(0, Ch);
      UMIDI.sendRpnValue(semitones << 7, Ch);
      UMIDI.endRpn(Ch);
  }
  if (midiD&MIDID_SER) {
      SMIDI.beginRpn(0, Ch);
      SMIDI.sendRpnValue(semitones << 7, Ch);
      SMIDI.endRpn(Ch);
  }
  sendToLog(
    "set pitch bend range on ch " +
    std::to_string(Ch) + " to be " + 
    std::to_string(semitones) + " semitones"
  );
}

void setMPEzone(byte masterCh, byte sizeOfZone) {
  if (midiD&MIDID_USB) {
      UMIDI.beginRpn(6, masterCh);
      UMIDI.sendRpnValue(sizeOfZone << 7, masterCh);
      UMIDI.endRpn(masterCh);
  }
  if (midiD&MIDID_SER) {
      SMIDI.beginRpn(6, masterCh);
      SMIDI.sendRpnValue(sizeOfZone << 7, masterCh);
      SMIDI.endRpn(masterCh);
  }
  sendToLog(
    "tried sending MIDI msg to set MPE zone, master ch " +
    std::to_string(masterCh) + ", zone of this size: " + std::to_string(sizeOfZone)
  );
}

void resetTuningMIDI() {
  /*
    currently the only way that microtonal
    MIDI works is via MPE (MIDI polyphonic expression).
    This assigns re-tuned notes to an independent channel
    so they can be pitched separately.
  
    if operating in a standard 12-EDO tuning, or in a
    tuning with steps that are all exact multiples of
    100 cents, then MPE is not necessary.
  */
  if (current.tuning().stepSize == 100.0) {
    MPEpitchBendsNeeded = 1;
  /*  this was an attempt to allow unlimited polyphony for certain EDOs. doesn't work in Logic Pro.
  } else if (round(current.tuning().cycleLength * current.tuning().stepSize) == 1200) {
    MPEpitchBendsNeeded = current.tuning().cycleLength / std::gcd(12, current.tuning().cycleLength);
  */
  } else {
    MPEpitchBendsNeeded = 255;
  }
  if (MPEpitchBendsNeeded > 15) {
    setMPEzone(1, 15);   // MPE zone 1 = ch 2 thru 16
    while (!MPEchQueue.empty()) {     // empty the channel queue
      MPEchQueue.pop();
    }
    for (byte i = 2; i <= 16; i++) {
      MPEchQueue.push(i);           // fill the channel queue
      sendToLog("pushed ch " + std::to_string(i) + " to the open channel queue");
    }
  } else {
    setMPEzone(1, 0);
  }
  // force pitch bend back to the expected range of 2 semitones.
  for (byte i = 1; i <= 16; i++) {
    if(midiD&MIDID_USB)UMIDI.sendControlChange(123, 0, i);
    if(midiD&MIDID_SER)SMIDI.sendControlChange(123, 0, i);
    setPitchBendRange(i, MPEpitchBendSemis);   
  }
}

void sendMIDImodulationToCh1() {
  if(midiD&MIDID_USB)UMIDI.sendControlChange(1, modWheel.curValue, 1);
  if(midiD&MIDID_SER)SMIDI.sendControlChange(1, modWheel.curValue, 1);
  sendToLog("sent mod value " + std::to_string(modWheel.curValue) + " to ch 1");
}

void sendMIDIpitchBendToCh1() {
  if(midiD&MIDID_USB)UMIDI.sendPitchBend(pbWheel.curValue, 1);
  if(midiD&MIDID_SER)SMIDI.sendPitchBend(pbWheel.curValue, 1);
  sendToLog("sent pb wheel value " + std::to_string(pbWheel.curValue) + " to ch 1");
}
void sendProgramChange() {
  if(midiD&MIDID_USB)UMIDI.sendProgramChange(programChange - 1, 1);
  if(midiD&MIDID_SER)SMIDI.sendProgramChange(programChange - 1, 1);
}

void setupMIDI() {
  usb_midi.setStringDescriptor("HexBoard MIDI");  // Initialize MIDI, and listen to all MIDI channels
  UMIDI.begin(MIDI_CHANNEL_OMNI);                 // This will also call usb_midi's begin()
  SMIDI.begin(MIDI_CHANNEL_OMNI);
  resetTuningMIDI();
  sendToLog("setupMIDI okay");
}

// @synth

#include "src/LEGACYsynth.h"     // the original one
// kinda has to be loaded here because it's still tied to the grid objects, eugh

// @animate
/*
  This section of the code handles
  LED animation responsive to key
  presses
*/
/*
  This section of the code handles the hex grid
      Hexagonal coordinates
        https://www.redblobgames.com/grids/hexagons/
        http://ondras.github.io/rot.js/manual/#hex/indexing
  The HexBoard contains a grid of 140 buttons with
  hexagonal keycaps. The processor has 10 pins connected
  to a multiplexing unit, which hotswaps between the 14 rows
  of ten buttons to allow all 140 inputs to be read in one
  program read cycle.
  There are 140 LED pixels on the Hexboard.
  LED instructions all go through the LED_PIN.
  It so happens that each LED pixel corresponds
  to one and only one hex button, so both a LED
  and its button can have the same index from 0-139.
  Since these parameters are pre-defined by the
  hardware build, the dimensions of the grid
  are therefore constants.
*/
#define COLCOUNT 10
#define ROWCOUNT 16
#define BTN_COUNT COLCOUNT*ROWCOUNT
/*
  The coordinate system used to locate hex buttons
  a certain distance and direction away relies on
  a preset array of coordinate offsets corresponding
  to each of the six linear directions on the hex grid.
  These cardinal directions are enumerated to make
  the code more legible for humans.
*/
#define HEX_DIRECTION_EAST 0
#define HEX_DIRECTION_NE   1
#define HEX_DIRECTION_NW   2
#define HEX_DIRECTION_WEST 3
#define HEX_DIRECTION_SW   4
#define HEX_DIRECTION_SE   5
// animation variables  E NE NW  W SW SE
int8_t vertical[] =   { 0,-1,-1, 0, 1, 1};
int8_t horizontal[] = { 2, 1,-1,-2,-1, 1};

uint64_t animFrame(timeStamp t) {     
  if (t == 0) {
    return 0;
  }          // 2^20 microseconds is close enough to 1 second
  return 1 + (((runTime - t) * animationFPS) >> 20);
}
void flagToAnimate(int8_t r, int8_t c) {
  if (! 
    (    ( r < 0 ) || ( r >= ROWCOUNT )
      || ( c < 0 ) || ( c >= (2 * COLCOUNT) )
      || ( ( c + r ) & 1 )
    )
  ) {
    hex[(10 * r) + (c / 2)].animate = 1;
  }
}
void animateMirror() {
  for (auto &h : hex) {                      // check every hex
    if ((!(h.isCmd)) && (h.MIDIch)) {                   // that is a held note     
      for (auto &n : hex) {                  // compare to every hex
        if ((!(n.isCmd)) && (!(n.MIDIch))) {            // that is a note not being played
          int16_t temp = h.stepsFromC - n.stepsFromC;   // look at difference between notes
          if (animationType == ANIMATE_OCTAVE) {              // set octave diff to zero if need be
            temp = positiveMod(temp, current.tuning().cycleLength);
          }
          if (temp == 0) {                                    // highlight if diff is zero
            n.animate = 1;
          }
        }
      }  
    }
  }
}
void animateOrbit() {
  for (auto &h : hex) {                               // check every hex
    if ((!(h.isCmd)) && (h.MIDIch) && ((h.inScale) || (!scaleLock))) {    // that is a held note
      byte tempDir = (animFrame(h.timePressed) % 6);
      flagToAnimate(h.coordRow + vertical[tempDir], h.coordCol + horizontal[tempDir]);       // different neighbor each frame
    }
  }
}
void animateRadial() {
  for (auto &h : hex) {                                // check every hex
    if (!(h.isCmd) && (h.inScale || !scaleLock)) {                                                // that is a note
      uint64_t radius = animFrame(h.timePressed);
      if ((radius > 0) && (radius < 16)) {                              // played in the last 16 frames
        byte steps = ((animationType == ANIMATE_SPLASH) ? radius : 1);  // star = 1 step to next corner; ring = 1 step per hex
        int8_t turtleRow = h.coordRow + (radius * vertical[HEX_DIRECTION_SW]);
        int8_t turtleCol = h.coordCol + (radius * horizontal[HEX_DIRECTION_SW]);
        for (byte dir = HEX_DIRECTION_EAST; dir < 6; dir++) {           // walk along the ring in each of the 6 hex directions
          for (byte i = 0; i < steps; i++) {                            // # of steps to the next corner 
            flagToAnimate(turtleRow,turtleCol);                         // flag for animation
            turtleRow += (vertical[dir] * (radius / steps));
            turtleCol += (horizontal[dir] * (radius / steps));
          }
        }
      }
    }      
  }    
}
void animateLEDs() {  
  for (auto &h : hex) {      
    h.animate = 0;
  }
  if (animationType) {
    switch (animationType) { 
      case ANIMATE_STAR: case ANIMATE_SPLASH:
        animateRadial();
        break;
      case ANIMATE_ORBIT:
        animateOrbit();
        break;
      case ANIMATE_OCTAVE: case ANIMATE_BY_NOTE:
        animateMirror();
        break;  
      default:
        break;
    }
  }
}

// @assignment
/*
  This section of the code contains broad
  procedures for assigning musical notes
  and related values to each button
  of the hex grid.
*/
// run this if the layout, key, or transposition changes, but not if color or scale changes
void assignPitches() {     
  sendToLog("assignPitch was called:");
  for (byte i = 0; i < pixelCount; i++) {
    if (!(hex[i].isCmd)) {
      // steps is the distance from C
      // the stepsToMIDI function needs distance from A4
      // it also needs to reflect any transposition, but
      // NOT the key of the scale.
      float N = stepsToMIDI(current.pitchRelToA4(hex[i].stepsFromC));
      if (N < 0 || N >= 128) {
        hex[i].note = UNUSED_NOTE;
        hex[i].bend = 0;
        hex[i].frequency = 0.0;
      } else {
        hex[i].note = ((N >= 127) ? 127 : round(N));
        hex[i].bend = (ldexp(N - hex[i].note, 13) / MPEpitchBendSemis);
        hex[i].frequency = MIDItoFreq(N);
      }
      sendToLog(
        "hex #" + std::to_string(i) + ", " +
        "steps=" + std::to_string(hex[i].stepsFromC) + ", " +
        "isCmd? " + std::to_string(hex[i].isCmd) + ", " +
        "note=" + std::to_string(hex[i].note) + ", " + 
        "bend=" + std::to_string(hex[i].bend) + ", " + 
        "freq=" + std::to_string(hex[i].frequency) + ", " + 
        "inScale? " + std::to_string(hex[i].inScale) + "."
      );
    }
  }
  sendToLog("assignPitches complete.");
}
void applyScale() {
  sendToLog("applyScale was called:");
  for (byte i = 0; i < pixelCount; i++) {
    if (!(hex[i].isCmd)) {
      if (current.scale().tuning == ALL_TUNINGS) {
        hex[i].inScale = 1;
      } else {
        byte degree = current.keyDegree(hex[i].stepsFromC); 
        if (degree == 0) {
          hex[i].inScale = 1;    // the root is always in the scale
        } else {
          byte tempSum = 0;
          byte iterator = 0;
          while (degree > tempSum) {
            tempSum += current.scale().pattern[iterator];
            iterator++;
          }  // add the steps in the scale, and you're in scale
          hex[i].inScale = (tempSum == degree);   // if the note lands on one of those sums
        }
      }
      sendToLog(
        "hex #" + std::to_string(i) + ", " +
        "steps=" + std::to_string(hex[i].stepsFromC) + ", " +
        "isCmd? " + std::to_string(hex[i].isCmd) + ", " +
        "note=" + std::to_string(hex[i].note) + ", " + 
        "inScale? " + std::to_string(hex[i].inScale) + "."
      );
    }
  }
  setLEDcolorCodes();
  sendToLog("applyScale complete.");
}
void applyLayout() {       // call this function when the layout changes
  sendToLog("buildLayout was called:");
  for (byte i = 0; i < pixelCount; i++) {
    if (!(hex[i].isCmd)) {        
      int8_t distCol = hex[i].coordCol - hex[current.layout().hexMiddleC].coordCol;
      int8_t distRow = hex[i].coordRow - hex[current.layout().hexMiddleC].coordRow;
      hex[i].stepsFromC = (
        (distCol * current.layout().acrossSteps) + 
        (distRow * (
          current.layout().acrossSteps + 
          (2 * current.layout().dnLeftSteps)
        ))
      ) / 2;  
      sendToLog(
        "hex #" + std::to_string(i) + ", " +
        "steps from C4=" + std::to_string(hex[i].stepsFromC) + "."
      );
    }
  }
  applyScale();        // when layout changes, have to re-apply scale and re-apply LEDs
  assignPitches();     // same with pitches
  sendToLog("buildLayout complete.");
}
void cmdOn(byte n) {   // volume and mod wheel read all current buttons
  switch (n) {
    case CMDB + 3:
      toggleWheel = !toggleWheel;
      break;
    case HARDWARE_V1_2:
      Hardware_Version = n;
      identifyHardware();
      break;
    default:
      // the rest should all be taken care of within the wheelDef structure
      break;
  }
}
void cmdOff(byte n) {   // pitch bend wheel only if buttons held.
  switch (n) {
    default:
      break;  // nothing; should all be taken care of within the wheelDef structure
  }
}

// @menu
#define GEM_DISABLE_GLCD       // this line is needed to get the B&W display to work
/*
  The GEM menu library accepts initialization
  values to set the width of various components
  of the menu display, as below.
*/
#define MENU_ITEM_HEIGHT 10
#define MENU_PAGE_SCREEN_TOP_OFFSET 10
#define MENU_VALUES_LEFT_OFFSET 78
// Create menu object of class GEM_u8g2. Supply its constructor with reference to u8g2 object we created earlier
GEM_u8g2 menu(
  u8g2, GEM_POINTER_ROW, GEM_ITEMS_COUNT_AUTO, 
  MENU_ITEM_HEIGHT, MENU_PAGE_SCREEN_TOP_OFFSET, MENU_VALUES_LEFT_OFFSET
);
void changeTranspose(); // i dunno why we have to do this
void rebootToBootloader(); // i dunno why we have to do this
void fakeButton() {} // i dunno why we have to do this

#include "src/LEGACYmenu.h"


#define CONTRAST_AWAKE 63
#define CONTRAST_SCREENSAVER 1
void setupGFX() {
  u8g2.begin();                       // Menu and graphics setup
  u8g2.setBusClock(1000000);          // Speed up display
  u8g2.setContrast(CONTRAST_AWAKE);   // Set contrast
  sendToLog("U8G2 graphics initialized.");
}
void screenSaver() {
  if (screenTime <= screenSaverTimeout) {
    screenTime = screenTime + lapTime;
    if (screenSaverOn) {
      screenSaverOn = 0;
      u8g2.setContrast(CONTRAST_AWAKE);
    }
  } else {
    if (!screenSaverOn) {
      screenSaverOn = 1;
      u8g2.setContrast(CONTRAST_SCREENSAVER);
    }
  }
}

// @interface
void readHexes() {
		//  EXAMPLE:
	//
	//  GLOBAL
	//  keyboard_obj hexBoard;
	//  ...
	//  loop()...
	//
	//  if (pinGrid.readTo(hexBoard.read_state, hexBoard.read_time)) {
	//    hexBoard.update();
	//    for (auto& h : hexBoard.hex) {
	//			instruction_t cmd = h.get_instructions();
	//			if (cmd.do == command::send_note_on) {
	//				// tryNoteOn(h);
	//			}
	//			if (cmd.do == do_note_off) {
	//				// tryNoteOff(h);
	//			}
	//      // etc.etc.
	//		}
	// }
  if (pinGrid.readTo(readHexState)) {
    for (byte i = 0; i < BTN_COUNT; i++) {
      // hex[i].interpBtnPress(readHexState[1][2] == LOW);
    }
    for (auto& h : hex) {   // For all buttons in the deck
      switch (h.btnState) {
        case btn_press: // just pressed
          h.timePressed = runTime;
          if (h.isCmd) {
            cmdOn(h.note);
          } else if (h.inScale || (!scaleLock)) {
            if (!(h.MIDIch)) {    
              if (MPEpitchBendsNeeded == 1) {
                h.MIDIch = 1;
              } else if (MPEpitchBendsNeeded <= 15) {
                h.MIDIch = 2 + positiveMod(h.stepsFromC, MPEpitchBendsNeeded);
              } else {
                if (MPEchQueue.empty()) {   // if there aren't any open channels
                  sendToLog("MPE queue was empty so did not play a midi note");
                } else {
                  h.MIDIch = MPEchQueue.front();   // value in MIDI terms (1-16)
                  MPEchQueue.pop();
                  sendToLog("popped " + std::to_string(h.MIDIch) + " off the MPE queue");
                }
              }
              if (h.MIDIch) {
                if(midiD&MIDID_USB)UMIDI.sendNoteOn(h.note, velWheel.curValue, h.MIDIch); // ch 1-16
                if(midiD&MIDID_SER)SMIDI.sendNoteOn(h.note, velWheel.curValue, h.MIDIch); // ch 1-16

                if(midiD&MIDID_USB)UMIDI.sendPitchBend(h.bend, h.MIDIch); // ch 1-16
                if(midiD&MIDID_SER)SMIDI.sendPitchBend(h.bend, h.MIDIch); // ch 1-16
              } 
            }
            //if (playbackMode != SYNTH_OFF) {
            if (playbackMode == SYNTH_POLY) {
              // operate independently of MIDI
              if (synthChQueue.empty()) {
                sendToLog("synth channels all firing, so did not add one");
              } else {
                h.synthCh = synthChQueue.front();
                synthChQueue.pop();
                sendToLog("popped " + std::to_string(h.synthCh) + " off the synth queue");
                setSynthFreq(h.frequency, h.synthCh);
              }
            //} else {    
                // operate in lockstep with MIDI
                //if (h.MIDIch) {
                //  replaceMonoSynthWith(h);
                //}
            // }
            }
          }
          break;
        case btn_release: // just released
          if (h.isCmd) {
            cmdOff(h.note);
          } else if (h.inScale || (!scaleLock)) {
            if (h.MIDIch) {    // but just in case, check
              if(midiD&MIDID_USB)UMIDI.sendNoteOff(h.note, velWheel.curValue, h.MIDIch);
              if(midiD&MIDID_SER)SMIDI.sendNoteOff(h.note, velWheel.curValue, h.MIDIch);
              if (MPEpitchBendsNeeded > 15) {
                MPEchQueue.push(h.MIDIch);
                sendToLog("pushed " + std::to_string(h.MIDIch) + " on the MPE queue");
              }
              h.MIDIch = 0;
            }
            //if (playbackMode && (playbackMode != SYNTH_POLY)) {
              //if (arpeggiatingNow == &h) {
              //  replaceMonoSynthWith(findNextHeldNote());
              //}
            //}
            if (playbackMode == SYNTH_POLY) {
              if (h.synthCh) {
                setSynthFreq(0, h.synthCh);
                synthChQueue.push(h.synthCh);
                h.synthCh = 0;
              }
            }
          }
          break;
        case btn_hold: // held
          break;
        default: // inactive
          break;
      }
    }
  }
}

void updateWheels() {  
  velWheel.setTargetValue();
  bool upd = velWheel.updateValue(runTime);
  if (upd) {
    sendToLog("vel became " + std::to_string(velWheel.curValue));
  }
  if (toggleWheel) {
    pbWheel.setTargetValue();
    upd = pbWheel.updateValue(runTime);
    if (upd) {
      sendMIDIpitchBendToCh1();
      updateSynthWithNewFreqs();
    }
  } else {
    modWheel.setTargetValue();
    upd = modWheel.updateValue(runTime);
    if (upd) {
      sendMIDImodulationToCh1();
    }
  }
}

void dealWithRotary() {
  rotary.invertDirection(rotaryInvert);
  if (menu.readyForKey()) {
    if (rotary.getClickFromBuffer()) {
      menu.registerKeyPress(GEM_KEY_OK);
      screenTime = 0;
    }
    int getTurn = rotary.getTurnFromBuffer();
    if (getTurn != 0) {
      menu.registerKeyPress((getTurn > 0) ? GEM_KEY_UP : GEM_KEY_DOWN);
      screenTime = 0;
    }
  }
}

void identifyHardware() {
  if (Hardware_Version == HARDWARE_V1_2) {
      midiD = MIDID_USB | MIDID_SER;
      audioD = AUDIO_PIEZO | AUDIO_AJACK;
      menuPageSynth.addMenuItem(menuItemAudioD, 2);
      globalBrightness = BRIGHT_DIM;
      setLEDcolorCodes();
      rotaryInvert = true;
  }
}

// @mainLoop
/*
  An Arduino program runs
  the setup() function once, then
  runs the loop() function on repeat
  until the machine is powered off.

  The RP2040 has two identical cores.
  Anything called from setup() and loop()
  runs on the first core.
  Anything called from setup1() and loop1()
  runs on the second core.

  On the HexBoard, the second core is
  dedicated to two timing-critical tasks:
  running the synth emulator, and tracking
  the rotary knob inputs.
  Everything else runs on the first core.
*/
bool coreZeroSetupDone = false;

softTimer LEDrefreshTimer;


void setup() {
  #if (defined(ARDUINO_ARCH_MBED) && defined(ARDUINO_ARCH_RP2040))
  TinyUSB_Device_Init(0);  // Manual begin() is required on core without built-in support for TinyUSB such as mbed rp2040
  #endif
  setupDevices();
  coreZeroSetupDone = true;
  setupMIDI();
  setupFileSystem();
  Wire.setSDA(OLED_sdaPin);
  Wire.setSCL(OLED_sclPin);
  setupGrid();
  applyLayout();
  resetSynthFreqs();
  strip.setPin(ledPin);
  strip.updateLength(pixelCount);
  strip.begin();    // INITIALIZE NeoPixel strip object
  strip.show();     // Turn OFF all pixels ASAP
  setLEDcolorCodes();
  setupGFX();
  setupMenu();
  for (byte i = 0; i < 5 && !TinyUSBDevice.mounted(); i++) {
    delay(1);  // wait until device mounted, maybe
  }
  LEDrefreshTimer.start(1'000'000 / target_LED_frame_rate_in_Hz, 0);
}
void loop() {   // run on first core
  timeTracker();  // Time tracking functions
  readHexes();       // Read and store the digital button states of the scanning matrix
  updateWheels();   // deal with the pitch/mod wheel
  // arpeggiate();      // arpeggiate if synth mode allows it
  if (LEDrefreshTimer.ifDone_thenRepeat()) {   // every 1/60s, run LED routines
    animateLEDs();     // deal with animations
    lightUpLEDs();      // refresh LEDs    
  } else {  // all other cycles, run OLED / menu routines
    screenSaver();  // Reduces wear-and-tear on OLED panel
    dealWithRotary();  // deal with menu
  }
}
void setup1() {  // set up on second core
  while (!coreZeroSetupDone) {1;}
  setupTaskMgr();
}
void loop1() {  // run on second core
  if (audioBuffer.roomToWrite() > 0) {
    nextSample();
  }
}
