#pragma once
#include "syntacticSugar.h"





// send this to syntactic sugar!

struct lerp_point {
	int value;
	float loc;
}
using lerp_vec = std::vector<lerp_point>;
int lerp_by_loc(lerp_vec lerp_points, float loc) {
	// LERP over this line
}
float lerp_by_value(lerp_vec lerp_points, int value) {
	// LERP over this line
}

// -------------
struct RGB {
	int R;
	int G;
	int B;
}

struct HSV {
	int H;
	int S;
	int V;
}

struct color {
	int red;
	int green;
	int blue;
	
}







struct color_table_t {
	std::vector<colorCode /*in adafruit format*/> table;
	colorCode lookup(float loc) {
		size_t index = /* lerp over size */ ;
		return table[index];
	}
	void generate_rainbow(int const_sat, int const_val) {
		// TBD
		// table[i] = strip.gamma32(ColorHSV(HSV));
	}
	void generate_gradient(lerp_vec gradient_points) {
		// TBD
		// table[i] = strip.gamma32(ColorHSV(HSV));
	}
	void generate_shades_of(/* HSV base_color*/) {
		// TBD
		lerp_vec temp;
		temp.emplace_back(/* black */, 0);
		temp.emplace_back(base_color, 0.5);
		temp.emplace_back(/* white */, 1);
		generate_gradient(temp);
	}
}

struct reactive_coloring_t {
	color_table_t gradient;        //color lookup table
	int* ptr_to_reactive_value; //i.e. timer, key pressure, wheel
	lerp_vec rule_table;        //choose color index based on value
	colorCode get_color() {
		return gradient.lookup(lerp_by_value(
			rule_table, *ptr_to_reactive_value
		));
	}
}