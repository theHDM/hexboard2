#pragma once
#include "src/includes.h"

#include <Wire.h>
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico/time.h"

const uint frequency_poll    =    16;
const uint rotary_rate_in_uS =   512;
const uint audio_buffer_size =  1024;

enum {
  btn_off     = 0b00,
  btn_press   = 0b01,
  btn_release = 0b10,
  btn_hold    = 0b11
};

void upd_btn_state(uint8_t &refState, bool press) {
  refState = (0b00011 & ((refState << 1) + press));
}

uint64_t runTime() {
  uint64_t temp = timer_hw->timerawh;
  return ((temp << 32) | timer_hw->timerawl);
}

class semaphore_obj {
private:  bool _state = false;
public:   void flag() {_state = true;}
          void clear() {_state = false;}
          bool state() {return _state;}
};

class pinGrid_obj {
private:
  bool				_isEnabled;
	bool				_cycle_mux_pins_first;
	byte_vec   	_muxPins;
  byte_vec   	_colPins;
  std::size_t _muxSize;
  std::size_t _colSize;
  int_matrix  _gridState;
  int_matrix  _outputMap;
  uint8_t     _muxCounter;
  uint8_t     _colCounter;
  uint16_t    _gridCounter;
  uint16_t    _muxMaxValue;
  bool        _readComplete;
  void resetCounter() {
    _readComplete = false;
    _gridCounter = 0;
  }
	void init_pin_states() {
		for (auto m : _muxPins) {pinMode(m, OUTPUT);}
		for (auto c : _colPins) {pinMode(c, INPUT_PULLUP);}
	}
	bool advanceMux() {
		_muxCounter = (++_muxCounter) % _muxMaxValue;
		for (uint8_t b = 0; b < _muxSize; b++) 
			{	digitalWrite(_muxPins[b], (_muxCounter >> b) & 1); }
		return (!(_muxCounter));
	}
	bool advanceCol() {
		pinMode(_colPins[_colCounter], INPUT);        // 0V / LOW
		_colCounter = (++_colCounter) % _colSize;
		pinMode(_colPins[_colCounter], INPUT_PULLUP); // +3.3V / HIGH
		return (!(_colCounter));
	}
public:    
  void setup(byte_vec muxPins, 
             byte_vec colPins, 
             int_matrix outputMap) {
    _muxPins = muxPins;
    _muxSize = _muxPins.size();
    _muxMaxValue = (1u << _muxSize);
    _colPins = colPins;
    _colSize = _colPins.size();
    _outputMap = outputMap;
    _gridState.resize(_colSize, byte_vec(_muxMaxValue));
    _muxCounter = 0;
    _colCounter = 0;
    resetCounter();
		init_pin_states();
  }
  void poll() {
    if (!(_readComplete)) {
			uint8_t press = digitalRead(_colPins[_colCounter]);
      upd_btn_state(_gridState[_colCounter][_muxCounter], (press == LOW));
      ++_gridCounter;
      if (cycle_mux_pins_first) {
				if (advanceMux()) {
					_readComplete = advanceCol();
				}
			} else {
				if (advanceCol()) {
					_readComplete = advanceMux();
				}
			}
    }
  }
  bool readTo(byte_vec &refTo) {
    if (_readComplete) {
      for (size_t i = 0; i < _outputMap.size(); i++) {
        for (size_t j = 0; j < _outputMap[i].size(); j++) {
          refTo[_outputMap[i][j]] = _gridState[i][j];
        }
      }
      resetCounter();
      return true;
    } else {
      return false;
    }
  }
};
pinGrid_obj pinGrid;

