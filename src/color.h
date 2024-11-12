#pragma once
#include "utils.h"

struct RGB_color {
	float r;
	float g;
	float b;
}
struct HSV_color {
	float h;
	float s;
	float v;
}
HSV_color RGB_to_HSV (RGB_color from) {
	
}
RGB_color HSV_to_RGB (HSV_color from) {
	
}
struct LCH_color {
	float l;
	float c;
	float h;
}
LCH_color HSV_to_LCH (HSV_color from) {
	
}
LCH_color RGB_to_LCH (RGB_color from) {
	return HSV_to_LCH(RGB_to_HSV(RGB_color));
}
HSV_color LCH_to_HSV (LCH_color from) {
	
}
RGB_color LCH_to_RGB (LCH_color from) {
	
}


// define color types
// conversions
// overload function
color_t lerp(color_t colorOne, color_t colorTwo,
		float yOne, float yTwo, float y) {
	// get colors as OKLCH
	// perceptual blend formula
	// return appropriate color_t
}
