#pragma once

#include "common.h"

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
	       double vfov, 
	       double aspectRatio,
	       double aperture,
	       double focusDist)
	{
		auto theta = degreesToRadians(vfov);
		auto h = tan(theta / 2);
		double viewportHeight = 2.0 * h;
		double viewportWidth = aspectRatio * viewportHeight;

		w = unit(lookfrom - lookat);
		u = unit(cross(vup, w));
		v = cross(w, u);

		origin = lookfrom;
		horizontal = focusDist * viewportWidth * u;
		vertical = focusDist *  viewportHeight * v;
		lowerLeftCorner = origin - (horizontal / 2.0) - (vertical / 2.0) - focusDist * w;

		lensRadius = aperture / 2;
	}

	ray getRay(double s, double t) const
	{
		vec3 rd = lensRadius * randomInUnitSphere();
		vec3 offset = u * rd.x() + v * rd.y();

		return ray(origin + offset,  lowerLeftCorner + s * horizontal + t * vertical - origin - offset);
	}
};