#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <vector>
#include <deque>
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include "pico/time.h"

// sorry, we have to use global variables
// because IRQ requires a static callback
// reference. otherwise i suppose this
// could be a library.

// rotary pin fire sequence takes about 3 milliseconds
// if knob turns are skipped, then lower this constant
const uint frequency_poll_rotary = 512;
uint rotary_poll_counter = 0;

// this is the number of microseconds needed for the multiplexer
// to take effect before reading the next hex button state.
// should be at least 10 microseconds. but if you set this
// constant too high, then key input will feel sluggish.
const uint frequency_poll_pinGrid = 32;
uint pinGrid_poll_counter = 0;

// there is a single timer controlling background hardware
// processes. i tried to use multiple hardware timers but
// for some reason it doesn't work with this chip.
// set this value to be less or equal to both constants.
const uint frequency_poll = 32;

const uint audio_buffer_size = 1024;

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
/*
class semaphore_obj {
private:  bool _state = false;
public:   void flag() {_state = true;}
          void clear() {_state = false;}
          bool state() {return _state;}
};
*/
class pinGrid_obj {
private:
  std::vector<uint8_t>   _muxPins;
  std::vector<uint8_t>   _colPins;
  std::size_t         _muxSize;
  std::size_t         _colSize;
  std::vector<std::vector<uint8_t>>  _gridState;
  std::vector<std::vector<int>>      _outputMap;
  uint8_t             _muxCounter;
  uint8_t             _colCounter;
  uint16_t            _gridCounter;
  uint16_t            _muxMaxValue;
  bool                _readComplete;
  void resetCounter() {
    _readComplete = false;
    _gridCounter = 0;
  }

public:    
  void setup(std::vector<uint8_t> muxPins, 
             std::vector<uint8_t> colPins, 
             std::vector<std::vector<int>> outputMap) {
    _muxPins = muxPins;
    _muxSize = _muxPins.size();
    _muxMaxValue = (1u << _muxSize);
    _colPins = colPins;
    _colSize = _colPins.size();
    _outputMap = outputMap;
    _gridState.resize(_colSize, std::vector<uint8_t>(_muxMaxValue));
    _muxCounter = 0;
    _colCounter = 0;
    resetCounter();
    for (uint8_t eachPin = 0; eachPin < _muxSize; eachPin++) {
      pinMode(_muxPins[eachPin], OUTPUT);
    }
    for (uint8_t eachPin = 0; eachPin < _colSize; eachPin++) {
      pinMode(_colPins[eachPin], INPUT_PULLUP);
    }
  }
  void poll() {
    if (!(_readComplete)) 
    {
      upd_btn_state(
        _gridState[_colCounter][_muxCounter], 
        (digitalRead(_colPins[_colCounter]) == LOW)
      );
      ++_gridCounter;
      _muxCounter = (++_muxCounter) % _muxMaxValue;
      for (uint8_t eachBit = 0; eachBit < _muxSize; eachBit++) 
      {
        digitalWrite(_muxPins[eachBit], (_muxCounter >> eachBit) & 1);
      }
      if (!(_muxCounter)) 
      {
        pinMode(_colPins[_colCounter], INPUT);        // 0V / LOW
        _colCounter = (++_colCounter) % _colSize;
        pinMode(_colPins[_colCounter], INPUT_PULLUP); // +3.3V / HIGH
        _readComplete = !(_colCounter);  // complete when _colcounter and _muxcounter at zero
      }
    }
  }
  bool readTo(std::vector<uint8_t> &refTo) {
    if (_readComplete) 
    {
      for (size_t i = 0; i < _outputMap.size(); i++) 
      {
        for (size_t j = 0; j < _outputMap[i].size(); j++) 
        {
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
  uint8_t _Apin;
  uint8_t _Bpin;
  uint8_t _Cpin;
  bool _invert;
  uint8_t _turnState;
  uint8_t _clickState;
  int8_t  _turnBuffer;
  uint64_t _lastClickTime;
  std::deque<uint64_t> _clickQueue;
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
  std::vector<uint8_t> _buffer;
  uint _bufferSize;
  uint _bufferFree;
  uint _bufferReadPtr;
  uint _bufferWritePtr;
  std::deque<uint8_t> _pwmPins;
  std::deque<uint8_t> _analogPins;
  uint8_t _sampleClockPin;
public:
  void setup_48kHz_clock(uint8_t pin) {
    // given sample Hz = 48000, crystal oscillator = 12mHz
    // using PLL function (datasheet 2.18)
    // cap clock speed at F_CPU = 133mHz
    // FB = 102, POSTDIV1 = 5, POSTDIV2 = 2
    // VCO freq = 1224 mHz, clock speed = 122.4 mHz
    // exactly 2550 CPU cycles in 1/48000 of a second
    // each sample PWM loops (510 CPU cycles) exactly 5 times
    // audio jitter should be almost non-existent
    // !!!
    _sampleClockPin = pin;
    set_sys_clock_pll(12'000'000 * 102, 5, 2);
    uint8_t slice = pwm_gpio_to_slice_num(pin);
    gpio_set_function(pin, GPIO_FUNC_PWM);      // set that pin as PWM
    pwm_set_phase_correct(slice, false);    
    pwm_set_wrap(slice, 2449);                  
    pwm_set_clkdiv(slice, 1.0f);                  // run at full clock speed
    pwm_set_gpio_level(pin, 0);                   // initialize at zero to prevent whining sound
    pwm_clear_irq(slice);
    pwm_set_irq_enabled (slice, true);            // every loop of the timer, trigger the PWM interrupt
    pwm_set_enabled(slice, true);                 // ENGAGE!
  }
  void setup_audio_feed(uint8_t pin, bool isAnalog) {
    if (!isAnalog) {
      _pwmPins.push_back(pin);
      // synthesize 8-bit sound
      // PWM_TOP = 254, with phase correct. Loop every (254 + 1) * 2 = 510 cycles.
      uint8_t slice = pwm_gpio_to_slice_num(pin);
      gpio_set_function(pin, GPIO_FUNC_PWM);      // set that pin as PWM
      pwm_set_phase_correct(slice, true);           
      pwm_set_wrap(slice, 254);                     
      pwm_set_clkdiv(slice, 1.0f);                  // run at full clock speed
      pwm_set_gpio_level(pin, 0);              // initialize at zero to prevent whining sound
      pwm_set_enabled(slice, true);                 // ENGAGE!
    } else {
      _analogPins.push_back(pin);
    }
  }
  void setup_buffer(uint size) {
    _bufferSize = size;
    _bufferFree = size;
    _buffer.resize(size);
    _bufferReadPtr = 0;
    _bufferWritePtr = 0;
  }
  void setup(std::vector<uint8_t> pwmPins, std::vector<uint8_t> analogPins, uint8_t clockPin) {
    setup_48kHz_clock(clockPin);
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
  uint8_t PWMslice() {
    return pwm_gpio_to_slice_num(_sampleClockPin);
  }
};
audioOut_obj audioOut;

static bool on_poll(__unused struct repeating_timer *t) {
  rotary_poll_counter += frequency_poll;
  if (rotary_poll_counter >= frequency_poll_rotary)
  { 
    rotary_poll_counter -= frequency_poll_rotary;
    rotary.poll();
  }
  pinGrid_poll_counter += frequency_poll;
  if (pinGrid_poll_counter >= frequency_poll_pinGrid)
  { 
    pinGrid_poll_counter -= frequency_poll_pinGrid;
    pinGrid.poll();
  }
  return true;
}

static void on_48kHz_tick() {
  pwm_clear_irq(audioOut.PWMslice());
  audioOut.send();          // write the next sample to all audio outs
}

struct repeating_timer io_timer;

void setupTimersCore0() {
  add_repeating_timer_us(frequency_poll, on_poll, NULL, &io_timer);
}

void setupTimersCore1() {
  irq_set_exclusive_handler(PWM_IRQ_WRAP, on_48kHz_tick);
  irq_set_enabled(PWM_IRQ_WRAP, true);      
  pwm_set_irq_enabled(audioOut.PWMslice(), true);
}
