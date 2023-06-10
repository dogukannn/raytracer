#pragma once

#include "ray.h"

struct hitRecord
{
	point3 p;
	vec3 normal;
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