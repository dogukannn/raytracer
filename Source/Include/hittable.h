#pragma once

#include "common.h"

struct material;

struct hitRecord
{
	point3 p;
	point3 nobp;
	vec3 normal;
	std::shared_ptr<material> mat_ptr;
	float t;
	bool frontFace;
	vec2 uv;
	vec3 tangent;
	vec3 bitangent;
	bool negate = false;
	bool negate_normal = false;

	vec3 dpdu;
	vec3 dpdv;

	inline void setFaceNormal(const ray &r, const vec3 &outwardNormal)
	{
		frontFace = dot(r.direction(), outwardNormal) < 0;
		normal = frontFace ? outwardNormal : -outwardNormal;
	}
};

struct hittable
{
	mat4 model;

	bool has_motion_blur = false;
	vec3 motion;

	virtual bool hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const = 0;
};