#pragma once
#include "utils.h"

// color definitions
struct HSV_color {
	float h; // 0..360
	float s; // 0..1
	float v; // 0..1
}
struct sRGB_color {
	int r; // 0..255
	int g; // 0..255
	int b; // 0..255
}
struct lRGB_color {
	float r; // 0..1
	float g; // 0..1
	float b; // 0..1
}
struct okLCH_color {
	float l; // 0..1
	float c; // 0..1
	float h; // 0..360
}
struct okLAB_color {
	float l; // 0..1
	float a; // -1..1
	float b; // -1..1
}

// conversion functions
int scaleTo255(float f) {
	return clip(round(255.0 * f),0,255);
}

sRGB_color convert_to_sRGB (HSV_color from) {
	int sextant = (int)std::floor(from.h / 60);
	float c = from.v * from.s;
	float x = c * (1 - std::abs(std::fmod(from.h / 60, 2) - 1));
	float m = from.v - c;
	int max = scaleTo255(c + m);
	int med = scaleTo255(x + m);
	int min = scaleTo255(    m);
	switch (sextant) {
		case 0: // red + some green
			return {max,med,min}; break;
		case 1: // green + some red
			return {med,max,min}; break;
		case 2: // green + some blue
			return {min,max,med}; break;
		case 3: // blue + some green
			return {min,med,max}; break;
		case 4: // blue + some red
			return {med,min,max}; break;
		default: // red + some blue
			return {max,min,med}; break;
	}
}
float gamma(float l) {
	return (l >= 0.0031308) ? (1.055 * pow(l, 1.0/2.4) - 0.055) : (12.92 * l);
}
sRGB_color convert_to_sRGB (lRGB_color from) {
	return {
		scaleTo255(gamma(from.r)),
		scaleTo255(gamma(from.g)),
		scaleTo255(gamma(from.b))
	};
}
float ungamma(float f) {
	return ((f > 0.04045) ? std::pow((f + 0.055) / 1.055, 2.4) : (f / 12.92));
}
lRGB_color convert_to_lRGB (sRGB_color from) {
	return {
		ungamma((float)from.r / 255.0),
		ungamma((float)from.g / 255.0),
		ungamma((float)from.b / 255.0)
	};
}
okLAB_color convert_to_okLAB (okLCH_color from) {
	cartesian ab = polar_to_cartesian(from.c, from.h);
	return {from.l, ab.x, ab.y};
}

okLAB_color convert_to_okLAB (lRGB_color from) {
	float Lg = 0.4121656120 * from.r + 0.5362752080 * from.g + 0.0514575653 * from.b;
	float Md = 0.2118591070 * from.r + 0.6807189584 * from.g + 0.1074065790 * from.b;
	float Sh = 0.0883097947 * from.r + 0.2818474174 * from.g + 0.6302613616 * from.b;
	Lg = std::cbrt(Lg);
	Md = std::cbrt(Md);
	Sh = std::cbrt(Sh);
	return {
		0.2104542553 * Lg + 0.7936177850 * Md - 0.0040720468 * Sh,
		1.9779984951 * Lg - 2.4285922050 * Md + 0.4505937099 * Sh,
		0.0259040371 * Lg + 0.7827717662 * Md - 0.8086757660 * Sh
	};
}
okLAB_color convert_to_okLAB (sRGB_color from) {
	return convert_to_okLAB(
		convert_to_lRGB(from)
	);
}
okLAB_color convert_to_okLAB (HSV_color from) {
	return convert_to_okLAB(
		convert_to_sRGB(from)
	);
}

lRGB_color convert_to_lRGB (okLAB_color from) {
	float Lg = from.l + 0.3963377774 * from.a + 0.2158037573 * from.b;
	float Md = from.l - 0.1055613458 * from.a - 0.0638541728 * from.b;
	float Sh = from.l - 0.0894841775 * from.a - 1.291485548  * from.b;
	Lg = pow(Lg, 3);
	Md = pow(Md, 3);
	Sh = pow(Sh, 3);
	float r =  4.0767416621 * Lg - 3.3077115913 * Md + 0.2309699292 * Sh;
	float g = -1.2684380046 * Lg + 2.6097574011 * Md - 0.3413193965 * Sh;
	float b = -0.0041960863 * Lg - 0.7034186147 * Md + 1.707614701  * Sh;
	return { clip(r,0.0,1.0), clip(g,0.0,1.0), clip(b,0.0,1.0) };
}

pixel_code convert_to_neoPixel (lRGB_color from) {
	int r = (scaleTo255(from.r) & 0xFF); // just to be sure
	int g = (scaleTo255(from.g) & 0xFF); // just to be sure
	int b = (scaleTo255(from.b) & 0xFF); // just to be sure
	return ((r << 16) | ((g << 8) | b));   // return 24-bit "un-gamma'd" RGB
}
pixel_code convert_to_neoPixel (okLAB_color from) {
	return convert_to_neoPixel(convert_to_lRGB(from));
}

// overload linterp function so that you can
// interpolate two LAB colors and return the
// pixel code.

okLAB_color linterp(okLAB_color colorOne, okLAB_color colorTwo,
									float yOne, float yTwo, float y) {
	return {
		linterp(colorOne.l, colorTwo.l, yOne, yTwo, y),
		linterp(colorOne.a, colorTwo.a, yOne, yTwo, y),
		linterp(colorOne.b, colorTwo.b, yOne, yTwo, y)
	};
}

// for Kite colors and rainbow
// need to also bring over the maximum chroma / saturation
// from kitecolors.cpp
