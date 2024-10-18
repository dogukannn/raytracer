#pragma once

#include "hittable.h"
#include "vec3.h"


struct triangle : public hittable
{
	point3 p1;
	point3 p2;
	point3 p3;
	vec3 normal;
	std::shared_ptr<material> mat_ptr;

	triangle() = default;
	triangle(const point3& p1, const point3& p2, const point3& p3, std::shared_ptr<material> m) :  p1(p1), p2(p2), p3(p3), mat_ptr(m) {}

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec) const override;
};

inline bool triangle::hit(const ray& r, double tMin, double tMax, hitRecord& rec) const
{
	// Triangle vertices
    const vec3& v0 = p1; // Assume v0_, v1_, and v2_ are triangle vertices defined in your class
    const vec3& v1 = p2;
    const vec3& v2 = p3;

    // Edge vectors of the triangle
    vec3 edge1 = v1 - v0;
    vec3 edge2 = v2 - v0;

    // Calculate the determinant
    vec3 h = cross(r.direction(), edge2);
    double a = dot(edge1, h);

    // Check if the ray is parallel to the triangle plane (determinant close to zero)
    if (fabs(a) < 1e-8)
        return false;

    double f = 1.0 / a;
    vec3 s = r.origin() - v0;
    double u = f * dot(s, h);

    // Check if the intersection lies outside the triangle
    if (u < 0.0 || u > 1.0)
        return false;

    vec3 q = cross(s, edge1);
    double v = f * dot(r.direction(), q);

    // Check if the intersection lies outside the triangle
    if (v < 0.0 || u + v > 1.0)
        return false;

    // Calculate the distance t along the ray to the intersection point
    double t = f * dot(edge2, q);

    // Check if the intersection point is within the acceptable t range
    if (t < tMin || t > tMax)
        return false;

    // Update the hit record with the intersection information
    rec.t = t;
    rec.p = r.at(t);
    vec3 outwardNormal = unit(cross(edge1, edge2));  // Normal of the triangle
    rec.setFaceNormal(r, outwardNormal);
    rec.mat_ptr = mat_ptr;

    return true;}
