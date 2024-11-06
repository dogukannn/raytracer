#pragma once

#include <cmath>
#include <limits>
#include <cfloat>     
#include <memory>


const float infinity = std::numeric_limits<float>::infinity();
const float pi = 3.1415926535897932385f;

inline double degreesToRadians(double degrees)
{
	return degrees * pi / 180.0;
}

inline double randomDouble()
{
	return rand() / (RAND_MAX + 1.0);
}

inline double randomDouble(double min, double max)
{
	return min + (max - min) * randomDouble();
}


inline float randomFloat()
{
	return rand() / (RAND_MAX + 1.0f);
}

inline float randomFloat(float min, float max)
{
	return min + (max - min) * randomFloat();
}

inline double clamp(double x, double min, double max)
{
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

#include "ray.h"
#include "vec3.h"






