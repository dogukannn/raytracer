#pragma once

#include "hittable.h"
#include "vec3.h"


struct sphere : public hittable
{
	point3 center;
	double radius;

	sphere() = default;
	sphere(const point3& cen, const double r) : center(cen), radius(r) {}

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec) const override;
};

inline bool sphere::hit(const ray& r, double tMin, double tMax, hitRecord& rec) const
{
	vec3 oc = r.origin() - center;
	auto a = r.direction().lengthSquared();
	auto half_b = dot(oc, r.direction());
	auto c = oc.lengthSquared() - radius * radius;

	auto discriminant = half_b * half_b - a * c;
	if (discriminant < 0) return false;
	auto sqrtd = std::sqrt(discriminant);

	auto root = (-half_b - sqrtd) / a;
	if(root < tMin || tMax < root)
	{
		root = (-half_b + sqrtd) / a;
		if (root < tMin || tMax < root)
			return false;
	}

	rec.t = root;
	rec.p = r.at(rec.t);
	vec3 outwardNormal = (rec.p - center) / radius;
	rec.setFaceNormal(r, outwardNormal);

	return true;
}
