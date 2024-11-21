#pragma once

#include "common.h"
#include "parser.h"

struct camera
{
	point3 origin;
	point3 lowerLeftCorner;
	vec3 horizontal;
	vec3 vertical;
	vec3 u, v, w;
	double lensRadius;

	camera(point3 lookfrom,
	       point3 lookat,
	       vec3 vup,
	       parser::Vec4f near_plane, 
	       double aspectRatio,
	       double aperture,
	       double focusDist)
	{

		w = unit(lookfrom - lookat);
		u = unit(cross(vup, w));
		v = cross(w, u);

		origin = lookfrom;
		horizontal = (near_plane.y - near_plane.x) * u;
		vertical = (near_plane.w - near_plane.z) * v;
		lowerLeftCorner = origin - (horizontal / 2.0) - (vertical / 2.0) - focusDist * w;

		lensRadius = aperture / 2;
	}

	ray getRay(float s, float t) const
	{
		return ray(origin,  unit(lowerLeftCorner + s * horizontal + t * vertical - origin));

	}
};
