#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "syntacticSugar.h"

// appears to work on https://binji.github.io/wasm-clang/
// tested Nov 7 2024
class pinGrid_obj {
    private:
        bool _isEnabled;
        bool _cycle_mux_pins_first;
        int_vec _colPins;
        int_vec _muxPins;
        std::size_t _colSize;
        std::size_t _muxSize;
        int_matrix _gridState;
        int _colCounter;
        int _muxCounter;
        int _gridCounter;
        int _muxMaxValue;
        bool _readComplete;
        void resetCounter() {
            _readComplete = false;
            _gridCounter = 0;
        }
        void init_pin_states() {
            for (auto &m : _muxPins) {
                pinMode(m, OUTPUT);
            }
            for (auto &c : _colPins) {
                pinMode(c, INPUT_PULLUP);
            }
        }
        bool advanceCol() {
            pinMode(_colPins[_colCounter], INPUT);
            _colCounter = (++_colCounter) % _colSize;
            pinMode(_colPins[_colCounter], INPUT_PULLUP);
            return (!(_colCounter));
        }
        bool advanceMux() {
            _muxCounter = (++_muxCounter) % _muxMaxValue;
            for (int b = 0; b < _muxSize; b++) {
                digitalWrite(_muxPins[b], (_muxCounter >> b) & 1);
            }
            return (!(_muxCounter));
        }
    public:    
        void setup(int_vec colPins, int_vec muxPins, bool cycleMuxFirst = true) {
            _cycle_mux_pins_first = cycleMuxFirst;
            _colPins = colPins;
            _colSize = _colPins.size();
            _muxPins = muxPins;
            _muxSize = _muxPins.size();
            _muxMaxValue = (1u << _muxSize);
            resize_matrix(_gridState, _colSize, _muxMaxValue);
            _colCounter = 0;
            _muxCounter = 0;
            resetCounter();
            init_pin_states();
        }
        void poll() {
            if (!(_readComplete)) {
                _gridState[_colCounter][_muxCounter] = 
                  digitalRead(_colPins[_colCounter]);
                //  analogRead(_colPins[_colCounter]);
                ++_gridCounter;
                if (_cycle_mux_pins_first) {
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
        bool readTo(int_matrix &refTo) {
            if (_readComplete) {
                for (size_t i = 0; i < _colSize; i++) {
                    for (size_t j = 0; j < _muxMaxValue; j++) {
                        refTo[i][j] = _gridState[i][j];
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