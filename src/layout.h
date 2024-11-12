#pragma once
#include "utils.h"

enum class command {
  none = 0,
  send_note_on = 1,
  send_note_off = 2,
  send_control_change = 3,
  send_program_change = 4,
  swap_palette = 5,
  swap_preset = 6,
  hardware_dip = 7,
  count
};

enum class param { // parameter codes
	null = -1,
	go_to_prev = 0,
  go_to_next = 1
}

struct instruction_t {
  command do = command::none;
  param param_1 = param::null;
  param param_2 = param::null;
};
/* 
instruction_t do_nothing;
instruction_t do_note_on = {.do = command::send_note_on};
instruction_t do_next_preset = {command::swap_preset, param::go_to_next};
*/
struct instruction_set_t {
  instruction_t on_press;  // default state = do nothing
  instruction_t on_release;
  instruction_t on_hold;
  instruction_t when_off;
};
/*
instruction_set_t music_note = {
	.on_press = do_note_on,
  .on_hold = do_note_change_pressure,
  .on_release = {command::send_note_off},
	.when_off = do_nothing // optional
};
instruction_set_t get_next_preset = {
	.on_press = do_next_preset
};
*/
struct button {
	// how to locate this button
	hex_t coordinates;
	int pixel_num;
	// history of the digital/analog read values
	std::map<time_uS,int> state;
  void update(int value, time_uS read_time) {
		state[read_time] = value;
		// state.clear() some range;
  }
	// the type of button expresses what
	// the button executes when it's acted on
	// e.g. this->type = music_note;
	// this should be set via the layout definition
	instruction_set_t type;
  instruction_t get_instructions() {
    // decide when to call:
		return type.on_press;
    // decide when to call:
		return type.on_hold;
    // decide when to call:
		return type.on_release;
    // decide when to call:
		return type.when_off;
	}
	// stored information related to this key
	// such as musical note, channel number,
	// key color rule, animation layer,
	// should be set via the layout definition
	// TO BE LINKED!
	//
};
// interface to set up hex button array,
// pass digital/analog state,
// and return/map/find by pixel/coordinates
struct keyboard_obj {
  vector<button> hex;
	button empty_hex; // unused hex when search fails
  int sizeA,
  int sizeB,
  int_matrix state_to_index; // one-to-one map, 2d coord to 1d coord
	std::map<int,int> pixel_to_index; // some-to-one map
	std::map<int,int> index_to_pixel; // on setup, flip to invert
	std::map<int,hex_t> index_to_coord; // map is not reversible

  void setup(int cols, int rows, 
						std::map<int,int> given_map_pixel_to_index,
						std::map<int,hex_t> given_map_index_to_coord) {
    sizeA = cols;
    sizeB = rows;
    resize_matrix(state_matrix, cols, rows);
    hex.resize(cols * rows);
		index_to_coord = given_map_index_to_coord;
		pixel_to_index = given_map_pixel_to_index;
		index_to_pixel = invert(pixel_to_index);
		for (int i = 0; i < hex.size(), ++i) {
			auto search = index_to_pixel.find(i);
			if (search == index_to_pixel.end()) {
				hex[i].pixel_num = -1;   // which means not-a-pixel
			} else {
				hex[i].pixel_num = search->second;
			}
			hex[i].coordinates = index_to_coord[i];
		}
  }

	// you can normally refer to a single button
	// by hex[i] where i is the given index.
	// but you should also be able to locate
	// the hex using a lookup from the map.
	// it will return hex.end() if not found --
	// check for this condition before doing anything
	// in the main sequence
	
	button& hex_at_pixel(int at_pixel_number) {
		auto search = pixel_to_index.find(at_pixel_number);
		if (search == pixel_to_index.end()) {
			return empty_hex;
		}
		return hex[search->second];
	}
	button& hex_at_coord(hex_t at_coord) {
		for (auto& i : hex) {
			if (i.coordinates == at_coord) {
				return i;
			}
		}
		return empty_hex;
	}
	// e.g.
  //	hex_at_coord({-3, 5}).animate = 1;
  void panic() {
    for (auto c : state_matrix) {
      fill(c.begin(),c.end(),-1);
    }
  }
  void update(int_matrix read_new_state, int_matrix read_time) {
    for (int i = 0; i < sizeA; ++i) {
      for (int j = 0; j < sizeB; ++j) {
				hex[map_to_array[i][j]].update(
          read_new_state[i][j], read_time[i][j]
				);
      }
    }
  }
};
