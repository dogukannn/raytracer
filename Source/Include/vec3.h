#pragma once

#include <cmath>
#include <iostream>

#include "common.h"

struct vec3i
{
	int e[3];

	vec3i() : e{0,0,0} {} 
	vec3i(int e0, int e1, int e2) : e{e0, e1, e2} {}

	int x() const { return e[0]; }
	int y() const { return e[1]; }
	int z() const { return e[2]; }

	vec3i operator-() const { return vec3i(-e[0], -e[1], -e[2]); }
	int operator[](int i) const { return e[i]; }
	int& operator[](int i) { return e[i]; }

	vec3i& operator+=(const vec3i& v)
	{
		e[0] += v.e[0];
		e[1] += v.e[1];
		e[2] += v.e[2];
		return *this;
	}

	vec3i& operator*=(const int t)
	{
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	vec3i& operator/=(const int t)
	{
		return *this *= 1 / t;
	}

	vec3i& operator/=(const double t)
	{
		return *this *= 1.0 / t;
	}

};


struct vec3
{
	float e[3];

	vec3() : e{0,0,0} {} 
	vec3(float e0, float e1, float e2) : e{e0, e1, e2} {}
	vec3(float e0) : e{e0, e0, e0} {}

	float x() const { return e[0]; }
	float y() const { return e[1]; }
	float z() const { return e[2]; }

	vec3 operator-() const { return vec3(-e[0], -e[1], -e[2]); }
	float operator[](int i) const { return e[i]; }
	float& operator[](int i) { return e[i]; }

	vec3& operator+=(const vec3& v)
	{
		e[0] += v.e[0];
		e[1] += v.e[1];
		e[2] += v.e[2];
		return *this;
	}

	vec3& operator*=(const float t)
	{
		e[0] *= t;
		e[1] *= t;
		e[2] *= t;
		return *this;
	}

	vec3& operator/=(const float t)
	{
		return *this *= 1 / t;
	}

	float length() const
	{
		return std::sqrt(lengthSquared());
	}

	float lengthSquared() const
	{
		return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
	}

	inline static vec3 random()
	{
		return vec3(randomFloat(), randomFloat(), randomFloat());
	}

	inline static vec3 random(float min, float max)
	{
		return vec3(randomFloat(min, max), randomFloat(min, max), randomFloat(min, max));
	}

	bool nearZero() const
	{
		const auto s = 1e-8;
		return (fabs(e[0]) < s) && (fabs(e[1]) < s) && (fabs(e[2]) < s);
	}
};

using point3 = vec3;
using color = vec3;

inline std::ostream& operator<<(std::ostream &out, const vec3 &v)
{
	return out << v.e[0] << " " << v.e[1] << " " << v.e[2];
}

inline vec3 operator+(const vec3 &u, const vec3 &v)
{
	return vec3(u.e[0] + v.e[0], u.e[1] + v.e[1], u.e[2] + v.e[2]);
}


inline vec3 operator-(const vec3 &u, const vec3 &v)
{
	return vec3(u.e[0] - v.e[0], u.e[1] - v.e[1], u.e[2] - v.e[2]);
}


inline vec3 operator*(const vec3 &u, const vec3 &v)
{
	return vec3(u.e[0] * v.e[0], u.e[1] * v.e[1], u.e[2] * v.e[2]);
}

inline vec3 operator*(float t, const vec3 &v)
{
	return vec3(t * v.e[0], t * v.e[1], t * v.e[2]);
}

inline vec3 operator*(const vec3 &v, float t)
{
	return t * v;
}

inline vec3 operator/(const vec3 &v, float t)
{
	return ( 1 / t ) * v;
}

inline double dot(const vec3 &u, const vec3 &v)
{
	return u.e[0] * v.e[0] +
		   u.e[1] * v.e[1] +
		   u.e[2] * v.e[2];
}

inline vec3 cross(const vec3 &u, const vec3 &v)
{
    return vec3(u.e[1] * v.e[2] - u.e[2] * v.e[1],
                u.e[2] * v.e[0] - u.e[0] * v.e[2],
                u.e[0] * v.e[1] - u.e[1] * v.e[0]);
}

inline vec3 unit(vec3 v)
{
	return v / v.length();
}

inline vec3 randomInUnitSphere()
{
	while (true)
	{
		auto p = vec3::random(-1, 1);
		if (p.length() >= 1) continue;
		return p;
	}
}

inline vec3 randomUnitVector()
{
	return unit(randomInUnitSphere());
}

inline vec3 randomInUnitDisk()
{
	while (true)
	{
		auto p = vec3(randomDouble(-1, 1), randomDouble(-1, 1), 0);
		if(p.lengthSquared() >= 1) continue;
		return p;
	}
}

inline vec3 reflect(const vec3 &v, const vec3 &n)
{
	return v - 2 * dot(v, n) * n;
}

inline vec3 refract(const vec3 &uv, const vec3 &n, double etaiOverEtat)
{
	auto cosTheta = fmin(dot(-uv, n), 1.0);
	vec3 rOutPerp = etaiOverEtat * (uv + cosTheta * n);
	vec3 rOutParallel = -sqrt(fabs(1.0 - rOutPerp.lengthSquared())) * n;
	return rOutPerp + rOutParallel;
}

inline vec3 fmin(vec3 a, vec3 b)
{
	return vec3(fmin(a[0], b[0]), fmin(a[1], b[1]), fmin(a[2], b[2]));
}

inline vec3 fmax(vec3 a, vec3 b)
{
	return vec3(fmax(a[0], b[0]), fmax(a[1], b[1]), fmax(a[2], b[2]));
}












