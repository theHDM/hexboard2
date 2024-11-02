#pragma once
#include "src/includes.h"

const uint8_t polyphony_limit = 8;
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

enum {
  simple_lookup = 0,
  generator_square = 1,
  generator_saw = 2,
  generator_triangle = 3,
  generator_hybrid = 4
};

struct instrument_t {
  uint8_t  type = generator_square;
  uint8_t  samples[256];
  uint     attack  = 0;    // in samples
  uint     decay   = 0;    // in samples
  uint8_t  sustain = 255;  // level [0..255]
  uint     release = 0;    // in samples
  void set_attack(uint ms) {
    attack = ms * sampleRate / 1000;
  }
  void set_decay(uint ms) {
    decay = ms * sampleRate / 1000;
  }
  void set_release(uint ms) {
    release = ms * sampleRate / 1000;
  }
  void set_waveform(uint8_t wave_table[256]) {
    type = simple_lookup;
    for (uint8_t i = 0; i < 256; i++) {
      samples[i] = wave_table[i];
    }
  }  
  bool operator==(const instrument_t &rhs) const {
    return this->type    == rhs.type    &&
        // this->samples == rhs.samples &&
           this->attack  == rhs.attack  &&
           this->decay   == rhs.decay   &&
           this->sustain == rhs.sustain &&
           this->release == rhs.release;
  }
};

/* 
  an oscillator object returns
  the position of a synthesized
  waveform based on a counter
  that is incremented based on
  a target frequency. works
  with 8-bit loops of 256 samples.
*/
class oscillator_t {
private:
  instrument_t _instrument;
  uint8_t  _stored_wave_form[256];
  bool     _playing = false;
  bool     _noteOn = false;
  float    _freq = 0;
  uint8_t  _modulation = 0;   // must be [0..127]
  uint8_t  _velocity = 0;     // must be [0..127]
  bool     _applyEQ = true;
  uint8_t  _eq = 8;
  uint16_t _incr = 0;  // inverse of frequency
  uint16_t _ctr = 0;   // waveform lookup counter
  uint64_t _smp = 0;   // samples since onset
  void     hybridParameters(uint8_t &refToShape, uint8_t &refToMod) {
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
  void     regenerateWaveForm() {
    uint8_t shape = _instrument.type;
    uint8_t mod = _modulation;
    if (shape == generator_hybrid) {
      hybridParameters(shape, mod);
    }
    switch (shape) {
    case generator_square:
      for (uint8_t i = 0; i < (128 + mod); i++) {
        _stored_wave_form[i] = 0;
      }
      for (uint8_t i = (128 + mod); i < 256; i++) {
        _stored_wave_form[i] = 255;
      }
      break;    
    case generator_saw:
      for (uint8_t i = 0; i < mod; i++) {
        _stored_wave_form[i] = 0;
      }
      for (uint8_t i = mod; i < (128 + mod); i++) {
        _stored_wave_form[i] = lerp(0,255,mod,128+mod,i);
      }
      for (uint8_t i = (128 + mod); i < 256; i++) {
        _stored_wave_form[i] = 255;
      }
      break;    
    case generator_triangle:
      for (uint8_t i = 0; i < (128 + mod); i++) {
        _stored_wave_form[i] = lerp(0,255,0,128+mod,i);
      }
      for (uint8_t i = (128 + mod); i < 256; i++) {
        _stored_wave_form[i] = lerp(0,255,256,128+mod,i);
      }
      break;
    default:
      for (uint8_t i = 0; i < 256; i++) {
        _stored_wave_form[i] = _instrument.samples[i];
      }
      break;
    }
  }
  void     recalculateEQ() {
    // EQ   8    7    6    5    4    3    2    1    0
    // dB  +4   +3   +1.5  0   -2   -4.5 -8   -14  -inf
    // dB = 20*ln(EQ)/ln(10).
         if (_freq <     8.0) {_eq = 0;} // -inf low pass
    else if (_freq <    60.0) {_eq = 8;} // +4dB rumble
    else if (_freq <   100.0) {_eq = 8;} // +4dB punch
    else if (_freq <   450.0) {_eq = 8;} // +4dB boom
    else if (_freq <  1000.0) {_eq = 8;} // +4dB honk
    else if (_freq <  1500.0) {_eq = 7;} // +3dB tin
    else if (_freq <  2000.0) {_eq = 6;} // +1.5 tin
    else if (_freq <  2500.0) {_eq = 5;} //  0dB crunch
    else if (_freq <  3000.0) {_eq = 4;} // -2dB crunch
    else if (_freq <  3500.0) {_eq = 3;} // -4.5 pierce
    else if (_freq <  4000.0) {_eq = 4;} // -2dB pierce
    else if (_freq <  4500.0) {_eq = 5;} //  0dB presence
    else if (_freq <  5000.0) {_eq = 6;} // +1.5 presence
    else if (_freq < 12500.0) {_eq = 7;} // +3dB sib
    else                      {_eq = 0;}
  }
public:
  bool isPlaying() {
    return _playing;
  }
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
      _incr = round(f * sampleIncr);
      if (_instrument.type == generator_hybrid) {
        regenerateWaveForm();
      }
      if (_applyEQ) {
        recalculateEQ();
      }
    }
  }
  bool can_be_modulated(uint8_t _generator) {
    return ((_generator == generator_triangle) 
         || (_generator == generator_square) 
         || (_generator == generator_saw));
  }
  void set_mod(uint8_t lvl) {
    if (lvl != _modulation) {
      _modulation = lvl;
      if (can_be_modulated(_instrument.type)) {
        regenerateWaveForm();
      }
    }
  }
  void set_vel(uint8_t lvl) {
    _velocity = lvl;
  }
  void set_instrument(instrument_t instr) {
    if (!(_instrument == instr)) {
      _instrument = instr;
      regenerateWaveForm();
    }
  }
  uint8_t nextSample() { 
    if (!(_playing)) {
      return 0;
    }
    _ctr += _incr;
    _smp++;
    uint8_t wave_raw = _stored_wave_form[(_ctr & 0xFF00) >> 8];
    uint8_t envelope = 0;
    if (_noteOn) {
      if (_smp >= (_instrument.attack + _instrument.decay)) {
        envelope = _instrument.sustain;
      } else if (_smp < _instrument.attack) {
        envelope = lerp((uint8_t)1,(uint8_t)255,0,_instrument.attack,_smp);          
      } else {
        envelope = lerp((uint8_t)255,_instrument.sustain,0,_instrument.decay,_smp-_instrument.attack);
      }
    } else if (_smp < _instrument.release) {
        envelope = lerp(_instrument.sustain,(uint8_t)0,0,_instrument.release,_smp);        
    }
		if (!(envelope)) {
			_playing = false;
			return 0;
		}
    return (wave_raw * envelope * _eq * _velocity) >> 18;
    // 26 bits: 8 (waveform) + 8 (envelope) + 3 (eq) + 7 (velocity)
    // downsample to 8 bits = shift 18 to the right
  }
};

