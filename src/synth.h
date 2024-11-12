#pragma once
#include "utils.h"

const int polyphony_limit = 8;
/*
  The hybrid synth sound blends between
  square, saw, and triangle waveforms
  at different frequencies. Said frequencies
  are controlled via constants here.
*/
const float transition_square = 220.0;
const float transition_saw_low = 440.0;
const float transition_saw_high = 880.0;
const float transition_triangle = 1760.0;

enum { // different instrument generator rules
  simple_lookup = 0,
  generator_square = 1,
  generator_saw = 2,
  generator_triangle = 3,
  generator_hybrid = 4
};
// an instrument is a definition of how to
// synthesize a sound. it contains
// 1. either a generator rule (i.e. square / saw wave)
// or an audio waveform loop
// 2. an ADSR envelope
// 3. some basic operational functions.
struct instrument_t {
  int  type = generator_square;
  int  samples[256];
  int  attack = 0;    // in samples
  int  decay = 0;    // in samples
  int  sustain = 255;  // level [0..255]
  int  release = 0;    // in samples
  void set_attack(int ms) {
    attack = ms * sampleRate / 1000;
  }
  void set_decay(int ms) {
    decay = ms * sampleRate / 1000;
  }
  void set_release(int ms) {
    release = ms * sampleRate / 1000;
  }
  void set_waveform(int wave_table[256]) {
    type = simple_lookup;
    for (int i = 0; i < 256; i++) {
      samples[i] = wave_table[i];
    }
  }  
  // consider a match if the parameters are the same
  // but ignore the treatment of the sample table
  // for now until i figure it out
  bool operator==(const instrument_t &rhs) const {
    return this->type    == rhs.type    &&
        // this->samples == rhs.samples &&
           this->attack  == rhs.attack  &&
           this->decay   == rhs.decay   &&
           this->sustain == rhs.sustain &&
           this->release == rhs.release;
  }
};
// an oscillator object returns
// the position of a synthesized
// waveform based on a counter
// that is incremented based on
// a target frequency. it uses
// loops of 256 8-bit samples [-127 to 127]. *** actually still 0 to 255 so change this soon
// the waveform is generated based
// on the definition of its associated
// instrument_t object (type of sound
// and its ADSR envelope).
class oscillator_t {
private:
  instrument_t _instrument;
  int _stored_wave_form[256];
  bool _playing = false;
  bool _noteOn = false;
  float _freq = 0;
  int _modulation = 0;   // must be [0..127]
  int _velocity = 0;     // must be [0..127]
  bool _applyEQ = true;
  int _eq = 8;
  int _incr = 0;  // inverse of frequency
  int _ctr = 0;   // waveform lookup counter
  int _smp = 0;   // samples since onset
  void hybridParameters(int &refToShape, int &refToMod) {
    // at low frequencies play a square wave
    // at middle frequencies play a saw wave
    // at high frequencies play a triangle wave
    // in between boundaries, create in-between shapes
    if (_freq < transition_square) {
      refToShape = generator_square;
      refToMod = 0;
    } else if (_freq < transition_saw_high) {
      refToShape = generator_saw;
      if (_freq < transition_saw_low) {
        // 127 = square, 0 = saw
        refToMod = lerp(0,127,transition_square,transition_saw_low,_freq);
      } else {
        refToMod = 0;
      }
    } else {
      refToShape = generator_triangle;
      if (_freq < transition_saw_low) {
        // 127 = saw, 0 = triangle
        refToMod = lerp(0,127,transition_saw_high,transition_triangle,_freq);
      } else {
        refToMod = 0;
      }
    }
  }
  void regenerateWaveForm() {
    // whenever this is called, build the waveform based on
    // the generator or wavetable lookup for this channel.
    // this usually happens if the mod wheel, underlying
    // frequency, or user option change could affect the wave shape
    int shape = _instrument.type;
    int mod = _modulation;
    if (shape == generator_hybrid) {
      hybridParameters(shape, mod);
    }
    switch (shape) { // generator functions to make wave tables
    case generator_square: // the mod wheel changes the duty cycle
      for (int i = 0; i < (128 + mod); i++) {
        _stored_wave_form[i] = 0;
      }
      for (int i = (128 + mod); i < 256; i++) {
        _stored_wave_form[i] = 255;
      }
      break;    
    case generator_saw: // the mod wheel changes the slope of the saw
	    // i.e. more mod makes the saw "squared off"
      for (int i = 0; i < mod; i++) {
        _stored_wave_form[i] = 0;
      }
      for (int i = mod; i < (128 + mod); i++) {
        _stored_wave_form[i] = lerp(0,255,mod,128+mod,i);
      }
      for (int i = (128 + mod); i < 256; i++) {
        _stored_wave_form[i] = 255;
      }
      break;    
    case generator_triangle: // the mod wheel moves the apex of the wave
	    // i.e. more mod makes the triangle more "sawtooth-like"
      for (int i = 0; i < (128 + mod); i++) {
        _stored_wave_form[i] = lerp(0,255,0,128+mod,i);
      }
      for (int i = (128 + mod); i < 256; i++) {
        _stored_wave_form[i] = lerp(0,255,256,128+mod,i);
      }
      break;
    default: // otherwise grab the existing wavetable
      for (int i = 0; i < 256; i++) {
        _stored_wave_form[i] = _instrument.samples[i];
      }
      break;
    }
  }
  void recalculateEQ() {
    // EQ   8    7    6    5    4    3    2    1    0
    // dB  +4   +3   +1.5  0   -2   -4.5 -8   -14  -inf
    // dB = 20*ln(EQ)/ln(10).
    // NOTE this is NOT real EQ, it's adjusting the volume
    // only based on the note frequency, not its harmonics
    //                                       EQ  MIDI notes
         if (_freq <     8.0) {_eq = 0;} // -inf below  0 (C -1)
    else if (_freq <    60.0) {_eq = 8;} // +4dB 0 to 34 (Bb1)
    else if (_freq <  1500.0) {_eq = 7;} // +3dB   to 90 (F#6)
    else if (_freq <  2000.0) {_eq = 6;} // +1.5   to 95 (B 6)
    else if (_freq <  2500.0) {_eq = 5;} //  0dB   to 99 (Eb7)
    else if (_freq <  3000.0) {_eq = 4;} // -2dB  to 102 (F#7)
    else if (_freq <  3500.0) {_eq = 3;} // -4.5  to 104 (Ab7)
    else if (_freq <  4000.0) {_eq = 4;} // -2dB  to 107 (B 7)
    else if (_freq <  4500.0) {_eq = 5;} //  0dB  to 109 (C#8)
    else if (_freq <  5000.0) {_eq = 6;} // +1.5  to 111 (Eb8)
    else if (_freq < 13290.0) {_eq = 7;} // +3dB  to 127.99 (G#9-1c)
    else                      {_eq = 0;}
  }
public:
  bool isPlaying() { 
    return _playing;
  }
  // calling noteOn / Off is a passive event
  // it sets the flags so that when next_sample()
  // is next called, this channel will be marked
  // as able to begin playing a note.
  void noteOn() { 
    _noteOn = true;
    _playing = true;
    _smp = 0;
    _ctr = 0;
  }
  void noteOff() {
    _noteOn = false;
    if (!(_instrument.release)) {
      _playing = false;
    }
    _smp = 0;
  }
  void set_frequency(float f, float sampleIncr) {
    if (f != _freq) {
      _freq = f;
     // _incr drives the speed of the wave table
     // lookup. it might be better to simply
     // use floats all around instead of force
     // to 16-bit integer, but for now keep it this way
      _incr = round(f * sampleIncr);
      if (_instrument.type == generator_hybrid) {
        regenerateWaveForm();
      }
      if (_applyEQ) {
        recalculateEQ();
      }
    }
  }
  bool can_be_modulated(int _generator) {
    return ((_generator == generator_triangle) 
         || (_generator == generator_square) 
         || (_generator == generator_saw));
  }
  void set_mod(int lvl) {
    if (lvl != _modulation) {
      _modulation = lvl;
      if (can_be_modulated(_instrument.type)) {
        regenerateWaveForm();
      }
    }
  }
  void set_vel(int lvl) {
    _velocity = lvl;
  }
  void set_instrument(instrument_t instr) {
    if (!(_instrument == instr)) {
      _instrument = instr;
      regenerateWaveForm();
    }
  }
  int nextSample() { 
    if (!(_playing)) {
      return 0;
    }
    _ctr += _incr;
    _smp++;
    int wave_raw = _stored_wave_form[(_ctr & 0xFF00) >> 8];
    int envelope = 0;
    if (_noteOn) {
      // apply attack decay sustain
      if (_smp >= (_instrument.attack + _instrument.decay)) {
        envelope = _instrument.sustain;
      } else if (_smp < _instrument.attack) {
        envelope = lerp((int)1,(int)255,0,_instrument.attack,_smp);          
      } else {
        envelope = lerp((int)255,_instrument.sustain,0,_instrument.decay,_smp-_instrument.attack);
      }
    } else if (_smp < _instrument.release) {
      // apply release
      envelope = lerp(_instrument.sustain,(int)0,0,_instrument.release,_smp);        
    }
    if (!(envelope)) {
      // if past the release length
      _playing = false;
      return 0;
    }
    return (wave_raw * envelope * _eq * _velocity) >> 18;
    // 26 bits: 8 (waveform) + 8 (envelope) + 3 (eq) + 7 (velocity)
    // downsample to 8 bits = shift 18 to the right
  }
};
// a synth is a collection of
// oscillators whose states are
// "mixed" to produce a stream
// of raw audio output.
class synth_t {
private:
  int _sample_rate;
  float _sample_increment;
  int_queue _channelQueue;
public:
  // sort of like MIDI, we want fixed channels
  // and to access via index, which we do via
  // the channel queue
  oscillator_t channel[polyphony_limit];
  void setup(int sample_rate) {
    // unlikely that the sample rate will change mid-execution.
    _sample_rate = sample_rate;
    // every instrument in the synth will use this value
    // as the basis for reading samples.
    // i suppose this could be stored as a float and then
    // push the floating math back to the synth.
    // because it's getting rounded anyway?
    // think it over.
    _sample_increment = 65536.0 / _sample_rate;
    // initialize the list of open polyphony channels
    for (int i = 0; i < polyphony_limit; i++) {
      _channelQueue.push(i + 1);
    }
  }
  void set_modulation(int lvl) {
    for (int i = 0; i < polyphony_limit; i++) {
      channel[i].set_mod(lvl);
    }
  }
  int kompressor(int original) { // this probably doesn't work
    // i think we're better off using power law to attenuate the
    // output based on number of voices playing simultaneously
    // scale output = 1.0 / sqrt(# voices). it works nicely
    // with base output = 64, see v1.0 of firmware LEGACYsynth.h
    // meanwhile keep this here for now.
         if (original >= 2040) {return 255;}
    else if (original >=  512) {return 160 + (original >> 4);}
    else if (original >=  256) {return 128 + (original >> 3);}
    else                       {return       (original >> 1);}
  }
  int tryNoteOn(instrument_t instr, float freq, int vel, int mod) {
    if (_channelQueue.empty()) {
      return 0; // the calling function should know that channel zero means nothing was played.
    }
    int ch = pop_front(_channelQueue) - 1;
    channel[ch].set_instrument(instr);
    channel[ch].set_frequency(freq, _sample_increment);
    channel[ch].set_vel(vel);
    channel[ch].set_mod(mod);
    channel[ch].noteOn();
    return ch + 1;
  }
  void tryNoteOff(int ch) {
    _channelQueue.push(ch);
    channel[ch - 1].noteOff();
    // probably not even necessary to call it "try", it just "do"es.
  }
  int count_voices_playing() {
    int temp = 0;
    for (int i = 0; i < polyphony_limit; i++) {
      temp += channel[i].isPlaying();
    }
    return temp;
  }
  int writeNextSample() {
    int mix = 0;
    int poly = 0;
    for (auto c : channel) {
      // either here, or in the synth::nextSample,
      // need to change to signed int expression.
      mix += c.nextSample();  // 8 bits each
      poly += c.isPlaying();
    }
    return kompressor(mix); // deprecate? compress ~ 3:1, normalize, and downsample
  }
};
synth_t synth;
// -------------------
// here are some instrument presets -- TODO: EXPRESS AS +/- 0..127
// -------------------
const int sine[256] = {
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
const int strings[256] = {
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
const int clarinet[256] = {
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
instrument_t default_hybrid_sound = {generator_hybrid,sine[0],10,200,192,100};
