#pragma once

#include "common.h"
#include "texture.h"


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
	{
		intensity = intensity / 255.0f;
	}

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
	{
		intensity = intensity / 255.0f;
	}

	color get_intensity(vec3 wi) const override
	{
		return intensity * size * size * fabs(dot(normal, -wi));
	}
	point3 get_position() const override
	{
		//create a basis and sample a position
		vec3 np = create_non_colinear_vector(normal);
		vec3 u = glm::normalize(cross(normal, np));
		vec3 v = glm::normalize(cross(normal, u));

		float u_offset = (- size / 2.0f) + (size)*area_light_u_offset;
		float v_offset = (- size / 2.0f) + (size)*area_light_v_offset;

		return position + u * u_offset + v * v_offset;
	}
};

struct spot_light : public light
{
	color intensity;
	point3 position;
	vec3 direction;
	float falloff;
	float coverage;

	spot_light(color intensity, point3 position, vec3 direction, float _falloff, float _coverage)
		: intensity(intensity), position(position), direction(direction), falloff(_falloff * 0.5f), coverage(_coverage * 0.5f)
	{
		coverage = glm::radians(coverage);
		falloff = glm::radians(falloff);
		intensity = intensity / 255.0f;
	}

	color get_intensity(vec3 wi) const override
	{
		float angle = glm::acos(glm::dot(glm::normalize(direction), glm::normalize(-wi)));

		if (angle > coverage)
			return color(0.0f);
		else if (angle < falloff)
			//return glm::vec3(intensity.x, 0, 0);
			return intensity;
		else
		{
			float d = glm::cos(angle) - glm::cos(coverage);
			float n = glm::cos(falloff) - glm::cos(coverage);
			float s = pow(d/n, 4.0f);
			return intensity * s;
		}
	}

	point3 get_position() const override { return position; }
};

struct directional_light
{
	color intensity;
	vec3 direction;

	directional_light(color intensity, vec3 direction)
		: intensity(intensity), direction(direction)
	{}
};

enum envmap_type
{
	spherical = 0,
	latlong,
};

struct spherical_directional_light
{
	std::shared_ptr<texture> env_texture;
	envmap_type type;

	spherical_directional_light(std::shared_ptr<texture> texture, envmap_type type)
		: env_texture(texture), type(type)
	{}

	void sample(vec3 normal, color& out_intensity, vec3& out_direction) const
	{
		if (type == envmap_type::spherical)
		{
			vec3 dir = generateUpperHemisphereVector();
			while (dot(dir, normal) < 0.0f)
			{
				dir = generateUpperHemisphereVector();
			}
			float r = (1.0f / pi) * ((acos(-dir.z) / sqrtf(dir.x * dir.x + dir.y * dir.y)));
			float u = (r * dir.x + 1) / 2.0f;
			float v = (r * -dir.y + 1) / 2.0f;

			color c = env_texture->value(u, v, dir) * 2.0f * pi;
			out_direction = dir;
			out_intensity = c;
		}
		else if (type == envmap_type::latlong)
		{
			vec3 dir = generateUpperHemisphereVector();
			while (dot(dir, normal) < 0.0f)
			{
				dir = generateUpperHemisphereVector();
			}
			float u = (1.0f + (atan2(dir.x, -dir.z) / pi) ) / 2.0f;
			float v = acos(dir.y) / pi;

			color c = env_texture->value(u, v, dir) * 2.0f * pi;
			out_direction = dir;
			out_intensity = c;
		}
	}
};