/*
  a synth is a collection of
  oscillators whose states are
  "mixed" to produce a stream
  of raw audio output
  consider using signed int -127 to 128 instead
	so that stuff is centered.

*/
class synth_t {
private:
	uint 	_sample_rate;
	float	_sample_increment;
  std::queue<uint8_t> _channelQueue;
public:
  oscillator_t channel[polyphony_limit];
  void setup(uint sample_rate) {    
    _sample_rate = sample_rate;
		_sample_increment = 65536.0 / _sample_rate;
		for (uint8_t i = 0; i < polyphony_limit; i++) {
      _channelQueue.push(i + 1);
    }
  }
  void set_modulation(uint8_t lvl) {
    for (uint8_t i = 0; i < polyphony_limit; i++) {
      channel[i].set_mod(lvl);
    }
  }
  uint8_t kompressor(uint16_t original) {
         if (original >= 2040) {return 255;}
    else if (original >=  512) {return 160 + (original >> 4);}
    else if (original >=  256) {return 128 + (original >> 3);}
    else                       {return       (original >> 1);}
  }
  uint8_t writeNextSample() {
    uint16_t mix = 0;
    uint8_t poly = 0;
    for (auto c : channel) {
      mix += c.nextSample();  // 8 bits each
      poly += c.isPlaying();
    }
    return kompressor(mix); // compress ~ 3:1, normalize, and downsample
  }
  uint8_t tryNoteOn(instrument_t instr, float freq, uint8_t vel, uint8_t mod) {
    if (_channelQueue.empty()) {
      return 0;
    }
    uint8_t ch = _channelQueue.front() - 1;
    _channelQueue.pop();
    channel[ch].set_instrument(instr);
    channel[ch].set_frequency(freq, _sample_increment);
    channel[ch].set_vel(vel);
    channel[ch].set_mod(mod);
    channel[ch].noteOn();
    return ch + 1;
  }
  void tryNoteOff(uint8_t ch) {
    _channelQueue.push(ch);
    channel[ch - 1].noteOff();
  }
  uint8_t count_voices_playing() {
    uint8_t temp = 0;
    for (uint8_t i = 0; i < polyphony_limit; i++) {
      temp += channel[i].isPlaying();
    }
    return temp;
  }
};
synth_t synth;
// -------------------
// here are some instrument presets
// -------------------
const uint8_t sine[256] = {
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
const uint8_t strings[256] = {
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
const uint8_t clarinet[256] = {
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