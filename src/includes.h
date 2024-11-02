#pragma once

#include <Arduino.h>

#include <vector>
using byte_vec = std::vector<uint8_t>;
using int_matrix = std::vector<std::vector<int>>;

#include <deque>
using time_queue = std::deque<uint64_t>;
using byte_queue = std::deque<uint8_t>;

template <typename T> T lerp(T xOne, T xTwo, float yOne, float yTwo, float y) {
  if (yOne == yTwo) {
    return xOne;
  } else if (y <= yOne) {
    return xOne;
  } else if (y >= yTwo) {
    return xTwo;
  } else {
    float w = (y - yOne) / (yTwo - yOne);
    return (xOne * (1 - w)) + (xTwo * w);
  }
}

struct hexCoord {  // axial coordinate system
  int q; // diagonal (upleft-downright); - downLeft, + upRight
  int r; // row (left-right); - up, + down
};