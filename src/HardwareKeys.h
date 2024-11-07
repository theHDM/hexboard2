// appears to work on https://binji.github.io/wasm-clang/
// tested Nov 7 2024


#include <iostream>

#include <vector>
using int_vec = std::vector<unsigned int>;
using int_matrix = std::vector<std::vector<int>>;

#include <deque>
using time_queue = std::deque<unsigned int>;
using byte_queue = std::deque<unsigned int>;

const int_vec muxPins = {4,5,2,3};
const int_vec colPins = {6,7,8,9,10,11,12,13,14,15};
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

// ---------------------------------------

class pinGrid_obj {
    private:
        bool		_isEnabled;
        bool		_cycle_mux_pins_first;
        int_vec   	_muxPins;
        int_vec   	_colPins;
        std::size_t _muxSize;
        std::size_t _colSize;
        int_matrix  _gridState;
        int_matrix  _outputMap;
        unsigned int     _muxCounter;
        unsigned int     _colCounter;
        unsigned int    _gridCounter;
        unsigned int    _muxMaxValue;
        bool        _readComplete;
        void resetCounter() {
            _readComplete = false;
            _gridCounter = 0;
        }
        void init_pin_states() {
            for (auto &m : _muxPins) {
                std::cout << "pinMode(" << m << ", OUTPUT);\n";
            }
            for (auto &c : _colPins) {
                std::cout << "pinMode(" << c << ", INPUT_PULLUP);\n";
            }
        }
        bool advanceMux() {
            _muxCounter = (++_muxCounter) % _muxMaxValue;
            std::cout << "digitalWrite";
            for (unsigned int b = 0; b < _muxSize; b++) {
                std::cout << "(" << _muxPins[b] << ", " << ((_muxCounter >> b) & 1) << "); ";
            }
            return (!(_muxCounter));
        }
        bool advanceCol() {
            std::cout << "pinMode(" << _colPins[_colCounter] << ", INPUT); ";
            _colCounter = (++_colCounter) % _colSize;
            std::cout << "pinMode(" << _colPins[_colCounter] << ", INPUT_PULLUP); ";
            return (!(_colCounter));
        }
    public:    
        void setup(int_vec muxPins, int_vec colPins, int_matrix outputMap, bool cycleMuxFirst = true) {
            _cycle_mux_pins_first = cycleMuxFirst;
            _muxPins = muxPins;
            _muxSize = _muxPins.size();
            _muxMaxValue = (1u << _muxSize);
            _colPins = colPins;
            _colSize = _colPins.size();
            _outputMap = outputMap;
            _gridState.resize(_colSize);
            for (auto& row : _gridState) {
                row.resize(_muxMaxValue);
            }
            _muxCounter = 0;
            _colCounter = 0;
            resetCounter();
            init_pin_states();
        }
        void poll() {
            if (!(_readComplete)) {
                std::cout << "digitalRead(" << _colPins[_colCounter] << ");\n";
                _gridState[_colCounter][_muxCounter] = (0b00011 & ((_gridState[_colCounter][_muxCounter] << 1) + 1));
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
            } else {
                std::cout << "readAlreadyDone\n";
            }
        }
        bool readTo(int_vec &refTo) {
            if (_readComplete) {
                std::cout << "readTo is ready\n";
                for (size_t i = 0; i < _outputMap.size(); i++) {
                    for (size_t j = 0; j < _outputMap[i].size(); j++) {
                        std::cout << i << "/" << j << ": " << _outputMap[i][j] << " gets " <<  _gridState[i][j] << ";\n";
                        refTo[_outputMap[i][j]] = _gridState[i][j];
                    }
                }
                resetCounter();
                return true;
            } else {
                std::cout << "readTo not ready\n";
                return false;
            }
        }
};
pinGrid_obj pinGrid;
int_vec hexagons;
int main() {
    hexagons.resize(160);
    pinGrid.setup(muxPins, colPins, mapGridToPixel); 
    for (int i = 0; i < 159; i++) {
        pinGrid.poll();
    }
    pinGrid.readTo(hexagons);
    pinGrid.poll();
    pinGrid.poll();
    pinGrid.readTo(hexagons);
    return 1;
}
