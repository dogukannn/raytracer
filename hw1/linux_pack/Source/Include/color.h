#pragma once

#include "vec3.h"

#include <iostream>

void write_color(std::ostream &out, const color& pixelColor)
{
	out << static_cast<int>(clamp(pixelColor.x(), 0.0, 255.999)) << ' '
		<< static_cast<int>(clamp(pixelColor.y(), 0.0, 255.999)) << ' '
		<< static_cast<int>(clamp(pixelColor.z(), 0.0, 255.999)) << std::endl;
}