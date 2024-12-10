#pragma once

#include <cassert>

#include "hittable.h"
#include "vec3.h"

#define RECALC_NORMAL
#define HIT_BARYCENTRIC
//#define HIT_MOLLER_TRUMBORE


struct triangle : public hittable
{
	point3 p1;
	point3 p2;
	point3 p3;

	vec2 uv1;
	vec2 uv2;
	vec2 uv3;

	vec3 normal;
    vec3 centroid;
	std::shared_ptr<material> mat_ptr;

	bool is_ply = false;

	triangle() = default;
	triangle(const point3& p1, const point3& p2, const point3& p3, std::shared_ptr<material> m) :  p1(p1), p2(p2), p3(p3), mat_ptr(m)
	{
		vec3 v0v1 = p2 - p1;
		vec3 v0v2 = p3 - p1;
		normal = cross(v0v1, v0v2);

		normal = unit(normal);
	}

	bool hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const override;
};

#ifdef HIT_MOLLER_TRUMBORE
inline bool triangle::hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* _model) const
{
    // Triangle vertices
    const vec3& v0 = p1;
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
    if(_model)
		rec.p = to_vec3((*_model * vec4(r.at(rec.t), 1.0f)));
    else
		rec.p = r.at(rec.t);


    vec3 outwardNormal = unit(cross(edge1, edge2));  // Normal of the triangle\

    rec.setFaceNormal(r, outwardNormal);
    if(_model)
		rec.normal = unit(to_vec3((_model->transpose().inverse()) * vec4(outwardNormal, 0.0f)));

    rec.mat_ptr = mat_ptr;
}
#endif
 

#ifdef HIT_BARYCENTRIC
//best results
inline bool triangle::hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* _model) const {
    const vec3& v0 = p1;
    const vec3& v1 = p2;
    const vec3& v2 = p3;
    
    // First compute intersection with plane
    double NdotRayDirection = dot(normal, r.direction());
    
    double d = -dot(normal, v0);
    double t = -(dot(normal, r.origin()) + d) / NdotRayDirection;
    
    if (t < tMin || t > tMax) {
        return false;
    }
    
    // Intersection point
    vec3 P = r.at(t);
    
    // Inside-out test
    vec3 C;  // Vector perpendicular to triangle's plane
    
    // Edge 0
    vec3 edge0 = v1 - v0;
    vec3 vp0 = P - v0;
    C = cross(edge0, vp0);
    if (dot(normal, C) < -1e-10) {
        return false;
    }
    
    // Edge 1
    vec3 edge1 = v2 - v1;
    vec3 vp1 = P - v1;
    C = cross(edge1, vp1);
    if (dot(normal, C) < -1e-10) {
        return false;
    }
    
    // Edge 2
    vec3 edge2 = v0 - v2;
    vec3 vp2 = P - v2;
    C = cross(edge2, vp2);
    if (dot(normal, C) < -1e-10) {
        return false;
    }
    
    // If we passed all tests, update hit record
    rec.t = t;

    if(_model)
		rec.p = to_vec3((*_model * vec4(r.at(rec.t), 1.0f)));
    else
		rec.p = r.at(rec.t);

	//calculate barycentric coords, and interpolate uv
	vec3 v0v1 = v1 - v0;
	vec3 v0v2 = v2 - v0;
	vec3 v0p = P - v0;

	float d00 = dot(v0v1, v0v1);
	float d01 = dot(v0v1, v0v2);
	float d11 = dot(v0v2, v0v2);
	float d20 = dot(v0p, v0v1);
	float d21 = dot(v0p, v0v2);

	float denom = d00 * d11 - d01 * d01;
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;

	rec.uv = u * uv1 + v * uv2 + w * uv3;


	rec.setFaceNormal(r, normal);
    if(_model)
    {
		rec.normal = unit(to_vec3((_model->transpose().inverse()) * vec4(normal, 0.0f)));
		if (!rec.frontFace)
			rec.normal = -rec.normal;
#ifdef RECALC_NORMAL
		vec3 tv0 = p1;
		vec3 tv1 = p2;
		vec3 tv2 = p3;

		tv0 = to_vec3(*_model * vec4(tv0, 1.0f));
		tv1 = to_vec3(*_model * vec4(tv1, 1.0f));
		tv2 = to_vec3(*_model * vec4(tv2, 1.0f));

    	vec3 tv0v1 = tv1 - tv0;
		vec3 tv0v2 = tv2 - tv0;
		rec.normal = unit(cross(tv0v1, tv0v2));

		ray newRay = r;
		vec4 origin = vec4(newRay.origin(), 1.0f);
		vec4 direction = vec4(newRay.direction(), 0.0f);

		vec4 newOrigin = *_model * origin;
		vec4 newDirection = *_model * direction;

		//calculate tangent and bitangent
		vec2 deltaUV1 = uv2 - uv1;
		vec2 deltaUV2 = uv3 - uv1;

		rec.tangent = (tv0v1 * deltaUV2.y() - tv0v2 * deltaUV1.y()) / (deltaUV1.x() * deltaUV2.y() - deltaUV1.y() * deltaUV2.x());
		rec.bitangent = (tv0v2 * deltaUV1.x() - tv0v1 * deltaUV2.x()) / (deltaUV1.x() * deltaUV2.y() - deltaUV1.y() * deltaUV2.x());

		newRay = ray(vec3(newOrigin.x(), newOrigin.y(), newOrigin.z()), vec3(newDirection.x(), newDirection.y(), newDirection.z()));

		rec.frontFace = dot(newRay.direction(), rec.normal) < 0;

		if (_model->determinant() < 0.0f)
		{
			rec.negate = true;
		}

		rec.negate_normal = is_ply;


#endif
    }


    rec.mat_ptr = mat_ptr;
    
    return true;
}
#endif
