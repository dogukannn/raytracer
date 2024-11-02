#pragma once

#include <cassert>

#include "hittable.h"
#include "vec3.h"


struct triangle : public hittable
{
	point3 p1;
	point3 p2;
	point3 p3;
	vec3 normal;
    vec3 centroid;
	std::shared_ptr<material> mat_ptr;

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


//inline bool triangle::hit(const ray& r, double tMin, double tMax, hitRecord& rec) const
//{
//	assert(r.direction().length() < 1.001 && r.direction().length() > 0.99 );  // Ensure the ray direction is normalized (unit length
//	
//	// Triangle vertices
//    const vec3& v0 = p1;
//	const vec3& v1 = p2;
//    const vec3& v2 = p3;
//
//    // Edge vectors of the triangle
//    vec3 edge1 = v1 - v0;
//    vec3 edge2 = v2 - v0;
//
//    // Calculate the determinant
//    vec3 h = cross(r.direction(), edge2);
//    double a = dot(edge1, h);
//
//    // Check if the ray is parallel to the triangle plane (determinant close to zero)
//    if (fabs(a) < 1e-8)
//        return false;
//
//    double f = 1.0 / a;
//    vec3 s = r.origin() - v0;
//    double u = f * dot(s, h);
//
//    // Check if the intersection lies outside the triangle
//    if (u < 0.0 || u > 1.0)
//        return false;
//
//    vec3 q = cross(s, edge1);
//    double v = f * dot(r.direction(), q);
//
//    // Check if the intersection lies outside the triangle
//    if (v < 0.0 || u + v > 1.0)
//        return false;
//
//    // Calculate the distance t along the ray to the intersection point
//    double t = f * dot(edge2, q);
//
//    // Check if the intersection point is within the acceptable t range
//    if (t < tMin || t > tMax)
//        return false;
//
//    // Update the hit record with the intersection information
//    rec.t = t;
//    rec.p = r.at(t);
//    vec3 outwardNormal = unit(cross(edge1, edge2));  // Normal of the triangle
//    rec.setFaceNormal(r, outwardNormal);
//    rec.mat_ptr = mat_ptr;
//
//    return true;
//}

//inline bool triangle::hit(const ray& r, double tMin, double tMax, hitRecord& rec) const {
//    //assert(r.direction().length() < 1.001 && r.direction().length() > 0.99); // Ensure the ray direction is normalized
//
//    const vec3& v0 = p1;
//    const vec3& v1 = p2;
//    const vec3& v2 = p3;
//
//    // Compute the barycentric coordinates (u, v)
//    vec3 v0v1 = v1 - v0;
//    vec3 v0v2 = v2 - v0;
//    vec3 pvec = cross(r.direction(), v0v2);
//    double det = dot(v0v1, pvec);
//
//    //backface cull
//	//if (det < 1e-8) return false;
//
//
//    // Ray is parallel to the triangle plane
//    if (fabs(det) < 1e-8)
//    {
//        return false;
//    }
//
//    double invDet = 1.0 / det;
//    vec3 tvec = r.origin() - v0;
//    double u = dot(tvec, pvec) * invDet;
//    if (u < -0.01 || u > 1)
//    {
//        return false;
//    }
//
//    vec3 qvec = cross(tvec, v0v1);
//    double v = dot(r.direction(), qvec) * invDet;
//    if (v < -0.01 || u + v > 1)
//    {
//        return false;
//    }
//
//    // Calculate the distance t along the ray to the intersection point
//    double t = dot(v0v2, qvec) * invDet;
//
//    if (t < tMin || t > tMax)
//    {
//        return false;
//    }
//
//    // Update the hit record
//    rec.t = t;
//    rec.p = r.at(t);
//    vec3 outwardNormal = unit(cross(v0v1, v0v2)); 
//    rec.setFaceNormal(r, outwardNormal);
//    rec.mat_ptr = mat_ptr;
//
//    return true;
//}


//best results
inline bool triangle::hit(const ray& r, double tMin, double tMax, hitRecord& rec, mat4* model) const {
    const vec3& v0 = p1;
    const vec3& v1 = p2;
    const vec3& v2 = p3;

    //// Compute edges and normal
    //vec3 v0v1 = v1 - v0;
    //vec3 v0v2 = v2 - v0;
    //vec3 normal = cross(v0v1, v0v2);

    //n
    
    // Normalize the normal
    //normal = unit(normal);
    
    // First compute intersection with plane
    double NdotRayDirection = dot(normal, r.direction());
    
    //if (fabs(NdotRayDirection) < 1e-8) {
    //    return false;  // Ray parallel to triangle plane
    //}
    
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
    if(model)
		rec.p = to_vec3((*model * vec4(P, 1.0f)));
    else
		rec.p = P;
    rec.setFaceNormal(r, normal);
    if(model)
		rec.normal = unit(to_vec3(model->inverse().transpose() * vec4(rec.normal, 0.0f)));
    rec.mat_ptr = mat_ptr;
    
    return true;
}

//inline bool triangle::hit(const ray& r, double tMin, double tMax, hitRecord& rec) const
//{
//	const vec3& v0 = p1;
//	const vec3& v1 = p2;
//	const vec3& v2 = p3;
//
//	vec3 edge1 = v1 - v0;
//	vec3 edge2 = v2 - v0;
//
//	// Calculate the determinant
//	vec3 h = cross(r.direction(), edge2);
//	double a = dot(edge1, h);
//
//	// Check if the ray is parallel to the triangle plane 
//	if (fabs(a) < 1e-5)
//		return false;
//
//	// Check for degenerate triangles (optional)
//	if (cross(edge1, edge2).length() < 1e-5)
//		return false;
//
//	double f = 1.0 / a;
//	vec3 s = r.origin() - v0;
//	double u = f * dot(s, h);
//
//	if (u < 0.0 || u > 1.0)
//		return false;
//
//	vec3 q = cross(s, edge1);
//	double v = f * dot(r.direction(), q);
//
//	if (v < 0.0 || u + v > 1.0)
//		return false;
//
//	double t = f * dot(edge2, q);
//
//	if (t < tMin || t > tMax)
//		return false;
//
//	// Optional: Back-face culling
//	//if (dot(r.direction(), cross(edge1, edge2)) > 0) 
//	//     return false; 
//
//	rec.t = t;
//	rec.p = r.at(t);
//	vec3 outwardNormal = unit(cross(edge1, edge2));
//	rec.setFaceNormal(r, outwardNormal);
//	rec.mat_ptr = mat_ptr;
//
//	return true;
//}
