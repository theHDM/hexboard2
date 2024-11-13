#pragma once
#include "utils.h"
#include "color.h"

enum class generate_gradient {
	shade_tint,
	define_stops,
	rainbow,
	kite
}; // e.g. generate_gradient::shade

// this object is initialized from a preset library
// and the output is a map of pixel codes which can
// be recalled based on a given stop position (from 0 to 1)
struct gradient_t {
	std::map<float, pixel_code> pixel_code_map;
	pixel_code lookup(float pos) {
		return pixel_code_map.lower_bound(pos);
	}

	size_t resolution; // number of steps minus one... i.e. [0..1>
	// more steps = smoother gradient, but longer to calculate
	generate_gradient method;
	// shade_tint base color inputs; -1 values if "untouched"
	HSV_color base_hsv = {-1,-1,-1};
	sRGB_color base_rgb = {-1,-1,-1};
	okLCH_color base_oklch = {-1,-1,-1};
	// define_stops options
	std::map<float, HSV_color> stops_hsv = {{-1,-1}};
	std::map<float, sRGB_color> stops_rgb = {{-1,-1}};
	std::map<float, okLCH_color> stops_oklch = {{-1,-1}};
	std::map<float, okLAB_color> stops_converted;
	// rainbow options HSL, HSV, HCL
	float constant_saturation = -1;
	float constant_chroma = -1;
	float constant_value = -1;
	float constant_lightness = -1;
	
	void generate_rainbow(int const_sat, int const_val) {
		// generate a rainbow of hues
		// direct calculation; no interpolation needed
		//
		// table[i] = color.as_neoPixel();
	}
	// kite options similar to rainbow i think.
	
	void generate_gradient() {
		// interpolate gradient colors and convert
		// to neopixel format ahead of time
		// overloaded lerp to blend colors perceptually
		for (float i = 0; i < 1; i += 1/(number_of_steps - 1)) {
			pixel_code_map[i] = convert_to_neoPixel(
				lerp_over_map(stops_converted, i)
			);
		}
	}
	void generate_defined_stops() {
		// convert any entered stops into okLAB
		for (auto i : stops_hsv) {
			if (i->first >= 0 && i->first <= 1) {
				stops_converted[i->first] = convert_to_okLAB(i->second);
			}
		}
		for (auto i : stops_rgb) {
			if (i->first >= 0 && i->first <= 1) {
				stops_converted[i->first] = convert_to_okLAB(i->second);
			}
		}
		for (auto i : stops_oklch) {
			if (i->first >= 0 && i->first <= 1) {
				stops_converted[i->first] = convert_to_okLAB(i->second);
			}
		}
		generate_gradient();
	}
	void generate_shade_tint() {
		stops_converted[0] = {0, 0, 0}; // black
		stops_converted[0] = {1, 0, 0}; // white
		if (base_oklch.l >= 0) {
			stops_converted[0.5] = convert_to_okLAB(base_oklch);
		} else if (base_hsv.v >= 0) {
			stops_converted[0.5] = convert_to_okLAB(base_hsv);
		} else if (base_rgb.r >= 0) {
			stops_converted[0.5] = convert_to_okLAB(base_rgb);
		}
		generate_gradient();
	}
}

// this defines a reactive value.
// 1. a pointer to an integer to "react" to
// 2. a map from values that integer could be, to reaction positions [0..1]
// at any point you can call pos() and it should real-time update.
struct reactive_position_t {
	int* ptr_to_reactive_value; // i.e. timer, key pressure, wheel
	std::map<int,float> rule_table; // e.g. -1 = 0.0  0 = 0.5  127 = 1.0
	float pos = lerp_over_map(rule_table, *ptr_to_reactive_value);
}

// this defines a "coloring rule", and contains
// 1. a gradient map of colors
// 2. a reactive position on the map
// call "get_color" to lookup the color that should be shown
//
struct coloring_rule_t {
	gradient_t gradient;        // color lookup table
	reactive_position_t react;
	pixel_code get_color() {
		return gradient.lookup(react.pos());
	}
}
