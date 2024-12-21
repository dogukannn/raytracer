#pragma once

#include "hittable.h"
#include "vec3.h"


struct sphere : public hittable
{
	point3 center;
	float radius;
	std::shared_ptr<material> mat_ptr;

	sphere() = default;
	sphere(const point3& cen, const double r, std::shared_ptr<material> m) : center(cen), radius(r), mat_ptr(m) {}

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const override;
};

inline bool sphere::hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const
{
	vec3 oc = r.origin() - center;
	auto a = glm::dot(r.direction(),r.direction());
	auto half_b = dot(oc, r.direction());
	auto c = glm::dot(oc,oc) - radius * radius;

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
		rec.p = glm::vec3((*model * vec4(r.at(rec.t), 1.0f)));
    else
		rec.p = r.at(rec.t);

	vec3 outwardNormal = (r.at(rec.t) - center) / radius;
	rec.setFaceNormal(r, outwardNormal);

	if (model)
		rec.normal = glm::normalize(glm::vec3(glm::inverse(glm::transpose(*model)) * vec4(outwardNormal, 0.0f)));
	rec.mat_ptr = mat_ptr;

	//auto tcenter = glm::vec3((*model * vec4(center, 1.0f)));
	auto tcenter = center;
	auto xyz = r.at(rec.t) - center;

	auto theta = std::acos(xyz.y / radius);
	auto phi = std::atan2(xyz.z , xyz.x);

	rec.uv.x = (-phi + pi) / (2 * pi);
	rec.uv.y = theta / pi;

	tcenter = glm::vec3((*model * vec4(center, 1.0f)));
	//tcenter = center;
	xyz = rec.p - tcenter;

	theta = std::acos(xyz.y / radius);
	phi = std::atan2(xyz.z , xyz.x);

	vec3 tangent, bitangent;

	tangent = (vec3(2.0 * pi * xyz.z, 0.0, -2.0 * pi *(xyz.x)));

	bitangent = (vec3(pi * xyz.y * cos(phi), -pi * radius * sin(theta), pi * xyz.y * sin(phi)));

	//tangent = (glm::vec3(*model * vec4(tangent, 0.0f)));
	//bitangent = (glm::vec3(*model * vec4(bitangent, 0.0f)));

	//bitangent = cross(rec.normal, tangent);

	//tangent = glm::vec3(model->inverse().transpose() * vec4(tangent, 0.0f));
	//bitangent = glm::vec3(model->inverse().transpose() * vec4(bitangent, 0.0f));

	//rec.normal = cross(glm::normalize(bitangent), glm::normalize(tangent));

	rec.tangent = tangent;
	rec.bitangent = bitangent;

	rec.dpdu = tangent;
	rec.dpdv = bitangent;

	if (glm::determinant(*model) < 0.0f)
	{
		//rec.frontFace = false; 
		//rec.normal = -rec.normal;
		rec.tangent = -tangent;
		rec.bitangent = -bitangent;
		rec.negate = true;
	}



	return true;
}
