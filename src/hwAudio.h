#pragma once
#include "hardware/pwm.h"       // library of code to access the processor's built in pulse wave modulation features
#include "syntacticSugar.h"

class ringBuffer_obj {
private:
    int_vec buffer;
    int indexRead;
    int indexWrite;
    int capacity;
    int spaceLeft;
public:
    ringBuffer_obj(int capacity) {
        this->capacity = capacity;
        this->spaceLeft = capacity;
        this->indexRead = 0;
        this->indexWrite = 0;
        buffer.resize(capacity);
    }
    int roomToWrite() {
        return spaceLeft;
    }
    int leftToRead() {
        return capacity - spaceLeft;
    }
    void write(int element) {
        buffer[indexWrite] = element;
        indexWrite = (indexWrite + 1) % capacity;
        --spaceLeft;
    }
    int read() {
        int element = buffer[indexRead];
        indexRead = (indexRead + 1) % capacity;
        ++spaceLeft;
        return element;
    }
};

ringBuffer_obj audioBuffer(128);

void setupAudio(int_vec pins) {
  for (auto pin : pins) {
    gpio_set_function(pin, GPIO_FUNC_PWM);      // set that pin as PWM
    int slice = pwm_gpio_to_slice_num(pin);
    pwm_set_phase_correct(slice, true);           // phase correct sounds better
    pwm_set_wrap(slice, 254);                     // 0 - 254 allows 0 - 255 level
    pwm_set_clkdiv(slice, 1.0f);                  // run at full clock speed
    pwm_set_gpio_level(pin, 0);        // initialize at zero to prevent whining sound
    pwm_set_enabled(slice, true);                 // ENGAGE!
  }
}

void audioPoll() {
  int level = 0;
  if (audioBuffer.leftToRead() > 0) {
    level = audioBuffer.read();
  }
  for (auto pin : pwmPins) {
    pwm_set_gpio_level(pin, level);      
  }
}