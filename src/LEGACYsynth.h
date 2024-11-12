// this is just a continuation of the Hexboard code but it's very long
// so it's in its own header.

#define SYNTH_OFF 0
#define SYNTH_MONO 1
#define SYNTH_ARPEGGIO 2
#define SYNTH_POLY 3
byte playbackMode = SYNTH_POLY;

#define WAVEFORM_SINE 0
#define WAVEFORM_STRINGS 1
#define WAVEFORM_CLARINET 2
#define WAVEFORM_HYBRID 7
#define WAVEFORM_SQUARE 8
#define WAVEFORM_SAW 9
#define WAVEFORM_TRIANGLE 10 
byte currWave = WAVEFORM_HYBRID;

// @synth
/*
  This section of the code handles audio
  output via the piezo buzzer and/or the
  headphone jack (on hardware v1.2 only)
*/


#define AUDIO_NONE 0
#define AUDIO_PIEZO 1
#define AUDIO_AJACK 2
#define AUDIO_BOTH 3
byte audioD = AUDIO_PIEZO | AUDIO_AJACK;
/*
  These definitions provide 8-bit samples to emulate.
  You can add your own as desired; it must
  be an array of 256 values, each from 0 to 255.
  Ideally the waveform is normalized so that the
  peaks are at 0 to 255, with 127 representing
  no wave movement.
*/
byte sine[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   2,   3,   3, 
    4,   5,   6,   7,   8,   9,  10,  12,  13,  15,  16,  18,  19,  21,  23,  25, 
    27,  29,  31,  33,  35,  37,  39,  42,  44,  46,  49,  51,  54,  56,  59,  62, 
    64,  67,  70,  73,  76,  79,  81,  84,  87,  90,  93,  96,  99, 103, 106, 109, 
  112, 115, 118, 121, 124, 127, 131, 134, 137, 140, 143, 146, 149, 152, 156, 159, 
  162, 165, 168, 171, 174, 176, 179, 182, 185, 188, 191, 193, 196, 199, 201, 204, 
  206, 209, 211, 213, 216, 218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 237, 
  239, 240, 242, 243, 245, 246, 247, 248, 249, 250, 251, 252, 252, 253, 254, 254, 
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 252, 252, 
  251, 250, 249, 248, 247, 246, 245, 243, 242, 240, 239, 237, 236, 234, 232, 230, 
  228, 226, 224, 222, 220, 218, 216, 213, 211, 209, 206, 204, 201, 199, 196, 193, 
  191, 188, 185, 182, 179, 176, 174, 171, 168, 165, 162, 159, 156, 152, 149, 146, 
  143, 140, 137, 134, 131, 127, 124, 121, 118, 115, 112, 109, 106, 103,  99,  96, 
    93,  90,  87,  84,  81,  79,  76,  73,  70,  67,  64,  62,  59,  56,  54,  51, 
    49,  46,  44,  42,  39,  37,  35,  33,  31,  29,  27,  25,  23,  21,  19,  18, 
    16,  15,  13,  12,  10,   9,   8,   7,   6,   5,   4,   3,   3,   2,   1,   1
};
byte strings[] = {
    0,   0,   0,   1,   3,   6,  10,  14,  20,  26,  33,  41,  50,  59,  68,  77, 
    87,  97, 106, 115, 124, 132, 140, 146, 152, 157, 161, 164, 166, 167, 167, 167, 
  165, 163, 160, 157, 153, 149, 144, 140, 135, 130, 126, 122, 118, 114, 111, 109, 
  106, 104, 103, 101, 101, 100, 100, 100, 100, 101, 101, 102, 103, 103, 104, 105, 
  106, 107, 108, 109, 110, 111, 113, 114, 115, 116, 117, 119, 120, 121, 123, 124, 
  126, 127, 129, 131, 132, 134, 135, 136, 138, 139, 140, 141, 142, 144, 145, 146, 
  147, 148, 149, 150, 151, 152, 152, 153, 154, 154, 155, 155, 155, 155, 154, 154, 
  152, 151, 149, 146, 144, 140, 137, 133, 129, 125, 120, 115, 111, 106, 102,  98, 
    95,  92,  90,  88,  88,  88,  89,  91,  94,  98, 103, 109, 115, 123, 131, 140, 
  149, 158, 168, 178, 187, 196, 205, 214, 222, 229, 235, 241, 245, 249, 252, 254, 
  255, 255, 255, 254, 253, 250, 248, 245, 242, 239, 236, 233, 230, 227, 224, 222, 
  220, 218, 216, 215, 214, 213, 212, 211, 210, 210, 209, 208, 207, 206, 205, 203, 
  201, 199, 197, 194, 191, 188, 184, 180, 175, 171, 166, 161, 156, 150, 145, 139, 
  133, 127, 122, 116, 110, 105,  99,  94,  89,  84,  80,  75,  71,  67,  64,  61, 
    58,  56,  54,  52,  50,  49,  48,  47,  46,  45,  45,  44,  43,  42,  41,  40, 
    39,  37,  35,  33,  31,  28,  25,  22,  19,  16,  13,  10,   7,   5,   2,   1
};
byte clarinet[] = {
    0,   0,   2,   7,  14,  21,  30,  38,  47,  54,  61,  66,  70,  72,  73,  74, 
    73,  73,  72,  71,  70,  71,  72,  74,  76,  80,  84,  88,  93,  97, 101, 105, 
  109, 111, 113, 114, 114, 114, 113, 112, 111, 110, 109, 109, 109, 110, 112, 114, 
  116, 118, 121, 123, 126, 127, 128, 129, 128, 127, 126, 123, 121, 118, 116, 114, 
  112, 110, 109, 109, 109, 110, 111, 112, 113, 114, 114, 114, 113, 111, 109, 105, 
  101,  97,  93,  88,  84,  80,  76,  74,  72,  71,  70,  71,  72,  73,  73,  74, 
    73,  72,  70,  66,  61,  54,  47,  38,  30,  21,  14,   7,   2,   0,   0,   2, 
    9,  18,  31,  46,  64,  84, 105, 127, 150, 171, 191, 209, 224, 237, 246, 252, 
  255, 255, 253, 248, 241, 234, 225, 217, 208, 201, 194, 189, 185, 183, 182, 181, 
  182, 182, 183, 184, 185, 184, 183, 181, 179, 175, 171, 167, 162, 158, 154, 150, 
  146, 144, 142, 141, 141, 141, 142, 143, 144, 145, 146, 146, 146, 145, 143, 141, 
  139, 136, 134, 132, 129, 128, 127, 126, 127, 128, 129, 132, 134, 136, 139, 141, 
  143, 145, 146, 146, 146, 145, 144, 143, 142, 141, 141, 141, 142, 144, 146, 150, 
  154, 158, 162, 167, 171, 175, 179, 181, 183, 184, 185, 184, 183, 182, 182, 181, 
  182, 183, 185, 189, 194, 201, 208, 217, 225, 234, 241, 248, 253, 255, 255, 252, 
  246, 237, 224, 209, 191, 171, 150, 127, 105,  84,  64,  46,  31,  18,   9,   2, 
};
/*
  The hybrid synth sound blends between
  square, saw, and triangle waveforms
  at different frequencies. Said frequencies
  are controlled via constants here.
*/
  #define TRANSITION_SQUARE    220.0
  #define TRANSITION_SAW_LOW   440.0
  #define TRANSITION_SAW_HIGH  880.0
  #define TRANSITION_TRIANGLE 1760.0
