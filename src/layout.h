#pragma once
#include "utils.h"

enum { // instruction codes
  no_instruction = 0,
  send_note_on = 1,
  send_note_off = 2,
  send_control_change = 3,
  send_program_change = 4,
  swap_palette = 5,
  swap_preset = 6,
  hardware_dip = 7,
  instruction_count
};

enum { // parameter codes
	null_param = -1,
	go_to_prev = 0,
  go_to_next = 1
}

struct instruction_t {
  int instruction_code = no_instruction;
  int parameter_1 = null_param;
  int parameter_2 = null_param;
};
/* 
instruction_t do_nothing;
instruction_t do_note_on = {.instruction_code = send_note_on};
instruction_t do_next_preset = {swap_preset, go_to_next};
*/
struct instruction_set_t {
  instruction_t on_press;  // default state = no instruction
  instruction_t on_release;
  instruction_t on_hold;
  instruction_t neutral;
};
/*
instruction_set_t music_note = {
	.on_press = do_note_on,
  .on_hold = do_note_change_pressure,
  .on_release = do_note_off,
	.neutral = do_nothing // optional
};
instruction_set_t get_next_preset = {
	.turns_on = do_next_preset
};
*/
struct hex_button_t {
	hexagon_coord_t coordinates;
	int pixel;
	
	int current_pressure;  
	//     -1 means off
	// 0..127 means on and send key pressure
	int prior_pressure;
	instruction_set_t on_key;

  void update(int p) {
    prior_pressure = current_pressure;
    current_pressure = p;
  }
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
      return on_key.stays_on_pressure_equal;
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

  int_matrix state_to_index;
  vector<hex_button_t> hex;
	
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
				hex[map_to_array[i][j]].update(
          key_state_to_pressure(state_matrix[i][j])
        );
      }
    }
  }
};
