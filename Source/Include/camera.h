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

	bool dof_enabled = false;
	float focusDistance;
	float aperture;

	camera(point3 lookfrom,
	       point3 lookat,
	       vec3 vup,
	       parser::Vec4f near_plane, 
		   bool _dof_enabled,
	       float _aperture,
	       float _focusDistance,
	       float nearDist)
	{
		w = unit(lookfrom - lookat);
		u = unit(cross(vup, w));
		v = cross(w, u);

		origin = lookfrom;
		horizontal = (near_plane.y - near_plane.x) * u;
		vertical = (near_plane.w - near_plane.z) * v;
		lowerLeftCorner = origin - (unit(horizontal) * fabs(near_plane.x)) - (unit(vertical) * fabs(near_plane.z)) - nearDist * w;

		dof_enabled = _dof_enabled;
		aperture = _aperture;
		focusDistance = _focusDistance;
	}

	ray getRay(float se, float te) const
	{
		if(!dof_enabled)
			return ray(origin,  lowerLeftCorner + se * horizontal + te * vertical - origin);

		auto q = lowerLeftCorner + se * horizontal + te * vertical;
		auto s = origin + (aperture * u * lens_x_offset) + (aperture * v * lens_y_offset);

		auto direction = unit(origin - q);

		auto tfd = focusDistance / dot(direction, -w);

		ray r(origin, direction);

		auto p = r.at(tfd);

		auto d = p - s;

		return ray(s, d);
	}
};