/*
  The poll interval represents how often a
  new sample value is emulated on the PWM
  hardware. It is the inverse of the digital
  audio sample rate. 24 microseconds has been
  determined to be the sweet spot, and corresponds
  to approximately 41 kHz, which is close to
  CD-quality (44.1 kHz). A shorter poll interval
  may produce more pleasant tones, but if the
  poll is too short then the code will not have
  enough time to calculate the new sample and
  the resulting audio becomes unstable and
  inaccurate. 
*/
/*
  Eight voice polyphony can be simulated. 
  Any more voices and the
  resolution is too low to distinguish;
  also, the code becomes too slow to keep
  up with the poll interval. This value
  can be safely reduced below eight if
  there are issues.
  
  Note this is NOT the same as the MIDI
  polyphony limit, which is 15 (based
  on using channel 2 through 16 for
  polyphonic expression mode).
*/
#define POLYPHONY_LIMIT 8
/*
  A basic EQ level can be stored to perform
  simple loudness adjustments at certain
  frequencies where human hearing is sensitive.

  By default it's off but you can change this
  flag to "true" to enable it. This may also
  be moved to a Advanced menu option.
*/
#define EQUAL_LOUDNESS_ADJUST true
/*
  This class defines a virtual oscillator.
  It stores an oscillation frequency in
  the form of an increment value, which is
  how much a counter would have to be increased
  every time the poll() interval is reached, 
  such that a counter overflows from 0 to 65,535
  back to zero at some frequency per second.
  
  The value of the counter is useful for reading
  a waveform sample, so that an analog signal
  can be emulated by reading the sample at each
  poll() based on how far the counter has moved
  towards 65,536.
*/

