#pragma once

#include "common.h"

struct material;

struct hitRecord
{
	point3 p;
	vec3 normal;
	std::shared_ptr<material> mat_ptr;
	double t;
	bool frontFace;

	inline void setFaceNormal(const ray &r, const vec3 &outwardNormal)
	{
		frontFace = dot(r.direction(), outwardNormal) < 0;
		normal = frontFace ? outwardNormal : -outwardNormal;
	}
};

struct hittable
{
	virtual bool hit(const ray& r, double tMin, double tMax, hitRecord& rec) const = 0;
};