class rotary_obj {
private:
  uint8_t       _Apin;
  uint8_t       _Bpin;
  uint8_t       _Cpin;
  bool          _invert;
  uint8_t       _turnState;
  uint8_t       _clickState;
  int8_t        _turnBuffer;
  uint64_t      _lastClickTime;
  time_queue    _clickQueue;
  const uint8_t stateMatrix[7][4] = {
    {0,4,1,0},
    {2,0,1,0},{2,3,1,0},{2,3,0,8},
    {5,4,0,0},{5,4,6,0},{5,0,6,16}
  };
public:
  void setup(uint8_t Apin, uint8_t Bpin, uint8_t Cpin) {
    _Apin = Apin;
    _Bpin = Bpin;
    _Cpin = Cpin;
    pinMode(_Apin, INPUT_PULLUP);
    pinMode(_Bpin, INPUT_PULLUP);
    pinMode(_Cpin, INPUT_PULLUP);
    _invert = false;
    _turnState = 0;
    _clickState = 0;
    _turnBuffer = 0;
    _lastClickTime = 0;
    _clickQueue.clear();
  }
  void invertDirection(bool invert) {_invert = invert;}
  void poll() {
    uint8_t A = digitalRead(_Apin);
    uint8_t B = digitalRead(_Bpin);
    uint8_t getRotation = (_invert ? ((A << 1) | B) : ((B << 1) | A));
    _turnState = stateMatrix[_turnState & 0b00111][getRotation];
    _turnBuffer += (_turnState & 0b01000) >> 3;
    _turnBuffer -= (_turnState & 0b10000) >> 4;
    upd_btn_state(_clickState, digitalRead(_Cpin) == LOW);
    switch (_clickState) { 
      case btn_press:
        _lastClickTime = runTime();
        break;
      case btn_release:
        _clickQueue.push_back(runTime() - _lastClickTime);
        break;
      default:
        break;
    }
  }
  int8_t getTurnFromBuffer() {
    int8_t x = ((_turnBuffer > 0) ? 1 : -1) * (_turnBuffer != 0);
    _turnBuffer -= x;
    return x;
  }
  uint64_t getClickFromBuffer() {
    uint64_t x = 0;
    if (!(_clickQueue.empty())) {
      x = _clickQueue.front();
      _clickQueue.pop_front();
    }
    return x;
  }
};
rotary_obj  rotary;

class audioOut_obj {
private:
  byte_vec   _buffer;
  uint       _bufferSize;
  uint       _bufferFree;
  uint       _bufferReadPtr;
  uint       _bufferWritePtr;
  byte_queue _pwmPins;
  byte_queue _analogPins;
public:
  void setup_audio_feed(uint8_t pin, bool isAnalog) {
    if (!isAnalog) {
      _pwmPins.push_back(pin);
      uint8_t slice = pwm_gpio_to_slice_num(pin);
      gpio_set_function(pin, GPIO_FUNC_PWM); // set that pin as PWM
      pwm_set_phase_correct(slice, true);    // synthesize 8-bit sound
      pwm_set_wrap(slice, 254);              // PWM_TOP = 254, with phase correct. Loop every (254 + 1) * 2 = 510 cycles.
      pwm_set_clkdiv(slice, 1.0f);           // run at full clock speed
      pwm_set_gpio_level(pin, 0);            // initialize at zero to prevent whining sound
      pwm_set_enabled(slice, true);          // ENGAGE!
    } else {
      _analogPins.push_back(pin);
    }
  }
  void setup_buffer(uint size) {
    _buffer.resize(size);
    _bufferSize = size;
    _bufferFree = size;
    _bufferReadPtr = 0;
    _bufferWritePtr = 0;
  }
  void setup(byte_vec pwmPins, byte_vec analogPins) {
    setup_buffer(audio_buffer_size);
    for (auto i : pwmPins)    {setup_audio_feed(i, false);}
    for (auto j : analogPins) {setup_audio_feed(j, true);}
  }
  void send() {
    uint8_t lvl = 0;
    if (_bufferFree != _bufferSize) {
      uint8_t lvl = _buffer[_bufferReadPtr];
      _bufferReadPtr = _bufferReadPtr++ % _bufferSize;
      _bufferFree++;
    }
    for (auto i : _pwmPins) {pwm_set_gpio_level(i, lvl);}
    for (auto j : _analogPins)     {analogWrite(j, lvl);}
  }
  bool buffer(uint8_t lvl) {
    if (_bufferFree == 0) {
      return false;
    }
    _buffer[_bufferWritePtr] = lvl;
    _bufferWritePtr = _bufferWritePtr++ % _bufferSize;
    _bufferFree--;
    return true;
  }
  uint free_bytes() {
    return _bufferFree;
  }
	float 
};
audioOut_obj audioOut;


uint cycle_count = 0;
constexpr uint rotary_cycles = rotary_rate_in_uS / 2 / frequency_poll;
static bool on_poll(__unused struct repeating_timer *t) {
	if ((++cycle_count) & 1) {
		// odd cycles
	  audioOut.send();  // write the next sample to all audio outs
		return true;
	}
	if ((cycle_count >> 1) >= rotary_cycles) { 
		cycle_count = cycle_count % (rotary_cycles << 1);
    rotary.poll();
  }
	pinGrid.poll();
  return true;
}

const uint sample_rate_in_Hz = 31250;

struct repeating_timer io_timer;
void start_processing_background_IO_tasks() {
  add_repeating_timer_us(frequency_poll, on_poll, NULL, &io_timer);
}
