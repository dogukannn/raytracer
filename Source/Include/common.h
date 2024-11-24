#pragma once

#include <cmath>
#include <limits>
#include <cfloat>     
#include <memory>
#include <random>


const float infinity = std::numeric_limits<float>::infinity();
const float pi = 3.1415926535897932385f;

inline float motion_blur_mp = 0.0f;

inline float sample_u_offset = 0.0f;
inline float sample_v_offset = 0.0f;

inline float lens_x_offset = 0.0f;
inline float lens_y_offset = 0.0f;

inline float area_light_u_offset = 0.0f;
inline float area_light_v_offset = 0.0f;

inline float roughness_u_offset = 0.0f;
inline float roughness_v_offset = 0.0f;

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

inline std::mt19937 gen;
inline std::uniform_real_distribution<float> dis(0.0f, 1.0f);

inline float frandom()
{
	return dis(gen);
}

#include "ray.h"
#include "vec3.h"


inline vec3 create_non_colinear_vector(vec3 v)
{
	if(abs(v.x()) < abs(v.y()) && abs(v.x()) < abs(v.z()))
	{
		return vec3(1.0f, v.y(), v.z());
	}
	else if (abs(v.y()) < abs(v.x()) && abs(v.y()) < abs(v.z()))
	{
		return vec3(v.x(), 1.0f, v.z());
	}
	else
	{
		return vec3(v.x(), v.y(), 1.0f);
	}
}






