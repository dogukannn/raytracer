#pragma once

#include "vec3.h"

#include <iostream>

void write_color(std::ostream &out, const color& pixelColor, int samplesPerPixel)
{
	auto scale = 1.0 / samplesPerPixel;
	auto r = std::sqrt(pixelColor.x() * scale);
	auto g = std::sqrt(pixelColor.y() * scale);
	auto b = std::sqrt(pixelColor.z() * scale);

	
	out << static_cast<int>(256 * clamp(r, 0.0, 0.999)) << ' '
		<< static_cast<int>(256 * clamp(g, 0.0, 0.999)) << ' '
		<< static_cast<int>(256 * clamp(b, 0.0, 0.999)) << std::endl;
}