class oscillator {
public:
  uint16_t increment = 0;
  uint16_t counter = 0;
  byte a = 127;
  byte b = 128;
  byte c = 255;
  uint16_t ab = 0;
  uint16_t cd = 0;
  byte eq = 0;
};
oscillator synth[POLYPHONY_LIMIT];          // maximum polyphony
std::queue<byte> synthChQueue;
const byte attenuation[] = {64,24,17,14,12,11,10,9,8}; // full volume in mono mode; equalized volume in poly.
//  buttonDef * arpeggiatingNow = nullptr;
//  uint64_t arpeggiateTime = 0;                // Used to keep track of when this note started playing in ARPEG mode
//  uint64_t arpeggiateLength = 65536;         // in microseconds. approx a 1/32 note at 114 BPM


void nextSample() {
  uint32_t mix = 0;
  byte voices = POLYPHONY_LIMIT;
  uint16_t p;
  byte t;
  byte level = 0;
  for (byte i = 0; i < POLYPHONY_LIMIT; i++) {
    if (synth[i].increment) {
      synth[i].counter += synth[i].increment; // should loop from 65536 -> 0        
      p = synth[i].counter;
      t = p >> 8;
      switch (currWave) {
        case WAVEFORM_SAW:                                                            break;
        case WAVEFORM_TRIANGLE: p = 2 * ((p >> 15) ? p : (65535 - p));                break;
        case WAVEFORM_SQUARE:   p = 0 - (p > (32768 - modWheel.curValue * 7 * 16));   break;
        case WAVEFORM_HYBRID:   if (t <= synth[i].a) {
                                  p = 0;
                                } else if (t < synth[i].b) {
                                  p = (t - synth[i].a) * synth[i].ab;
                                } else if (t <= synth[i].c) {
                                  p = 65535;
                                } else {
                                  p = (256 - t) * synth[i].cd;
                                };                                                  break;
        case WAVEFORM_SINE:     p = sine[t] << 8;                                   break;
        case WAVEFORM_STRINGS:  p = strings[t] << 8;                                break;
        case WAVEFORM_CLARINET: p = clarinet[t] << 8;                               break;
        default:                                                                  break;
      }
      mix += (p * synth[i].eq);  // P[16bit] * EQ[3bit] =[19bit]
    } else {
      --voices;
    }
  }
  if (voices) {
    mix *= attenuation[voices]; // [19bit]*atten[6bit] = [25bit]
    mix *= velWheel.curValue; // [25bit]*vel[7bit]=[32bit], poly+ 
    level = mix >> 24;
    audioBuffer.write(level);  // [32bit] - [8bit] = [24bit]
  }
}
// RUN ON CORE 1
byte isoTwoTwentySix(float f) {
  /*
    a very crude implementation of ISO 226
    equal loudness curves
      Hz dB  Amp ~ sqrt(10^(dB/10))
      200  0  8
      800 -3  6   
    1500  0  8
    3250 -6  4
    5000  0  8
  */
  if ((f < 8.0) || (f > 12500.0)) {   // really crude low- and high-pass
    return 0;
  } else {
    if (EQUAL_LOUDNESS_ADJUST) {
      if ((f <= 200.0) || (f >= 5000.0)) {
        return 8;
      } else {
        if (f < 1500.0) {
          return 6 + 2 * (float)(abs(f-800) / 700);
        } else {
          return 4 + 4 * (float)(abs(f-3250) / 1750);
        }
      }
    } else {
      return 8;
    }
  }
}
void setSynthFreq(float frequency, byte channel) {
  byte c = channel - 1;
  if (frequency == 0) {
    synth[c].increment = 0;
    return;
  } else {
    float f = frequency * exp2(pbWheel.curValue * PITCH_BEND_SEMIS / 98304.0);
    synth[c].counter = 0;
    synth[c].increment = round(f * actual_audio_sample_period_in_uS * 0.065536);   // cycle 0-65535 at resultant frequency
    synth[c].eq = isoTwoTwentySix(f);
    if (currWave == WAVEFORM_HYBRID) {
      if (f < TRANSITION_SQUARE) {
        synth[c].b = 128;
      } else if (f < TRANSITION_SAW_LOW) {
        synth[c].b = (byte)(128 + 127 * (f - TRANSITION_SQUARE) / (TRANSITION_SAW_LOW - TRANSITION_SQUARE));
      } else if (f < TRANSITION_SAW_HIGH) {
        synth[c].b = 255;
      } else if (f < TRANSITION_TRIANGLE) {
        synth[c].b = (byte)(127 + 128 * (TRANSITION_TRIANGLE - f) / (TRANSITION_TRIANGLE - TRANSITION_SAW_HIGH));
      } else {
        synth[c].b = 127;
      }
      if (f < TRANSITION_SAW_LOW) {
        synth[c].a = 255 - synth[c].b;
        synth[c].c = 255;
      } else {
        synth[c].a = 0;
        synth[c].c = synth[c].b;
      }
      if (synth[c].a > 126) {
        synth[c].ab = 65535;
      } else {
        synth[c].ab = 65535 / (synth[c].b - synth[c].a - 1);
      }
      synth[c].cd = 65535 / (256 - synth[c].c);
    }
  }
}
/*
// USE THIS IN MONO OR ARPEG MODE ONLY
buttonDef* findNextHeldNote() {
  //traverse to end
  buttonDef* begin{hex};
  buttonDef* end{hex + std::size(hex)};
  for (buttonDef* iter = arpeggiatingNow; iter != end; ++iter) {
    if ((iter->MIDIch) && (!(iter->isCmd))) {
      return iter;
    }
  }
  for (buttonDef* iter = begin; iter != arpeggiatingNow; ++iter) {
    if ((iter->MIDIch) && (!(iter->isCmd))) {
      return iter;
    }
  }
  return nullptr;
}
void replaceMonoSynthWith(buttonDef& h) {
  if (arpeggiatingNow == &h) return;
  arpeggiatingNow->synthCh = 0;
  arpeggiatingNow = &h;
  if (arpeggiatingNow != nullptr) {
    h.synthCh = 1;
    setSynthFreq(h.frequency, 1);
  } else {
    setSynthFreq(0, 1);
  }
}
*/
void resetSynthFreqs() {
  while (!synthChQueue.empty()) {
    synthChQueue.pop();
  }
  for (auto &v : synth) {
    v.increment = 0;
    v.counter = 0;
  }
  for (auto &h : hex) {
    h.synthCh = 0;
  }
  if (playbackMode == SYNTH_POLY) {
    for (byte i = 0; i < POLYPHONY_LIMIT; i++) {
      synthChQueue.push(i + 1);
    }
  }
}
void updateSynthWithNewFreqs() {
  for (auto &h : hex) {
    if (!(h.isCmd)) {
      if (h.synthCh) {
        setSynthFreq(h.frequency,h.synthCh);           // pass all notes thru synth again if the pitch bend changes
      }
    }
  }
}

void arpeggiate() {
  if (playbackMode == SYNTH_ARPEGGIO) {
  //  if (runTime - arpeggiateTime > arpeggiateLength) {
    //   arpeggiateTime = runTime;
  //    replaceMonoSynthWith(*findNextHeldNote());
  //  }
  }
}