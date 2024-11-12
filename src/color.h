#pragma once
#include "syntacticSugar.h"
#include "convertColors.h"


struct gradient_t {
	size_t number_of_steps; // even steps of 0 thru 1 inclusive
	// colorCode is the neoPixel integer format
	std::map<float, colorCode> color_code_map;
	colorCode lookup(float loc) {
		return color_code_table.lower_bound(loc);
	}
	
	void generate_rainbow(int const_sat, int const_val) {
		// generate a rainbow of hues
		// direct calculation; no interpolation needed
		//
		// table[i] = color.as_neoPixel();
	}
	// use the rgb/hsv color type, not the colorCode
	color_t lerp(color_t colorOne, color_t colorTwo,
		float yOne, float yTwo, float y) {
		// get colors as OKLCH
		// perceptual blend formula
		// return appropriate color_t
	}
	void generate_gradient(std::map<float, color_t> gradient_map) {
		// interpolate gradient colors and convert
		// to neopixel format ahead of time
		// overloaded lerp to blend colors perceptually
		// then you can do
		for (float i = 0; i < 1; i += 1/(number_of_steps - 1)) {
			color_code_map[i] = lerp_over_map(gradient_map, i).as_neoPixel;
		}
	}
	// use the rgb/hsv color type, not the colorCode
	void generate_shades_of(/* HSV base_color*/) {
		// TBD
		std::map<float, color_t> shade;
		shade[0] = /* black */;
		shade[0.5] = base_color;
		shade[1] = /* white */;
		generate_gradient(shade);
	}
}

struct reactive_coloring_t {
	gradient_t gradient;        //color lookup table
	int* ptr_to_reactive_value; //i.e. timer, key pressure, wheel
	std::map<int,float> rule_table; // e.g. -1 = 0.0  0 = 0.5  127 = 1.00
	colorCode get_color() {
		return gradient.lookup(
			lerp_over_map(
				rule_table, *ptr_to_reactive_value
			)
		);
	}
	
}
