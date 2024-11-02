#pragma once

#include "hittable.h"
#include "vec3.h"


struct sphere : public hittable
{
	point3 center;
	double radius;
	std::shared_ptr<material> mat_ptr;

	sphere() = default;
	sphere(const point3& cen, const double r, std::shared_ptr<material> m) : center(cen), radius(r), mat_ptr(m) {}

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const override;
};

inline bool sphere::hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const
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

    if(model)
		rec.p = to_vec3((*model * vec4(r.at(rec.t), 1.0f)));
    else
		rec.p = r.at(rec.t);

	vec3 outwardNormal = (r.at(rec.t) - center) / radius;
	rec.setFaceNormal(r, outwardNormal);
	rec.normal = to_vec3(model->inverse().transpose() * vec4(rec.normal, 0.0f));
	rec.mat_ptr = mat_ptr;

	return true;
}
