#pragma once
#include "syntacticSugar.h"

enum {
  no_instruction = 0,
  note_on = 1,
  note_off = 2,
  control_change = 3,
  program_change = 4,
  palette_swap = 5,
  preset_swap = 6,
  hardware_dip = 7,
  instruction_count
};

struct instruction_t {
  int instruction_code;
  int parameter;
};

struct instruction_set_t {
  instruction_t stays_off;
  instruction_t turns_off;
  instruction_t turns_on;
  instruction_t stays_on_pressure_equal;
  instruction_t stays_on_pressure_increase;
  instruction_t stays_on_pressure_decrease;
};

struct hex_button_t {
  int current_pressure;  // -1 means off. 0..127 means on and send key pressure
  int prior_pressure;
  instruction_set_t on_key;
  instruction_t get_instructions() {
    if (current_pressure > prior_pressure) {
      if (prior_pressure == -1) {
        return on_key.turns_on;
      } else {
        return on_key.stays_on_pressure_increase;
      }
    } else if (current_pressure < prior_pressure) {
      if (current_pressure == -1) {
        return on_key.turns_off;
      } else {
        return on_key.stays_on_pressure_decrease;
      }
    } else if (current_pressure == -1) {
      return on_key.stays_off;
    } else {
      return on_key_stays_on_pressure_equal;
    }
  }
};

struct keyboard_obj {
  int sizeA,
  int sizeB,
  int_matrix state_matrix;
  int key_on_threshold;
  int pressure_min_threshold;
  int pressure_max_threshold;
  int_matrix map_to_array;
  vector<hex_button_t> hex;
  void update_hex(hex_button_t& h, int p) {
    h.prior_pressure = h.current_pressure;
    h.current_pressure = p;
  }
  int key_state_to_pressure(int state_value) {
    if (state_value < key_on_threshold) {
      return -1;
    }
    if (state_value < pressure_min_threshold) {
      return 0;
    }
    if (state_value > pressure_max_threshold) {
      return 127;
    }
    return lerp(0,127,pressure_min_threshold,pressure_max_threshold,state_value);
  }
  void setup(int cols, int rows) {
    sizeA = cols;
    sizeB = rows;
    resize_matrix(state_matrix, cols, rows);
    hex.resize(cols * rows);
  }
  void panic() {
    for (auto c : state_matrix) {
      fill(c.begin(),c.end(),-1);
    }
  }
  void update(int_matrix read_new_state) {
    state_matrix = read_new_state;
    for (int i = 0; i < sizeA; ++i) {
      for (int j = 0; j < sizeB; ++j) {
        update_hex(
          hex[map_to_array[i][j]], 
          key_state_to_pressure(state_matrix[i][j])
        );
      }
    }
  }
};
