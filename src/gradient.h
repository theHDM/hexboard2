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
	// pixel_code is the neoPixel integer format
	pixel_code lookup(float pos) {
		return pixel_code_map.lower_bound(pos);
	}
	size_t resolution; // number of steps minus one... i.e. [0..1>
	// more steps = smoother gradient, but longer to calculate
	
	generate_gradient method;
	// shade_tint options
		// HSV_color_type base_hsv;
		// RGB_color_type base_rgb;
		// OKLCH_color_type base_oklch;
	// define_stops options
		// std::map<float, HSV_color_type> stops_hsv;
		// std::map<float, RGB_color_type> stops_rgb;
		// std::map<float, OKLCH_color_type> stops_oklch;
	// rainbow options HSL, HSV, HCL
		// float constant_saturation;
		// float constant_chroma;
		// float constant_value;
		// float constant_lightness;
	void generate_rainbow(int const_sat, int const_val) {
		// generate a rainbow of hues
		// direct calculation; no interpolation needed
		//
		// table[i] = color.as_neoPixel();
	}
	// kite options similar to rainbow i think.
	
	// use the rgb/hsv color type, not the colorCode
	void generate_gradient(convert to other kinds);
	void generate_gradient(std::map<float, color_t> gradient_map) {
		// interpolate gradient colors and convert
		// to neopixel format ahead of time
		// overloaded lerp to blend colors perceptually
		// then you can do

		// convert gradient map into OKLAB
		for (float i = 0; i < 1; i += 1/(number_of_steps - 1)) {
			
			color_code_map[i] = linterp(convert_to_gradient_map, i).as_neoPixel;
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
