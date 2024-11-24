#pragma once

#include "common.h"

struct light
{
	virtual color get_intensity(vec3 wi) const = 0;
	virtual point3 get_position() const = 0;
};

struct point_light : public light
{
	color intensity;
	point3 position;

	point_light(color intensity, point3 position)
		: intensity(intensity), position(position)
	{}

	color get_intensity(vec3 wi) const override { return intensity; }
	point3 get_position() const override { return position; }
};

struct area_light : public light
{
	color intensity;
	point3 position;
	vec3 normal;
	float size;

	area_light(color intensity, point3 position, vec3 normal, float size)
		: intensity(intensity), position(position), normal(normal), size(size)
	{}

	color get_intensity(vec3 wi) const override
	{
		return intensity * size * size * abs(dot(normal, -wi));
	}
	point3 get_position() const override
	{
		//create a basis and sample a position
		vec3 np = create_non_colinear_vector(normal);
		vec3 u = unit(cross(normal, np));
		vec3 v = unit(cross(normal, u));

		float u_offset = (- size / 2.0f) + (size)*area_light_u_offset;
		float v_offset = (- size / 2.0f) + (size)*area_light_v_offset;

		return position + u * u_offset + v * v_offset;
	}
};