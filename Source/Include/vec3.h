#pragma once

#include <cmath>
#include <iostream>

#include "common.h"

using vec4 = glm::vec4;
using vec3 = glm::vec3;
using point3 = glm::vec3;
using color = glm::vec3;
using mat4 = glm::mat4;
using mat3 = glm::mat3;
using vec2 = glm::vec2;


inline glm::vec3 reflect(const glm::vec3 &v, const glm::vec3 &n)
{
	return v - 2 * dot(v, n) * n;
}

inline glm::vec3 refract(const glm::vec3 &uv, const glm::vec3 &n, float etaiOverEtat)
{
	float cosTheta = fmin(dot(-uv, n), 1.0);
	glm::vec3 rOutPerp = etaiOverEtat * (uv + cosTheta * n);
	glm::vec3 rOutParallel = -sqrt(fabs(1.0f - glm::dot(rOutPerp, rOutPerp)) * n);
	return rOutPerp + rOutParallel;
}

inline glm::vec3 fmin(glm::vec3 a, glm::vec3 b)
{
	return glm::vec3(fmin(a[0], b[0]), fmin(a[1], b[1]), fmin(a[2], b[2]));
}

inline glm::vec3 fmax(glm::vec3 a, glm::vec3 b)
{
	return glm::vec3(fmax(a[0], b[0]), fmax(a[1], b[1]), fmax(a[2], b[2]));
}

inline float half_average(glm::vec3 a)
{
	return (a.x + a.y + a.z) / 6.0f;
}

inline float luminance(glm::vec3 a)
{
	return 0.2126f * a.x + 0.7152f * a.y + 0.0722f * a.z;
}








