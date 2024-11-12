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
#include <queue> // make sure that all queue objects are removed and changed to d.e.queue.
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

// function that guarantees the mod value is always positive.
int positiveMod(int n, int d) {
  return (((n % d) + d) % d);
}
// extensions of linear interpolation used in this project

// eventually replace this one with template lerp
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

#include <map>
using lerp_map_t = std::map<float, int>;
using lerp_inverse_map_t = std::map<int, float>;
lerp_inverse_map_t invert(lerp_map_t lerp_map) {
  lerp_inverse_map_t temp;
  for (auto i : lerp_map) {
    temp[i->second] = i->first;
  }
  return temp;
}
template <typename T_key, typename T_value>
T_value lerp_over_map(const std::map<T_key, T_value>& lerp_map, T_key pos) {
  auto LB = lerp_map.lower_bound(pos);
  auto UB = std::next(LB);
  return lerp(LB->second, UB->second, LB->first, UB->first, pos);
}

/*
  it would be nice to have a wrapper for the queue function
  to return the front of the queue and pop it, because
  it doesn't seem to do that automatically?
*/
