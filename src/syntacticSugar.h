#pragma once

using colorCode = unsigned long int;

#include <vector>
using int_vec = std::vector<int>;
using int_matrix = std::vector<std::vector<int>>;

void resize_matrix(int_matrix& m, size_t sizeA, size_t sizeB) {
  m.resize(sizeA);
  for (auto& a : m) {
    a.resize(sizeB);
  }
}

#include <functional>
using func = std::function<void()>;

#include <deque>
#include <queue>                // standard C++ library construction to store open channels in microtonal mode (use "std::queue" to invoke it)
using time_queue = std::deque<unsigned long long int>;
using int_queue = std::deque<int>;

#include <string>               // standard C++ library string classes (use "std::string" to invoke it); these do not cause the memory corruption that Arduino::String does.
using str = std::string;

enum {
  btn_off     = 0b00,
  btn_press   = 0b01,
  btn_release = 0b10,
  btn_hold    = 0b11
};

/*
  C++ returns a negative value for 
  negative N % D. This function
  guarantees the mod value is always
  positive.
*/
int positiveMod(int n, int d) {
  return (((n % d) + d) % d);
}
/*
  There may already exist linear interpolation
  functions in the standard library. This one is helpful
  because it will do the weighting division for you.
  It only works on byte values since it's intended
  to blend color values together. A better C++
  coder may be able to allow automatic type casting here.
*/
byte byteLerp(byte xOne, byte xTwo, float yOne, float yTwo, float y) {
  float weight = (y - yOne) / (yTwo - yOne);
  int temp = xOne + ((xTwo - xOne) * weight);
  if (temp < xOne) {temp = xOne;}
  if (temp > xTwo) {temp = xTwo;}
  return temp;
}

template <typename T>
T lerp(T xOne, T xTwo, float yOne, float yTwo, float y) {
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
/*
  it would be nice to have a wrapper for the queue function
  to return the front of the queue and pop it, because
  it doesn't seem to do that automatically?
*/