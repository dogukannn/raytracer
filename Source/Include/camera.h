#pragma once

#include "common.h"

struct camera
{
	point3 origin;
	point3 lowerLeftCorner;
	vec3 horizontal;
	vec3 vertical;

	camera()
	{
		constexpr double viewportHeight = 2.0;
		constexpr double viewportWidth = aspectRatio * viewportHeight;
		constexpr double focalLength = 1.0;

		origin = point3(0, 0, 0);
		horizontal = vec3(viewportWidth, 0, 0);
		vertical = vec3(0, viewportHeight, 0);
		lowerLeftCorner = origin - (horizontal / 2.0) - (vertical / 2.0) - vec3(0, 0, focalLength);
	}

	ray getRay(double u, double v) const
	{
		return ray(origin, lowerLeftCorner + u * horizontal + v * vertical - origin);
	}
};